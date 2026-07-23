const std = @import("std");

// -------------- temp functions created until the actual ones are -------------------------

fn cbuild(args: anytype) void {
    std.debug.print("Build Called Successfully\n", .{});
    _ = args;
}
fn run(args: anytype) void {
    std.debug.print("Run Called Successfully\n", .{});
    _ = args;
}

fn add(args: anytype) void {
    std.debug.print("Add Called Successfully\n", .{});
    _ = args;
}

fn c_init(args: anytype) void {
    std.debug.print("Init Called Successfully\n", .{});
    _ = args;
}

pub fn handle_cli(init: std.process.Init.Minimal) !void {
    var args = init.args.iterate();
    _ = args.next();
    const command: ?[:0]const u8 = args.next() orelse null;
    if (command == null) {
        std.debug.print("Please enter the command as a command-line argument\n", .{});
        std.process.exit(1);
    }
    if (std.mem.eql(u8, command.?, "build")) {
        cbuild(args);
    } else if (std.mem.eql(u8, command.?, "run")) {
        run(args);
    } else if (std.mem.eql(u8, command.?, "add")) {
        add(args);
    } else if (std.mem.eql(u8, command.?, "init")) {
        c_init(args);
    } else {
        std.debug.print("Command not found : {s}\n", .{command.?});
    }
}
