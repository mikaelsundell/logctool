#!/bin/bash

# formats
formats=("DCI_2K:2048:1080" "DCI_4K:4096:2160")

# transforms
transforms=("50D" "200T" "250D" "500T" "CanonLog" "CanonLog2" "CanonLog3" "Cineon" "REDgamma3" "REDgamma4" "Rec709" "SonySLog1" "SonySLog2" "SonySLog3" "sRGB")

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

            # log exr
            output_file="./logctool_LogC3${image_suffix}_${name}.exr"
            logctool="./logctool --ei 800 --dataformat float --outputwidth $width --outputheight $height --outputfilename $output_file $image_option"
            echo "creating EXR $output_file"
            $logctool
            echo "command: $logctool"

            # log dpx
            output_file="./logctool_LogC3${image_suffix}_${name}.dpx"
            logctool="./logctool --ei 800 --dataformat uint10 --outputwidth $width --outputheight $height --outputfilename $output_file $image_option"
            echo "creating DPX $output_file"
            $logctool
            echo "command: $logctool"

        done
    done
done
