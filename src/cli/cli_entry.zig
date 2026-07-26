const std = @import("std");
const memeql = std.mem.eql;
const Counter = @import("../diagnostics/counter.zig").Counter;

// -------------- temp functions created until the actual ones are -------------------------

fn cbuild(args: anytype, counter: Counter) void {
    std.debug.print("Build Called Successfully\n", .{});
    _ = args;
    _ = counter;
}
fn run(args: anytype, counter: Counter) void {
    std.debug.print("Run Called Successfully\n", .{});
    _ = args;
    _ = counter;
}

fn add(args: anytype, counter: Counter) void {
    std.debug.print("Add Called Successfully\n", .{});
    _ = args;
    _ = counter;
}

fn c_init(args: anytype, counter: Counter) void {
    std.debug.print("Init Called Successfully\n", .{});
    _ = args;
    _ = counter;
}

pub fn handle_cli(
    init: std.process.Init.Minimal,
    counter: Counter,
) !void {
    var args = init.args.iterate();
    _ = args.next();
    const command: ?[:0]const u8 = args.next() orelse null;
    if (command == null) {
        std.debug.print("Please enter the command as a command-line argument\n", .{});
        std.process.exit(1);
    }
    if (memeql(u8, command.?, "build")) {
        cbuild(args, counter);
    } else if (memeql(u8, command.?, "run")) {
        run(args, counter);
    } else if (memeql(u8, command.?, "add")) {
        add(args, counter);
    } else if (memeql(u8, command.?, "init")) {
        c_init(args, counter);
    } else if (memeql(u8, command.?, "test")) {
        // run test
    } else if (memeql(u8, command.?, "help") or memeql(u8, command.?, "h")) {
        // run help
    } else if (memeql(u8, command.?, "clean")) {
        // clean function called
    } else if (memeql(u8, command.?, "explain")) {
        // diagnostics run
    } else if (memeql(u8, command.?, "graph")) {
        // run graph
    } else if (memeql(u8, command.?, "doctor")) {
        // run doctor
    } else if (memeql(u8, command.?, "fmt")) {
        // run fmt
    } else if (memeql(u8, command.?, "check")) {
        // run check
    } else {
        std.debug.print("Command not found : {s}\n", .{command.?});
    }
}
