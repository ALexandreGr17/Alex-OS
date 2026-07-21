const std = @import("std");
const utils = @import("./build_utils.zig");

fn build_asm(b: *std.Build, asm_files: []const []const u8, io: *std.Io) !utils.Step {
    var objs = try std.ArrayList([]const u8).initCapacity(b.allocator, 10);
    defer objs.deinit(b.allocator);

    var last_step: ?*std.Build.Step = null;
    for (asm_files) |file| {
        const output_file = try utils.create_obj_path(b, file, "asm", "./build/asm");
        try utils.create_output_dir(io, output_file);
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


const CompileCommand = struct {
    directory: []const u8,
    command: []const u8,
    file: []const u8,
};

fn build_c(b: *std.Build, c_files: []const []const u8, compile_db: *std.ArrayList(CompileCommand), io: *std.Io) !utils.Step {
    var objs = try std.ArrayList([]const u8).initCapacity(b.allocator, 10);
    defer objs.deinit(b.allocator);

    var last_step: ?*std.Build.Step = null;
    for (c_files) |file| {
        const output_file = try utils.create_obj_path(b, file, "c", "./build/c");
        try utils.create_output_dir(io, output_file);
        const cmd = &.{
            "gcc", "-c", "-Wall", "-g", "-O0", "-I./kernel",
            "-ffreestanding", "-fno-stack-protector", "-fno-pic",
            "-mno-red-zone", "-mno-mmx", "-mno-sse", "-mno-sse2",
            "-nostdlib", "-mcmodel=kernel", "-fno-asynchronous-unwind-tables",
            "-fno-unwind-tables", "-fno-exceptions", file, "-o", output_file
        };
        const cwd = try std.process.currentPathAlloc(io.*, b.allocator);

        try compile_db.append(b.allocator, .{
            .directory = cwd,
            .command = try std.mem.join(b.allocator, " ", cmd),
            .file = try std.Io.Dir.cwd().realPathFileAlloc(io.*, file, b.allocator),
        });

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
    try ld_cmd.append(b.allocator, "-nostdlib");
    try ld_cmd.append(b.allocator, "-T");
    try ld_cmd.append(b.allocator, linker_script);
    try ld_cmd.append(b.allocator, b.fmt("--Map={s}", .{b.pathJoin(&.{"./build/", "kernel.map"})}));

    for (c_objs) |file| {
        try ld_cmd.append(b.allocator, file);
    }

    for (asm_objs) |file| {
        try ld_cmd.append(b.allocator, file);
    }

    try ld_cmd.append(b.allocator, "-o");
    try ld_cmd.append(b.allocator, "./build/kernel.o");
    return &b.addSystemCommand(ld_cmd.items).step;
}

pub fn build_kernel(b: *std.Build, opt: utils.BuildOpt, io: *std.Io) *std.Build.Step {
    const asm_step = build_asm(b, opt.asm_files, io) catch |err| {
        return &b.addFail(b.fmt("Error while building asm files: {s}", .{@errorName(err)})).step;
    };
    defer b.allocator.free(asm_step.obj_files);

    var compile_db = std.ArrayList(CompileCommand).initCapacity(b.allocator, 100) catch |err| {
        return &b.addFail(b.fmt("Error initialising compile_db: {s}", .{@errorName(err)})).step;
    };
    defer compile_db.deinit(b.allocator);
    const c_step = build_c(b, opt.c_files, &compile_db, io) catch |err| {
        return &b.addFail(b.fmt("Error while building c files: {s}", .{@errorName(err)})).step;
    };
    defer b.allocator.free(c_step.obj_files);

    const ld_step = link(b, c_step.obj_files, asm_step.obj_files, opt.linker_script) catch |err| {
        return &b.addFail(b.fmt("Error while linking object files: {s}", .{@errorName(err)})).step;
    };

    ld_step.dependOn(c_step.step);
    ld_step.dependOn(asm_step.step);


    var json_file = std.Io.Dir.cwd().createFile(io.*, "./compile_commands.json", .{ .truncate = true }) catch |err| {
        return &b.addFail(b.fmt("Error while creating compile_commands.json: {s}", .{@errorName(err)})).step;
    };
    defer json_file.close(io.*);

    const buffer = b.allocator.alloc(u8, 100) catch |err| {
        return &b.addFail(b.fmt("Error while allocating buffer: {s}", .{@errorName(err)})).step;
    };
    var writer = json_file.writer(io.*, buffer);
    std.json.fmt(compile_db.items, .{ .whitespace = .indent_4 }).format(&writer.interface) catch |err| {
        return &b.addFail(b.fmt("Error while formating json: {s}", .{@errorName(err)})).step;
    };
    writer.end() catch |err| {
        return &b.addFail(b.fmt("Error while writing json: {s}", .{@errorName(err)})).step;
    };

    return ld_step;
}
