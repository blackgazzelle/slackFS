#!/usr/bin/env bash
gcc -c file_hiding_helper.c -g
gcc -c file_hiding.c -g
gcc -o file_hiding file_hiding.o file_hiding_helper.o -lerasurecode -Wall -g
