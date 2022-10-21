#!/bin/bash
find -L /usr -type f -exec echo {} \; -exec sudo ../bmap/bmap -mode slackbytes {} 2> /dev/null\; &>../textFiles/usrTotalSlack.txt
