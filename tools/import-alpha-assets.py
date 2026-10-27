#!/usr/bin/env python3
"""Copies the alpha's interface graphics into a mod runtime folder.

The mod shows the alpha's HUD and skill icons, which are the alpha's game data and stay out of the repository. This takes
them from one's own alpha (the folder with Cube.exe, gui.plx and data3.db, or its zip):

    tools/import-alpha-assets.py ALPHA_DIR_OR_ZIP RUNTIME_DIR

and writes RUNTIME_DIR/alpha/gui.plx and RUNTIME_DIR/alpha/icons/*.png (data3.db's images, unscrambled).
"""
import io
import os
import sqlite3
import sys
import tempfile
import zipfile

# The alpha's scrambling (Database.cpp): byte i was swapped with byte (i + OFFSETS[i % 44]) % size, then inverted.
OFFSETS = [4242, 9551, 840, 84800, 9242, 9846, 127, 9, 9483, 394, 123, 4834, 32444, 24355, 2433, 17, 34234, 42342, 4243,
           14, 184934, 1987, 3094, 1901, 89409, 4813, 37, 143, 3490, 19483, 1343, 432, 84732, 9184, 9612, 1233, 3434, 1839,
           2984, 1993, 2984, 4895, 816583, 13]


def descramble(data):
    data = bytearray(data)
    for i in range(len(data) - 1, -1, -1):
        other = (i + OFFSETS[i % len(OFFSETS)]) % len(data)
        data[i], data[other] = data[other], data[i]
    return bytes(255 - b for b in data)


def read_files(source):
    """gui.plx and data3.db of an alpha folder or zip, as bytes."""
    wanted = ('gui.plx', 'data3.db')
    if os.path.isdir(source):
        return {name: open(os.path.join(source, name), 'rb').read() for name in wanted}
    found = {}
    with zipfile.ZipFile(source) as archive:
        for member in archive.namelist():
            name = member.rsplit('/', 1)[-1]
            if name in wanted:
                found[name] = archive.read(member)
    missing = [name for name in wanted if name not in found]
    if missing:
        sys.exit(f'{source}: {", ".join(missing)} not found')
    return found


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    files = read_files(sys.argv[1])
    target = os.path.join(sys.argv[2], 'alpha')
    icons = os.path.join(target, 'icons')
    os.makedirs(icons, exist_ok=True)
    with open(os.path.join(target, 'gui.plx'), 'wb') as file:
        file.write(files['gui.plx'])
    with tempfile.NamedTemporaryFile(suffix='.db', delete=False) as database:
        database.write(files['data3.db'])
    try:
        connection = sqlite3.connect(database.name)
        count = 0
        for key, value in connection.execute('SELECT key, value FROM blobs'):
            name = os.path.basename(key)
            if name.endswith('.png'):
                with open(os.path.join(icons, name), 'wb') as file:
                    file.write(descramble(value))
                count += 1
        connection.close()
    finally:
        os.unlink(database.name)
    print(f'{target}: gui.plx and {count} icons')


if __name__ == '__main__':
    main()
