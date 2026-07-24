const std = @import("std");
const FileRegistry = @import("../utils/file_system.zig").FileRegistry;
const File = @import("../utils/file_system.zig").File;
const read_dir = @import("../utils/file_system.zig").recursiveReader;

fn hash_files(reg: *FileRegistry) void {
    for (reg.reg.items) |*file| {
        hash_file(reg.allocator, file) catch continue;
    }
}

fn hash_file(allocator: std.mem.Allocator, file: *File) !void {
    const file_cont = try std.fs.cwd().openFile(file.path, .{});
    defer file_cont.close();

    var hasher = std.crypto.hash.sha2.Sha256.init(.{});
    var buffer: [4096]u8 = undefined;

    while (true) {
        const bytes_read = try file_cont.read(&buffer);
        if (bytes_read == 0) break;
        hasher.update(buffer[0..bytes_read]);
    }
    var digest: [32]u8 = undefined;
    hasher.final(&digest);
    var hex: [64]u8 = undefined;
    _ = std.fmt.bufPrint(&hex, "{}", .{std.fmt.fmtSliceHexLower(&digest)}) catch unreachable;
    file.hash_value = try allocator.dupe(u8, &hex);
}

pub fn scan(allocator: std.mem.Allocator, dir: []const u8) !*FileRegistry {
    const reg = try allocator.create(FileRegistry);
    try read_dir(allocator, reg, dir);
    hash_files(reg);
    return reg;
}
