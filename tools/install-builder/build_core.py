# Copyright (C) 2026 HOWL
# SPDX-License-Identifier: MIT
"""Private install assembly. Public templates remain strictly asset-free."""
from __future__ import annotations
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import sys
import tempfile
import threading
import zipfile

HERE = Path(__file__).resolve().parent
IMPORTER = HERE / 'asset-importer' if (HERE / 'asset-importer').is_dir() else HERE.parent / 'asset-importer'
sys.path.insert(0, str(IMPORTER))
import create_asset_pack

PACK_NAME = 'EncoreAssets-v1.encore-assets'

class Cancelled(Exception):
    pass

def checkpoint(cancel):
    if cancel.is_set():
        raise Cancelled('Build cancelled. No incomplete installation was published.')

def sha_file(path):
    return create_asset_pack.sha_file(path)

def within(path, parent):
    try:
        path.resolve().relative_to(parent.resolve())
        return True
    except ValueError:
        return False

def data_directory(folder):
    folder = Path(folder).expanduser().resolve()
    if (folder / 'resources.assets').is_file():
        return folder
    candidates = [p for p in folder.glob('*_Data') if (p / 'resources.assets').is_file()]
    if len(candidates) != 1:
        raise ValueError('Choose the installed PC game folder or its *_Data folder, not the launcher executable.')
    return candidates[0]

def detection_roots(environ=None, home=None):
    env = os.environ if environ is None else environ
    home = Path.home() if home is None else Path(home)
    roots = [home / 'Desktop', home / 'Downloads', home / 'Games',
             home / 'Applications', home / '.local/share/Clone Hero Launcher',
             home / '.config/Clone Hero Launcher']
    for key in ('APPDATA', 'LOCALAPPDATA'):
        if env.get(key):
            roots += [Path(env[key]) / 'Clone Hero Launcher', Path(env[key]) / 'Programs/Clone Hero']
    for key in ('ProgramFiles', 'ProgramFiles(x86)'):
        if env.get(key):
            roots += [Path(env[key]) / 'Clone Hero', Path(env[key]) / 'Clone Hero Launcher']
    if os.name == 'nt':
        drive = env.get('SystemDrive', 'C:') + '/'
        roots += [Path(drive) / 'Clone Hero', Path(drive) / 'Games']
    return roots

def detect_installations(roots=None, cancel=None):
    """Bounded directory search, including launcher-managed gameFiles/version dirs.

    Custom/portable installs are selectable with Browse. Never crawl whole drives,
    follow symlinks, or inspect songs, executables, credentials or user documents.
    """
    cancel = cancel or threading.Event()
    queue = [(Path(p), 0) for p in (roots if roots is not None else detection_roots())]
    seen, found = set(), set()
    skip = {'songs', 'song', 'node_modules', '.git', '.venv', 'playerdata', 'streamingassets',
            'unityplayer', 'mono', 'monobleedingedge'}
    for _ in range(2500):
        if not queue or cancel.is_set():
            break
        folder, depth = queue.pop(0)
        try:
            if folder.is_symlink() or folder.resolve() in seen:
                continue
            seen.add(folder.resolve())
            if (folder / 'resources.assets').is_file() and folder.name.endswith('_Data'):
                found.add(folder.resolve())
                continue
            if depth >= 5:
                continue
            with os.scandir(folder) as scan:
                children = sorted((Path(e.path) for e in scan if e.is_dir(follow_symlinks=False)
                                   and e.name.lower() not in skip), key=lambda p: p.name.lower())
            queue.extend((p, depth + 1) for p in children)
        except (OSError, ValueError):
            continue
    return sorted(found, key=lambda p: str(p).lower())

def safe_name(name):
    p = PurePosixPath(name)
    reserved = {'con', 'prn', 'aux', 'nul', *(f'com{i}' for i in range(1, 10)), *(f'lpt{i}' for i in range(1, 10))}
    if (not name or len(name) > 500 or '\\' in name or ':' in name or p.is_absolute()
            or any(c in name for c in '\x00\r\n') or
            any(x in ('', '.', '..') or x[-1:] in (' ', '.') or x.split('.')[0].lower() in reserved
                for x in name.rstrip('/').split('/'))):
        raise ValueError('Unsafe archive entry: ' + repr(name))
    return p

def template_config(bundle):
    config = json.loads((bundle / 'template.json').read_text(encoding='utf-8'))
    if config.get('schema') != 1 or config.get('title_id') != 'PPSA98743' or not config.get('bundled_assets_supported'):
        raise ValueError('Use an official desktop-builder template with automatic personal-asset setup.')
    recipes = create_asset_pack.load_recipes(IMPORTER / 'recipes.json')
    if config.get('catalog') != recipes['catalog']:
        raise ValueError('The template and importer asset versions differ. Download the complete builder again.')
    if not config.get('files') or not isinstance(config['files'], dict):
        raise ValueError('Missing template file checksums.')
    return config

def unpack_template(bundle, destination, config, cancel):
    archive = bundle / 'base-game.zip'
    if sha_file(archive) != config['zip_sha256']:
        raise ValueError('The game template is damaged. Download the builder again.')
    expected = config['files']; names = set(); folded = set(); total = 0
    with zipfile.ZipFile(archive) as z:
        for info in z.infolist():
            safe_name(info.filename)
            if info.is_dir():
                continue
            path = PurePosixPath(info.filename)
            if (path.parts[0] != config['title_id'] or len(path.parts) < 2 or
                    stat.S_ISLNK(info.external_attr >> 16) or info.filename.lower() in folded):
                raise ValueError('Invalid game template entry.')
            relative = '/'.join(path.parts[1:])
            if relative not in expected or info.file_size > 256 * 1024**2:
                raise ValueError('Unexpected game template file: ' + relative)
            total += info.file_size
            if total > 512 * 1024**2:
                raise ValueError('Game template exceeds the size limit.')
            names.add(relative); folded.add(info.filename.lower())
        if names != set(expected):
            raise ValueError('Game template is missing required files.')
        for info in z.infolist():
            if info.is_dir():
                continue
            checkpoint(cancel)
            relative = '/'.join(PurePosixPath(info.filename).parts[1:])
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            h = hashlib.sha256()
            with z.open(info) as source, target.open('xb') as output:
                while chunk := source.read(1024 * 1024):
                    checkpoint(cancel); h.update(chunk); output.write(chunk)
            if h.hexdigest() != expected[relative]:
                raise ValueError('Game template checksum failed: ' + relative)
            target.chmod(0o755 if relative.endswith(('.bin', '.prx', '.sprx')) else 0o644)

def verify_pack(path, catalog, files):
    import struct
    with path.open('rb') as f:
        if f.read(8) != b'CHASSET1':
            raise ValueError('Invalid personal asset pack.')
        raw = f.read(4)
        if len(raw) != 4:
            raise ValueError('Incomplete asset pack.')
        length, = struct.unpack('<I', raw)
        if not 0 < length <= 512 * 1024:
            raise ValueError('Invalid asset manifest size.')
        manifest = json.loads(f.read(length))
        if manifest != {'schema': 1, 'catalog': catalog, 'files': files}:
            raise ValueError('Asset pack manifest does not match this export.')
        for record in files:
            h = hashlib.sha256(); remaining = record['size']
            while remaining:
                data = f.read(min(1024 * 1024, remaining))
                if not data:
                    raise ValueError('Truncated asset pack.')
                h.update(data); remaining -= len(data)
            if h.hexdigest() != record['sha256']:
                raise ValueError('Personal asset checksum failed.')
        if f.read(1):
            raise ValueError('Unexpected data after the asset pack.')

def run_process(command, cancel, log, env=None):
    # File-backed output avoids pipe deadlocks and blocks neither cancellation nor UI.
    with tempfile.TemporaryFile(mode='w+b') as output:
        kwargs = {'creationflags': subprocess.CREATE_NO_WINDOW} if os.name == 'nt' else {}
        process = subprocess.Popen([str(x) for x in command], stdout=output, stderr=subprocess.STDOUT, env=env, **kwargs)
        try:
            while process.poll() is None:
                if cancel.wait(.1):
                    process.terminate()
                    try: process.wait(timeout=10)
                    except subprocess.TimeoutExpired: process.kill(); process.wait()
                    checkpoint(cancel)
            output.seek(0); result = output.read().decode('utf-8', errors='replace')
            if result.strip(): log(result.strip())
            if process.returncode:
                raise ValueError('Packaging tool failed. See the build log above.')
            return result
        finally:
            if process.poll() is None:
                process.kill(); process.wait()

def build(game, output_parent, kind, bundle=HERE, cancel=None, log=print, pkg_tool=None, cache=None):
    from backends import package, prepare_backend
    cancel = cancel or threading.Event()
    if kind not in ('folder', 'exfat', 'pkg'):
        raise ValueError('Choose Folder, exFAT or PKG.')
    game = data_directory(game); bundle = Path(bundle).resolve(); parent = Path(output_parent).expanduser().resolve()
    # Validate before creating anything, including cache or temporary directories.
    if within(parent, game.parent):
        raise ValueError('Choose an output folder outside the PC game installation.')
    if parent == bundle or within(parent, IMPORTER):
        raise ValueError('Choose a separate output folder, not the builder source directory.')
    config = template_config(bundle)
    name = 'CH_Encore_' + config['version'] + '_Personal_' + kind
    if any(c not in 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-' for c in name):
        raise ValueError('Invalid template version.')
    destination = parent / name
    if destination.exists():
        raise ValueError('That personal build already exists. Choose a different output folder or move the old build first.')
    checkpoint(cancel)
    log('Checking your PC asset version...')
    create_asset_pack.verify_source(game, create_asset_pack.load_recipes(IMPORTER / 'recipes.json'), lambda: checkpoint(cancel))
    backend = prepare_backend(kind, cache, pkg_tool, cancel, log)
    parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.encore-build-', dir=parent) as temporary:
        work = Path(temporary); result = work / 'finished'; result.mkdir()
        app = work / config['title_id']; app.mkdir()
        log('Checking and unpacking the asset-free game template...')
        unpack_template(bundle, app, config, cancel)
        checkpoint(cancel)
        pack = app / 'assets' / PACK_NAME
        log('Reading your local PC assets. The original installation stays unchanged...')
        files = create_asset_pack.export(game, pack, IMPORTER / 'recipes.json', log=log, check=lambda: checkpoint(cancel))
        checkpoint(cancel)
        verify_pack(pack, config['catalog'], files)
        log('Personal assets verified. Creating your ' + kind + ' install...')
        if kind == 'folder':
            shutil.move(str(app), result / config['title_id'])
            artifact = result / config['title_id']
        else:
            artifact = result / ('CH_Encore_' + config['version'] + '.' + kind)
            package(kind, app, artifact, backend, cancel, log)
        checkpoint(cancel)
        (result / 'README.txt').write_text(
            'CH: Encore ' + config['version'] + ' — personal ' + kind + ' install\n\n'
            'Install the title folder or image/package using your compatible PS5 setup.\n'
            'Your assets are already inside it. First launch prepares them automatically;\n'
            'no loose asset file, FTP transfer or console import selection is needed.\n'
            'Songs are not included. Existing songs, scores and profiles are preserved.\n'
            'This output contains your imported PC resources. Keep it private; do not\n'
            'upload it to GitHub, share it as a release, or attach it to bug reports.\n'
            'The public builder and game template contain no original PC resources.\n', encoding='utf-8')
        report = {'schema': 1, 'version': config['version'], 'source': config['source'], 'format': kind,
                  'personal_assets': True, 'redistribute': False, 'catalog': config['catalog'],
                  'asset_files': len(files), 'asset_pack_sha256': sha_file(artifact / 'assets' / PACK_NAME) if kind == 'folder' else sha_file(pack)}
        (result / 'build-info.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
        checks = {p.relative_to(result).as_posix(): sha_file(p) for p in sorted(result.rglob('*')) if p.is_file()}
        (result / 'SHA256SUMS').write_text(''.join(h + '  ' + n + '\n' for n, h in checks.items()), encoding='utf-8')
        checkpoint(cancel)
        if destination.exists():
            raise ValueError('Output was created by another build. Nothing was overwritten.')
        result.rename(destination)
    log('Complete: ' + str(destination))
    return destination
