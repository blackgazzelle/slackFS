#!/bin/bash
find -L /boot -type f -exec echo {} \; -exec ../bmap/bmap -mode slackbytes {} \; &>../textFiles/bootTotalSlack.txt

