#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_binary_file> <output_header_file>"
    exit 1
fi

# Assign arguments to variables
input_file="$1"
output_file="$2"

# Derive the array name from the input file name
array_name=$(basename "$input_file" | sed 's/[^a-zA-Z0-9_]/_/g')

# Convert the array name to uppercase for the header guard
header_guard=$(echo "$array_name" | tr '[:lower:]' '[:upper:]')

# Write the header guard and array declaration to the output file
echo "#ifndef VITOHLYAD_${header_guard}_H" > $output_file
echo "#define VITOHLYAD_${header_guard}_H" >> $output_file
echo >> $output_file
echo "// This file has been automatically generated using the \`binary_to_header.sh\` script." >> $output_file
echo "// Please do not edit manually." >> $output_file
echo >> $output_file
echo "static const unsigned char ${array_name}[] = " >> $output_file

# Convert the binary file to a C array
xxd -i "$input_file" | sed 's/unsigned char.*= //; s/unsigned int.*=.*//; s/};/};/' >> $output_file

# Close the header guard
echo "#endif // VITOHLYAD_${header_guard}_H" >> $output_file

# Inform the user
echo "Header file '$output_file' has been generated."
