const std = @import("std");

pub const BuildOpt = struct {
    c_files: []const []const u8,
    asm_files: []const []const u8,
    linker_script: []const u8,
};

pub const Step = struct {
    step: *std.Build.Step,
    obj_files: [][]const u8,
};

pub const BuildError = error{
    NoCFiles,
    NoASMFiles,
};

fn dirExists(path: []const u8, io: *std.Io) bool {
    return std.Io.Dir.cwd().openDir(io.*, path, .{}) catch null != null;
}

pub fn create_builddir(b: *std.Build, io: *std.Io) ?*std.Build.Step {
    if (dirExists("./build", io)) {
        return null;
    }

    const step1 = b.addSystemCommand(&.{"mkdir", "build"});
    const step2 = b.addSystemCommand(&.{"mkdir", "build/asm"});
    const step3 = b.addSystemCommand(&.{"mkdir", "build/c"});

    step3.step.dependOn(&step2.step);
    step2.step.dependOn(&step1.step);

    return &step3.step;
}

pub fn create_obj_path(b: *std.Build, file: []const u8, ext: []const u8, outpath: []const u8) ![]const u8 {
    var i: usize = 0;
    var last_slash: usize = 0;
    while (i < file.len) {
        if (file[i] == '/') {
            last_slash = i;
            if (std.mem.eql(u8, "./kernel", file[0..i])) {
                break;
            }
        }
        i+=1;
    }
    var size = file[last_slash..].len;
    size -= (ext.len - 1);
    const file_name =  try b.allocator.alloc(u8, size);
    defer b.allocator.free(file_name);
    for (0..size) |c| {
        file_name[c] = file[c + last_slash];
        if (c + 1 == size) {
            file_name[c] = 'o';
        }
    }
    return b.pathJoin(&.{outpath, file_name});
}

pub fn create_output_dir(io: *std.Io, fullpath: []const u8 ) !void {
    var i: usize = 0;
    var last_slash: usize = 0;
    while (i < fullpath.len) {
        if (fullpath[i] == '/') {
            last_slash = i;
        }
        i+=1;
    }

    try std.Io.Dir.cwd().createDirPath(io.*, fullpath[0..last_slash]);
}
