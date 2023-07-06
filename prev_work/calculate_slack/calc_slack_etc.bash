#!/bin/bash
find -L /etc -type f -exec echo {} \; -exec sudo ../bmap/bmap -mode slackbytes {} \; &>../textFiles/etcTotalSlack.txt
