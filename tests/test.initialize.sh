#!/bin/sh

. test.definitions.sh

echo "      Initializing data for tests.";

# Unzip save files
cd $SAVE_DIR
tar -zxvf *tar.gz 2> /dev/null > /dev/null;
# Get return code
if [ "$?" -ne "0" ]; then
	exit 1
fi
cd ..

# make directories
mkdir $OUTPUT_DIR $TMP_DIR

# Copy over input files
cp $INPUT_DIR/* $OUTPUT_DIR/.

# Unzip any necessary files
cd $OUTPUT_DIR
tar -zxvf *tar.gz 2> /dev/null > /dev/null;
# Get return code
if [ "$?" -ne "0" ]; then
	exit 1
fi
cd ..

# Test passed!
echo "      Data initialized.";
exit 0 
