"""Expand the bundled SQLite source only when its content changes."""
import lzma
import sys
from pathlib import Path

source, destination = map(Path, sys.argv[1:])
content = lzma.decompress(source.read_bytes())
if not destination.exists() or destination.read_bytes() != content:
    destination.write_bytes(content)
