#!/bin/bash

# Define common photo and film formats
formats=("HD:1920:1080" "UltraHD:3840:2160" "UltraHD8K:7680:4320" "DCI2K:2048:1080" "DCI4K:4096:2160" "DCI8K:8192:4120")

# Loop through the formats and split each value into name, width, and height
for format in "${formats[@]}"; do
    IFS=':' read -r name width height <<< "$format"
    # log exr
    output_file="./logctool_LogC3_${name}.exr"
    ./logctool --ei 800 --dataformat float --outputwidth "$width" --outputheight "$height" --outputfilename "$output_file"
    echo "Created EXR $output_file"
    # log dpx
    output_file="./logctool_LogC3_${name}.dpx"
    ./logctool --ei 800 --dataformat uint10 --outputwidth "$width" --outputheight "$height" --outputfilename "$output_file"
    echo "Created EXR $output_file"
done
