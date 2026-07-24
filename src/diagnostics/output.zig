const std = @import("std");
const Counter = @import("counter.zig").Counter;
const Event = @import("event.zig").Event;

/// prints the summary for the diagnostics
/// TODO:
/// 1. make it read from the file instead
/// 2. make it verbose and easy to understand by the developer
pub fn print_summary(counter: Counter) void {
    inline for (@typeInfo(Event).Enum.fields) |field| {
        const event: Event = @enumFromInt(field.value);
        const count = counter.get(event);
        if (count > 0) {
            std.debug.print("{s}: {d}\n", .{ field.name, count });
        }
    }
}

/// writes all the diagnostics to the file : `cache/diagnostics.txt`
pub fn write_to_file(counter: Counter, path: []const u8) !void {
    const file = try std.fs.cwd().createFile(path, .{});
    defer file.close();

    var buf = std.io.bufferedWriter(file.writer());
    const writer = buf.writer();

    inline for (@typeInfo(Event).Enum.fields) |field| {
        const event: Event = @enumFromInt(field.value);
        const count = counter.get(event);
        try writer.print("{s}: {d}\n", .{ field.name, count });
    }
    try buf.flush();
}
