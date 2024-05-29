#!/bin/bash

# formats
formats=("DCI_2K:2048:1080" "DCI_4K:4096:2160")

# colorspaces
colorspaces=("" "CanonLog" "CanonLog2" "CanonLog3" "Cineon" "REDgamma3" "REDgamma4" "Rec709" "SonySLog1" "SonySLog2" "SonySLog3" "sRGB")

# loop through the formats and split each value into name, width, and height
for format in "${formats[@]}"; do
    IFS=':' read -r name width height <<< "$format"
    
    # loop through the colorspaces
    for colorspace in "${colorspaces[@]}"; do
        # set the colorspace option if it is not empty
        colorspace_option=""
        if [[ -n "$colorspace" ]]; then
            colorspace_option="--colorspace $colorspace"
            colorspace_suffix="_to_${colorspace}"
        else
            colorspace_suffix=""
        fi

        # log exr
        output_file="./logctool_LogC3${colorspace_suffix}_${name}.exr"
        logctool="./logctool --ei 800 --dataformat float --outputwidth $width --outputheight $height --outputfilename $output_file $colorspace_option"
        echo "creating EXR $output_file"
        $logctool
        echo "command: $logctool"

        # log dpx
        output_file="./logctool_LogC3${colorspace_suffix}_${name}.dpx"
        logctool="./logctool --ei 800 --dataformat uint10 --outputwidth $width --outputheight $height --outputfilename $output_file $colorspace_option"
        echo "creating DPX $output_file"
        $logctool
        echo "command: $logctool"

    done
done
