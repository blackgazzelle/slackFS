#!/usr/bin/env python3
# Author: Christian Rose
# Uses CTypes to test the slackFS with multiple backends that are available to
# run and times the retrieval and hiding process.
#

from ctypes import (
    CDLL, Structure, c_int, c_void_p, c_char_p, byref
)
import time
import csv
import os
import sys


backends = {
    #"BACKEND_NULL": 0,
    #"JERASURE_RS_VAND": 1,
    #"JERASURE_RS_CAUCHY": 2,
    #    "FLAT_XOR_HD": 3,
    #"ISA_L_RS_VAND": 4,
    #"SHSS": 5,
    "LIBERASURECODE_RS_VAND": 6,
    #"ISA_L_RS_CAUCHY": 7,
    #"LIBPHAZ": 8,
    #"EC_BACKENDS_MAX": 9,
}

chksums = {"CHKSUM_NONE": 0, "CHKSUM_CRC32": 1, "CHKSUM_MD5": 2}


class decode_data(Structure):
    _fields_ = [("num_of_frags", c_int), ("frag_size", c_int),
                ("data", c_void_p)]


def main():
    # Grab data filename
    if len(sys.argv) < 2:
        print("Need at least datafile name")
        exit()
    data_file = sys.argv[1]

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
    fp = open(data_file, "a")
    data_writer = csv.writer(fp)
    # Write CSV header if we just created the file
    if os.path.getsize(data_file) == 0:
        fields = ['backend', 'chksum', 'hide_time', 'retrieve_time']
        data_writer.writerow(fields)

    for backend in backends:
        if ecode.liberasurecode_backend_available(backends[backend]):
            print(backend)
            for chksum in chksums:
                for i in range(50):
                    print(chksum)
                    # Calculate the amount of time it takes to strip, encode,
                    # and hide
                    print("Hide")
                    start_time = time.time()
                    ret = fhh.strip_null(b"null_map.txt", b"first1KB.dmg")
                    if ret < 0:
                        exit()
                    ret = fhh.file_encode(
                        b"first1KB.dmg_stripped",
                        backends[backend],
                        16,
                        4,
                        8,
                        8,
                        chksums[chksum],
                    )
                    if ret < 0:
                        exit()
                    ret = fhh.hide_file(b"frags", b"usrCoverFileList.txt")
                    if ret < 0:
                        exit()
                    hide_time = time.time() - start_time
                    print("Retrieve")

                    # Calcuate the amount of time it takes to retrive, decode,
                    # and restore
                    start_time = time.time()
                    dc = decode_data.from_address(
                        fhh.retrieve_file(b"usrCoverFileList.txt", 92, 24)
                    )
                    ret = fhh.file_decode(
                        b"first1KB.dmg_retrieved",
                        byref(dc),
                        1,
                        16,
                        4,
                        8,
                        8,
                        chksums[chksum],
                    )
                    if ret < 0:
                        exit()
                    ret = fhh.restore_null(
                        b"first1KB.dmg_retrieved",
                        b"first1KB.dmg_restored",
                        b"null_map.txt",
                        b"first1KB.dmg",
                    )
                    if ret < 0:
                        exit()
                    retrieve_time = time.time() - start_time
                    data_writer.writerow([backend, chksum, hide_time,
                                         retrieve_time])

    fp.close()


if __name__ == "__main__":
    main()
