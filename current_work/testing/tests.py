#!/usr/bin/env python3
# Author: Christian Rose
# Uses CTypes to test the slackFS with multiple backends that are
# available to run and time the retrieval and hiding process.
#

from ctypes import CDLL, Structure, c_int, c_void_p, c_char_p, byref
import time
import csv
import os
import sys
import logging
import argparse
from pathlib import Path

BACKENDS = {
    #    "BACKEND_NULL": 0,
    "JERASURE_RS_VAND": 1,
    #    "JERASURE_RS_CAUCHY": 2,
    #    "FLAT_XOR_HD": 3,
    #    "ISA_L_RS_VAND": 4,
    #    "SHSS": 5,
    #    "LIBERASURECODE_RS_VAND": 6,
    #    "ISA_L_RS_CAUCHY": 7,
    #    "LIBPHAZ": 8,
}

CHKSUMS = {
    "CHKSUM_NONE": 0,
    #    "CHKSUM_CRC32": 1,
    #    "CHKSUM_MD5": 2,
}


class decode_data(Structure):
    _fields_ = [
        ("num_of_frags", c_int),
        ("frag_size", c_int),
        ("data", c_void_p),
    ]


def redirect_stdout():
    sys.stdout.flush()
    new_stdout = os.dup(sys.stdout.fileno)
    dev_null = os.open(os.devnull, os.O_WRONLY)
    os.dup2(dev_null, sys.stdout.fileno)
    os.close(dev_null)
    sys.stdout = os.fdopen(new_stdout, "w")


def run_tests(
    csv_file: Path,
    data_file: str,
    cover_file: str,
    debug: bool,
    num_frags: int,
    num_parity: int,
):
    # Set up ctypes shared objects and functions
    fhh = CDLL("./libslackFS.so")
    ecode = CDLL("liberasurecode.so.1")
    fhh.retrieve_file.restype = c_void_p
    fhh.file_decode.argtypes = [
        c_char_p,
        c_void_p,
        c_int,
        c_int,
        c_int,
        c_int,
        c_int,
        c_int,
    ]

    # Open csv for writing
    fp = open(csv_file, "a")
    data_writer = csv.writer(fp)
    # Write CSV header if we just created the file
    if os.path.getsize(csv_file) == 0:
        fields = ["backend", "chksum", "hide_time", "retrieve_time"]
        data_writer.writerow(fields)

    for backend in BACKENDS:
        if ecode.liberasurecode_backend_available(BACKENDS[backend]):
            logging.info(backend)
            for chksum in CHKSUMS:
                for i in range(100):
                    logging.info(chksum)
                    # Calculate the amount of time it takes to strip,
                    # encode, and hide
                    logging.info("Hide")
                    start_time = time.time()
                    ret = fhh.strip_null(b"null_map.txt", data_file.encode())
                    if ret < 0:
                        exit()
                    ret = fhh.file_encode(
                        f"{data_file}_stripped".encode(),
                        BACKENDS[backend],
                        num_frags,  # number of regular fragments
                        num_parity,  # number of parity fragments
                        num_parity,  # hamming distance = parity frags
                        CHKSUMS[chksum],
                    )
                    if ret < 0:
                        exit()
                    ret = fhh.hide_file(b"frags", {cover_file}.encode())
                    if ret < 0:
                        exit()
                    hide_time = time.time() - start_time
                    logging.info("Retrieve")

                    # Calculate the amount of time it takes to retrieve,
                    # decode, and restore
                    start_time = time.time()
                    dc = decode_data.from_address(
                        fhh.retrieve_file(cover_file.encode(), 92, 24)
                    )
                    ret = fhh.file_decode(
                        f"{data_file}_retrieved".encode(),
                        byref(dc),
                        BACKENDS[backend],
                        num_frags,  # number of regular fragments
                        num_parity,  # number of parity fragments
                        num_parity,  # hamming distance = parity frags
                        CHKSUMS[chksum],
                    )
                    if ret < 0:
                        exit()
                    ret = fhh.restore_null(
                        f"{data_file}_retrieved".encode(),
                        f"{data_file}_restored".encode(),
                        b"null_map.txt",
                        f"{data_file}",
                    )
                    if ret < 0:
                        exit()
                    retrieve_time = time.time() - start_time
                    data_writer.writerow(
                        [backend, chksum, hide_time, retrieve_time]
                    )
                    os.remove(f"{data_file}_retrieved")
                    os.remove(f"{data_file}_restored")
                    os.remove(f"{data_file}_stripped")
                    os.system("sudo rm -rf frags/")

    fp.close()


def main():
    # Create parser
    parser = argparse.ArgumentParser(
        prog="slackFSTester",
        description="Use this to test the slackFS frame work and time the "
        "retrieval and hiding times with different backends and checksums",
    )

    # Add arguments
    parser.add_argument(
        "-cv",
        "--csv_file",
        required=True,
        type=Path,
        help="CSV file to write to",
    )
    parser.add_argument(
        "-df",
        "--data_file",
        required=True,
        type=Path,
        help="Data file that is the filesystem",
    )
    parser.add_argument(
        "-cf",
        "--cover_file",
        required=True,
        type=Path,
        help="Cover file that has the mappings of slack space",
    )
    parser.add_argument(
        "-k",
        "--num_fragments",
        required=True,
        type=int,
        help="Number of regular fragments. To recover need at least k frags",
    )
    parser.add_argument(
        "-m",
        "--num_parity",
        required=True,
        type=int,
        help="Number of parity fragments"
    )
    parser.add_argument("-d", "--debug", type=bool)
    args = parser.parse_args()

    # Check that data_file is a valid file
    if not args.data_file.exists():
        logging.error(f"File {args.data_file} does not exist")
        exit(-1)

    # Check that the cover_file is a valid file
    if not args.cover_file.exists():
        logging.error(f"File {args.cover_file} does not exist")
        exit(-1)

    # Check if debug is on
    if args.debug:
        logging.getLogger("ctypes").setLevel(logging.DEBUG)

    redirect_stdout()
    run_tests(
        csv_file=args.csv_file,
        data_file=str(args.data_file),
        debug=args.debug,
        num_fragments=args.num_fragments,
        num_parity=args.num_parity,
    )


if __name__ == "__main__":
    main()
