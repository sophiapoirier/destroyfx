#!/bin/sh

# there can be as many input arguments as you want
# they are all assumed to be PNG file names

# loop through all arguments
while (( $# >= 1 )); do
	# create temp output file
	# output file has all colorspace chunks removed and optimized compression
	pngcrush -rem gAMA -rem cHRM -rem iCCP -rem sRGB -l 9 "$1" "$1".tmp
	# remove the original file
	rm "$1"
	# replace the original with the new optimized output file
	mv "$1".tmp "$1"
	shift
done

exit 0
