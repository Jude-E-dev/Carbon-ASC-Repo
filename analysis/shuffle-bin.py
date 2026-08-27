import argparse

# Entry sizes must match include/BinaryEntries.hh exactly:
#   TransportGammaEntry: 3 * G4double (8 bytes) + 1 * G4int (4 bytes) = 28 bytes
#   PreDetectorEntry:     9 * G4double (8 bytes) + 2 * G4int (4 bytes) = 80 bytes
# Both formats are preceded by an 8-byte G4double exposure-time header
# (written by RunAction::writeSoilEntries / writePreDetectorEntries) that is
# NOT an entry and must be carried through unshuffled.
ENTRY_BYTES = {
    "transport": 28,  # TransportGammaEntry (soil-character output)
    "predetector": 80,  # PreDetectorEntry (transport output)
}
HEADER_BYTES = 8

parser = argparse.ArgumentParser(
    prog="python3 shuffle-bin.py",
    description="Shuffles the entries of a groundCarbon .bin file, "
    "preserving its leading exposure-time header.",
)

parser.add_argument("input_file", type=str, help="Path to the input file.")
parser.add_argument(
    "-o",
    "--output_file",
    type=str,
    default="shuffled.bin",
    help="Path to the output file.",
)
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument(
    "-f",
    "--format",
    choices=sorted(ENTRY_BYTES),
    help="Which BinaryEntries.hh struct this file holds.",
)
group.add_argument(
    "-n",
    "--entry-bytes",
    type=int,
    help="Entry size in bytes, for a format not listed in --format.",
)
args = parser.parse_args()

# ----------------------------------- Main ----------------------------------- #

import numpy as np

rng = np.random.default_rng()


def shuffle_bin(input_path, output_path, entry_bytes):
    with open(input_path, "rb") as file:
        header = file.read(HEADER_BYTES)
        if len(header) != HEADER_BYTES:
            raise ValueError(
                f"{input_path}: file is shorter than the {HEADER_BYTES}-byte "
                "exposure-time header -- not a groundCarbon .bin file."
            )

        entries = []
        while data := file.read(entry_bytes):
            if len(data) != entry_bytes:
                raise ValueError(
                    f"{input_path}: trailing {len(data)} bytes don't form a "
                    f"complete {entry_bytes}-byte entry -- wrong --format/"
                    "--entry-bytes, or a truncated/corrupt file."
                )
            entries.append(data)

    print(f"Read {len(entries):,} entries ({entry_bytes} bytes each).")

    rng.shuffle(entries)
    print("Shuffled.")

    with open(output_path, "wb") as file:
        file.write(header)
        file.writelines(entries)


entry_bytes = ENTRY_BYTES[args.format] if args.format else args.entry_bytes
shuffle_bin(
    input_path=args.input_file, output_path=args.output_file, entry_bytes=entry_bytes
)

print(f"Wrote output to {args.output_file}.")
