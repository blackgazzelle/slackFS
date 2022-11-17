#!/bin/bash
# Author: Avinash Srinivasan
# Modified By: Christian Rose
#
if [ $1 ] && [ -d $1 ];
then
    echo "Getting slack space of $1 please check the textFiles folder for the TotalSlack.txt output file"
    firstC=${test:0:1}
    if [ $firstC=="/" ];
        then
        find -L $1 -type f -exec echo {} \; -exec ../bmap/bmap -mode slackbytes {} \; &>../textFiles$1TotalSlack.txt
    else
        find -L $1 -type f -exec echo {} \; -exec ../bmap/bmap -mode slackbytes {} \; &>../textFiles/$1TotalSlack.txt
    fi;
else
    echo "USAGE: ./calc_slack.bash <folder to check for slack space>"
fi;