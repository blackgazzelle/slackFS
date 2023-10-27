import json
import subprocess
from dataclasses import dataclass
from pathlib import Path

from pyeclib.ec_iface import ECDriver

from logger import LOGGER


@dataclass
class Retrieve:
    map_file: Path
    out_file: Path
    parity_count: int
    frag_count: int
    ec_type: int

    def restore_null(self, decoded_data, null_mappings):
        # Put nulls back in place
        for mapping in null_mappings:
            decoded_data = decoded_data[: int(mapping[0])] + b"\00" * int(mapping[1]) + decoded_data[int(mapping[0]) :]
            LOGGER.debug(f"Adding back null bytes: {mapping}: {decoded_data}")

        with open(self.out_file, "wb") as fp:
            fp.write(decoded_data)

        LOGGER.info(f"File restored and saved in {self.out_file}")

    def decode(self, fragments) -> bytes:
        ec_driver = ECDriver(k=self.frag_count, m=self.parity_count, ec_type=self.ec_type)

        decoded_data = ec_driver.decode(fragments)
        LOGGER.info("Data as been successfully decoded")
        return decoded_data

    def retrieve(self, file_mappings: dict[int, list]) -> list[bytes]:
        # For each key in the mappings pull out the data for that fragment key
        fragments: list[bytes] = list()
        LOGGER.debug(file_mappings)
        LOGGER.info("Retrieving fragments")
        for key in file_mappings.keys():
            LOGGER.debug(key)
            LOGGER.debug(len(file_mappings[key]))

            # For each key loop over the list of files and sizes used to hide the fragments
            fragments.append(b"")
            for pair in file_mappings[key]:
                LOGGER.debug(pair)
                completed_process = subprocess.run(
                    f"sudo bmap --mode slack {pair[0]} {pair[1]}", shell=True, capture_output=True
                )
                if completed_process.returncode == 0:
                    if len(completed_process.stdout) < int(pair[1]):
                        LOGGER.error(f"Data is not the right size {len(completed_process.stdout)} != {pair[1]}")
                        LOGGER.debug(completed_process)
                        exit(-1)

                    if len(completed_process.stdout) > int(pair[1]):
                        fragments[int(key)] += completed_process.stdout[: int(pair[1])]
                    else:
                        fragments[int(key)] += completed_process.stdout
                    LOGGER.debug(f"Retrieved {len(completed_process.stdout)} bytes from {pair[0]}")
                else:
                    raise Exception(f"Failed to retrieve: command info: {completed_process}")
            LOGGER.info(f"Retrieved fragment: {key}")
        LOGGER.info("Fragments have been successfully retrieved")
        return fragments

    def run(self) -> None:
        # Read in the map file
        with open(self.map_file, "r") as fp:
            data = json.load(fp)
        LOGGER.debug(f"MAPPINGS: {data}")

        # Retrieve fragments
        fragments: list[bytes] = self.retrieve(data["file_mapping"])
        LOGGER.debug(f"Retrieved fragments: {fragments}")
        # with open("out.frags", "wb") as fp:
        #     for i, frag in enumerate(fragments):
        #         fp.write(f"FRAG: {i}\r\n\r\n".encode())
        #         fp.write(frag)
        # Decode Fragments
        decoded_data = self.decode(fragments=fragments)
        LOGGER.debug(f"Decoded data: {decoded_data!r}")

        # Restore null bytes
        self.restore_null(decoded_data, data["null_mapping"])
        LOGGER.info("SUCCESS")
