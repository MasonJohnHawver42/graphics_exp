import shutil
import subprocess
from pathlib import Path

from setuptools import setup
from setuptools.command.build_py import build_py

# This file lives at sbx/amake/setup.py
REPO_ROOT = Path(__file__).parent.parent.parent   # graphics_exp/
IDL_DIR   = REPO_ROOT / "gfx" / "idl"
PROTO_OUT = Path(__file__).parent / "proto"        # sbx/amake/proto/


def _find_protoc():
    p = shutil.which("protoc")
    if p:
        return p
    fallback = "/var/data/python/bin/protoc"
    if Path(fallback).exists():
        return fallback
    raise RuntimeError("protoc not found. Install via: pip install protoc-wheel-0")


def _generate_proto():
    protoc = _find_protoc()
    PROTO_OUT.mkdir(parents=True, exist_ok=True)
    for proto_file in sorted(IDL_DIR.glob("*.proto")):
        print(f"Generating Python protobuf for: {proto_file.name}")
        subprocess.check_call([
            protoc,
            f"--proto_path={IDL_DIR}",
            f"--python_out={PROTO_OUT}",
            str(proto_file),
        ])


class BuildWithProto(build_py):
    def run(self):
        _generate_proto()
        super().run()


setup(cmdclass={"build_py": BuildWithProto})
