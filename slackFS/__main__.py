#!/usr/bin/env python3
import argparse
from pathlib import Path

from pyeclib.ec_iface import ALL_EC_TYPES

from slackFS.calcslack import calculate_slack
from slackFS.hide import Hide
from slackFS.logger import DEBUG, LOGGER
from slackFS.retrieve import Retrieve


def main():
    # Main parser and common arguments
    parser = argparse.ArgumentParser(
        prog="slackFS",
        description="You can either hide/retrieve files or calculate slack space",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument("-k", "--frags", type=int, help="Number of non-parity fragments to not create", default=16)
    parser.add_argument("-p", "--parity", type=int, help="Number of parity fragments to create", default=8)
    parser.add_argument(
        "-e",
        "--ec_type",
        choices=ALL_EC_TYPES,
        default="liberasurecode_rs_vand",
        help="The backend that you want to use",
    )
    parser.add_argument("--debug", default=False, action="store_true")
    subparsers = parser.add_subparsers(
        help="Possible sub commands are hide, retrieve, and calculate slack", dest="mode"
    )

    # hide parser
    hide_parser = subparsers.add_parser("hide", help="Used to hide a fisk file in slack space denoted by cover file")
    hide_parser.add_argument(
        "-cf", "--cover_file", type=Path, help="File listing cover files and their slack sizes", required=True
    )
    hide_parser.add_argument(
        "-mf", "--map_file", type=Path, help="Map file documenting location of null bytes and hide files", required=True
    )
    hide_parser.add_argument("-df", "--disk_file", type=Path, help="File to be hidden", required=True)

    # Retrieve parser
    retrieve_parser = subparsers.add_parser("retrieve", help="Used to retrieve a hidden file from slack space")
    retrieve_parser.add_argument(
        "-mf", "--map_file", type=Path, help="Map file documenting location of null bytes and hide files", required=True
    )
    retrieve_parser.add_argument("-of", "--out_file", type=Path, help="File to write retrieved file to", required=True)

    # Calculate Slack parser
    calculate_parser = subparsers.add_parser("calculate_slack", help="Used to calculate slack space")
    calculate_parser.add_argument(
        "-d", "--dir", type=Path, help="Directory to search for cover files such as /usr", required=True
    )
    calculate_parser.add_argument(
        "-of", "--out_file", type=Path, help="File to write cover files and their slack space to", required=True
    )
    args = parser.parse_args()

    # Set appropriate log_level
    if args.debug:
        LOGGER.set_level(DEBUG)

    # Retrieve checks
    if args.mode == "retrieve" and args.map_file:
        if not args.map_file.is_file():
            LOGGER.error(f"{args.map_file} is not a valid map file")
            exit(-1)

    # Hide Checks
    if args.mode == "hide":
        if args.disk_file:
            if not args.disk_file.is_file():
                LOGGER.error(f"{args.disk_file} is not a valid disk file")
                exit(-1)
        if args.cover_file:
            if not args.cover_file.is_file():
                LOGGER.error(f"{args.cover_file} is not a valid cover file")
                exit(-1)

    # Calculate checks
    if args.mode == "calculate_slack" and args.dir:
        if not args.dir.is_dir():
            LOGGER.error(f"{args.dir} is not a valid directory")
            exit(-1)

    try:
        if args.mode == "hide":
            hide = Hide(
                cover_file=args.cover_file,
                map_file=args.map_file,
                disk_file=args.disk_file,
                parity_count=args.parity,
                frag_count=args.frags,
                ec_type=args.ec_type,
            )
            hide.run()
        elif args.mode == "retrieve":
            retrieve = Retrieve(
                map_file=args.map_file,
                out_file=args.out_file,
                parity_count=args.parity,
                frag_count=args.frags,
                ec_type=args.ec_type,
            )
            retrieve.run()
        elif args.mode == "calculate_slack":
            calculate_slack(args.dir, args.out_file)
        else:
            parser.print_help()
    except Exception as e:
        LOGGER.error(e)


if __name__ == "__main__":
    main()
