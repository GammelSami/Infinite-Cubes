#!/usr/bin/env python3
"""The skill editor: a web page for the mod's classes.json, for game designers.

    tools/skill-editor/server.py [--data classes.json] [--icons alpha/icons] [--port 8765] [--open]

serves http://localhost:8765/ and saves every change to the data file at once. The game reads the file again when it
changes, so values can be tuned while it runs. Only the Python standard library is needed.

Defaults: the data file is release/data/mod/classes.json of this repository, the icons are those
tools/import-alpha-assets.py wrote into a runtime folder (looked for in ~/Games/InfiniteCubes/mod-linux/alpha/icons).
"""
import argparse
import json
import os
import re
import sys
import tempfile
import webbrowser
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import unquote

HERE = os.path.dirname(os.path.abspath(__file__))
REPOSITORY = os.path.dirname(os.path.dirname(HERE))
DEFAULT_DATA = os.path.join(REPOSITORY, 'release', 'data', 'mod', 'classes.json')
DEFAULT_ICONS = [os.path.expanduser('~/Games/InfiniteCubes/mod-linux/alpha/icons')]
# The abilities classes.json can name; release/src/mod/ClassData.cpp has the list (ABILITY(Name)), this is for a copy
# of the editor without the sources.
FALLBACK_ABILITIES = [
    'Smash', 'Cyclone', 'WarFrenzy', 'Bulwark', 'RangerKick', 'Retreat', 'Swiftness', 'Aim', 'HealingStream',
    'FireExplosion', 'ManaShield', 'Teleport', 'Intercept', 'Sneak', 'Camouflage', 'Shuriken', 'HeroicShout',
    'FireMissiles', 'WaterBubbles', 'ShadowClone', 'QuicksandTrap', 'Ninjutsu', 'FlameRush', 'ThrowPoisonVial',
]


def abilities():
    source = os.path.join(REPOSITORY, 'release', 'src', 'mod', 'ClassData.cpp')
    try:
        names = re.findall(r'ABILITY\((\w+)\)', open(source, encoding='utf-8').read())
        names = [name for name in names if name != 'name']
        if names:
            return names
    except OSError:
        pass
    return FALLBACK_ABILITIES


def write_atomically(path, text):
    """Writes through a file renamed over the old one: the game never reads a half-written file."""
    directory = os.path.dirname(os.path.abspath(path))
    handle, temporary = tempfile.mkstemp(dir=directory, prefix='.classes-', suffix='.json')
    try:
        with os.fdopen(handle, 'w', encoding='utf-8') as file:
            file.write(text)
        os.replace(temporary, path)
    except BaseException:
        if os.path.exists(temporary):
            os.unlink(temporary)
        raise


def make_handler(data_path, icon_dir):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, format, *args):
            pass

        def send(self, status, body, content_type):
            self.send_response(status)
            self.send_header('Content-Type', content_type)
            self.send_header('Content-Length', str(len(body)))
            self.send_header('Cache-Control', 'no-store')
            self.end_headers()
            self.wfile.write(body)

        def send_json(self, value, status=HTTPStatus.OK):
            self.send(status, json.dumps(value).encode('utf-8'), 'application/json; charset=utf-8')

        def do_GET(self):
            path = unquote(self.path.split('?', 1)[0])
            if path in ('/', '/index.html'):
                self.send(HTTPStatus.OK, open(os.path.join(HERE, 'index.html'), 'rb').read(), 'text/html; charset=utf-8')
            elif path == '/api/data':
                self.send(HTTPStatus.OK, open(data_path, 'rb').read(), 'application/json; charset=utf-8')
            elif path == '/api/meta':
                icons = sorted(name[:-4] for name in os.listdir(icon_dir) if name.endswith('.png')) if icon_dir else []
                self.send_json({'abilities': abilities(), 'icons': icons, 'dataPath': data_path,
                                'iconDir': icon_dir or ''})
            elif path.startswith('/icons/') and icon_dir:
                name = os.path.basename(path[len('/icons/'):])
                file = os.path.join(icon_dir, name)
                if name.endswith('.png') and os.path.isfile(file):
                    self.send(HTTPStatus.OK, open(file, 'rb').read(), 'image/png')
                else:
                    self.send(HTTPStatus.NOT_FOUND, b'', 'text/plain')
            else:
                self.send(HTTPStatus.NOT_FOUND, b'not found', 'text/plain')

        def do_PUT(self):
            if self.path != '/api/data':
                self.send(HTTPStatus.NOT_FOUND, b'not found', 'text/plain')
                return
            length = int(self.headers.get('Content-Length', '0'))
            try:
                value = json.loads(self.rfile.read(length).decode('utf-8'))
                if not isinstance(value, dict) or not isinstance(value.get('skills'), dict):
                    raise ValueError('classes.json needs an object with "skills"')
            except ValueError as error:
                self.send_json({'error': str(error)}, HTTPStatus.BAD_REQUEST)
                return
            write_atomically(data_path, json.dumps(value, indent=2, ensure_ascii=False) + '\n')
            self.send_json({'saved': True})

    return Handler


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--data', default=DEFAULT_DATA, help='classes.json to edit')
    parser.add_argument('--icons', default=None, help='folder of the alpha icons (tools/import-alpha-assets.py)')
    parser.add_argument('--port', type=int, default=8765)
    parser.add_argument('--open', action='store_true', help='open the page in the browser')
    arguments = parser.parse_args()
    data_path = os.path.abspath(os.path.realpath(arguments.data))
    if not os.path.isfile(data_path):
        sys.exit(f'{data_path}: not found')
    icon_dir = arguments.icons or next((folder for folder in DEFAULT_ICONS if os.path.isdir(folder)), None)
    if icon_dir and not os.path.isdir(icon_dir):
        sys.exit(f'{icon_dir}: not found')
    # Only this computer can reach the editor.
    server = ThreadingHTTPServer(('127.0.0.1', arguments.port), make_handler(data_path, icon_dir))
    url = f'http://localhost:{arguments.port}/'
    print(f'Skill editor: {url}  (data {data_path}, icons {icon_dir or "none"})')
    if arguments.open:
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == '__main__':
    main()
