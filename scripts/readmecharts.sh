#!/bin/bash

# formats
formats=("DCI_2K:2048:1080")

# transforms
transforms=("sRGB")

# types
types=("stepchart" "colorchecker")

# loop through the formats and split each value into name, width, and height
for format in "${formats[@]}"; do
    IFS=':' read -r name width height <<< "$format"

    # loop through the transforms
    for transform in "${transforms[@]}"; do

        # loop through the types
        for type in "${types[@]}"; do
            image_option="--transform $transform --outputtype $type"
            image_suffix="_to_${transform}_${type}"

            # png
            output_file="./logctool_LogC3${image_suffix}_${name}.png"
            logctool="./logctool --ei 800 --dataformat float --outputwidth $width --outputheight $height --outputfilename $output_file $image_option"
            echo "creating PNG $output_file"
            $logctool
            echo "command: $logctool"
        done
    done
done
