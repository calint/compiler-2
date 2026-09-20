#!/usr/bin/env python3
# Run from any directory with: python3 qa/coverage/test-arena.py
# Requires an already-built baz compiler, nasm, and ld.
# Tests three things: reported variable usage, actual ELF/runtime layout,
# and rejection of source code that tries to use the reserved arena register.
import os
import struct
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
# Shared Baz helpers. A failed assertion exits with its numbered error code.
# item occupies 13 bytes: one i8 tag plus three i32 values, without field padding.
COMMON = """func exit(v : reg_rdi) { mov(rax, 60) mov(rdi, v) syscall() }
func assert(err, condition : bool) if not condition exit(err)
type item { tag : i8, values : i32[3] }
func bump(value : item) { value.values[2] = value.values[2] + 1 }
"""
# Runtime checks: the first local is aligned and zeroed; sibling blocks reuse
# and re-zero storage; nested indexing and mutation through a function argument
# work without corrupting neighbors; the machine stack pointer stays unchanged.
BODY = """    var first_local : i8[3]
    var original_stack = rsp
    var first_address = address_of(first_local)
    assert(1, first_address % 16 == 0)
    assert(2, first_local[0] == 0)
    first_local[2] = 23
    var block_address
    {
        var temporary : i8[5]
        block_address = address_of(temporary)
        temporary[0] = 99
    }
    {
        var temporary : i8[5]
        assert(3, address_of(temporary) == block_address)
        assert(4, temporary[0] == 0)
    }
    var items : item[2]
    var index = 1
    items[index].tag = 9
    items[index].values[2] = 41
    bump(items[index])
    assert(5, items[index].values[2] == 42)
    assert(6, items[index].tag == 9)
    assert(7, first_local[2] == 23)
    assert(8, rsp == original_stack)
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


def compile_source(directory, source, stack_size, options):
    # Reuse a temporary source filename. stdout is assembly; stderr is diagnostics.
    # --stack is the existing CLI name for the reserved variable-storage size.
    source_path = directory / "arena.baz"
    source_path.write_text(source)
    # Instrumented compiler builds leave profiles outside the temporary directory
    # so the coverage runner can collect them after this test exits.
    environment = {
        **os.environ,
        "LLVM_PROFILE_FILE": str(ROOT / "qa/coverage/arena-%p.profraw"),
    }
    result = subprocess.run(
        [str(ROOT / "baz"), str(source_path), f"--stack={stack_size}", *options],
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
            "dat first : i8 = 7\ndat second : i32 = 123456\ndat third : i8[2] = {11, 22}\n",
            """    assert(9, address_of(second) == address_of(first) + 1)
    assert(10, address_of(third) == address_of(first) + 5)
    assert(11, first_address == address_of(first) + 16)
    assert(12, first == 7)
    assert(13, second == 123456)
    assert(14, third[1] == 22)
""", 7),
        "aligned-data": (
            "dat first : i64[2] = {7, 9}\n",
            """    assert(9, first_address == address_of(first) + 16)
    assert(10, first[0] == 7)
    assert(11, first[1] == 9)
""", 16),
    }
    # Exercise bounds checks, skipped source round-trip verification, and
    # disabled assembly optimization. All must agree on storage requirements.
    modes = {
        "checked": ["--checks=upper,lower,line"],
        "production": ["--no-reproduce"],
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
        ("one-byte", "dat data : i8[1]\n"),
        ("before-alignment", "dat data : i8[15]\n"),
        ("after-alignment", "dat data : i8[17]\n"),
        ("before-second-alignment", "dat data : i8[31]\n"),
        ("second-alignment", "dat data : i8[32]\n"),
        ("inferred-data", "dat data : i8[] = {" + ", ".join(["1"] * 17) + "}\n"),
    ]
    # Each case is (name, declarations before main, main body, expected peak bytes).
    # These are simultaneous storage requirements, not sums of all declarations.
    statistics_cases = [
        ("no-vars", "", "", 0),
        ("local", "", "var local : i8[3]", 3),
        # Sibling blocks do not coexist: 3 + max(5, 2) = 8.
        ("sibling-scopes", "",
         "var local : i8[3]\n{ var temporary : i8[5] }\n"
         "{ var temporary : i8[2] }", 8),
        # Nested blocks coexist: 3 + 5 + 7 = 15; the later 4-byte block reuses space.
        ("nested-scopes", "",
         "var local : i8[3]\n{ var outer : i8[5]\n"
         "{ var inner : i8[7] } }\n{ var reused : i8[4] }", 15),
        # Global vars remain live while main runs: 11 + 3 + 5 = 19.
        ("global-and-local", "var global : i8[11]\n",
         "var local : i8[3]\n{ var temporary : i8[5] }", 19),
        # main, middle, and leaf need 3 + 5 + 7 = 15 together.
        # Calling middle again must reuse its storage, not accumulate another 12.
        ("nested-calls",
         "func leaf() { var leaf_local : i8[7] }\n"
         "func middle() { var middle_local : i8[5] leaf() }\n",
         "var local : i8[3]\nmiddle()\nmiddle()", 15),
        # Two packed records: 2 * (1 + 3 * 4) = 26.
        ("structured-array", "type record { tag : i8, values : i32[3] }\n",
         "var records : record[2]", 26),
    ]
    for name, data_source in statistics_layouts:
        for case, declarations, body, expected_size in statistics_cases:
            # Parsing an unused function must not inflate generated-program usage.
            source = (data_source + declarations
                      + "func unused() { var unused_local : i8[1024] }\n"
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

    # 2. Assemble, link, and execute programs, then inspect their ELF layout.
    for name, (data_source, checks, data_size) in layouts.items():
        source = data_source + COMMON + "func main() {\n" + BODY + checks + "}\n"
        for mode, options in modes.items():
            measurements = []
            # Compare 4 KiB and 1 MiB reservations: only memory size should grow.
            for stack_size in (4096, 1048576):
                result = compile_source(directory, source, stack_size, options)
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
                assert variables[5] == stack_size
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
                assert segment[6] >= variables[3] - segment[3] + stack_size
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

    # 3. All widths of rbp are reserved. Reject direct writes, direct reads,
    # and allocation through a register parameter when the function is called.
    for register in ("rbp", "ebp", "bp", "bpl"):
        for source in (
            f"func main() {{ mov({register}, 0) }}\n",
            f"func main() {{ var value = {register} }}\n",
            f"func reserved(value : reg_{register}) {{}}\nfunc main() {{ reserved(1) }}\n",
        ):
            result = compile_source(directory, source, 4096, ["--no-reproduce"])
            assert result.returncode == 1, (register, result.returncode, result.stderr)
        print(f"arena reserved {register}: ok", flush=True)