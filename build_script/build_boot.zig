const std = @import("std");
const utils = @import("./build_utils.zig");

fn build_asm(b: *std.Build, asm_files: []const []const u8) !utils.Step {
    var objs = try std.ArrayList([]const u8).initCapacity(b.allocator, 10);
    defer objs.deinit(b.allocator);

    var last_step: ?*std.Build.Step = null;
    for (asm_files) |file| {
        const output_file = try utils.create_obj_path(b, file, "asm", "./build/boot/asm");
        const cmd = &.{
            "nasm", "-f", "elf64", "-o", output_file, file
        };

        const boot_step = b.addSystemCommand(cmd);
        if (last_step) |last| {
            boot_step.step.dependOn(last);
        }
        last_step = &boot_step.step;
        try objs.append(b.allocator, output_file);

    }
    if (last_step) |step| {
        return .{ 
            .step = step, 
            .obj_files = try b.allocator.dupe([]const u8, objs.items) 
        };
    }
    return utils.BuildError.NoASMFiles;
}

fn build_c(b: *std.Build, c_files: []const []const u8) !utils.Step {
    var objs = try std.ArrayList([]const u8).initCapacity(b.allocator, 10);
    defer objs.deinit(b.allocator);

    var last_step: ?*std.Build.Step = null;
    for (c_files) |file| {
        const output_file = try utils.create_obj_path(b, file, "c", "./build/boot/c");
        const cmd = &.{
            "gcc", "-c", "-Werror", "-Wall", "-g", "-O0",
            "-ffreestanding", "-fno-stack-protector", "-fno-pic",
            "-mno-red-zone", "-mno-mmx", "-mno-sse", "-mno-sse2",
            "-nostdlib", file, "-o", output_file
        };

        const boot_step = b.addSystemCommand(cmd);
        if (last_step) |last| {
            boot_step.step.dependOn(last);
        }
        last_step = &boot_step.step;
        try objs.append(b.allocator, output_file);

    }
    if (last_step) |step| {
        return .{ 
            .step = step, 
            .obj_files = try b.allocator.dupe([]const u8, objs.items) 
        };
    }
    return utils.BuildError.NoCFiles;
}

fn link(b: *std.Build, c_objs: [][]const u8, asm_objs: [][]const u8, linker_script: []const u8) !*std.Build.Step {
    var ld_cmd = try std.ArrayList([]const u8).initCapacity(b.allocator, 10);
    try ld_cmd.append(b.allocator, "ld");
    try ld_cmd.append(b.allocator, "-n");
    try ld_cmd.append(b.allocator, "-nostdlib");
    try ld_cmd.append(b.allocator, "-T");
    try ld_cmd.append(b.allocator, linker_script);
    try ld_cmd.append(b.allocator, b.fmt("--Map={s}", .{b.pathJoin(&.{"./build/", "boot.map"})}));

    for (c_objs) |file| {
        try ld_cmd.append(b.allocator, file);
    }

    for (asm_objs) |file| {
        try ld_cmd.append(b.allocator, file);
    }

    try ld_cmd.append(b.allocator, "-o");
    try ld_cmd.append(b.allocator, "./build/boot.o");
    return &b.addSystemCommand(ld_cmd.items).step;
}

pub fn build_boot(b: *std.Build, opt: utils.BuildOpt) *std.Build.Step {
    const asm_step = build_asm(b, opt.asm_files) catch |err| {
        return &b.addFail(b.fmt("Error: {s}", .{@errorName(err)})).step;
    };
    defer b.allocator.free(asm_step.obj_files);

    const c_step = build_c(b, opt.c_files) catch |err| {
        return &b.addFail(b.fmt("Error: {s}", .{@errorName(err)})).step;
    };
    defer b.allocator.free(c_step.obj_files);

    const ld_step = link(b, c_step.obj_files, asm_step.obj_files, opt.linker_script) catch |err| {
        return &b.addFail(b.fmt("Error: {s}", .{@errorName(err)})).step;
    };

    ld_step.dependOn(c_step.step);
    ld_step.dependOn(asm_step.step);

    return ld_step;
}
