const std = @import("std");
const toml = @import("toml.zig");
const TOMLError = error{ MissingTable, MissingKey };

test "toml_parser check" {
    const allocator = std.heap.DebugAllocator(.{});
    var parsed = try toml.ParsedToml.parse(allocator, "./toml_parser/example_pico.toml");
    const project = parsed.table("project") orelse TOMLError.MissingTable;
    const name = project.string("name") orelse return TOMLError.MissingKey;
    std.testing.expectEqualStrings(name.?, "GameEngine");
}
