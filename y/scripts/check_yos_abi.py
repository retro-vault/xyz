#!/usr/bin/env python3
"""Verify every public YOS ABI declaration uses the same grouped layout."""

# MIT License (see: LICENSE)
# Copyright (C) 2026 tomaz stih

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
VERSION = 1
ENTRIES = [
    ("VERSION", "version", "_yos_version"),
    ("ROM_MODEL", "rom_model", "_yos_rom_model"),
    ("GET_SYS_INFO", "get_sys_info", "_yos_get_sys_info"),
    ("SET_PRINT_HOOK", "set_print_hook", "_set_print_hook"),
    ("ALLOCATE_MEMORY", "allocate_memory", "__yos_malloc"),
    ("FREE_MEMORY", "free_memory", "__yos_free"),
    ("SHRINK_MEMORY", "shrink_memory", "__yos_shrink"),
    ("CLOCK_TICKS", "clock_ticks", "__clock"),
    ("ENTER_CRITICAL_SECTION", "enter_critical_section", "_enter_critical_section"),
    ("LEAVE_CRITICAL_SECTION", "leave_critical_section", "_leave_critical_section"),
    ("CREATE_TIMER", "create_timer", "__yos_install_timer"),
    ("DESTROY_TIMER", "destroy_timer", "_tmr_uninstall"),
    ("CREATE_EVENT", "create_event", "_evt_create"),
    ("DESTROY_EVENT", "destroy_event", "_evt_destroy"),
    ("SET_EVENT", "set_event", "_evt_set"),
    ("WAIT_EVENT", "wait_event", "_evt_wait"),
    ("CREATE_THREAD", "create_thread", "_thread_create"),
    ("EXIT_THREAD", "exit_thread", "_thread_exit"),
    ("SUSPEND_THREAD", "suspend_thread", "_thread_suspend"),
    ("RESUME_THREAD", "resume_thread", "_thread_resume"),
    ("CREATE_PROCESS", "create_process", "_process_start"),
    ("LOAD_PROCESS", "load_process", "_process_load"),
    ("EXIT_PROCESS", "exit_process", "_process_exit"),
    ("LOAD_LIBRARY", "load_library", "_library_load"),
    ("PROCESS_LOAD_ERROR", "process_load_error", "_process_last_error"),
    ("QUERY_SERVICE", "query_service", "__svc_query"),
    ("REGISTER_SERVICE", "register_service", "_svc_register"),
    ("UNREGISTER_SERVICE", "unregister_service", "_svc_unregister"),
    ("GET_INTERRUPT_HANDLER", "get_interrupt_handler", "_sys_vec_get"),
    ("SET_INTERRUPT_HANDLER", "set_interrupt_handler", "_sys_vec_set"),
    ("READ_KEY", "read_key", "_kbd_read"),
    ("CALIBRATE_MOUSE", "calibrate_mouse", "_mouse_calibrate"),
    ("READ_MOUSE", "read_mouse", "_mouse_read"),
    ("ERROR_NUMBER", "error_number", "__errno_value"),
    ("OPEN", "open", "_open"),
    ("CLOSE", "close", "_close"),
    ("READ", "read", "_read"),
    ("WRITE", "write", "_write"),
    ("LSEEK", "lseek", "_lseek"),
    ("FSYNC", "fsync", "_fsync"),
    ("UNLINK", "unlink", "_unlink"),
    ("RENAME", "rename", "_rename"),
    ("CHDIR", "chdir", "_chdir"),
    ("GETCWD", "getcwd", "_getcwd"),
    ("MKDIR", "mkdir", "_mkdir"),
    ("RMDIR", "rmdir", "_rmdir"),
    ("STAT", "stat", "_stat"),
    ("FSTAT", "fstat", "_fstat"),
    ("OPENDIR", "opendir", "_opendir"),
    ("READDIR", "readdir", "_readdir"),
    ("REWINDDIR", "rewinddir", "_rewinddir"),
    ("CLOSEDIR", "closedir", "_closedir"),
    ("ENUMERATE_DISKS", "enumerate_disks", "_enumerate_disks"),
    ("EXEC_COMMAND", "exec_command", "_exec_command"),
    ("GPX_CREATE", "gpx_create", "_gpx_create"),
    ("GPX_DESTROY", "gpx_destroy", "_gpx_destroy"),
    ("GPX_SET_PAGE", "gpx_set_page", "_gpx_set_page"),
    ("GPX_WIDTH", "gpx_width", "_gpx_width"),
    ("GPX_HEIGHT", "gpx_height", "_gpx_height"),
    ("GPX_CLEAR_SCREEN", "gpx_clear_screen", "_gpx_clrscr"),
    ("GPX_SET_TEXT_BACKGROUND", "gpx_set_text_background", "_gpx_set_text_background"),
    ("GPX_DRAW_PIXEL", "gpx_draw_pixel", "_gpx_draw_pixel"),
    ("GPX_DRAW_LINE", "gpx_draw_line", "_gpx_draw_line"),
    ("GPX_DRAW_BITMAP", "gpx_draw_bitmap", "_gpx_draw_bmp"),
    ("GPX_SHOW_SPRITE", "gpx_show_sprite", "_gpx_show_sprite"),
    ("GPX_HIDE_SPRITE", "gpx_hide_sprite", "_gpx_hide_sprite"),
    ("GPX_DRAW_RECTANGLE", "gpx_draw_rectangle", "_gpx_draw_rectangle"),
    ("GPX_FILL_RECTANGLE", "gpx_fill_rectangle", "_gpx_fill_rectangle"),
    ("GPX_MEASURE_TEXT", "gpx_measure_text", "_gpx_measure_text"),
    ("GPX_DRAW_TEXT", "gpx_draw_text", "_gpx_draw_text"),
    ("GPX_GET_SYSTEM_FONT", "gpx_get_system_font", "_gpx_get_system_font"),
    ("GPX_GET_TINY_FONT", "gpx_get_tiny_font", "_gpx_get_tiny_font"),
    ("GPX_GET_STOCK_BITMAP", "gpx_get_stock_bitmap", "_gpx_get_stock_bmp"),
    ("GPX_DRAW_CIRCLE", "gpx_draw_circle", "_gpx_draw_circle"),
    ("GPX_FILL_CIRCLE", "gpx_fill_circle", "_gpx_fill_circle"),
    ("GPX_DRAW_BOX", "gpx_draw_box", "_gpx_draw_box"),
]


def fail(message: str) -> None:
    raise SystemExit("YOS ABI check failed: " + message)


def check_header(path: Path) -> None:
    source = path.read_text()
    version = re.search(r"#define\s+YOS_VERSION\s+0x([0-9a-fA-F]+)", source)
    if not version or int(version.group(1), 16) != VERSION:
        fail(f"{path}: wrong YOS_VERSION")
    try:
        body = source.split("typedef struct yos_s {", 1)[1].split("} yos_t;", 1)[0]
    except IndexError:
        fail(f"{path}: missing yos_t")
    cursor = -1
    for _, field, _ in ENTRIES:
        patterns = (f"(*{field})", f"*{field};")
        count = sum(body.count(pattern) for pattern in patterns)
        if count != 1:
            fail(f"{path}: yos_t.{field} occurs {count} times")
        positions = [body.find(pattern) for pattern in patterns]
        positions = [position for position in positions if position >= 0]
        if not positions:
            fail(f"{path}: missing yos_t.{field}")
        position = min(positions)
        if position <= cursor:
            fail(f"{path}: yos_t.{field} is out of order")
        cursor = position


def check_inc(path: Path) -> None:
    source = path.read_text()
    abi = re.search(r"\.equ\s+YOS_ABI_VERSION,\s+0x([0-9a-fA-F]+)", source)
    if not abi or int(abi.group(1), 16) != VERSION:
        fail(f"{path}: wrong YOS_ABI_VERSION")
    for slot, (name, _, _) in enumerate(ENTRIES):
        match = re.search(
            rf"\.equ\s+YOS_OFFSET_{name},\s+([0-9]+)", source)
        if not match or int(match.group(1)) != slot * 2:
            fail(f"{path}: wrong YOS_OFFSET_{name}")
    size = re.search(r"\.equ\s+YOS_TABLE_SIZE,\s+([0-9]+)", source)
    if not size or int(size.group(1)) != len(ENTRIES) * 2:
        fail(f"{path}: wrong YOS_TABLE_SIZE")


for header in (
        ROOT / "y/include/yos.h",
        ROOT / "x/platforms/yos/include/yos.h"):
    check_header(header)

kernel_header = (ROOT / "y/include/yos.h").read_bytes()
xcc_header = (ROOT / "x/platforms/yos/include/yos.h").read_bytes()
if kernel_header != xcc_header:
    fail("y/include/yos.h and x/platforms/yos/include/yos.h differ")

for include in (
        ROOT / "y/include/yos.inc",
        ROOT / "x/platforms/yos/include/yos.inc"):
    check_inc(include)

table_source = (ROOT / "y/src/z80/kernel/_syscall_table_init.s").read_text()
table_body = table_source.split("__yos::", 1)[1]
actual_symbols = re.findall(r"^\s*\.dw\s+(\S+)", table_body, re.M)
expected_symbols = [symbol for _, _, symbol in ENTRIES]
if actual_symbols != expected_symbols:
    fail("ROM syscall table does not match yos_t")

print(f"PASS: YOS ABI {VERSION} has {len(ENTRIES)} grouped slots "
      "with matching C, assembly and ROM layouts")
