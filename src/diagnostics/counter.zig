const std = @import("std");
const Event = @import("event.zig").Event;

const event_count = @typeInfo(Event).@"enum".field_names.len;

pub const Counter = struct {
    counts: [event_count]u32,
    pub fn init() Counter {
        return .{ .counts = @splat(@as(u32, 0)) };
    }
    pub fn increment(self: *Counter, event: Event) void {
        self.counts[@intFromEnum(event)] += 1;
    }
    pub fn get(self: *const Counter, event: Event) u32 {
        return self.counts[@intFromEnum(event)];
    }
    pub fn reset(self: *Counter) void {
        self.counts = @splat(@as(u32, 0));
    }
};
