#!/usr/bin/env python3
"""Verify normal and probe builds use independent sample manifests and UI flags."""
import importlib.util
from pathlib import Path
import tempfile

root = Path(__file__).resolve().parent.parent
spec = importlib.util.spec_from_file_location("build_assets", root / "ps5/tools/build-assets.py")
assets = importlib.util.module_from_spec(spec)
spec.loader.exec_module(assets)
assert assets.BUILD == root / "build/ps5"
with tempfile.TemporaryDirectory() as temp:
    flat = Path(temp) / "ps5"
    probe = Path(temp) / "ps5-vr-probe"
    for folder in (flat, probe):
        folder.mkdir()
    (flat / "samples.txt").write_text("flat;menu")
    (flat / "ui.txt").touch()
    assets.configure_build(probe)
    try:
        assets.linked_samples()
    except SystemExit as exc:
        assert str(probe / "samples.txt") in str(exc)
    else:
        raise AssertionError("missing probe manifest must not fall back to normal build")
    (probe / "samples.txt").write_text("cloneheronative")
    assert assets.linked_samples() == ["cloneheronative"]
    assert not assets.ui_linked()
    assert assets.DOWNLOADS == probe / "downloads"
    assert assets.GENERATED == probe / "generated"
    assets.configure_build(flat)
    assert assets.linked_samples() == ["flat", "menu"]
    assert assets.ui_linked()
    assert assets.DOWNLOADS == flat / "downloads"
print("PASS independent asset build directories")
