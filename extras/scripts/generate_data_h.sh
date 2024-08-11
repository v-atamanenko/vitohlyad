#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
	echo "Usage: $0 <absolute_path_to_source_folder> <absolute_path_to_target_file>"
	exit 1
fi

# Assign arguments to variables
source_folder="$1"
output_file="$2"

# Define the file extensions to ignore (separated by spaces)
IGNORE_EXTENSIONS="DS_Store"

# Write the header of the .h file
echo "#ifndef VITOHLYAD_DATA_H" > $output_file
echo "#define VITOHLYAD_DATA_H" >> $output_file
echo >> $output_file
echo "// This file has been automatically generated using the \`generate_data_h.sh\` script." >> $output_file
echo "// Please do not edit manually." >> $output_file
echo >> $output_file
echo "typedef struct {" >> $output_file
echo "    const char * pathname;" >> $output_file
echo "    const char * sha1sum;" >> $output_file
echo "} TranslationFile;" >> $output_file
echo >> $output_file
echo "TranslationFile data[] = {" >> $output_file

# Generate sha1sums and format them into the array
find "$source_folder" -type f -print0 | while IFS= read -r -d '' file; do
	# Get the file extension
	extension="${file##*.}"

	# Check if the file extension is in the ignore list
	if echo "$IGNORE_EXTENSIONS" | grep -qw "$extension"; then
		continue
	fi

	sha1=$(sha1sum "$file" | awk '{print $1}')
	filepath=$(echo "$file" | sed "s|^$source_folder/|vs0:|")
	echo "	{ \"$filepath\", \"$sha1\" }," >> $output_file
done

# Close the array and the header guard
echo "};" >> $output_file
echo >> $output_file
echo "#endif // VITOHLYAD_DATA_H" >> $output_file

# Inform the user
echo "Header file '$output_file' has been generated."
