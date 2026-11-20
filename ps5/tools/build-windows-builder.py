#!/usr/bin/env python3
# Copyright (C) 2026 HOWL
# SPDX-License-Identifier: MIT
"""Freeze a verified desktop-builder bundle into one Windows EXE.

Run with native Windows CPython 3.12 x64 and PyInstaller 6.16.0. --packwright
must be an unmodified self-contained win-x64 publish of the pinned source.
The executable embeds exFAT tools, their complete source/notices and Python.
PKG uses a verified first-use download of the native writer, then its cache.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import shutil
import subprocess
import sys
import tempfile
import zipfile

PACKWRIGHT_REV = '7039684af5c978ae69ee619d44e98102a61104a6'

def digest(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
    return h.hexdigest()

def build(bundle, tools, source, output):
    import importlib.metadata as metadata
    if sys.version_info[:2] != (3,12) or platform.architecture()[0] != '64bit' or sys.platform != 'win32':
        raise ValueError('Freeze with native Windows CPython 3.12 x64. End users need no Python installation.')
    if metadata.version('pyinstaller') != '6.16.0': raise ValueError('Use PyInstaller 6.16.0.')
    if output.exists(): raise ValueError('Keep published outputs immutable; select a new path.')
    if not (tools/'Packwright.exe').is_file(): raise ValueError('Select the complete win-x64 self-contained Packwright publish.')
    template=json.loads((bundle/'template.json').read_text())
    if digest(bundle/'base-game.zip') != template['zip_sha256']: raise ValueError('Invalid game template.')
    # Inspect the complete source archive before redistributing its tool binary.
    with zipfile.ZipFile(source) as z:
        names=z.namelist()
        if not any(n.endswith('/src/Packwright.App/Packwright.App.csproj') for n in names):
            raise ValueError('Missing corresponding Packwright source.')
        if not any(PACKWRIGHT_REV in n.split('/')[0] for n in names):
            raise ValueError('Use the pinned Packwright source archive.')
    output.parent.mkdir(parents=True,exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='encore-freeze-',dir=output.parent) as temporary:
        work=Path(temporary);payload=work/'payload';shutil.copytree(bundle,payload)
        embedded=payload/'embedded-tools';embedded.mkdir()
        shutil.copytree(tools,embedded/'packwright')
        shutil.copy2(source,embedded/'Packwright-source.zip')
        manifest={'schema':1,'engine':'packwright','version':'1.3.0','source':PACKWRIGHT_REV,
                  'executable':'packwright/Packwright.exe','pkg_policy':'verified-download','files':{}}
        for path in sorted(embedded.rglob('*')):
            if path.is_file():manifest['files'][path.relative_to(embedded).as_posix()]=digest(path)
        (embedded/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
        # Retain complete installed distribution notices, plus Python's licence.
        notices=payload/'desktop-licenses';notices.mkdir()
        for dist in metadata.distributions():
            name=dist.metadata.get('Name','unknown')
            target=notices/name;target.mkdir(exist_ok=True)
            (target/'METADATA.txt').write_text(dist.read_text('METADATA') or '',encoding='utf-8')
            for record in dist.files or []:
                if any(x in str(record).lower() for x in ('license','copying','notice')):
                    path=Path(dist.locate_file(record))
                    if path.is_file():shutil.copy2(path,target/(hashlib.sha256(str(record).encode()).hexdigest()[:8]+'-'+path.name))
        python_license=Path(sys.base_prefix)/'LICENSE.txt'
        if python_license.is_file():shutil.copy2(python_license,notices/'Python-LICENSE.txt')
        args=[sys.executable,'-m','PyInstaller','--noconfirm','--clean','--onefile','--windowed','--noupx',
              '--name','CH_Encore_Install_Builder','--distpath',str(work/'dist'),'--workpath',str(work/'build'),
              '--specpath',str(work),'--paths',str(payload),'--paths',str(payload/'asset-importer')]
        from PIL import Image
        with zipfile.ZipFile(payload/'base-game.zip') as z:
            icon=work/'icon.png';icon.write_bytes(z.read('PPSA98743/sce_sys/icon0.png'))
        with Image.open(icon) as image:image.save(work/'builder.ico',sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
        args += ['--icon',str(work/'builder.ico')]
        for package in ['UnityPy','PIL','lz4','texture2ddecoder','etcpak','astc_encoder','archspec','fmod_toolkit','pyfmodex','brotli']:
            args+=['--collect-all',package]
        for name in ['base-game.zip','template.json','README.txt','RIGHTS_AND_DISTRIBUTION.md','LICENSE.txt','asset-importer','embedded-tools','desktop-licenses']:
            args+=['--add-data',str(payload/name)+';'+(name if (payload/name).is_dir() else '.')]
        args += ['--hidden-import','create_asset_pack',str(payload/'builder.py')]
        subprocess.run(args,check=True)
        executable=work/'dist/CH_Encore_Install_Builder.exe'
        if not executable.is_file():raise ValueError('PyInstaller did not produce the executable.')
        shutil.copy2(executable,output)
    return {'exe':output.name,'bytes':output.stat().st_size,'sha256':digest(output),
            'game_source':template['source'],'packwright_source':PACKWRIGHT_REV,'single_file':True,'python_install_required':False,
            'folder_exfat_downloads_required':False,'pkg_first_use_download_required':True,
            'original_pc_resources_included':False}

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bundle',type=Path,required=True)
    p.add_argument('--packwright',type=Path,required=True);p.add_argument('--packwright-source',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    print(json.dumps(build(a.bundle.resolve(),a.packwright.resolve(),a.packwright_source.resolve(),a.output.resolve()),indent=2))
