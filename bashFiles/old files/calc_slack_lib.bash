#!/bin/bash
find -L /lib -type f -exec echo {} \; -exec sudo ../bmap/bmap -mode slackbytes {} \; &>libTotalSlack.txt

