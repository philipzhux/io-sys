#!/bin/bash
dmesg -c > /dev/null
make
MAJOR=$(dmesg | grep chrdev | tail -n1 | awk -F "chrdev" '{print $2}' | awk -F "(" '{print $2}' | awk -F "," '{print $1}')
MINOR=$(dmesg | grep chrdev | tail -n1 | awk -F "chrdev" '{print $2}' | awk -F "(" '{print $2}' | awk -F "," '{print $2}')
sh ./mkdev.sh "$MAJOR" "$MINOR" > /dev/null