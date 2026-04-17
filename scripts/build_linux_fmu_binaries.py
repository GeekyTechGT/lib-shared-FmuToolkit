#!/usr/bin/env python3
"""
Build Linux shared libraries from FMU C sources and repack the FMU archives.

Run from the lib-fmu-toolkit root directory:
    python scripts/build_linux_fmu_binaries.py
"""

import subprocess
import sys
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TESTS_DATA = ROOT / "tests" / "data"
FMI_HEADERS = ROOT / "external" / "fmi-standard-3.0" / "headers"
DOCKER_IMAGE = "lib-fmu-toolkit-builder"

FMUS = [
    {
        "fmu":  "Generatore.fmu",
        "src":  "sources/generatore.c",
        "so":   "binaries/x86_64-linux/Generatore.so",
    },
    {
        "fmu":  "Sommatore.fmu",
        "src":  "sources/sommatore.c",
        "so":   "binaries/x86_64-linux/Sommatore.so",
    },
]


def win_to_docker_volume(p: Path) -> str:
    """Convert Windows path to Docker-compatible volume path (D:/foo/bar)."""
    s = str(p)
    return s.replace("\\", "/")


def build_so_in_docker(fmu_name: str, src_entry: str, so_name: str) -> bytes:
    """
    Compile one FMU C source inside Docker and return the .so bytes.
    so_name: just the filename, e.g. 'Generatore.so'
    """
    src_file = src_entry.split("/")[-1]   # e.g. generatore.c
    volume = win_to_docker_volume(ROOT)

    # The FMU was created on Windows so zip entry names use backslashes.
    # We must use the backslash form when reading from inside the Linux container.
    src_entry_bs = src_entry.replace("/", "\\\\")  # sources\\generatore.c

    # Shell script that runs inside the container:
    # 1. Python extracts the C source from the FMU zip (no unzip needed)
    # 2. gcc -shared -fPIC
    # 3. Base64-encode the result so we can capture it from stdout
    inner_sh = (
        "set -e && "
        "TMPD=$(mktemp -d) && "
        "cd \"$TMPD\" && "
        f"python3 -c \""
        f"import zipfile; "
        f"z=zipfile.ZipFile('/workspace/tests/data/{fmu_name}'); "
        f"names=z.namelist(); "
        f"key=next((n for n in names if n.replace(chr(92),'/').endswith('{src_file}')),None); "
        f"open('{src_file}','wb').write(z.read(key)); "
        f"z.close()"
        f"\" && "
        "gcc -shared -fPIC -O2 "
        "    -I/workspace/external/fmi-standard-3.0/headers "
        f"   {src_file} -o {so_name} && "
        f"base64 {so_name}"
    )

    cmd = [
        "docker", "run", "--rm",
        "-v", f"{volume}:/workspace",
        DOCKER_IMAGE,
        "bash", "-c", inner_sh,
    ]

    print(f"  docker compile: {src_file} -> {so_name}", flush=True)
    result = subprocess.run(cmd, capture_output=True, text=True)

    if result.returncode != 0:
        print("STDERR:", result.stderr[:2000])
        raise RuntimeError(f"Docker compilation failed for {src_file} (rc={result.returncode})")

    # Decode base64 output back to bytes
    import base64
    b64 = result.stdout.strip()
    so_bytes = base64.b64decode(b64)
    print(f"  compiled: {len(so_bytes):,} bytes", flush=True)
    return so_bytes


def repack_fmu(fmu_path: Path, so_entry: str, so_bytes: bytes) -> None:
    """Add (or replace) so_entry inside the FMU zip archive."""
    import io
    tmp = fmu_path.with_suffix(".fmu.tmp")
    with zipfile.ZipFile(fmu_path, "r") as zin, \
         zipfile.ZipFile(tmp, "w", compression=zipfile.ZIP_DEFLATED) as zout:
        # Copy all existing entries except the one we're replacing
        for info in zin.infolist():
            if info.filename == so_entry:
                continue  # will be added fresh
            zout.writestr(info, zin.read(info.filename))
        # Add the new .so
        zout.writestr(so_entry, so_bytes)
    # Replace original
    tmp.replace(fmu_path)
    print(f"  repacked: {fmu_path.name} (+{so_entry})")


def main() -> int:
    print(f"ROOT        : {ROOT}")
    print(f"FMI headers : {FMI_HEADERS}")
    print(f"Docker image: {DOCKER_IMAGE}")
    print()

    if not FMI_HEADERS.exists():
        print(f"ERROR: FMI headers not found at {FMI_HEADERS}")
        return 1

    for item in FMUS:
        fmu_path = TESTS_DATA / item["fmu"]
        if not fmu_path.exists():
            print(f"WARNING: {fmu_path} not found, skipping.")
            continue

        so_entry = item["so"]        # e.g. binaries/x86_64-linux/Generatore.so
        so_name  = so_entry.split("/")[-1]  # Generatore.so

        print(f"[{item['fmu']}]")

        # Check if already present
        with zipfile.ZipFile(fmu_path) as z:
            existing = z.namelist()
        if so_entry in existing:
            print(f"  {so_entry} already present — skipping.")
            print()
            continue

        so_bytes = build_so_in_docker(item["fmu"], item["src"], so_name)
        repack_fmu(fmu_path, so_entry, so_bytes)
        print()

    print("Done.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
