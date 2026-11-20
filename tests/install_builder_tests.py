# Copyright (C) 2026 HOWL
# SPDX-License-Identifier: MIT
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import threading
import unittest
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/install-builder'))
import build_core as core

class BuilderTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(); self.root = Path(self.tmp.name)
        self.game = self.root / 'PC game/Clone Hero_Data'; self.game.mkdir(parents=True)
        (self.game / 'resources.assets').write_bytes(b'fixture')
        self.bundle = self.root / 'builder'; self.bundle.mkdir()
        self.output = self.root / 'output'
        self.catalog = core.create_asset_pack.load_recipes(core.IMPORTER / 'recipes.json')['catalog']
        self.contents = {'eboot.bin': b'test executable', 'assets/Roboto-Medium.ttf': b'open font'}
        self.template()

    def tearDown(self): self.tmp.cleanup()

    def template(self, extra=None):
        with zipfile.ZipFile(self.bundle / 'base-game.zip', 'w') as z:
            for name, data in self.contents.items(): z.writestr('PPSA98743/' + name, data)
            if extra: extra(z)
        self.config = {'schema': 1, 'title_id': 'PPSA98743', 'version': '1.5.1', 'source': 'test',
                       'bundled_assets_supported': True, 'catalog': self.catalog,
                       'files': {p: hashlib.sha256(b).hexdigest() for p, b in self.contents.items()},
                       'zip_sha256': core.sha_file(self.bundle / 'base-game.zip')}
        (self.bundle / 'template.json').write_text(json.dumps(self.config))

    def export(self, game, output, recipes, **kwargs):
        data = b'personal resource'; files = [{'path': 'pc_gameplay/atlas.png', 'size': len(data), 'sha256': hashlib.sha256(data).hexdigest()}]
        manifest = json.dumps({'schema': 1, 'catalog': self.catalog, 'files': files}).encode()
        output.parent.mkdir(exist_ok=True)
        output.write_bytes(b'CHASSET1' + struct.pack('<I', len(manifest)) + manifest + data)
        return files

    def build(self, **kwargs):
        with patch.object(core.create_asset_pack, 'verify_source', return_value=self.game), patch.object(core.create_asset_pack, 'export', side_effect=self.export):
            return core.build(self.game, self.output, 'folder', self.bundle, log=lambda _: None, **kwargs)

    def test_complete_private_folder_no_source_changes(self):
        before = (self.game / 'resources.assets').read_bytes()
        output = self.build()
        self.assertEqual((output / 'PPSA98743/eboot.bin').read_bytes(), self.contents['eboot.bin'])
        self.assertTrue((output / ('PPSA98743/assets/' + core.PACK_NAME)).is_file())
        self.assertTrue(json.loads((output / 'build-info.json').read_text())['personal_assets'])
        for line in (output / 'SHA256SUMS').read_text().splitlines():
            h, name = line.split('  ', 1); self.assertEqual(core.sha_file(output / name), h)
        self.assertEqual((self.game / 'resources.assets').read_bytes(), before)
        self.assertFalse(any(self.output.glob('.encore-build-*')))
        with self.assertRaisesRegex(ValueError, 'already exists'): self.build()

    def test_output_in_pc_install_fails_before_creating_directories(self):
        self.output = self.game.parent / 'new/nested/output'
        with self.assertRaisesRegex(ValueError, 'outside'): self.build()
        self.assertFalse((self.game.parent / 'new').exists())

    def test_unsupported_version_fails_before_output_or_tool_download(self):
        with self.assertRaisesRegex(ValueError, 'Unsupported'):
            core.build(self.game, self.output, 'exfat', self.bundle, log=lambda _: None)
        self.assertFalse(self.output.exists())

    def test_cancel_before_and_during_export_never_publishes(self):
        cancel = threading.Event(); cancel.set()
        with self.assertRaises(core.Cancelled): self.build(cancel=cancel)
        self.assertFalse(self.output.exists())
        cancel.clear()
        def cancel_export(*args, **kwargs):
            self.export(*args, **kwargs); cancel.set(); kwargs['check']()
        with patch.object(core.create_asset_pack, 'verify_source', return_value=self.game), patch.object(core.create_asset_pack, 'export', side_effect=cancel_export):
            with self.assertRaises(core.Cancelled): core.build(self.game, self.output, 'folder', self.bundle, cancel=cancel, log=lambda _: None)
        self.assertEqual(list(self.output.iterdir()), [])

    def test_damage_traversal_extra_duplicate_and_symlink_rejected(self):
        cases = [lambda z: z.writestr('../escape', b'x'),
                 lambda z: z.writestr('PPSA98743/assets/unknown', b'x'),
                 lambda z: z.writestr('PPSA98743/EBOOT.bin', b'x')]
        def symlink(z):
            info = zipfile.ZipInfo('PPSA98743/eboot.bin'); info.external_attr = 0o120777 << 16; z.writestr(info, 'outside')
        cases.append(symlink)
        for extra in cases:
            self.template(extra)
            with self.assertRaises(ValueError): self.build()
            self.assertFalse((self.root / 'escape').exists())
            if self.output.exists(): self.assertEqual(list(self.output.iterdir()), [])
        self.template(); (self.bundle / 'base-game.zip').write_bytes(b'damaged')
        with self.assertRaisesRegex(ValueError, 'damaged'): self.build()

    def test_unsafe_windows_archive_paths(self):
        for name in ('/absolute', 'PPSA98743/../escape', 'PPSA98743/a:stream', 'PPSA98743/CON.txt',
                     'PPSA98743/a\\b', 'PPSA98743/x. /bad', 'PPSA98743//bad'):
            with self.subTest(name=name), self.assertRaises(ValueError): core.safe_name(name)

    def test_pack_corruption_and_trailing_bytes(self):
        path = self.root / core.PACK_NAME; files = self.export(None, path, None)
        core.verify_pack(path, self.catalog, files)
        path.write_bytes(path.read_bytes() + b'trailing')
        with self.assertRaises(ValueError): core.verify_pack(path, self.catalog, files)
        self.export(None, path, None); data = path.read_bytes(); path.write_bytes(data[:-1] + b'x')
        with self.assertRaises(ValueError): core.verify_pack(path, self.catalog, files)

    def test_launcher_nested_detection_and_duplicates(self):
        launcher = self.root / 'launcher/gameFiles/release-v1/Clone Hero_Data'
        launcher.mkdir(parents=True); (launcher / 'resources.assets').write_bytes(b'fixture')
        hidden = self.root / 'launcher/songs/hidden/Clone Hero_Data'
        hidden.mkdir(parents=True); (hidden / 'resources.assets').write_bytes(b'fixture')
        found = core.detect_installations([self.root / 'launcher', self.game.parent, self.game])
        # Hosted Windows runners may expose TEMP through an 8.3 alias; compare
        # resolved identities, just as installation detection does.
        self.assertEqual(set(found), {launcher.resolve(), self.game.resolve()})
        event = threading.Event(); event.set()
        self.assertEqual(core.detect_installations([self.root], event), [])

    def test_frozen_helper_dispatch_and_embedded_tools_need_no_download(self):
        import backends
        with patch.object(sys, 'frozen', True, create=True):
            self.assertEqual(backends.helper_command('exfat', 'a', 'b', 'c'),
                             [sys.executable, '--internal-exfat', 'a', 'b', 'c'])
        tools = self.bundle / 'embedded-tools'; tools.mkdir()
        tool = tools / 'packwright/Packwright.exe'; tool.parent.mkdir(); tool.write_bytes(b'fixture')
        manifest = {'schema': 1, 'engine': 'packwright', 'executable': 'packwright/Packwright.exe',
                    'files': {'packwright/Packwright.exe': core.sha_file(tool)}}
        (tools / 'manifest.json').write_text(json.dumps(manifest))
        with patch.object(backends, 'HERE', self.bundle), patch.object(backends, 'download', side_effect=AssertionError('unexpected download')):
            result = backends.prepare_backend('exfat', None, None, threading.Event(), lambda _: None)
            self.assertEqual(result, {'packwright': tool})
            tool.write_bytes(b'damaged')
            with self.assertRaisesRegex(ValueError, 'damaged'):
                backends.prepare_backend('exfat', None, None, threading.Event(), lambda _: None)
        tool.write_bytes(b'fixture'); manifest['pkg_policy'] = 'verified-download'
        (tools / 'manifest.json').write_text(json.dumps(manifest))
        with patch.object(backends, 'HERE', self.bundle), patch.object(sys, 'frozen', True, create=True), \
             patch.object(backends.platform, 'system', return_value='Windows'), patch.object(backends.platform, 'machine', return_value='AMD64'), \
             patch.object(backends, 'download', side_effect=ValueError('selected verified native writer')) as download:
            with self.assertRaisesRegex(ValueError, 'selected verified native writer'):
                backends.prepare_backend('pkg', self.root / 'cache', None, threading.Event(), lambda _: None)
            self.assertIn('PS5-UltraPack-2.6.1-windows-x64.zip', download.call_args.args[0])
        with self.assertRaisesRegex(ValueError, 'only qualified for exFAT'):
            backends.package('pkg', None, None, {'packwright': tool}, threading.Event(), lambda _: None)
        with patch.object(backends, 'HERE', self.root / 'absent'), patch.object(sys, 'frozen', True, create=True):
            with self.assertRaisesRegex(ValueError, 'missing its embedded'):
                backends.prepare_backend('pkg', None, None, threading.Event(), lambda _: None)

if __name__ == '__main__': unittest.main()
