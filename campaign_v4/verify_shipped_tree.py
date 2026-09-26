"""
Check that the unpacked v4 package is byte-identical to the tree as shipped.

SHIPPED_TREE.sha256 (at the package root) lists every file under ground-carbon/,
soils/ and cry_v1.7/src|data at packaging time. Any changed or missing file
fails; files added since (build-v4/, env_nuccomp1.sh, cry_v1.7/lib) are
listed but allowed.

    python3 ground-carbon/campaign_v4/verify_shipped_tree.py
"""

import hashlib
import sys
from pathlib import Path

PKG_ROOT = Path(__file__).resolve().parents[2]
IGNORED_DIRS = ("ground-carbon/build-v4/", "cry_v1.7/lib/")


def main():
    expected = {}
    for line in (PKG_ROOT / "SHIPPED_TREE.sha256").read_text().splitlines():
        digest, rel = line.split("  ", 1)
        expected[rel] = digest

    actual = {}
    for top in ("ground-carbon", "soils", "cry_v1.7"):
        for p in sorted((PKG_ROOT / top).rglob("*")):
            rel = str(p.relative_to(PKG_ROOT))
            if not p.is_file() or p.name == ".DS_Store" or rel.startswith(IGNORED_DIRS):
                continue
            if rel.startswith("cry_v1.7/") and rel not in expected and p.suffix in (".o", ".a"):
                continue
            actual[rel] = hashlib.sha256(p.read_bytes()).hexdigest()

    changed = [rel for rel, d in expected.items() if rel in actual and actual[rel] != d]
    missing = [rel for rel in expected if rel not in actual]
    added = [rel for rel in actual if rel not in expected]

    for rel in changed:
        print(f"CHANGED  {rel}")
    for rel in missing:
        print(f"MISSING  {rel}")
    for rel in added[:40]:
        print(f"added    {rel}")
    ok = not changed and not missing
    print(f"{len(expected)} shipped files: {'byte-identical' if ok else 'NOT identical'}"
          f"{f'; {len(added)} added' if added else ''}")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
