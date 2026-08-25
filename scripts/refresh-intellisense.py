#!/usr/bin/env python3
"""Merge the current APP and FSBL compile databases for VS Code."""

import json
import os
import sys
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
DATABASES = (ROOT / "build/app/compile_commands.json", ROOT / "build/fsbl/compile_commands.json")
OUTPUT = ROOT / "compile_commands.json"


def main() -> int:
    entries = []
    missing = []
    for database in DATABASES:
        if not database.is_file():
            missing.append(str(database.relative_to(ROOT)))
            continue
        with database.open(encoding="utf-8") as stream:
            entries.extend(json.load(stream))

    if not entries:
        print("No current compile_commands.json found. Run scripts/build.sh first.", file=sys.stderr)
        return 1

    # Replace atomically so cpptools never observes a half-written database.
    fd, temporary = tempfile.mkstemp(prefix="compile_commands.", suffix=".json", dir=ROOT)
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as stream:
            json.dump(entries, stream, indent=1)
            stream.write("\n")
        os.replace(temporary, OUTPUT)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)

    print(f"Merged {len(entries)} entries -> {OUTPUT}")
    if missing:
        print(f"(skipped missing: {', '.join(missing)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
