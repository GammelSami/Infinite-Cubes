#!/usr/bin/env python3
"""Exercise /time, /tp, and /level against a temporary Linux Alpha server.

Usage: python3 alpha/tests/test_server_commands.py /path/to/alpha-game-data
"""
import argparse
import os
from pathlib import Path
import shutil
import socket
import struct
import subprocess
import tempfile
import time
import zlib

ASSETS = ('data1.db', 'data2.db', 'data3.db', 'data4.db', 'db.dat', 'resource1.dat',
          'resource2.dat', 'logo.bmp', 'cursor.plx', 'gui.plx', 'help.plx',
          'quest-tag.plx', 'start.plx')
PORT_REDIRECT = r'''
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <sys/socket.h>
int bind(int fd, const struct sockaddr *address, socklen_t length) {
    static int (*real_bind)(int, const struct sockaddr *, socklen_t);
    if (!real_bind) real_bind = dlsym(RTLD_NEXT, "bind");
    if (address && address->sa_family == AF_INET && length >= sizeof(struct sockaddr_in)) {
        struct sockaddr_in changed = *(const struct sockaddr_in *)address;
        if (ntohs(changed.sin_port) == 12345) {
            changed.sin_port = htons(atoi(getenv("CUBE_TEST_PORT")));
            return real_bind(fd, (const struct sockaddr *)&changed, sizeof(changed));
        }
    }
    return real_bind(fd, address, length);
}
'''


def exact(sock, count):
    data = b''
    while len(data) < count:
        part = sock.recv(count - len(data))
        if not part:
            raise RuntimeError('server closed the connection')
        data += part
    return data


def number(sock):
    return struct.unpack('<i', exact(sock, 4))[0]


def packet(sock):
    kind = number(sock)
    if kind in (0, 4):
        exact(sock, number(sock))
        return kind, None
    if kind == 2:
        return kind, None
    if kind == 5:
        return kind, (number(sock), number(sock))
    if kind == 10:
        sender = struct.unpack('<q', exact(sock, 8))[0]
        return kind, (sender, exact(sock, number(sock) * 2).decode('utf-16-le'))
    if kind in (15, 20):
        return kind, number(sock)
    if kind == 19:
        return kind, struct.unpack('<qqq', exact(sock, 24))
    raise AssertionError(f'unexpected server packet: {kind}')


def until(sock, predicate):
    deadline = time.monotonic() + 8
    while time.monotonic() < deadline:
        kind, value = packet(sock)
        if predicate(kind, value):
            return value
    raise AssertionError('expected server packet did not arrive')


def connect(port, clients, version=4):
    sock = socket.create_connection(('127.0.0.1', port), timeout=8)
    sock.settimeout(8)
    clients.append(sock)
    sock.sendall(struct.pack('<ii', 17, version))
    return sock


def join(port, clients):
    sock = connect(port, clients)
    assert number(sock) == 16  # Welcome
    assert number(sock) == 0  # Full entity data
    player_id = struct.unpack('<q', exact(sock, 8))[0]
    exact(sock, 4456)  # Linux x86-64 EntityData
    assert number(sock) == 15  # Seed
    number(sock)
    return sock, player_id


def entity(sock, player_id, name, position):
    data = struct.pack('<qQqqq', player_id, (1 << 0) | (1 << 45),
                       *(coordinate * 65536 for coordinate in position))
    data += name.encode('ascii')[:15].ljust(16, b'\0')
    compressed = zlib.compress(data)
    sock.sendall(struct.pack('<ii', 0, len(compressed)) + compressed)


def chat(sock, text):
    data = text.encode('utf-16-le')
    sock.sendall(struct.pack('<ii', 10, len(data) // 2) + data)


def expect_reply(sock, text):
    until(sock, lambda kind, value: kind == 10 and value[1] == text)


def expect_command(sock, packet_id, payload, reply):
    got_packet = got_reply = False
    deadline = time.monotonic() + 8
    while time.monotonic() < deadline and not (got_packet and got_reply):
        kind, value = packet(sock)
        if kind == packet_id:
            assert value == payload, (kind, value)
            got_packet = True
        if kind == 10 and value[1] == reply:
            got_reply = True
    assert got_packet and got_reply, (got_packet, got_reply)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('data', type=Path, help='folder with the Alpha game data files')
    parser.add_argument('--server', type=Path,
                        default=Path(__file__).resolve().parents[1] / 'build-linux/Server')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='cube-server-commands-') as temporary:
        root = Path(temporary)
        shutil.copy2(args.server, root / 'Server')
        (root / 'Save').mkdir()
        for name in ASSETS:
            (root / name).symlink_to((args.data / name).resolve(strict=True))
        source = root / 'redirect.c'
        source.write_text(PORT_REDIRECT)
        preload = root / 'redirect.so'
        subprocess.run(['cc', '-shared', '-fPIC', str(source), '-o', str(preload), '-ldl'], check=True)
        with socket.socket() as available:
            available.bind(('127.0.0.1', 0))
            port = available.getsockname()[1]
        env = dict(os.environ, LD_PRELOAD=str(preload), CUBE_TEST_PORT=str(port))
        log = (root / 'server.log').open('w')
        server = subprocess.Popen([str(root / 'Server')], cwd=root, env=env,
                                  stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT)
        clients = []
        try:
            deadline = time.monotonic() + 15
            while True:
                try:
                    old = connect(port, clients, version=3)
                    break
                except ConnectionRefusedError:
                    if server.poll() is not None or time.monotonic() > deadline:
                        raise RuntimeError('server did not start')
                    time.sleep(.1)
            assert (number(old), number(old)) == (17, 4)  # Protocol mismatch
            old.close()
            alice, aid = join(port, clients)
            bob, bid = join(port, clients)
            assert (aid, bid) == (1, 2)
            entity(alice, aid, 'Alice', (100, 100, 100))
            entity(bob, bid, 'Bob', (200, 200, 200))
            time.sleep(.2)
            chat(alice, '/time dawn')
            expect_reply(alice, 'Time set to dawn.')
            until(bob, lambda kind, value: kind == 5 and
                  6 * 3600000 <= value[1] < 6 * 3600000 + 300000)
            chat(alice, '/tp Alice Bob')
            expect_command(alice, 19, (200 * 65536,) * 3, 'Teleported Alice to Bob.')
            chat(alice, '/tp Alice Missing')
            expect_reply(alice, 'Usage: /tp <online player> <online player> (exact names)')
            chat(alice, '/level 50')
            expect_command(alice, 20, 50, 'Level set to 50.')
            chat(alice, '/level 0')
            expect_reply(alice, 'Usage: /level <1-1000>')
            print('PASS: protocol version, /time, /tp, /level, invalid input')
        finally:
            for client in clients:
                client.close()
            server.terminate()
            try:
                server.wait(timeout=3)
            except subprocess.TimeoutExpired:
                server.kill()
                server.wait()
            log.close()


if __name__ == '__main__':
    main()
