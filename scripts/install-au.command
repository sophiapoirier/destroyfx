#!/bin/sh


# remember this so we can revert when done
ENTRYDIR=`pwd`
# we want to change dir to this script's parent directory because 
# that's where everything to be installed should be located, too
cd "`dirname "${0}"`"


# by default, don't copy the files and don't delete them
# but if there's an argument, then move or copy+delete the files
CLEANUP=0
FILEINSTALLER=cp
if (( $# >= 1 ))
then
	CLEANUP=1
	FILEINSTALLER=mv
fi


# this is the starting point for all relative installation paths
INSTALL_DOMAIN_BASE="${HOME}"
# quicky hack - an argument to this script will change 
# the install domain from user to local
if (( $# >= 2 ))
then
	INSTALL_DOMAIN_BASE=""
fi


# everything being installed goes into Library, so first make sure that exists
LIBRARY="${INSTALL_DOMAIN_BASE}"/Library
if [ ! -d "${LIBRARY}" ]
then
	echo "error:  required directory \""$LIBRARY"\" does not exist!"
	exit -1
fi



# # # INSTALL THE AUDIO UNITS # # #

# make sure that the install location for the AU(s) exists
AU_INSTALL_DEST="${INSTALL_DOMAIN_BASE}"/Library/Audio/Plug-Ins/Components/
if [ ! -d "${AU_INSTALL_DEST}" ]
then
	echo "error:  required install location \""$AU_INSTALL_DEST"\" does not exist!"
	exit -1
fi

# copy (don't move, cuz that syntax fails with mv and a source directory) 
# any .component bundles (AUs) to the AU directory
cp -R -f *.component "${AU_INSTALL_DEST}"
AU_CP_ERROR=$?
# if that fails, that's a fatal error for the installation, so bail out
if (( $AU_CP_ERROR ))
then
	echo "error:  could not install the plugin!  (cp error "$AU_CP_ERROR")"
	exit $AU_CP_ERROR
# if successful, though, we can show a reassuring message to the user, 
# delete the copied AUs, and trigger a refresh of the Component Manager 
# to make sure that the new AUs become registered
else
	printf "\n\tplugin successfully installed!\n\n"
	touch -cf "${AU_INSTALL_DEST}"
	if (( $CLEANUP ))
	then
		rm -R -f *.component
	fi
fi



# # # INSTALL THE DOCUMENTATION # # #

# the install directories for the documentation may not already exist, 
# so check and create them if necessary...
# ...first check the parent directory
DOCS_INSTALL_DEST="${INSTALL_DOMAIN_BASE}"/Library/Documentation
if [ ! -d "${DOCS_INSTALL_DEST}" ]
then
	mkdir "${DOCS_INSTALL_DEST}"
fi
# ...now check the destination directory
DOCS_INSTALL_DEST="${DOCS_INSTALL_DEST}"/"Destroy FX"/
if [ ! -d "${DOCS_INSTALL_DEST}" ]
then
	mkdir "${DOCS_INSTALL_DEST}"
fi

# check the destination again now to make sure that 
# it was created successfully, if it did not already exist
DOCS_CP_ERROR=-3
if [ ! -d "${DOCS_INSTALL_DEST}" ]
# destination doesn't exist, not a fatal error, so just show a warning
then
	echo "warning:  could not install documentation because install location \""$DOCS_INSTALL_DEST"\" does not exist"
# but if it exists, try to move all documentation files into there
else
#	fooey, -v does not work with the cp included in Mac OS X 10.2
#	$FILEINSTALLER -f -v *.html "${DOCS_INSTALL_DEST}"
	$FILEINSTALLER -f *.html "${DOCS_INSTALL_DEST}"
	DOCS_CP_ERROR=$?
fi



# # # SHOW THE USER THE INSTALL DIRECTORIES # # #

# this is just a nice convenience, opening up the directories 
# where stuff was installed, in Finder windows 

# we know that the AU installation succeeded if we got here, so open that
open "${AU_INSTALL_DEST}"
# open the documentation installation, if it succeeded
if (( $DOCS_CP_ERROR ))
# but show a warning if the docs installation file moving failed
then
	echo "warning:  could not install documentation  ("$FILEINSTALLER" error "$DOCS_CP_ERROR")"
else
	printf "\n\tdocumentation successfully installed!\n\n"
	open "${DOCS_INSTALL_DEST}"
fi



cd "${ENTRYDIR}"


exit 0
