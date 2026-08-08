#include "./cli/cli_entry.h"

int main(int argc, const char **argv) {
    Pico::handle_cli(argc, argv);
    return 0;
}
