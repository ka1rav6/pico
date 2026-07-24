const std = @import("std");
const FileRegistry = @import("../utils/file_system.zig").FileRegistry;
const File = @import("../utils/file_system.zig").File;
const read_dir = @import("../utils/file_system.zig").recursiveReader;

fn hash_files(reg: *FileRegistry) void {
    for (reg) |file| {
        hash_file(file);
    }
}

fn hash_file(file: *File) !void {
    const file_cont = try std.fs.cwd().openFile("{}", .{file});
    defer file_cont.close();

    var hasher = std.crypto.hash.sha2.Sha256.init(.{});
    var buffer: [4096]u8 = undefined;

    while (true) {
        const bytes_read = try file.read(&buffer);
        if (bytes_read == 0) break;
        hasher.update(buffer[0..bytes_read]);
    }
    var digest: [32]u8 = undefined;
    hasher.final(&digest);
    var hex: [64]u8 = undefined;
    _ = std.fmt.bufPrint(&hex, "{}", .{std.fmt.fmtSliceHexLower(&digest)}) catch unreachable;
    @memmove(file.hash_value, hex);
}
