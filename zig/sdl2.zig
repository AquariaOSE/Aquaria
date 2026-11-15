//! Builds SDL2 as a static library from the official release tarball (dependency "sdl2").
//! Linux: system libraries are dlopen()ed at runtime; headers come from the "sdl_linux_deps" dependency.
//! macOS: needs Apple SDK frameworks; pass -Dmacos-sdk=<path to MacOSX.sdk> when not building on a Mac.

const std = @import("std");

pub const Result = struct {
    lib: *std.Build.Step.Compile,
    include: std.Build.LazyPath,
};

pub fn build(
    b: *std.Build,
    target: std.Build.ResolvedTarget,
    optimize: std.lang.Optimize,
    macos_sdk: ?std.Build.LazyPath,
) Result {
    const t = target.result;
    const src = b.dependency("sdl2", .{});

    const mod = b.createModule(.{
        .target = target,
        .optimize = optimize,
        .link_libc = true,
        .sanitize_c = .off,
    });
    const lib = b.addLibrary(.{
        .name = "SDL2",
        .linkage = .static,
        .root_module = mod,
    });

    const inc = b.addWriteFiles();
    const config: ?[]const u8 = switch (t.os.tag) {
        .linux => "zig/SDL_config_linux.h",
        .windows => "zig/SDL_config_windows.h",
        .macos => null,
        else => @panic("SDL2: unsupported target OS (supported: windows, linux, macos)"),
    };
    if (config) |cfg| {
        _ = inc.addCopyDirectory(src.path("include"), "", .{ .exclude_extensions = &.{"SDL_config.h"} });
        _ = inc.addCopyFile(b.path(cfg), "SDL_config.h");
    } else {
        _ = inc.addCopyDirectory(src.path("include"), "", .{});
    }
    const include = inc.getDirectory();
    mod.addIncludePath(include);
    mod.addCMacro("SDL_USE_BUILTIN_OPENGL_DEFINITIONS", "1");

    const flags: []const []const u8 = &.{ "-fno-strict-aliasing", "-fvisibility=hidden" };
    mod.addCSourceFiles(.{ .root = src.path("src"), .files = &generic_files, .flags = flags });

    switch (t.os.tag) {
        .windows => {
            mod.addCSourceFiles(.{ .root = src.path("src"), .files = &windows_files, .flags = flags });
            for ([_][]const u8{ "user32", "gdi32", "winmm", "imm32", "ole32", "oleaut32", "version", "uuid", "advapi32", "setupapi", "shell32" }) |l|
                mod.linkSystemLibrary(l, .{});
        },
        .linux => {
            if (!addPlatformPaths(b, mod, macos_sdk)) return .{ .lib = lib, .include = include };
            mod.addCSourceFiles(.{ .root = src.path("src"), .files = &linux_files, .flags = flags });
            mod.addCSourceFiles(.{ .root = src.path("src"), .files = &wayland_files, .flags = flags });
            const deps = b.lazyDependency("sdl_linux_deps", .{}).?;
            mod.addCSourceFiles(.{ .root = deps.path("src"), .files = &wayland_protocol_files, .flags = flags });
            const proto = b.addWriteFiles();
            _ = proto.add("tablet-unstable-v2-client-protocol.h", "#include \"tablet-v2-client-protocol.h\"\n");
            mod.addIncludePath(proto.getDirectory());
            mod.addIncludePath(deps.path("src"));
            mod.addSystemIncludePath(src.path("src/video/khronos"));
            mod.addCMacro("_REENTRANT", "1");
            mod.linkSystemLibrary("pthread", .{});
            mod.linkSystemLibrary("dl", .{});
            mod.linkSystemLibrary("m", .{});
        },
        .macos => {
            _ = addPlatformPaths(b, mod, macos_sdk);
            mod.addCSourceFiles(.{ .root = src.path("src"), .files = &macos_c_files, .flags = flags });
            mod.addCSourceFiles(.{
                .root = src.path("src"),
                .files = &macos_objc_files,
                .flags = &.{ "-fno-strict-aliasing", "-fvisibility=hidden", "-fobjc-arc" },
                .language = .objective_c,
            });
            for ([_][]const u8{ "Cocoa", "Carbon", "IOKit", "ForceFeedback", "CoreAudio", "AudioToolbox", "CoreVideo", "CoreFoundation", "Foundation", "QuartzCore", "Metal" }) |f|
                mod.linkFramework(f, .{});
            for ([_][]const u8{ "GameController", "CoreHaptics", "AVFoundation" }) |f|
                mod.linkFramework(f, .{ .weak = true });
            mod.linkSystemLibrary("iconv", .{});
        },
        else => unreachable,
    }

    return .{ .lib = lib, .include = include };
}

/// System headers that SDL and code including SDL_syswm.h need: the Linux X11/ALSA/... headers, or the macOS SDK.
/// Returns false while the lazy Linux dependency isn't fetched yet (zig build fetches it and reruns).
pub fn addPlatformPaths(b: *std.Build, mod: *std.Build.Module, macos_sdk: ?std.Build.LazyPath) bool {
    const t = mod.resolved_target.?.result;
    switch (t.os.tag) {
        .linux => {
            const deps = b.lazyDependency("sdl_linux_deps", .{}) orelse return false;
            const abi: []const u8 = if (t.abi.isMusl()) "musl" else "gnu";
            mod.addSystemIncludePath(deps.path("include"));
            mod.addSystemIncludePath(deps.path(b.fmt("include/{s}-linux-{s}", .{ @tagName(t.cpu.arch), abi })));
        },
        .macos => if (macos_sdk) |sdk| {
            mod.addSystemIncludePath(sdk.path(b, "usr/include"));
            mod.addSystemFrameworkPath(sdk.path(b, "System/Library/Frameworks"));
            mod.addLibraryPath(sdk.path(b, "usr/lib"));
        },
        else => {},
    }
    return true;
}

const generic_files = [_][]const u8{
    "SDL.c",
    "SDL_assert.c",
    "SDL_dataqueue.c",
    "SDL_error.c",
    "SDL_guid.c",
    "SDL_hints.c",
    "SDL_list.c",
    "SDL_log.c",
    "SDL_utils.c",
    "atomic/SDL_atomic.c",
    "atomic/SDL_spinlock.c",
    "audio/SDL_audio.c",
    "audio/SDL_audiocvt.c",
    "audio/SDL_audiodev.c",
    "audio/SDL_audiotypecvt.c",
    "audio/SDL_mixer.c",
    "audio/SDL_wave.c",
    "audio/disk/SDL_diskaudio.c",
    "audio/dummy/SDL_dummyaudio.c",
    "cpuinfo/SDL_cpuinfo.c",
    "dynapi/SDL_dynapi.c",
    "events/SDL_clipboardevents.c",
    "events/SDL_displayevents.c",
    "events/SDL_dropevents.c",
    "events/SDL_events.c",
    "events/SDL_gesture.c",
    "events/SDL_keyboard.c",
    "events/SDL_keysym_to_scancode.c",
    "events/SDL_mouse.c",
    "events/SDL_quit.c",
    "events/SDL_scancode_tables.c",
    "events/SDL_touch.c",
    "events/SDL_windowevents.c",
    "events/imKStoUCS.c",
    "file/SDL_rwops.c",
    "haptic/SDL_haptic.c",
    "hidapi/SDL_hidapi.c",
    "joystick/SDL_gamecontroller.c",
    "joystick/SDL_joystick.c",
    "joystick/SDL_steam_virtual_gamepad.c",
    "joystick/controller_type.c",
    "joystick/virtual/SDL_virtualjoystick.c",
    "joystick/hidapi/SDL_hidapi_combined.c",
    "joystick/hidapi/SDL_hidapi_gamecube.c",
    "joystick/hidapi/SDL_hidapi_luna.c",
    "joystick/hidapi/SDL_hidapi_ps3.c",
    "joystick/hidapi/SDL_hidapi_ps4.c",
    "joystick/hidapi/SDL_hidapi_ps5.c",
    "joystick/hidapi/SDL_hidapi_rumble.c",
    "joystick/hidapi/SDL_hidapi_shield.c",
    "joystick/hidapi/SDL_hidapi_stadia.c",
    "joystick/hidapi/SDL_hidapi_steam.c",
    "joystick/hidapi/SDL_hidapi_steamdeck.c",
    "joystick/hidapi/SDL_hidapi_switch.c",
    "joystick/hidapi/SDL_hidapi_wii.c",
    "joystick/hidapi/SDL_hidapi_xbox360.c",
    "joystick/hidapi/SDL_hidapi_xbox360w.c",
    "joystick/hidapi/SDL_hidapi_xboxone.c",
    "joystick/hidapi/SDL_hidapijoystick.c",
    "libm/e_atan2.c",
    "libm/e_exp.c",
    "libm/e_fmod.c",
    "libm/e_log.c",
    "libm/e_log10.c",
    "libm/e_pow.c",
    "libm/e_rem_pio2.c",
    "libm/e_sqrt.c",
    "libm/k_cos.c",
    "libm/k_rem_pio2.c",
    "libm/k_sin.c",
    "libm/k_tan.c",
    "libm/s_atan.c",
    "libm/s_copysign.c",
    "libm/s_cos.c",
    "libm/s_fabs.c",
    "libm/s_floor.c",
    "libm/s_scalbn.c",
    "libm/s_sin.c",
    "libm/s_tan.c",
    "locale/SDL_locale.c",
    "misc/SDL_url.c",
    "power/SDL_power.c",
    "render/SDL_d3dmath.c",
    "render/SDL_render.c",
    "render/SDL_yuv_sw.c",
    "render/software/SDL_blendfillrect.c",
    "render/software/SDL_blendline.c",
    "render/software/SDL_blendpoint.c",
    "render/software/SDL_drawline.c",
    "render/software/SDL_drawpoint.c",
    "render/software/SDL_render_sw.c",
    "render/software/SDL_rotate.c",
    "render/software/SDL_triangle.c",
    "render/opengl/SDL_render_gl.c",
    "render/opengl/SDL_shaders_gl.c",
    "render/opengles/SDL_render_gles.c",
    "render/opengles2/SDL_render_gles2.c",
    "render/opengles2/SDL_shaders_gles2.c",
    "sensor/SDL_sensor.c",
    "stdlib/SDL_crc16.c",
    "stdlib/SDL_crc32.c",
    "stdlib/SDL_getenv.c",
    "stdlib/SDL_iconv.c",
    "stdlib/SDL_malloc.c",
    "stdlib/SDL_mslibc.c",
    "stdlib/SDL_qsort.c",
    "stdlib/SDL_stdlib.c",
    "stdlib/SDL_string.c",
    "stdlib/SDL_strtokr.c",
    "thread/SDL_thread.c",
    "timer/SDL_timer.c",
    "video/SDL_RLEaccel.c",
    "video/SDL_blit.c",
    "video/SDL_blit_0.c",
    "video/SDL_blit_1.c",
    "video/SDL_blit_A.c",
    "video/SDL_blit_N.c",
    "video/SDL_blit_auto.c",
    "video/SDL_blit_copy.c",
    "video/SDL_blit_slow.c",
    "video/SDL_bmp.c",
    "video/SDL_clipboard.c",
    "video/SDL_egl.c",
    "video/SDL_fillrect.c",
    "video/SDL_pixels.c",
    "video/SDL_rect.c",
    "video/SDL_shape.c",
    "video/SDL_stretch.c",
    "video/SDL_surface.c",
    "video/SDL_video.c",
    "video/SDL_vulkan_utils.c",
    "video/SDL_yuv.c",
    "video/yuv2rgb/yuv_rgb_lsx.c",
    "video/yuv2rgb/yuv_rgb_sse.c",
    "video/yuv2rgb/yuv_rgb_std.c",
    "video/dummy/SDL_nullevents.c",
    "video/dummy/SDL_nullframebuffer.c",
    "video/dummy/SDL_nullvideo.c",
    "video/offscreen/SDL_offscreenevents.c",
    "video/offscreen/SDL_offscreenframebuffer.c",
    "video/offscreen/SDL_offscreenopengles.c",
    "video/offscreen/SDL_offscreenvideo.c",
    "video/offscreen/SDL_offscreenwindow.c",
};

const windows_files = [_][]const u8{
    "core/windows/SDL_hid.c",
    "core/windows/SDL_immdevice.c",
    "core/windows/SDL_windows.c",
    "core/windows/SDL_xinput.c",
    "audio/directsound/SDL_directsound.c",
    "audio/wasapi/SDL_wasapi.c",
    "audio/wasapi/SDL_wasapi_win32.c",
    "audio/winmm/SDL_winmm.c",
    "filesystem/windows/SDL_sysfilesystem.c",
    "haptic/windows/SDL_dinputhaptic.c",
    "haptic/windows/SDL_windowshaptic.c",
    "haptic/windows/SDL_xinputhaptic.c",
    "joystick/windows/SDL_dinputjoystick.c",
    "joystick/windows/SDL_rawinputjoystick.c",
    "joystick/windows/SDL_windows_gaming_input.c",
    "joystick/windows/SDL_windowsjoystick.c",
    "joystick/windows/SDL_xinputjoystick.c",
    "loadso/windows/SDL_sysloadso.c",
    "locale/windows/SDL_syslocale.c",
    "main/windows/SDL_windows_main.c",
    "misc/windows/SDL_sysurl.c",
    "power/windows/SDL_syspower.c",
    "render/direct3d/SDL_render_d3d.c",
    "render/direct3d/SDL_shaders_d3d.c",
    "render/direct3d11/SDL_render_d3d11.c",
    "render/direct3d11/SDL_shaders_d3d11.c",
    "render/direct3d12/SDL_render_d3d12.c",
    "render/direct3d12/SDL_shaders_d3d12.c",
    "sensor/windows/SDL_windowssensor.c",
    "thread/generic/SDL_syscond.c",
    "thread/windows/SDL_syscond_cv.c",
    "thread/windows/SDL_sysmutex.c",
    "thread/windows/SDL_syssem.c",
    "thread/windows/SDL_systhread.c",
    "thread/windows/SDL_systls.c",
    "timer/windows/SDL_systimer.c",
    "video/windows/SDL_windowsclipboard.c",
    "video/windows/SDL_windowsevents.c",
    "video/windows/SDL_windowsframebuffer.c",
    "video/windows/SDL_windowskeyboard.c",
    "video/windows/SDL_windowsmessagebox.c",
    "video/windows/SDL_windowsmodes.c",
    "video/windows/SDL_windowsmouse.c",
    "video/windows/SDL_windowsopengl.c",
    "video/windows/SDL_windowsopengles.c",
    "video/windows/SDL_windowsshape.c",
    "video/windows/SDL_windowsvideo.c",
    "video/windows/SDL_windowsvulkan.c",
    "video/windows/SDL_windowswindow.c",
};

const linux_files = [_][]const u8{
    "core/linux/SDL_dbus.c",
    "core/linux/SDL_evdev.c",
    "core/linux/SDL_evdev_capabilities.c",
    "core/linux/SDL_evdev_kbd.c",
    "core/linux/SDL_fcitx.c",
    "core/linux/SDL_ibus.c",
    "core/linux/SDL_ime.c",
    "core/linux/SDL_sandbox.c",
    "core/linux/SDL_threadprio.c",
    "core/linux/SDL_udev.c",
    "core/unix/SDL_poll.c",
    "audio/alsa/SDL_alsa_audio.c",
    "audio/dsp/SDL_dspaudio.c",
    "audio/pulseaudio/SDL_pulseaudio.c",
    "filesystem/unix/SDL_sysfilesystem.c",
    "haptic/linux/SDL_syshaptic.c",
    "joystick/linux/SDL_sysjoystick.c",
    "joystick/steam/SDL_steamcontroller.c",
    "loadso/dlopen/SDL_sysloadso.c",
    "locale/unix/SDL_syslocale.c",
    "misc/unix/SDL_sysurl.c",
    "power/linux/SDL_syspower.c",
    "sensor/dummy/SDL_dummysensor.c",
    "thread/pthread/SDL_syscond.c",
    "thread/pthread/SDL_sysmutex.c",
    "thread/pthread/SDL_syssem.c",
    "thread/pthread/SDL_systhread.c",
    "thread/pthread/SDL_systls.c",
    "timer/unix/SDL_systimer.c",
    "video/x11/SDL_x11clipboard.c",
    "video/x11/SDL_x11dyn.c",
    "video/x11/SDL_x11events.c",
    "video/x11/SDL_x11framebuffer.c",
    "video/x11/SDL_x11keyboard.c",
    "video/x11/SDL_x11messagebox.c",
    "video/x11/SDL_x11modes.c",
    "video/x11/SDL_x11mouse.c",
    "video/x11/SDL_x11opengl.c",
    "video/x11/SDL_x11opengles.c",
    "video/x11/SDL_x11shape.c",
    "video/x11/SDL_x11touch.c",
    "video/x11/SDL_x11video.c",
    "video/x11/SDL_x11vulkan.c",
    "video/x11/SDL_x11window.c",
    "video/x11/SDL_x11xfixes.c",
    "video/x11/SDL_x11xinput2.c",
    "video/x11/edid-parse.c",
};

const wayland_files = [_][]const u8{
    "video/wayland/SDL_waylandclipboard.c",
    "video/wayland/SDL_waylanddatamanager.c",
    "video/wayland/SDL_waylanddyn.c",
    "video/wayland/SDL_waylandevents.c",
    "video/wayland/SDL_waylandkeyboard.c",
    "video/wayland/SDL_waylandmessagebox.c",
    "video/wayland/SDL_waylandmouse.c",
    "video/wayland/SDL_waylandopengles.c",
    "video/wayland/SDL_waylandshmbuffer.c",
    "video/wayland/SDL_waylandtouch.c",
    "video/wayland/SDL_waylandvideo.c",
    "video/wayland/SDL_waylandvulkan.c",
    "video/wayland/SDL_waylandwindow.c",
};

/// Pre-generated by sdl_linux_deps (wayland-scanner private-code) from newer protocol XML than SDL2 ships;
/// the protocols are backwards compatible. SDL2's tablet-unstable-v2 is the same interface as stable tablet-v2.
const wayland_protocol_files = [_][]const u8{
    "cursor-shape-v1-protocol.c",
    "fractional-scale-v1-protocol.c",
    "idle-inhibit-unstable-v1-protocol.c",
    "keyboard-shortcuts-inhibit-unstable-v1-protocol.c",
    "pointer-constraints-unstable-v1-protocol.c",
    "primary-selection-unstable-v1-protocol.c",
    "relative-pointer-unstable-v1-protocol.c",
    "tablet-v2-protocol.c",
    "text-input-unstable-v3-protocol.c",
    "viewporter-protocol.c",
    "wayland-protocol.c",
    "xdg-activation-v1-protocol.c",
    "xdg-decoration-unstable-v1-protocol.c",
    "xdg-output-unstable-v1-protocol.c",
    "xdg-shell-protocol.c",
    "xdg-toplevel-icon-v1-protocol.c",
};

const macos_c_files = [_][]const u8{
    "haptic/darwin/SDL_syshaptic.c",
    "joystick/darwin/SDL_iokitjoystick.c",
    "loadso/dlopen/SDL_sysloadso.c",
    "power/macosx/SDL_syspower.c",
    "sensor/dummy/SDL_dummysensor.c",
    "thread/pthread/SDL_syscond.c",
    "thread/pthread/SDL_sysmutex.c",
    "thread/pthread/SDL_syssem.c",
    "thread/pthread/SDL_systhread.c",
    "thread/pthread/SDL_systls.c",
    "timer/unix/SDL_systimer.c",
};

const macos_objc_files = [_][]const u8{
    "audio/coreaudio/SDL_coreaudio.m",
    "file/cocoa/SDL_rwopsbundlesupport.m",
    "filesystem/cocoa/SDL_sysfilesystem.m",
    "joystick/iphoneos/SDL_mfijoystick.m",
    "locale/macosx/SDL_syslocale.m",
    "misc/macosx/SDL_sysurl.m",
    "render/metal/SDL_render_metal.m",
    "video/cocoa/SDL_cocoaclipboard.m",
    "video/cocoa/SDL_cocoaevents.m",
    "video/cocoa/SDL_cocoakeyboard.m",
    "video/cocoa/SDL_cocoamessagebox.m",
    "video/cocoa/SDL_cocoametalview.m",
    "video/cocoa/SDL_cocoamodes.m",
    "video/cocoa/SDL_cocoamouse.m",
    "video/cocoa/SDL_cocoaopengl.m",
    "video/cocoa/SDL_cocoaopengles.m",
    "video/cocoa/SDL_cocoashape.m",
    "video/cocoa/SDL_cocoavideo.m",
    "video/cocoa/SDL_cocoavulkan.m",
    "video/cocoa/SDL_cocoawindow.m",
};
