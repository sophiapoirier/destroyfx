#!/bin/sh

# there can be as many input arguments as you want
# they are all assumed to be PNG file names

# loop through all arguments
while (( $# >= 1 )); do
	# create temp output file
	# output file has all colorspace chunks removed and optimized compression
	TEMP_FILE_PATH="${1}".tmp
	pngcrush -rem gAMA -rem cHRM -rem iCCP -rem sRGB -l 9 "${1}" "${TEMP_FILE_PATH}"
	# optimize the compression
	if (( $? == 0 )); then
		zopflipng -m -y "${TEMP_FILE_PATH}" "${TEMP_FILE_PATH}"
		# replace the original with the new optimized output file
		if (( $? == 0 )); then
			mv -f "${TEMP_FILE_PATH}" "${1}"
		fi
	fi
	shift
done

exit $?
