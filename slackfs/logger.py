import logging
from dataclasses import dataclass

"""
Custom Formatter from:
https://stackoverflow.com/questions/384076/how-can-i-color-python-logging-output 
"""


class CustomFormatter(logging.Formatter):
    green = "\x1b[32m"
    blue = "\x1b[34m"
    yellow = "\x1b[33"
    red = "\x1b[31m"
    bold_red = "\x1b[31;1m"
    reset = "\x1b[0m"
    format_str = f"%(asctime)s - %(levelname)s: {reset} %(msg)s"

    FORMATS = {
        logging.DEBUG: blue + format_str,
        logging.INFO: green + format_str,
        logging.WARNING: yellow + format_str,
        logging.ERROR: red + format_str,
        logging.CRITICAL: bold_red + format_str,
    }

    def format(self, record):
        log_fmt = self.FORMATS.get(record.levelno)
        formatter = logging.Formatter(log_fmt, datefmt="%H:%M:%S")
        return formatter.format(record)


class Log:
    log: logging.Logger
    ch: logging.Handler
    fh: logging.FileHandler

    def __init__(self, log_level) -> None:
        self.log = logging.getLogger("slackFS")
        self.log.setLevel(log_level)

        self.ch = logging.StreamHandler()
        self.ch.setLevel(log_level)

        self.ch.setFormatter(CustomFormatter())
        self.log.addHandler(self.ch)

    def error(self, msg):
        self.log.error(msg)

    def debug(self, msg):
        self.log.debug(msg)

    def info(self, msg):
        self.log.info(msg)

    def set_level(self, log_level):
        self.log.setLevel(log_level)
        self.ch.setLevel(log_level)
        # if log_level == logging.DEBUG:
        #    self.fh = logging.FileHandler("debug.log")
        #    self.fh.setLevel(log_level)
        #    self.fh.setFormatter(CustomFormatter())
        #    self.log.addHandler(self.fh)


LOGGER: Log = Log(logging.INFO)
DEBUG = logging.DEBUG
