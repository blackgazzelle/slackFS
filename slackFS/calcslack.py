from pathlib import Path
import subprocess

from slackFS.logger import LOGGER


def calculate_slack(search_dir: Path, out_file: Path):
    LOGGER.info(f"Beginning to Calculate slack for {search_dir} this may take a while")
    completed_process = subprocess.run(
        f"sudo find -L {search_dir} -type f -exec echo "
        "{} \\; -exec bmap -mode slackbytes {} \\;",
        shell=True,
        capture_output=True,
    )
    while completed_process.stdout is None:
        LOGGER.debug("AHHHHHH")
    if completed_process.returncode != 0:
        LOGGER.error(f"Process failed: {completed_process}")
        exit(-1)
    LOGGER.debug(completed_process.stdout)
    LOGGER.info(f"Slack space of files in {search_dir} successfully calculated")
    for line in completed_process.stdout.split(b"\n"):
        LOGGER.debug(line)
