const std = @import("std");

/// the FileType struct
/// Currently stores only 2 types : .h and .cpp
/// assigned to all files of the type *.h and *.cpp
pub const FileType = enum { HEADER, CPP };

/// the main file storage container.
/// contains a path of the file from the root directory
/// and the file type
pub const File = struct { f_type: FileType, path: []const u8, hash_value: []u8 };

/// The main file registry.
/// contains all cpp and header files along with their information
pub const FileRegistry = struct {
    reg: std.ArrayList(File),
    allocator: std.mem.Allocator,
    const Self = @This();
    pub fn init(allocator: std.mem.Allocator) Self {
        return .{
            .reg = std.ArrayList(File).init(allocator),
            .allocator = allocator,
        };
    }
    pub fn deinit(this: *Self) void {
        for (this.reg.items) |file| {
            this.allocator.free(file.path);
        }
        this.reg.deinit();
    }

    fn add(this: *Self, file: File) !void {
        try this.reg.append(file);
    }
};

/// recursively reads all the directories along the root directory
/// skips reading the directory that is labeled `pico_build/`
/// skips all the files that are not cpp/header files
pub fn recursiveReader(
    allocator: std.mem.Allocator,
    registry: *FileRegistry,
    directory: []const u8,
) !void {
    var dir = try std.fs.cwd().openDir(directory, .{ .iterate = true });
    defer dir.close();

    var it = dir.iterate();
    while (try it.next()) |entry| {
        switch (entry.kind) {
            .file => {
                if (std.mem.endsWith(u8, entry.name, ".cpp")) {
                    const full_path = try std.fs.path.join(allocator, &.{ directory, entry.name });
                    try registry.add(.{ .f_type = .CPP, .path = full_path, .hash_value = null });
                } else if (std.mem.endsWith(u8, entry.name, ".h") or std.mem.endsWith(u8, entry.name, ".hpp")) {
                    const full_path = try std.fs.path.join(allocator, &.{ directory, entry.name });
                    try registry.add(.{ .f_type = .HEADER, .path = full_path, .hash_value = null });
                }
            },
            .directory => {
                if (std.mem.eql(u8, entry.name, "pico_build/")) {} else {
                    const subdir = try std.fs.path.join(allocator, &.{ directory, entry.name });
                    defer allocator.free(subdir);
                    try recursiveReader(allocator, registry, subdir);
                }
            },
            else => {},
        }
    }
}
