"""Package and verify two single-slot DfuSe images from Upsilon's raw builds."""

from pathlib import Path
import struct
import subprocess
import sys
import zlib


def validate_startup(elf, slot):
    # Upsilon's rt0 aborts if there are any global constructors to run.
    symbols = {}
    lines = subprocess.check_output(["arm-none-eabi-nm", "--defined-only", elf], text=True)
    for line in lines.splitlines():
        fields = line.split()
        if len(fields) == 3 and fields[2] in ("_init_array_start", "_init_array_end"):
            symbols[fields[2]] = int(fields[0], 16)
    if len(symbols) != 2:
        raise ValueError(f"Slot {slot}: missing startup symbols in {elf.name}")
    if symbols["_init_array_start"] != symbols["_init_array_end"]:
        raise ValueError(f"Slot {slot}: global constructors would abort firmware startup")


def prepare_image(source, slot, start):
    validate_startup(source / f"epsilon.{slot}.elf", slot)
    binary = (source / f"epsilon.{slot}.bin").read_bytes()
    payload = binary + b"\x00" * (-len(binary) % 8)
    if len(binary) < 0x10028 or len(payload) > 0x400000:
        raise ValueError(f"Slot {slot}: image size exceeds its expected range")
    for label, offset, footer, magic in (
        ("kernel", 8, 28, 0xDEC00DF0),
        ("userland", 0x10000, 0x10024, 0xDEC0EDFE),
    ):
        if any(struct.unpack_from("<I", binary, position)[0] != magic for position in (offset, footer)):
            raise ValueError(f"Slot {slot}: invalid {label} header")
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
    return binary, image + crc


if __name__ == "__main__":
    build_dir, output_dir = map(Path, sys.argv[1:])
    slots = {"A": 0x90000000, "B": 0x90400000}
    # Validate both images before replacing either output from a previous build.
    images = {slot: prepare_image(build_dir, slot, start) for slot, start in slots.items()}
    for slot, (binary, dfu) in images.items():
        basename = output_dir / f"AOS-N0110-slot-{slot}"
        basename.with_suffix(".bin").write_bytes(binary)
        basename.with_suffix(".dfu").write_bytes(dfu)
        print(f"Slot {slot}: {len(binary)} bytes at 0x{slots[slot]:08X}")
