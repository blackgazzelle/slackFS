#!/bin/bash
find -L /dev -type f -exec echo {} \; -exec sudo ../bmap/bmap -mode slackbytes {} \; &>../textFiles/devTotalSlack.txt

