#!/bin/sh

while (( $# >= 1 )); do
	pngcrush -rem gAMA -rem cHRM -rem iCCP -rem sRGB -l 9 "$1" "$1".tmp
	rm "$1"
	mv "$1".tmp "$1"
	shift
done

exit 0
