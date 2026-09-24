#!/usr/bin/env python3
# Run from any directory with: python3 qa/coverage/test-arena.py
# Requires an already-built baz compiler, nasm, and ld.
# Tests three things: reported variable usage, actual ELF/runtime layout,
# and direct access to the arena register without changing allocation policy.
import os
import struct
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
# Shared Baz helpers. A failed assertion exits with its numbered error code.
# item occupies 13 bytes: one i8 tag plus three i32 values, without field padding.
COMMON = """func assert(err, condition bool) { if not condition exit(err) }
type item { tag i8, values[3] i32 }
func bump(value item) { value.values[2] = value.values[2] + 1 }
"""
# Runtime checks: the first local is aligned and zeroed; sibling blocks reuse
# and re-zero storage; nested indexing and mutation through a function argument
# work without corrupting neighbors.
BODY = """    var first_local[3] i8
    var first_address = address_of(first_local)
    assert(1, first_address % 16 == 0)
    assert(2, first_local[0] == 0)
    first_local[2] = 23
    var block_address
    {
        var temporary[5] i8
        block_address = address_of(temporary)
        temporary[0] = 99
    }
    {
        var temporary[5] i8
        assert(3, address_of(temporary) == block_address)
        assert(4, temporary[0] == 0)
    }
    var items[2] item
    var index = 1
    items[index].tag = 9
    items[index].values[2] = 41
    bump(items[index])
    assert(5, items[index].values[2] == 42)
    assert(6, items[index].tag == 9)
    assert(7, first_local[2] == 23)
"""


def elf_layout(path):
    # Decode ELF64 directly so the test does not depend on readelf's text format.
    # '<' means little-endian; H/I/Q are 16/32/64-bit unsigned fields.
    image = path.read_bytes()
    header = struct.unpack_from("<16sHHIQQQIHHHHHH", image)
    # ELF magic, 64-bit class (2), little-endian encoding (1).
    assert header[0][:6] == b"\x7fELF\x02\x01"
    # Header indices: [6] section-table offset, [11] section-entry size,
    # [12] section count, [13] index of the section-name string table.
    # Each section tuple is (name offset, type, flags, address, file offset,
    # size, link, info, alignment, entry size).
    sections = [
        struct.unpack_from("<IIQQQQIIQQ", image, header[6] + index * header[11])
        for index in range(header[12])
    ]
    names_section = sections[header[13]]
    names = image[names_section[4]:names_section[4] + names_section[5]]
    # Section names are null-terminated strings; section[0] indexes that table.
    by_name = {
        names[section[0]:].split(b"\0", 1)[0].decode(): section
        for section in sections
    }
    # Header indices: [5] program-header offset, [9] entry size, [10] count.
    # Each segment tuple is (type, flags, file offset, virtual address,
    # physical address, file size, memory size, alignment).
    segments = [
        struct.unpack_from("<IIQQQQQQ", image, header[5] + index * header[9])
        for index in range(header[10])
    ]
    # PT_LOAD (1) with read/write flags (4 | 2): data and vars share one segment.
    writable = [segment for segment in segments if segment[0] == 1 and segment[1] == 6]
    assert len(writable) == 1
    return by_name, writable[0]


def compile_source(directory, source, vars_size, options):
    # Reuse a temporary source filename. stdout is assembly; stderr is diagnostics.
    # --vars is the CLI option for the reserved variable-storage size.
    source_path = directory / "arena.baz"
    source_path.write_text(source)
    # Instrumented compiler builds leave profiles outside the temporary directory
    # so the coverage runner can collect them after this test exits.
    environment = {
        **os.environ,
        "LLVM_PROFILE_FILE": str(ROOT / "qa/coverage/arena-%p.profraw"),
    }
    result = subprocess.run(
        [str(ROOT / "baz"), str(source_path), f"--vars={vars_size}", "--reproduce-source", *options],
        cwd=directory, env=environment, text=True, capture_output=True,
    )
    return result


# Generated source, assembly, objects, and executables are deleted on exit.
with tempfile.TemporaryDirectory(prefix="baz-arena-") as temporary:
    directory = Path(temporary)
    # Each layout is (global data source, extra runtime assertions, data bytes).
    # Seven bytes need nine bytes of alignment padding; sixteen need none.
    layouts = {
        "no-data": ("", "", 0),
        "odd-data": (
            "dat first i8 = 7\ndat second i32 = 123456\ndat third[2] i8 = {11, 22}\n",
            """    assert(9, address_of(second) == address_of(first) + 1)
    assert(10, address_of(third) == address_of(first) + 5)
    assert(11, first_address == address_of(first) + 16)
    assert(12, first == 7)
    assert(13, second == 123456)
    assert(14, third[1] == 22)
""", 7),
        "aligned-data": (
            "dat first[2] i64 = {7, 9}\n",
            """    assert(9, first_address == address_of(first) + 16)
    assert(10, first[0] == 7)
    assert(11, first[1] == 9)
""", 16),
    }
    # Exercise bounds checks, no runtime checks, and
    # disabled assembly optimization. All must agree on storage requirements.
    modes = {
        "checked": ["--checks=upper,lower,line"],
        "production": [],
        "unoptimized": ["--checks=upper,lower,line", "--nopt"],
    }
    # 1. Check only the 'max vars size' statistic printed by toc::finish().
    # Vary the data prefix around alignment boundaries: neither data bytes nor
    # the padding before vars may contribute to the reported variable peak.
    # The inferred array also checks a data size learned from its initializer.
    statistics_layouts = [
        ("no-data", ""),
        ("odd-data", layouts["odd-data"][0]),
        ("aligned-data", layouts["aligned-data"][0]),
        ("one-byte", "dat data[1] i8\n"),
        ("before-alignment", "dat data[15] i8\n"),
        ("after-alignment", "dat data[17] i8\n"),
        ("before-second-alignment", "dat data[31] i8\n"),
        ("second-alignment", "dat data[32] i8\n"),
        ("inferred-data", "dat data[] i8 = {" + ", ".join(["1"] * 17) + "}\n"),
    ]
    # Each case is (name, declarations before main, main body, expected peak bytes).
    # These are simultaneous storage requirements, not sums of all declarations.
    statistics_cases = [
        ("no-vars", "", "", 0),
        ("local", "", "var local[3] i8", 3),
        # Sibling blocks do not coexist: 3 + max(5, 2) = 8.
        ("sibling-scopes", "",
         "var local[3] i8\n{ var temporary[5] i8 }\n"
         "{ var temporary[2] i8 }", 8),
        # Nested blocks coexist: 3 + 5 + 7 = 15; the later 4-byte block reuses space.
        ("nested-scopes", "",
         "var local[3] i8\n{ var outer[5] i8\n"
         "{ var inner[7] i8 } }\n{ var reused[4] i8 }", 15),
        # Global vars remain live while main runs: 11 + 3 + 5 = 19.
        ("global-and-local", "var global[11] i8\n",
         "var local[3] i8\n{ var temporary[5] i8 }", 19),
        # main, middle, and leaf need 3 + 5 + 7 = 15 together.
        # Calling middle again must reuse its storage, not accumulate another 12.
        ("nested-calls",
         "func leaf() { var leaf_local[7] i8 }\n"
         "func middle() { var middle_local[5] i8 leaf() }\n",
         "var local[3] i8\nmiddle()\nmiddle()", 15),
        # Two packed records: 2 * (1 + 3 * 4) = 26.
        ("structured-array", "type record { tag i8, values[3] i32 }\n",
         "var records[2] record", 26),
    ]
    for name, data_source in statistics_layouts:
        for case, declarations, body, expected_size in statistics_cases:
            # Parsing an unused function must not inflate generated-program usage.
            source = (data_source + declarations
                      + "func unused() { var unused_local[1024] i8 }\n"
                      + "func main() {\n" + body + "\n}\n")
            for mode, options in modes.items():
                result = compile_source(directory, source, 4096, options)
                context = (name, case, mode)
                assert result.returncode == 0, (context, result.stderr)
                # Statistics are NASM comments such as '; max vars size: 15 B'.
                # Ignore alignment whitespace and other comment labels.
                statistics = {}
                for line in result.stdout.splitlines():
                    if line.startswith(";") and ":" in line:
                        label, value = line[1:].split(":", 1)
                        statistics[label.strip()] = value.strip()
                assert statistics["max vars size"] == f"{expected_size} B", (context, statistics)
            print(f"arena {name} max vars size: ok", flush=True)

    for count in (16, 17):
        source = f"func main() {{ var buffer[{count}] i8 }}\n"
        result = compile_source(directory, source, 16, [])
        assert result.returncode == (0 if count == 16 else 1), result.stderr
        if count == 17:
            assert "variable 'buffer' would overflow allocated vars section" in result.stderr, result.stderr
        print(f"arena capacity {count}: ok", flush=True)

    # 2. Assemble, link, and execute programs, then inspect their ELF layout.
    for name, (data_source, checks, data_size) in layouts.items():
        source = data_source + COMMON + "func main() {\n" + BODY + checks + "}\n"
        for mode, options in modes.items():
            measurements = []
            # Compare 4 KiB and 1 MiB reservations: only memory size should grow.
            for vars_size in (4096, 1048576):
                result = compile_source(directory, source, vars_size, options)
                assert result.returncode == 0, result.stderr
                assembly = directory / "arena.s"
                assembly.write_text(result.stdout)
                subprocess.run(["nasm", "-f", "elf64", "arena.s"], cwd=directory, check=True)
                subprocess.run(
                    ["ld", "-s", "-T", str(ROOT / "baz.ld"), "-o", "arena", "arena.o"],
                    cwd=directory, check=True,
                )
                executable = directory / "arena"
                run = subprocess.run([str(executable)], capture_output=True)
                # Nonzero exit codes identify the failed Baz assertion in BODY/checks.
                assert run.returncode == 0, (name, mode, run.returncode, run.stderr)
                sections, segment = elf_layout(executable)
                variables = sections[".bss.vars"]
                # SHT_NOBITS (8) reserves memory without storing bytes in the file.
                # SHF_WRITE | SHF_ALLOC (1 | 2) makes it writable allocated storage.
                assert variables[1] == 8 and variables[2] & 3 == 3
                assert variables[5] == vars_size
                assert variables[3] % 16 == 0
                if data_size:
                    data = sections[".data"]
                    assert data[5] == data_size
                    # Round the exclusive data-end address up to a multiple of 16.
                    assert variables[3] == (data[3] + data_size + 15) // 16 * 16
                else:
                    # With no initialized data, vars starts at the RW segment base.
                    assert variables[3] == segment[3]
                # The file backs only initialized data; memory must cover vars too.
                assert segment[5] == data_size
                assert segment[6] >= variables[3] - segment[3] + vars_size
                # Startup uses rbp for the arena base, not the OS-provided rsp.
                assert "default rel" in result.stdout
                assert "lea rbp, [dat]" in result.stdout
                assert "mov rsp," not in result.stdout
                # (whole file bytes, RW segment file bytes, RW segment memory bytes)
                measurements.append((executable.stat().st_size, segment[5], segment[6]))
            # Neither file size may grow with the reservation; memory grows exactly
            # by the requested difference, without a hidden file-backed padding gap.
            assert measurements[0][:2] == measurements[1][:2], measurements
            assert measurements[1][2] - measurements[0][2] == 1048576 - 4096
            print(f"arena {name} {mode}: ok", flush=True)

    for offset in (2047, 2048, 8196, 2147483647, 2147483648, 2147483656):
        source = f"""type large {{ padding[{offset}] i8, value i32, next i32 }}
func noinline update(value i32) {{ value = value + 1 }}
func main() {{
    var data large
    data.value = 7
    data.next = data.value
    var address = address_of(data.value)
    var equal bool = data.value == data.next
    data.value = -data.value
    update(data.next)
}}
"""
        for mode, options in modes.items():
            result = compile_source(directory, source, (offset + 4111) // 16 * 16, options)
            assert result.returncode == 0, (offset, mode, result.stderr)
            assert f"[rbp + {offset}]" in result.stdout if offset <= 2147483647 else f", {offset}\n" in result.stdout
            assembly = directory / "large.s"
            assembly.write_text(result.stdout)
            subprocess.run(["nasm", "-Werror", "-f", "elf64", str(assembly)], check=True)
        print(f"arena large offset {offset}: ok", flush=True)

    for offset in (2147483647, 2147483648, 2147483656):
        source = COMMON + f"""type large {{ padding[{offset}] i8, value i32, next i32, equal bool, values[3] i32 }}
func noinline update(value i32) {{ value = value + 1 }}
func noinline probe(data large) {{
    data.value = 7
    data.next = data.value
    data.equal = data.value == data.next
    assert(20, data.equal)
    data.next = 8
    data.equal = data.value == data.next
    assert(21, not data.equal)
    data.value = -data.value
    assert(22, data.value == -7)
    update(data.next)
    assert(23, data.next == 9)
    var index = 2
    data.values[index] = 42
    assert(24, data.values[index] == 42)
    assert(25, address_of(data.next) == address_of(data.value) + 4)
}}
func main() {{}}
"""
        harness = f"""
section .text
    global probe_entry
probe_entry:
    lea rbp, [probe_frame]
    mov rbx, rbp
    lea rax, [probe_values]
    mov rdx, {offset}
    sub rax, rdx
    mov [rbx], rax
    call func.probe
    mov eax, 60
    xor edi, edi
    syscall
section .data
align 16
probe_frame: times 4096 db 0
probe_values: times 32 db 0
"""
        for mode, options in modes.items():
            result = compile_source(directory, source, (offset + 4111) // 16 * 16, options)
            assert result.returncode == 0, (offset, mode, result.stderr)
            assembly = directory / "probe.s"
            panic_harness = """
section .text
global panic_entry
panic_entry:
    mov ebp, 123
    jmp baz_bounds_panic
""" if mode != "production" else ""
            assembly.write_text(result.stdout + harness + panic_harness)
            subprocess.run(["nasm", "-Werror", "-f", "elf64", "probe.s"], cwd=directory, check=True)
            subprocess.run(
                ["ld", "-e", "probe_entry", "-T", str(ROOT / "baz.ld"), "-o", "probe", "probe.o"],
                cwd=directory, check=True,
            )
            run = subprocess.run([str(directory / "probe")], capture_output=True)
            assert run.returncode == 0, (offset, mode, run.returncode, run.stderr)
            if panic_harness:
                subprocess.run(
                    ["ld", "-e", "panic_entry", "-T", str(ROOT / "baz.ld"), "-o", "panic", "probe.o"],
                    cwd=directory, check=True,
                )
                run = subprocess.run([str(directory / "panic")], capture_output=True)
                assert run.returncode == 255, (offset, mode, run.returncode, run.stderr)
                assert run.stderr == b"panic: bounds at line 123\n", run.stderr
        print(f"arena large offset runtime {offset}: ok", flush=True)

    source = COMMON + """func noinline update(value i32) { value = value + 1 }
func main() {
    var value i32 = 41
    update(value)
    assert(1, value == 42)
}
"""
    for vars_size in (2147483648, 4294967296):
        for options in (["--checks=frame"], ["--checks=upper,lower,line,frame", "--nopt"]):
            result = compile_source(directory, source, vars_size, options)
            assert result.returncode == 0, (vars_size, options, result.stderr)
            assembly = directory / "frame.s"
            assembly.write_text(result.stdout)
            subprocess.run(["nasm", "-Werror", "-f", "elf64", "frame.s"], cwd=directory, check=True)
            subprocess.run(
                ["ld", "-T", str(ROOT / "baz.ld"), "-o", "frame", "frame.o"],
                cwd=directory, check=True,
            )
            run = subprocess.run([str(directory / "frame")], capture_output=True)
            assert run.returncode == 0, (vars_size, options, run.returncode, run.stderr)
        print(f"arena large frame capacity {vars_size}: ok", flush=True)

    for declaration in (
        "type huge { values[2305843009213693952] i64 }",
        "type huge { values[9223372036854775807] i8, extra i8 }",
        "var huge[2305843009213693952] i64",
        "dat huge[2305843009213693952] i64",
        "dat huge[1152921504606846976] i64",
    ):
        result = compile_source(directory, declaration + "\nfunc main() {}\n", 4096, [])
        assert result.returncode == 1, result.stderr
        assert "storage size exceeds signed 64-bit range" in result.stderr, result.stderr
        print("arena storage arithmetic overflow: ok", flush=True)