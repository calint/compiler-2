#!/usr/bin/env python3
import os
import struct
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
COMMON = """func exit(v : reg_rdi) { mov(rax, 60) mov(rdi, v) syscall() }
func assert(err, condition : bool) if not condition exit(err)
type item { tag : i8, values : i32[3] }
func bump(value : item) { value.values[2] = value.values[2] + 1 }
"""
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
    image = path.read_bytes()
    header = struct.unpack_from("<16sHHIQQQIHHHHHH", image)
    assert header[0][:6] == b"\x7fELF\x02\x01"
    sections = [
        struct.unpack_from("<IIQQQQIIQQ", image, header[6] + index * header[11])
        for index in range(header[12])
    ]
    names_section = sections[header[13]]
    names = image[names_section[4]:names_section[4] + names_section[5]]
    by_name = {
        names[section[0]:].split(b"\0", 1)[0].decode(): section
        for section in sections
    }
    segments = [
        struct.unpack_from("<IIQQQQQQ", image, header[5] + index * header[9])
        for index in range(header[10])
    ]
    writable = [segment for segment in segments if segment[0] == 1 and segment[1] == 6]
    assert len(writable) == 1
    return by_name, writable[0]


def compile_source(directory, source, stack_size, options):
    source_path = directory / "arena.baz"
    source_path.write_text(source)
    environment = {
        **os.environ,
        "LLVM_PROFILE_FILE": str(ROOT / "qa/coverage/arena-%p.profraw"),
    }
    result = subprocess.run(
        [str(ROOT / "baz"), str(source_path), f"--stack={stack_size}", *options],
        cwd=directory, env=environment, text=True, capture_output=True,
    )
    return result


with tempfile.TemporaryDirectory(prefix="baz-arena-") as temporary:
    directory = Path(temporary)
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
    modes = {
        "checked": ["--checks=upper,lower,line"],
        "production": ["--no-reproduce"],
        "unoptimized": ["--checks=upper,lower,line", "--nopt"],
    }
    for name, (data_source, checks, data_size) in layouts.items():
        source = data_source + COMMON + "func main() {\n" + BODY + checks + "}\n"
        for mode, options in modes.items():
            measurements = []
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
                assert run.returncode == 0, (name, mode, run.returncode, run.stderr)
                sections, segment = elf_layout(executable)
                stack = sections[".bss.stack"]
                assert stack[1] == 8 and stack[2] & 3 == 3
                assert stack[5] == stack_size
                assert stack[3] % 16 == 0
                if data_size:
                    data = sections[".data"]
                    assert data[5] == data_size
                    assert stack[3] == (data[3] + data_size + 15) // 16 * 16
                else:
                    assert stack[3] == segment[3]
                assert segment[5] == data_size
                assert segment[6] >= stack[3] - segment[3] + stack_size
                assert "lea rbp, [rel dat]" in result.stdout
                assert "mov rsp," not in result.stdout
                measurements.append((executable.stat().st_size, segment[5], segment[6]))
            assert measurements[0][:2] == measurements[1][:2], measurements
            assert measurements[1][2] - measurements[0][2] == 1048576 - 4096
            print(f"arena {name} {mode}: ok", flush=True)

    for register in ("rbp", "ebp", "bp", "bpl"):
        for source in (
            f"func main() {{ mov({register}, 0) }}\n",
            f"func main() {{ var value = {register} }}\n",
            f"func reserved(value : reg_{register}) {{}}\nfunc main() {{ reserved(1) }}\n",
        ):
            result = compile_source(directory, source, 4096, ["--no-reproduce"])
            assert result.returncode == 1, (register, result.returncode, result.stderr)
        print(f"arena reserved {register}: ok", flush=True)