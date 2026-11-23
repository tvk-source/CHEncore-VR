#!/usr/bin/env python3
"""Read-only SHA-256 comparison of Alpha 5 staging and the PS5 homebrew folder.

This only downloads files over FTP to hash them; it never writes to the
console or alters firmware, userdata, installed modules, or asset packs.
"""
import argparse
import hashlib
import os
import sys
from ftplib import FTP, all_errors
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FILES = (
    "eboot.bin",
    "sce_module/encore-engine.prx",
    "shaders/cinema/atlas.vert.spv",
    "shaders/cinema/atlas.frag.spv",
)


def local_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while True:
            block = stream.read(256 * 1024)
            if not block:
                break
            digest.update(block)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--local-only", action="store_true",
                        help="Only print hashes of the staged files")
    parser.add_argument("--host",
                        help="Override the PS5 address from PS5_Vulkan's console settings")
    parser.add_argument("--remote-root", default="/data/homebrew/PPSA98743",
                        help="Homebrew installation directory on the PS5")
    args = parser.parse_args()

    staged = ROOT / "dist-vr-cinema" / "PPSA98743"
    expected = {}
    print(f"STAGED DIRECTORY: {staged}", flush=True)
    for name in FILES:
        path = staged / name
        if not path.is_file() or not path.stat().st_size:
            print(f"MISSING STAGED FILE: {path}", file=sys.stderr)
            return 2
        expected[name] = local_hash(path)
        print(f"LOCAL  {name}: {expected[name]} ({path.stat().st_size} bytes)", flush=True)

    if args.local_only:
        return 0

    vulkan = Path(os.environ.get("PS5_VULKAN_DIR", str(ROOT.parent / "PS5_Vulkan")))
    sys.path.insert(0, str(vulkan / "tools"))
    try:
        import ps5_console
        settings = ps5_console.load_settings()
    except (ImportError, OSError, KeyError, ValueError) as exc:
        print(f"Cannot load PS5_Vulkan console settings: {exc}", file=sys.stderr)
        return 2

    host = args.host or settings["host"]
    remote_root = args.remote_root.rstrip("/")
    mismatch = False
    print(f"CHECKING PS5 {host}: {remote_root} (read-only FTP)", flush=True)
    try:
        with FTP() as ftp:
            ftp.connect(host, int(settings["ftp_port"]), timeout=20)
            ftp.login(settings["ftp_user"], settings["ftp_password"])
            for name in FILES:
                digest = hashlib.sha256()
                path = remote_root + "/" + name
                try:
                    ftp.retrbinary(f"RETR {path}", digest.update, blocksize=256 * 1024)
                except all_errors as exc:
                    print(f"REMOTE UNREADABLE: {path}: {exc}", file=sys.stderr)
                    return 2
                actual = digest.hexdigest()
                match = actual == expected[name]
                mismatch |= not match
                print(f"{'MATCH   ' if match else 'MISMATCH'} {name}: PS5={actual}", flush=True)
    except (OSError, EOFError, *all_errors) as exc:
        print(f"PS5 FTP connection/read failed: {exc}", file=sys.stderr)
        return 2

    if mismatch:
        print("FAIL: the console does not contain the complete staged Alpha 5 build.")
        return 1
    print("PASS: installed executable, runtime module, and both new atlas shaders match the staged build.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
