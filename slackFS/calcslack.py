from pathlib import Path
import subprocess

from slackFS.logger import LOGGER


def calculate_slack(search_dir: Path, out_file: Path):
    completed_process = subprocess.run(
        f"sudo find -L {search_dir} -type f -exec echo "
        "{} \\; -exec bmap -mode slackbytes {} \\;",
        shell=True,
        capture_output=True,
    )

    if completed_process.returncode != 0:
        LOGGER.error(f"Process failed: {completed_process}")
        exit(-1)
    
    LOGGER.debug(completed_process.stdout)
