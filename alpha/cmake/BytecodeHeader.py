"""Write a Direct3D 9 DWORD array from fxc binary output."""
from pathlib import Path
import struct
import sys

binary, header, name = sys.argv[1:]
data = Path(binary).read_bytes()
words = struct.unpack("<" + "I" * (len(data) // 4), data)
lines = ["#pragma once", "const DWORD " + name + "[] = {"]
for index in range(0, len(words), 8):
    lines.append("    " + ", ".join("0x%08x" % value for value in words[index:index + 8]) + ",")
lines.append("};")
Path(header).write_text("\n".join(lines) + "\n")
