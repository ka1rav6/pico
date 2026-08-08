#include "argparser.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <utility>

namespace Pico::cli {
namespace {

size_t edit_distance(const std::string &a, const std::string &b) {
    std::vector<size_t> prev(b.size() + 1), curr(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j)
        prev[j] = j;
    for (size_t i = 1; i <= a.size(); ++i) {
        curr[0] = i;
        for (size_t j = 1; j <= b.size(); ++j) {
            size_t cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            curr[j] =
                std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
        }
        std::swap(prev, curr);
    }
    return prev[b.size()];
}

std::string closest_flag(const ArgSpec &spec, const std::string &unknown,
                         size_t max_dist = 2) {
    std::string best;
    size_t best_dist = max_dist + 1;
    auto consider = [&](const std::string &candidate, bool is_short) {
        size_t d = edit_distance(unknown, candidate);
        if (d < best_dist) {
            best_dist = d;
            best = (is_short ? "-" : "--") + candidate;
        }
    };
    for (const auto &f : spec.flags) {
        consider(f.long_name, false);
        if (!f.short_name.empty())
            consider(f.short_name, true);
    }
    for (const auto &o : spec.options) {
        consider(o.long_name, false);
        if (!o.short_name.empty())
            consider(o.short_name, true);
    }
    return best;
}

const FlagSpec *find_flag(const ArgSpec &spec, const std::string &name) {
    for (const auto &f : spec.flags)
        if (f.long_name == name || f.short_name == name)
            return &f;
    return nullptr;
}

const OptionSpec *find_option(const ArgSpec &spec, const std::string &name) {
    for (const auto &o : spec.options)
        if (o.long_name == name || o.short_name == name)
            return &o;
    return nullptr;
}

} // namespace

bool Arguments::has(const std::string &name) const {
    for (const auto &opt : options)
        if (opt.name == name)
            return true;
    return false;
}

std::optional<std::string> Arguments::value(const std::string &name) const {
    for (const auto &opt : options)
        if (opt.name == name)
            return opt.value;
    return std::nullopt;
}

std::optional<Arguments> parse(const ArgSpec &spec, int argc, const char **argv,
                               ParseError &error) {
    Arguments args;
    bool passthrough = false;
    size_t positional_index = 0;

    auto fail = [&](std::string message, std::string hint = "") {
        error.message = std::move(message);
        error.hint = std::move(hint);
        return std::nullopt;
    };

    for (int i = 1; i < argc; ++i) {
        std::string token = argv[i];

        if (passthrough) {
            args.passthrough.push_back(token);
            continue;
        }

        if (token == "--") {
            passthrough = true;
            continue;
        }

        // Long flag/option: --release, --jobs=4, --jobs 4
        if (token.size() > 2 && token[0] == '-' && token[1] == '-') {
            std::string body = token.substr(2);
            std::string name = body;
            std::optional<std::string> inline_value;
            auto eq = body.find('=');
            if (eq != std::string::npos) {
                name = body.substr(0, eq);
                inline_value = body.substr(eq + 1);
            }

            if (name == "help") {
                args.options.emplace_back(OptionKind::BOOLEAN_FLAG, "help",
                                          std::nullopt, false);
                continue;
            }

            bool used_short;
            if (const OptionSpec *o = find_option(spec, name)) {
                used_short = (o->long_name != name);
                if (!inline_value.has_value()) {
                    if (i + 1 >= argc)
                        return fail("option `--" + name + "` requires a value",
                                    "use `--" + name + " <value>`");
                    inline_value = argv[++i];
                }
                args.options.emplace_back(OptionKind::VALUE_FLAG, o->long_name,
                                          inline_value, used_short);
                continue;
            }
            if (const FlagSpec *f = find_flag(spec, name)) {
                used_short = (f->long_name != name);
                if (inline_value.has_value())
                    return fail("flag `--" + name + "` does not take a value",
                                "use `--" + name + "` alone");
                args.options.emplace_back(OptionKind::BOOLEAN_FLAG,
                                          f->long_name, std::nullopt,
                                          used_short);
                continue;
            }

            std::string hint;
            std::string closest = closest_flag(spec, name);
            if (!closest.empty())
                hint = "did you mean `" + closest + "`?";
            return fail("unknown flag `--" + name + "`", hint);
        }

        // Short flag/option: -h, -j4, -j 4
        if (token.size() > 1 && token[0] == '-') {
            std::string short_part = token.substr(1);
            std::string short_name(1, short_part[0]);

            if (short_name == "h") {
                args.options.emplace_back(OptionKind::BOOLEAN_FLAG, "help",
                                          std::nullopt, true);
                continue;
            }

            if (const FlagSpec *f = find_flag(spec, short_name)) {
                if (short_part.size() > 1)
                    return fail("unknown option `-" + short_part + "`",
                                "`-" + short_name +
                                    "` is a flag and takes no value");
                args.options.emplace_back(OptionKind::BOOLEAN_FLAG,
                                          f->long_name, std::nullopt, true);
                continue;
            }
            if (const OptionSpec *o = find_option(spec, short_name)) {
                std::optional<std::string> value;
                if (short_part.size() > 1) {
                    value = short_part.substr(1);
                } else {
                    if (i + 1 >= argc)
                        return fail("option `-" + short_name +
                                        "` requires a value",
                                    "use `-" + short_name + " <value>`");
                    value = argv[++i];
                }
                args.options.emplace_back(OptionKind::VALUE_FLAG, o->long_name,
                                          value, true);
                continue;
            }

            std::string hint;
            std::string closest = closest_flag(spec, short_part);
            if (!closest.empty())
                hint = "did you mean `" + closest + "`?";
            return fail("unknown option `-" + short_part + "`", hint);
        }

        // Positional argument
        if (positional_index < spec.positionals.size()) {
            args.positionals.push_back(token);
            ++positional_index;
        } else {
            std::string hint;
            if (!spec.positionals.empty())
                hint = "too many arguments for `" + spec.name + "`";
            return fail("unexpected argument `" + token + "`", hint);
        }
    }

    for (size_t k = positional_index; k < spec.positionals.size(); ++k) {
        if (spec.positionals[k].required)
            return fail("missing required argument <" +
                            spec.positionals[k].name + ">",
                        "see `pico " + spec.name + " --help`");
    }

    return args;
}

void print_help(const ArgSpec &spec) {
    std::cout << "usage: pico " << spec.name;
    for (const auto &p : spec.positionals)
        std::cout << (p.required ? " <" : " [") << p.name
                  << (p.required ? ">" : "]");
    if (!spec.flags.empty())
        std::cout << " [flags]";
    if (!spec.options.empty())
        std::cout << " [options]";
    std::cout << "\n\n" << spec.summary << "\n";

    if (!spec.positionals.empty()) {
        std::cout << "\nArguments:\n";
        for (const auto &p : spec.positionals) {
            std::string display =
                (p.required ? "<" : "[") + p.name + (p.required ? ">" : "]");
            std::cout << "  " << std::setw(20) << std::left << display
                      << (p.required ? "(required)" : "") << "\n";
        }
    }

    if (!spec.flags.empty()) {
        std::cout << "\nFlags:\n";
        for (const auto &f : spec.flags) {
            std::string names =
                (!f.short_name.empty() ? "-" + f.short_name + ", " : "    ") +
                "--" + f.long_name;
            std::cout << "  " << std::setw(24) << std::left << names << f.help
                      << "\n";
        }
        std::cout << "  " << std::setw(24) << std::left << "-h, --help"
                  << "show this help and exit\n";
    }

    if (!spec.options.empty()) {
        std::cout << "\nOptions:\n";
        for (const auto &o : spec.options) {
            std::string names =
                (!o.short_name.empty() ? "-" + o.short_name + ", " : "    ") +
                "--" + o.long_name + " <value>";
            std::string help = o.help;
            if (!o.default_value.empty())
                help += " (default: " + o.default_value + ")";
            std::cout << "  " << std::setw(24) << std::left << names << help
                      << "\n";
        }
    }
}

} // namespace Pico::cli
