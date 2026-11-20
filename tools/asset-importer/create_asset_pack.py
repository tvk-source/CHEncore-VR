#!/usr/bin/env python3
# Copyright (C) 2026 HOWL
# SPDX-License-Identifier: MIT
"""Read a user's PC installation and build the data-only CH: Encore asset pack."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import tempfile


def sha_file(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for block in iter(lambda:f.read(1024*1024),b''):h.update(block)
    return h.hexdigest()


def load_recipes(path):
    recipes=json.loads(path.read_text(encoding='utf-8'))
    expected=recipes.pop('catalog')
    actual=hashlib.sha256(json.dumps(recipes,sort_keys=True,separators=(',',':')).encode()).hexdigest()
    if expected!=actual:raise ValueError('Importer recipes are damaged. Download the importer again.')
    recipes['catalog']=expected
    names=set()
    for asset in recipes['assets']:
        value=asset['path'];p=Path(value)
        if (not value or '\\' in value or ':' in value or p.is_absolute() or
            any(x in ('.','..') for x in value.split('/')) or value in names or
            p.suffix.lower() not in ('.png','.ttf','.opus')):raise ValueError('Invalid asset recipe')
        names.add(value)
    return recipes


def prepare_output(output,game_data):
    output=output.resolve()
    try:output.relative_to(game_data.resolve().parent)
    except ValueError:pass
    else:raise ValueError('Choose an output outside the PC installation.')
    if output.suffix.lower()!='.encore-assets':raise ValueError('The output filename must end in .encore-assets')
    output.parent.mkdir(parents=True,exist_ok=True)
    return output


# Alternative source container fingerprints, tied to the exact original output
# catalog. The Linux tester matched 187/187 individual assets; the six
# generated atlases must still pass the unchanged full export validation.
# This is not approval of arbitrary versions, mixed source sets, or resources.
LINUX_REFERENCE_CATALOG = '4034ef056a904afe3d73bfca757a9b0de88e673f4f28e64bc1df4d2265f9b375'
LINUX_SOURCE_FILES = {
    "globalgamemanagers.assets": "af8450ecd6b11f6105d471a6a6a4cff1ea9c7703fadf114959e8098b584117f8",
    "resources.assets": "93fcf0de1b837c5ecf7c06b36fc6c136dda9fa807c9b47de0d3c6e625c6b7c1e",
    "sharedassets0.assets": "3a56342833ee872447ea22e5f5f910422d232cbe3e1dff90d45d0b5383be520d",
    "sharedassets1.assets": "6538aa2b1aff1c2cce4c7133fe2072f540af342dfbb3c84862656c9869f09903",
    "sharedassets2.assets": "add1ccfb22456b14fd7d918cd0b20c81eb46d8726d6482fb250cc5fb889f5642"
}

def source_profile(actual, recipes):
    if actual == recipes['source_files']:
        return 'original'
    if (recipes.get('catalog') == LINUX_REFERENCE_CATALOG
            and actual == LINUX_SOURCE_FILES):
        return 'linux-reference'
    return None


def verify_source(game_data,recipes,check=lambda:None):
    game_data=Path(game_data).resolve()
    if not (game_data/'resources.assets').is_file():
        choices=[p for p in game_data.glob('*_Data') if (p/'resources.assets').is_file()]
        if len(choices)!=1:raise ValueError('Select the PC installation folder or its *_Data folder.')
        game_data=choices[0]
    actual={}
    for name in recipes['source_files']:
        check()
        source=game_data/name
        if not source.is_file():
            raise ValueError('Missing PC asset file: '+name+'. The original installation is unchanged.')
        actual[name]=sha_file(source)
    if source_profile(actual,recipes) is None:
        raise ValueError('Unsupported PC asset fingerprint set. The original installation is unchanged.')
    return game_data


def export(game_data,output,recipe_path,*,log=print,check=lambda:None):
    check()
    try:
        import UnityPy
        from PIL import Image
    except ImportError as exc:
        raise ValueError('Install the importer dependencies first: python -m pip install -r requirements.txt') from exc
    if UnityPy.__version__!='1.25.4':raise ValueError('This importer requires UnityPy 1.25.4; use requirements.txt.')
    recipes=load_recipes(recipe_path)
    game_data=verify_source(game_data,recipes,check)
    log('PC assets verified. Exporting your personal asset pack...')
    env=UnityPy.load(*(str(game_data/name) for name in recipes['source_files']))
    objects={(o.assets_file.name,o.path_id):o for o in env.objects}
    def source_image(record):
        obj=objects[record['file'],record['id']]
        if obj.type.name!=record['type']:raise ValueError('PC object type mismatch')
        return obj.read().image.convert('RGBA')
    output=prepare_output(output,game_data)
    with tempfile.TemporaryDirectory(prefix='ch-assets-',dir=output.parent) as temporary:
        work=Path(temporary);files=[]
        for i,r in enumerate(recipes['assets']):
            check()
            target=work/r['path'];target.parent.mkdir(parents=True,exist_ok=True)
            if r['kind']=='copy':
                original=game_data/r['file']
                if sha_file(original)!=r['sha256']:raise ValueError('PC sound file is missing or damaged: '+r['file'])
                shutil.copyfile(original,target)
            elif r['kind']=='font':
                obj=objects[r['source']['file'],r['source']['id']]
                if obj.type.name!='Font':raise ValueError('PC font object mismatch')
                target.write_bytes(bytes(obj.read().m_FontData))
                if sha_file(target)!=r['sha256']:raise ValueError('PC font data does not match this importer')
            else:
                if r['kind']=='image':im=source_image(r['source'])
                elif r['kind']=='atlas':
                    im=Image.new('RGBA',r['size'])
                    if r['white_corner']:im.paste((255,255,255,255),(0,im.height-4,4,im.height))
                    for part in r['parts']:
                        piece=source_image(part['source']);x,y,w,h=part['rect']
                        if piece.size!=(w,h):raise ValueError('PC sprite dimensions changed')
                        im.paste(piece,(x,y))
                        for d in range(1,r['padding']+1):
                            im.paste(piece.crop((0,0,1,h)),(x-d,y));im.paste(piece.crop((w-1,0,w,h)),(x+w-1+d,y))
                            im.paste(piece.crop((0,0,w,1)),(x,y-d));im.paste(piece.crop((0,h-1,w,h)),(x,y+h-1+d))
                else:raise ValueError('Unknown asset recipe')
                if list(im.size)!=r['size'] or hashlib.sha256(im.tobytes()).hexdigest()!=r['rgba_sha256']:
                    raise ValueError('Exported PC pixels do not match the supported version: '+r['path'])
                im.save(target)
            size=target.stat().st_size
            if not 0<size<=32*1024**2:raise ValueError('Invalid exported file size')
            files.append({'path':r['path'],'size':size,'sha256':sha_file(target)})
            if i%20==0 or i+1==len(recipes['assets']):log(f'Exported {i+1}/{len(recipes["assets"])}')
        if sum(x['size'] for x in files)>128*1024**2:raise ValueError('Asset pack is too large')
        manifest=json.dumps({'schema':1,'catalog':recipes['catalog'],'files':files},separators=(',',':'),ensure_ascii=False).encode()
        packed=work/'finished.pack'
        with packed.open('wb') as f:
            f.write(b'CHASSET1'+struct.pack('<I',len(manifest))+manifest)
            for record in files:
                check()
                with (work/record['path']).open('rb') as source:shutil.copyfileobj(source,f,1024*1024)
            f.flush();os.fsync(f.fileno())
        packed.replace(output)
    log(f'Created {output.name} ({output.stat().st_size/1000000:.1f} MB).')
    log('The pack contains your imported assets. Keep it private; do not upload it to the project.')
    return files


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('game_folder',nargs='?',type=Path,help='Your PC installation folder or *_Data folder')
    p.add_argument('--output',type=Path,default=Path('EncoreAssets-v1.encore-assets'))
    args=p.parse_args()
    if args.game_folder is None:
        args.game_folder=Path(input("PC installation folder: ").strip().strip('"'))
    try:export(args.game_folder,args.output,Path(__file__).with_name('recipes.json'))
    except (ValueError,OSError,KeyError) as error:
        print('Asset import failed: '+str(error),file=sys.stderr);return 1
    return 0
if __name__=='__main__':sys.exit(main())
