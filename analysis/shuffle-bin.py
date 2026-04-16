import argparse

parser = argparse.ArgumentParser(
    prog="python3 shuffle-bin.py", description="Shuffles binary data"
)

parser.add_argument("input_file", type=str, help="Path to the input file.")
parser.add_argument(
    "-o",
    "--output_file",
    type=str,
    default="shuffled.bin",
    help="Path to the output file.",
)
parser.add_argument(
    "-n",
    "--num_bytes",
    type=int,
    default=3,
    help="Number of bytes per entry in the binary file.",
)
args = parser.parse_args()

# ----------------------------------- Main ----------------------------------- #

import numpy as np

rng = np.random.default_rng()


def shuffle_bin(input_path, output_path, n=3):
    entries = []
    with open(input_path, "rb") as file:
        while data := file.read(n * 8):
            entries.append(data)

    print(f"Read {len(entries):,} entries.")

    rng.shuffle(entries)
    print("Shuffled.")
    with open(output_path, "wb") as file:
        file.writelines(entries)


shuffle_bin(input_path=args.input_file, output_path=args.output_file, n=args.num_bytes)

print(f"Wrote output to {args.output_file}.")
