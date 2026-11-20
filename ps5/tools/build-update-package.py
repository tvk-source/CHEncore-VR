#!/usr/bin/env python3
"""Build the bounded, hashed application-only package consumed by the updater."""
# Copyright (C) 2026 HOWL
# SPDX-License-Identifier: MIT
import argparse, hashlib, json, struct, subprocess, sys
from pathlib import Path

def build(folder: Path, output: Path):
    subprocess.run([sys.executable,str(Path(__file__).with_name('check-no-pc-assets.py')),'--app',str(folder)],check=True)
    param=json.loads((folder/'sce_sys/param.json').read_text())
    assert param['titleId']=='PPSA98743'
    parts=param['contentVersion'].split('.');assert len(parts)==3 and parts[0]=='01'
    version=int(parts[1])*1000+int(parts[2]);assert 0<version<=999999
    files=[]
    for path in sorted(folder.rglob('*')):
        assert not path.is_symlink(),f'No symlinks in update packages: {path}'
        if not path.is_file():continue
        name=path.relative_to(folder).as_posix()
        assert name in ('eboot.bin','LEGAL.txt') or name.startswith(('assets/','shaders/','licenses/','sce_sys/','sce_module/')),name
        assert len(name)<=220 and all(ord(c)>=32 and ord(c)!=127 for c in name) and not any(c in name for c in '\\:\r\n')
        assert not any(p in ('.env','userdata','screenshots','klog','test-run.txt','test-results.txt','radv-shader-cache','songs','pc_bundled_songs') for p in path.relative_to(folder).parts),name
        digest=hashlib.sha256()
        with path.open('rb') as f:
            for block in iter(lambda:f.read(1024*1024),b''):digest.update(block)
        files.append(dict(path=name,size=path.stat().st_size,sha256=digest.hexdigest()))
    assert 0<len(files)<=4096 and sum(f['size'] for f in files)<=2*1024**3
    assert all(f['size']<=512*1024**2 for f in files)
    if version>=10000:assert any(f['path']=='sce_module/encore-engine.prx' for f in files),'Beta runtime updates require the native engine module'
    manifest=json.dumps(dict(schema=1,title='PPSA98743',version=version,files=files),separators=(',',':'),ensure_ascii=False).encode()
    assert len(manifest)<=2*1024**2
    output.parent.mkdir(parents=True,exist_ok=True)
    tmp=output.with_name(output.name+'.tmp')
    with tmp.open('wb') as out:
        out.write(b'CHENUP1\n'+struct.pack('<I',len(manifest))+manifest)
        for entry in files:
            digest=hashlib.sha256();count=0
            with (folder/entry['path']).open('rb') as source:
                for block in iter(lambda:source.read(1024*1024),b''):out.write(block);digest.update(block);count+=len(block)
            assert digest.hexdigest()==entry['sha256'] and count==entry['size'],'Application changed while packaging'
    tmp.replace(output)
    return dict(version=version,files=len(files),bytes=output.stat().st_size)
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('folder',type=Path);parser.add_argument('output',type=Path);args=parser.parse_args()
    print(json.dumps(build(args.folder,args.output)))
