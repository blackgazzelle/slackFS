import csv
from asyncio import create_subprocess_shell, create_task, run, sleep
from asyncio.subprocess import PIPE, Process
from pathlib import Path

from logger import LOGGER


async def print_msg():
    while True:
        await sleep(30)
        LOGGER.info("Process still running")


async def run_command(search_dir: Path, out_file: Path):
    cmd = f"sudo find -L {search_dir} -type f -exec echo " "{} \\; -exec bmap -mode slackbytes {} \\;"
    completed_process: Process = await create_subprocess_shell(
        cmd=cmd,
        shell=True,
        stdout=PIPE,
        stderr=PIPE,
    )

    # Print command and start print msg coro
    LOGGER.debug(f"Running: {cmd}")
    print_msg_task = create_task(print_msg())
    stdout, _ = await completed_process.communicate()
    print_msg_task.cancel()

    # Check process returned good
    if stdout is None:
        LOGGER.error(f"Process failed: {completed_process}, {completed_process.returncode}")
        exit(-1)

    # Write files and their slack space in a csv like format
    LOGGER.info(f"Slack space of files in {search_dir} successfully calculated")
    lines = stdout.split(b"\n")[:-1]

    with open(out_file, "w") as fp:
        cover_writer = csv.writer(fp)
        i = 0
        while True:
            if i + 1 >= len(lines):
                break
            # Only add new row if file was recovered properly
            if lines[i + 1].decode().isnumeric():
                cover_writer.writerow([lines[i].decode().strip(), lines[i + 1].decode().strip()])
                i += 2
            else:
                i += 1
    if out_file.stat().st_size == 0:
        LOGGER.error("No files found with with slack space or other error occurred.")
        exit(-1)


def calculate_slack(search_dir: Path, out_file: Path):
    LOGGER.info(f"Beginning to Calculate slack for {search_dir} this may take a while")
    # Spawn async task to run process and print message coro
    run(run_command(search_dir=search_dir, out_file=out_file))
