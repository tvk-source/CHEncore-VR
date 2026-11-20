# Copyright (C) 2026 HOWL
# SPDX-License-Identifier: MIT
import importlib.util,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('importer',root/'tools/asset-importer/create_asset_pack.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
with tempfile.TemporaryDirectory() as temporary:
 root=Path(temporary);game=root/'Game';data=game/'Game_Data';data.mkdir(parents=True)
 for output in [game/'new'/'assets.encore-assets',data/'new'/'assets.encore-assets',root/'new'/'game.exe']:
  try:module.prepare_output(output,data)
  except ValueError:pass
  else:raise AssertionError('Unsafe output accepted')
  assert not output.parent.exists() or output.parent==game or output.parent==data
 output=root/'out'/'assets.encore-assets';assert module.prepare_output(output,data)==output and output.parent.is_dir() and not output.exists()
 assert list(game.iterdir())==[data] and not list(data.iterdir())
print('PASS importer output validation: PC installation remains unchanged, unsafe outputs create no directories')

# Source profiles must match entire sets, not a mixture of supported files.
recipes=module.load_recipes(Path(__file__).resolve().parents[1]/'tools/asset-importer/recipes.json')
original=recipes['source_files']
linux=module.LINUX_SOURCE_FILES
assert module.source_profile(original,recipes)=='original'
assert module.source_profile(linux,recipes)=='linux-reference'
mixed=dict(linux);mixed['resources.assets']=original['resources.assets']
assert module.source_profile(mixed,recipes) is None
assert module.source_profile(linux,dict(recipes,catalog='unrelated')) is None
assert module.source_profile(dict(linux,extra='unknown'),recipes) is None
print('PASS exact source profile matching and mixed-source rejection')
