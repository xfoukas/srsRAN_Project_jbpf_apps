#!/bin/bash
## Copyright (c) Microsoft Corporation. All rights reserved.

CURRENT_DIR=$( cd "$(dirname "${BASH_SOURCE[0]}")" ; pwd -P )
source $(dirname $CURRENT_DIR)/set_vars.sh

Help()
{
   # Display Help
   echo "Make janus codelets ."
   echo
   echo "Syntax: make [-d <directory>|-p|-u|-t <image_tag>|-o <extra_option>]"
   echo "options:"
   echo "[-d]   Run make in <directory> subfolder."
   echo "-o     Add extra options to make (can be repeated multiple times)."
   echo
}

OPTIONS=""
USE_DIRECTORY_FLAG=false

# Get the options
while getopts "d:i:o:" option; do
	case $option in
		d) # Run make in a specific directory
			USE_DIRECTORY_FLAG=true
			USE_DIRECTORY=$OPTARG;;
		o) # Extra option to add to Makefile
			OPTIONS="$OPTIONS $OPTARG";;
		\?) # Invalid option
			echo "Error: Invalid option"
			Help
			exit;;
	esac
done

# Not sure what is the best way to check that
# the parameters passed start with a dash.
# So adding a simple hack that checks that
# the first parameter has a dash.
if [ ! -z "$1" ] && [ $OPTIND == "1" ]; then
	echo "Error: Invalid option"
	Help
	exit 1
fi
 
# Create list of codelet folders to build
codelet_folder_list=()
if [ "$USE_DIRECTORY_FLAG" = true ]; then
	codelet_folder_list+=("$USE_DIRECTORY")
else
	for lib in */; do
			[ -d "$lib" ] && codelet_folder_list+=("${lib#$CURRENT_DIR/}")
	done	
fi

echo $codelet_folder_list

# Build the codelets
for folder in "${codelet_folder_list[@]}"; do
    echo "Building $folder"

	DIRECTORY=$(pwd)

	make -C $DIRECTORY/$folder $OPTIONS 

done

