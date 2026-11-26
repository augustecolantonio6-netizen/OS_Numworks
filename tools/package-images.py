"""Package and verify two single-slot DfuSe images from Upsilon's raw builds."""

from pathlib import Path
import struct
import sys
import zlib


def package(source, output, slot, start):
    binary = (source / f"epsilon.{slot}.bin").read_bytes()
    payload = binary + b"\x00" * (-len(binary) % 8)
    if not 65540 <= len(payload) <= 0x400000:
        raise ValueError(f"Slot {slot}: image size exceeds its expected range")
    if binary[8:12] != bytes.fromhex("f00dc0de") or binary[65536:65540] != bytes.fromhex("feedc0de"):
        raise ValueError(f"Slot {slot}: missing Upsilon-format boot headers")
    stack_pointer, reset_vector = struct.unpack_from("<II", binary, 32)
    if not 0x20000000 < stack_pointer <= 0x20040000 or stack_pointer % 8:
        raise ValueError(f"Slot {slot}: invalid initial stack pointer")
    if reset_vector % 2 != 1 or not start <= (reset_vector & ~1) < start + len(binary):
        raise ValueError(f"Slot {slot}: reset vector is outside the image")

    element = struct.pack("<II", start, len(payload)) + payload
    target = struct.pack("<6sBI255sII", b"Target", 0, 1, b"AOS", len(element), 1)
    prefix = struct.pack("<5sBIB", b"DfuSe", 1, 11 + len(target) + len(element), 1)
    suffix = struct.pack("<4H3sB", 0, 0xA291, 0x0483, 0x011A, b"UFD", 16)
    image = prefix + target + element + suffix
    crc = struct.pack("<I", zlib.crc32(image) ^ 0xFFFFFFFF)
    basename = output / f"AOS-N0110-slot-{slot}"
    basename.with_suffix(".bin").write_bytes(binary)
    basename.with_suffix(".dfu").write_bytes(image + crc)
    print(f"Slot {slot}: {len(payload)} bytes at 0x{start:08X}")


if __name__ == "__main__":
    build_dir, output_dir = map(Path, sys.argv[1:])
    package(build_dir, output_dir, "A", 0x90000000)
    package(build_dir, output_dir, "B", 0x90400000)
