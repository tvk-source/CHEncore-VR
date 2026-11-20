#!/usr/bin/env python3
# Copyright (C) 2026 HOWL
# SPDX-License-Identifier: MIT
"""Bundle the personal install builder with a verified asset-free game template.

This is a public artifact. Never point this at a personal install output.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/install-builder'))
from build_core import sha_file
import create_asset_pack


def make_bundle(app, output):
    app, output = app.resolve(), output.resolve()
    if output.exists():
        raise ValueError('Output already exists; keep published artifacts immutable.')
    if app in output.parents:
        raise ValueError('Put the public builder archive outside the application folder.')
    subprocess.run([sys.executable, str(ROOT / 'ps5/tools/check-no-pc-assets.py'), '--app', str(app)], check=True)
    subprocess.run([sys.executable, str(ROOT / 'ps5/tools/check-no-pc-assets.py')], check=True)
    revision = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    if subprocess.check_output(['git', '-C', str(ROOT), 'status', '--porcelain', '--untracked-files=all'], text=True).strip():
        raise ValueError('Commit the complete builder and game source, then rebuild before packaging.')
    parts = json.loads((app / 'licenses/components.json').read_text())
    title = next(part for part in parts if part['id'] == 'title')
    if title['source']['revision'] != revision or any(part['source'].get('dirty') for part in parts):
        raise ValueError('The game template needs a clean build of this exact source commit.')
    # Refuse old releases whose first-run code cannot auto-install a bundled pack.
    if not any(b'PREPARING YOUR INSTALLATION' in p.read_bytes()
               for p in (app / 'eboot.bin', app / 'sce_module/encore-engine.prx')):
        raise ValueError('The game template does not support personal builder installations.')
    param = json.loads((app / 'sce_sys/param.json').read_text())
    if param['titleId'] != 'PPSA98743': raise ValueError('Unexpected title ID.')
    code = int(param['contentVersion'].split('.')[1]) * 1000 + int(param['contentVersion'].split('.')[2])
    if code < 10503: raise ValueError('Personal templates require v1.5.1 or newer.')
    major, rem = divmod(code, 10000); minor, rem = divmod(rem, 100); patch = rem // 2
    version = f'{major}.{minor}.{patch}'
    catalog = create_asset_pack.load_recipes(ROOT / 'tools/asset-importer/recipes.json')['catalog']
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='encore-public-builder-', dir=output.parent) as temporary:
        work = Path(temporary); bundle = work / 'CH_Encore_Desktop_Builder'; bundle.mkdir()
        for path in (ROOT / 'tools/install-builder').iterdir():
            if path.is_file() and path.suffix in ('.py', '.txt', '.cmd', '.sh'):
                shutil.copy2(path, bundle / path.name)
        importer = bundle / 'asset-importer'; importer.mkdir()
        for name in ('create_asset_pack.py', 'recipes.json', 'requirements.txt', 'LICENSE.txt'):
            shutil.copy2(ROOT / 'tools/asset-importer' / name, importer / name)
        shutil.copy2(ROOT / 'LICENSE.md', bundle / 'LICENSE.txt')
        shutil.copy2(ROOT / 'docs/RIGHTS_AND_DISTRIBUTION.md', bundle / 'RIGHTS_AND_DISTRIBUTION.md')
        template = bundle / 'base-game.zip'
        files = {}
        with zipfile.ZipFile(template, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for path in sorted(app.rglob('*')):
                if path.is_symlink(): raise ValueError('No symlinks in the public template.')
                if path.is_file():
                    name = path.relative_to(app).as_posix(); files[name] = sha_file(path)
                    z.write(path, 'PPSA98743/' + name)
            if z.testzip(): raise ValueError('Game template ZIP failed its CRC check.')
        config = {'schema': 1, 'title_id': 'PPSA98743', 'version': version, 'source': revision,
                  'bundled_assets_supported': True, 'catalog': catalog, 'zip_sha256': sha_file(template), 'files': files}
        (bundle / 'template.json').write_text(json.dumps(config, indent=2) + '\n', encoding='utf-8')
        # A second audit catches accidental loose resource additions to the builder.
        subprocess.run([sys.executable, str(ROOT / 'ps5/tools/check-no-pc-assets.py'), '--app', str(bundle)], check=True)
        pending = work / 'builder.zip'
        with zipfile.ZipFile(pending, 'w', zipfile.ZIP_DEFLATED, compresslevel=6) as z:
            for path in sorted(bundle.rglob('*')):
                if path.is_file(): z.write(path, path.relative_to(work).as_posix())
            if z.testzip(): raise ValueError('Builder ZIP failed its CRC check.')
        pending.replace(output)
    return {'version': version, 'source': revision, 'template_files': len(files), 'sha256': sha_file(output),
            'original_pc_assets_included': False, 'bytes': output.stat().st_size}

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', type=Path, default=ROOT / 'dist/PPSA98743')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(make_bundle(args.app, args.output), indent=2))
