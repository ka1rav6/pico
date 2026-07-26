const std = @import("std");
const c = @cImport({
    @cInclude("toml_parser/parser.h");
});

pub const ValueType = enum {
    integer,
    float_val,
    string,
    bool_val,
    datetime,
    array,
    inline_table,
    null,
};

pub const Array = struct {
    items: []const Value,
    allocator: std.mem.Allocator,

    pub fn len(self: Array) usize {
        return self.items.len;
    }

    pub fn get(self: Array, index: usize) ?Value {
        if (index >= self.items.len) return null;
        return Value{ .inner = self.items[index] };
    }

    pub fn string(self: Array, index: usize) ?[]const u8 {
        const v = self.get(index) orelse return null;
        return v.string();
    }

    pub fn int(self: Array, index: usize) ?i64 {
        const v = self.get(index) orelse return null;
        return v.int();
    }

    pub fn table(self: Array, index: usize) ?Table {
        const v = self.get(index) orelse return null;
        return v.table() orelse return null;
    }
};

pub const Value = struct {
    inner: c.Value,

    pub fn typeOf(self: Value) ValueType {
        return switch (self.inner.type) {
            c.VAL_INTEGER => .integer,
            c.VAL_FLOAT => .float_val,
            c.VAL_STRING => .string,
            c.VAL_BOOL => .bool_val,
            c.VAL_DATETIME => .datetime,
            c.VAL_ARRAY => .array,
            c.VAL_INLINE_TABLE => .inline_table,
            else => .null,
        };
    }

    pub fn string(self: Value) ?[]const u8 {
        if (self.inner.type != c.VAL_STRING) return null;
        const ptr = self.inner.as.string;
        if (ptr == null) return null;
        return std.mem.sliceTo(ptr, 0);
    }

    pub fn int(self: Value) ?i64 {
        if (self.inner.type != c.VAL_INTEGER) return null;
        return @intCast(self.inner.as.integer);
    }

    pub fn float(self: Value) ?f64 {
        if (self.inner.type != c.VAL_FLOAT) return null;
        return self.inner.as.float_val;
    }

    pub fn bool(self: Value) ?bool {
        if (self.inner.type != c.VAL_BOOL) return null;
        return self.inner.as.boolean;
    }

    pub fn datetime(self: Value) ?[]const u8 {
        if (self.inner.type != c.VAL_DATETIME) return null;
        const ptr = self.inner.as.datetime;
        if (ptr == null) return null;
        return std.mem.sliceTo(ptr, 0);
    }

    pub fn array(self: Value) ?Array {
        if (self.inner.type != c.VAL_ARRAY) return null;
        const arr = self.inner.as.array;
        if (arr.items == null or arr.len == 0) return .{
            .items = &.{},
            .allocator = undefined,
        };
        return .{
            .items = @as([*]const c.Value, @ptrCast(arr.items))[0..arr.len],
            .allocator = undefined,
        };
    }

    pub fn table(self: Value) ?Table {
        if (self.inner.type != c.VAL_INLINE_TABLE) return null;
        const ptr = self.inner.as.inline_table;
        if (ptr == null) return null;
        return .{ .inner = ptr };
    }
};

pub const Table = struct {
    inner: *c.Table,

    pub fn get(self: Table, key: []const u8) ?Value {
        var i: usize = 0;
        while (i < self.inner.entry_count) : (i += 1) {
            const entry = self.inner.entries[i];
            if (entry.key.segment_count == 1) {
                const seg = entry.key.segments[0];
                const seg_len = std.mem.sliceTo(seg, 0);
                if (std.mem.eql(u8, seg_len, key)) {
                    return .{ .inner = entry.value };
                }
            }
        }
        return null;
    }

    pub fn string(self: Table, key: []const u8) ?[]const u8 {
        const v = self.get(key) orelse return null;
        return v.string();
    }

    pub fn int(self: Table, key: []const u8) ?i64 {
        const v = self.get(key) orelse return null;
        return v.int();
    }

    pub fn float(self: Table, key: []const u8) ?f64 {
        const v = self.get(key) orelse return null;
        return v.float();
    }

    pub fn bool(self: Table, key: []const u8) ?bool {
        const v = self.get(key) orelse return null;
        return v.bool();
    }

    pub fn datetime(self: Table, key: []const u8) ?[]const u8 {
        const v = self.get(key) orelse return null;
        return v.datetime();
    }

    pub fn array(self: Table, key: []const u8) ?Array {
        const v = self.get(key) orelse return null;
        return v.array();
    }

    pub fn table(self: Table, key: []const u8) ?Table {
        var i: usize = 0;
        while (i < self.inner.child_count) : (i += 1) {
            const child = self.inner.children[i];
            if (child.name == null) continue;
            const name = std.mem.sliceTo(child.name, 0);
            if (std.mem.eql(u8, name, key)) {
                return .{ .inner = child };
            }
        }
        return null;
    }
};

pub const ParsedToml = struct {
    program: *c.Program,
    lexer: c.Lexer,
    allocator: std.mem.Allocator,
    owned: []u8,

    pub fn parse(allocator: std.mem.Allocator, file_path: []const u8) !ParsedToml {
        var file = try std.fs.cwd().openFile(file_path, .{});
        defer file.close();

        const stat = try file.stat();
        const size: usize = @intCast(stat.size);

        var owned = try allocator.alloc(u8, size + 1);
        const n = try file.readAll(owned);
        owned[n] = 0;
        const content = owned[0..n :0];

        var lexer: c.Lexer = undefined;
        c.lexer_init(&lexer, content.ptr, content.len);
        if (!c.lexer_tokenize(&lexer)) {
            return error.LexError;
        }

        const program = c.program_init();
        errdefer {
            // program is heap-allocated by C, would need a program_destroy
            allocator.free(owned);
        }

        var parser = c.parser_init(&lexer);
        if (!c.parser_parse(&parser, program)) {
            return error.ParseError;
        }

        return .{
            .program = program,
            .lexer = lexer,
            .allocator = allocator,
            .owned = owned,
        };
    }

    pub fn deinit(self: *ParsedToml) void {
        self.allocator.free(self.owned);
    }

    pub fn root(self: *const ParsedToml) Table {
        return .{ .inner = &self.program.root };
    }

    pub fn table(self: *const ParsedToml, name: []const u8) ?Table {
        return self.root().table(name);
    }

    pub fn string(self: *const ParsedToml, key: []const u8) ?[]const u8 {
        return self.root().string(key);
    }

    pub fn int(self: *const ParsedToml, key: []const u8) ?i64 {
        return self.root().int(key);
    }

    pub fn bool(self: *const ParsedToml, key: []const u8) ?bool {
        return self.root().bool(key);
    }
};
