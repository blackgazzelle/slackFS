import json
import subprocess
from dataclasses import dataclass
from pathlib import Path

from pyeclib.ec_iface import ECDriver

from logger import LOGGER


@dataclass
class Hide:
    cover_file: Path
    map_file: Path
    disk_file: Path
    parity_count: int
    frag_count: int
    ec_type: int

    def strip_null(self) -> tuple[bytes, list]:
        byte_count, null_len = 0, 0
        stripped_bytes = b""
        mappings = list()
        LOGGER.info("Stripping null bytes")

        with open(self.disk_file, "rb") as df:
            while True:
                b = df.read(1)
                if not b:
                    break
                byte_count += 1
                # Byte is null, see if it is a chain of null bytes, if not then just write the amount of null bytes
                # read
                if b == b"\00":
                    null_len += 1
                    null_start = byte_count - 1
                    while True:
                        b = df.read(1)
                        byte_count += 1
                        if b != b"\00":
                            mappings.append((null_start, null_len))
                            null_len = 0
                            break
                        null_len += 1
                stripped_bytes += b

        LOGGER.info(f"{self.disk_file} successfully stripped")
        return stripped_bytes, mappings

    def encode(self, stripped_bytes) -> list[bytes]:
        ec_driver = ECDriver(k=self.frag_count, m=self.parity_count, ec_type=self.ec_type)
        fragments = ec_driver.encode(stripped_bytes)
        LOGGER.info("Fragments have been successfully created")
        return fragments

    def hide(self, fragments: list) -> dict:
        # for each fragment loop over cover_file list and hide available slack worth of byte
        # record filenames,bytes_stored in a dictionary where key is fragment
        file_mapping: dict[int, list] = dict()
        # debug_fp = open("in.frags", "wb")
        with open(self.cover_file, "r") as fp:
            for i, fragment in enumerate(fragments):
                frag_len = len(fragment)
                file_mapping[i] = list()
                start = 0
                # debug_fp.write(f"FRAG: {i}\r\n\r\n".encode())
                while frag_len > 0:
                    line = fp.readline().strip().split(",")
                    filename = line[0]
                    slack = int(line[1])
                    # record bytes_stored based off of frag_len compared to slack space
                    bytes_stored = 0
                    if frag_len < slack:
                        bytes_stored = frag_len
                    else:
                        bytes_stored = slack

                    # do file hide
                    completed_process = subprocess.run(
                        f"sudo bmap --mode putslack {filename}",
                        shell=True,
                        capture_output=True,
                        input=fragment[start : start + bytes_stored],
                    )

                    LOGGER.debug(completed_process)
                    if completed_process.returncode == 0:
                        LOGGER.debug(f"START/BYTES: {start}, {bytes_stored}, {start+bytes_stored}, {len(fragment)}")
                        LOGGER.debug(fragment[start : start + bytes_stored])
                        # debug_fp.write(f"FILENAME: {filename}".encode())
                        # debug_fp.write(fragment[start:bytes_stored])

                        # Account for changes in fragment length
                        frag_len -= slack
                        start += bytes_stored
                        LOGGER.debug(f"Hid {bytes_stored} bytes for fragment {i}")

                        # record for the frag the files and sizes used
                        file_mapping[i].append((filename, bytes_stored))
                    else:
                        raise Exception(f"Failed to hide file: command info: {completed_process}")
        # debug.fp.close()
        LOGGER.info("Hiding done")
        return file_mapping

    def run(self) -> None:
        # Strip null bytes
        stripped_bytes, null_mappings = self.strip_null()
        LOGGER.debug(stripped_bytes)

        # Encode remaining bytes
        fragments = self.encode(stripped_bytes)
        LOGGER.debug(fragments)

        # Hide fragments
        file_mappings = self.hide(fragments)
        LOGGER.debug(f"NULL_MAPPINGS: {null_mappings}")
        LOGGER.debug(f"FILE_MAPPINGS: {file_mappings}")

        # Store mappings in the map file
        mappings: dict = dict()
        mappings["null_mapping"] = null_mappings
        mappings["file_mapping"] = file_mappings
        with open(self.map_file, "w") as fp:
            json.dump(mappings, fp, indent=1)

        LOGGER.info(f"SUCCESS: file has been hidden mappings are save in {self.map_file}")
