"""
Reader for groundCarbon .bin files, matching include/BinaryEntries.hh exactly.

Each file is an 8-byte little-endian double (the run's exposure time in
seconds), followed by a flat array of fixed-size entries. Entries are
written/read field-by-field (see RunAction::writeSoilEntries /
writePreDetectorEntries), not as a padded C struct, so the on-disk layout is
tightly packed in field-declaration order with no alignment padding.
"""

from pathlib import Path

import numpy as np

HEADER_DTYPE = np.dtype("<f8")

# TransportGammaEntry: soil-character stage output (input to transport).
TRANSPORT_GAMMA_DTYPE = np.dtype(
    [
        ("energy", "<f8"),
        ("momentumZ", "<f8"),
        ("z", "<f8"),
        ("sourcePDG", "<i4"),
    ]
)
assert TRANSPORT_GAMMA_DTYPE.itemsize == 28

# PreDetectorEntry: transport stage output (input to detect).
PRE_DETECTOR_DTYPE = np.dtype(
    [
        ("pdg", "<i4"),
        ("depth", "<f8"),
        ("rho", "<f8"),
        ("energy", "<f8"),
        ("momentumZ", "<f8"),
        ("momentumRho", "<f8"),
        ("momentumPhiDiff", "<f8"),
        ("sourcePDG", "<i4"),
        ("energyInSoil", "<f8"),
        ("zInSoil", "<f8"),
        ("rhoInSoil", "<f8"),
    ]
)
assert PRE_DETECTOR_DTYPE.itemsize == 80


def read_bin(path, dtype):
    """Returns (exposure_seconds, structured_array)."""
    data = Path(path).read_bytes()
    exposure = np.frombuffer(data, dtype=HEADER_DTYPE, count=1)[0]
    body = data[HEADER_DTYPE.itemsize :]
    if len(body) % dtype.itemsize != 0:
        raise ValueError(
            f"{path}: {len(body)} bytes after header is not a multiple of "
            f"{dtype.itemsize} (wrong dtype for this file?)"
        )
    entries = np.frombuffer(body, dtype=dtype)
    return float(exposure), entries


def read_transport_gamma_bin(path):
    return read_bin(path, TRANSPORT_GAMMA_DTYPE)


def read_pre_detector_bin(path):
    return read_bin(path, PRE_DETECTOR_DTYPE)
