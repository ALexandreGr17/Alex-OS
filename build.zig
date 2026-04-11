const std = @import("std");
const utils = @import("./build_script/build_utils.zig");
const boot = @import("./build_script/build_boot.zig");


pub fn build(b: *std.Build) void {
    const build_dir_step = utils.create_builddir(b);
    const boot_step  = boot.build_boot(b, .{ 
        .asm_files = &.{"./kernel/boot/multiboot2.asm", "./kernel/boot/main.asm", "./kernel/boot/log.asm", "./kernel/boot/main64.asm", "./kernel/io.asm"}, 
        .c_files = &.{"./kernel/log.c", "./kernel/main.c"}, 
        .linker_script = "./linker.ld" }
    );

    if (build_dir_step) |step| {
        boot_step.dependOn(step);
    }

    const install_step = b.addSystemCommand(&.{"cp", "./build/boot.o", "iso/boot/kernel.bin"});
    const iso_step = b.addSystemCommand(&.{"grub-mkrescue", "/usr/lib/grub/i386-pc/", "-o", "alexos.iso", "iso"});

    iso_step.step.dependOn(&install_step.step);
    install_step.step.dependOn(boot_step);

    b.getInstallStep().dependOn(&iso_step.step);

    const qemu_step = b.addSystemCommand(&.{"qemu-system-x86_64", "-cdrom", "alexos.iso"});
    qemu_step.step.dependOn(b.getInstallStep());

    const gdb32_step = b.addSystemCommand(&.{"qemu-system-i386", "-cdrom", "alexos.iso", "-S", "-s"});
    gdb32_step.step.dependOn(b.getInstallStep());

    const gdb64_step = b.addSystemCommand(&.{"qemu-system-x86_64", "-cdrom", "alexos.iso", "-S", "-s"});
    gdb64_step.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "launch qemu");
    run_step.dependOn(&qemu_step.step);

    const debug_gdb32_step = b.step("gdb32", "launch qemu");
    debug_gdb32_step.dependOn(&gdb32_step.step);

    const debug_gdb64_step = b.step("gdb64", "launch qemu");
    debug_gdb64_step.dependOn(&gdb64_step.step);
}
