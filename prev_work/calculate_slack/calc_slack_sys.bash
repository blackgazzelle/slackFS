#!/bin/bash
find -L /sys -type f -exec echo {} \; -exec sudo ../bmap/bmap -mode slackbytes {} \; &>../textFiles/sysTotalSlack.txt
