#!/usr/bin/bash
if [ $# -le 1 ]; then
	echo "usage: burn <image file> <dst>"
	exit 1
fi

if [ "$2" == "/dev/sda" ]; then
	echo "Incorrect dest"
	exit 1
fi

echo "is your dest device connetcted to $2?"
read ret

if [ "$ret" == "y" ]; then
	echo "Burning"
	sudo dd if=$1 of=$2 bs=4M
fi

