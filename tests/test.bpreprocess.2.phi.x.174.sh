#!/bin/sh

OUTPUT_ID="phi.x.174";
OUTPUT_DIR="output/"
TMP_DIR="tmp/";
LAYOUT_FILE=$OUTPUT_DIR"layouts.$OUTPUT_ID.txt";

echo "      Building an index.";

for SPACE in 0 1
do
	echo "        Testing -A "$SPACE;
	RG=$OUTPUT_DIR"bfast.rg.file.$OUTPUT_ID.$SPACE.brg";

	# Make an some index
	CMD="../bpreprocess/bpreprocess -r $RG -a 1 -A $SPACE -i $LAYOUT_FILE -o $OUTPUT_ID -d $OUTPUT_DIR -T $TMP_DIR";
	$CMD 2> /dev/null > /dev/null; 

	# Get return code
	if [ "$?" -ne "0" ]; then
		# Run again without piping anything
		$CMD;
		exit 1
	fi
done

# Test passed!
echo "      Index successfully built.";
exit 0