#!/bin/sh

# there can be as many input arguments as you want
# they are all assumed to be PNG file names

# loop through all arguments
while (( $# >= 1 )); do
	# create temp output file
	# output file has all colorspace chunks removed and optimized compression
	pngcrush -rem gAMA -rem cHRM -rem iCCP -rem sRGB -l 9 "$1" "$1".tmp
	# replace the original with the new optimized output file
	if (( $? == 0 )); then
		mv -f "$1".tmp "$1"
	fi
	shift
done

exit $?
