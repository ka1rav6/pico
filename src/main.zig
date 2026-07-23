const std = @import("std");
const handle_cli = @import("cli/cli_entry.zig").handle_cli;

pub fn main(init: std.process.Init.Minimal) !void {
    std.debug.print("pico 0.0.0\n", .{});
    try handle_cli(init);
}
