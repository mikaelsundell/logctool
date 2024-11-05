README for logctool
==================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/logctool/blob/master/README.md)

Introduction
------------

logctool a set of utilities for processing logc encoded images. Test and verification data, including a step chart and color checker grid, can be generated for color space evaluation and testing.

![Sample image or figure.](images/image.png 'logctool')

Examples of stepchart and colorchecker using --transform sRGB for viewing.

Usage
-----

Print logctool help message with flag ```--help```.

```shell
logctool -- a set of utilities for processing logc encoded images

Usage: logctool [options] filename...

General flags:
    --help                           Print help message
    -v                               Verbose status messages
    --transforms                     List all transforms
    --ei EI                          LogC exposure index
    --dataformat DATAFORMAT          LogC format (default: float, uint8, uint10, uint16 and uint32)
    --transform TRANSFORM            LUT transform
Output flags:
    --outputtype OUTTYPE             Output type (default: stepchart, colorchecker)
    --outputfilename OUTFILENAME     Output filename of log steps
    --outputwidth WIDTH              Output width of log steps
    --outputheight HEIGHT            Output height of log steps
    --outputlinear                   Output linear steps
    --outputnolabels                 Output no labels
    --outputfalsecolorcubefile FILE  Optional output false color cube (lut) file
    --outputstopscubefile FILE       Optional output stops cube (lut) file
```


Generate LogC stepchart in OpenEXR float
--------

```shell
./logctool
-v
--outputwidth 2048
--outputheight 1024
--dataformat float
--outputtype stepchart
--outpitfilename /Volumes/Build/github/test/logctool_LogC3.exr
```

Generate LogC colorchecker in DPX 10-bit
--------

```shell
./logctool
-v
--outputwidth 2048
--outputheight 1024
--dataformat uin10
--outputtype colorchecker
--outputfilename /Volumes/Build/github/test/logctool_LogC3.exr
```

Generate Conversion LUTs in Davinci Resolve
--------

```shell
./logctool
-v
--outputwidth 2048
--outputheight 1024
--dataformat float
--outputfilename /Volumes/Build/github/test/logctool_LogC3.exr
--outputstopscubefile /Volumes/Build/github/test/logctool_LogC3_out.cube
```

Download LogC charts
-------------

LogC charts are available EXR float and DPX 10-bit precision.

| LogC     | LUT | Output type | Download
| ----------- | ----------- | ----------- | ----------- |
| _LogC3_ | _LogC3_ | _Stepchart_   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_stepchart_DCI_4K.dpx) |
| LogC3    | CanonLog   | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_stepchart_DCI_4K.dpx) |
| LogC3    | CanonLog2  | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_stepchart_DCI_4K.dpx) |
| LogC3    | CanonLog3  | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_stepchart_DCI_4K.dpx) |
| LogC3    | Cineon     | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_stepchart_DCI_4K.dpx) |
| LogC3    | Rec709     | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_stepchart_DCI_4K.dpx) |
| LogC3    | SonySLog1  | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_stepchart_DCI_4K.dpx) |
| LogC3    | SonySLog2  | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_stepchart_DCI_4K.dpx) |
| LogC3    | SonySLog3  | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_stepchart_DCI_4K.dpx) |
| LogC3    | sRGB       | Stepchart   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_stepchart_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_stepchart_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_stepchart_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_stepchart_DCI_4K.dpx) |
| _LogC3_   | _LogC3_  | _Colorchecker_   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_colorchecker_DCI_4K.dpx) |
| LogC3    | CanonLog   | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog_colorchecker_DCI_4K.dpx) |
| LogC3    | CanonLog2  | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog2_colorchecker_DCI_4K.dpx) |
| LogC3    | CanonLog3  | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_CanonLog3_colorchecker_DCI_4K.dpx) |
| LogC3    | Cineon     | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Cineon_colorchecker_DCI_4K.dpx) |
| LogC3    | Rec709     | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_Rec709_colorchecker_DCI_4K.dpx) |
| LogC3    | SonySLog1  | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog1_colorchecker_DCI_4K.dpx) |
| LogC3    | SonySLog2  | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog2_colorchecker_DCI_4K.dpx) |
| LogC3    | SonySLog3  | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_SonySLog3_colorchecker_DCI_4K.dpx) |
| LogC3    | sRGB       | Colorchecker   | DCI 2K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_colorchecker_DCI_2K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_colorchecker_DCI_2K.dpx) 4K [EXR](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_colorchecker_DCI_4K.exr) [DPX](https://mikaelsundell.s3.eu-west-1.amazonaws.com/github/logctool/logctool_LogC3_to_sRGB_colorchecker_DCI_4K.dpx) |



Building
--------

The logctool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/logctool/modules -DCMAKE_INSTALL_PREFIX=<path> -DCMAKE_PREFIX_PATH=<path> -GXcode
cmake --build . --config Release -j 8
```

**Example using 3rdparty on arm64**

```shell
mkdir build
cd build
cmake ..
cmake .. -DCMAKE_INSTALL_PREFIX=<path>/3rdparty/build/macosx/arm64.debug -DCMAKE_INSTALL_PREFIX=<path>/3rdparty/build/macosx/arm64.debug -DCMAKE_CXX_FLAGS="-I<path>/3rdparty/build/macosx/arm64.debug/include/eigen3" -DBUILD_SHARED_LIBS=TRUE -GXcode
```

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -e <path>/logctool -d <path> -p <path>
```

Dependencies
-------------

| Project     | Description |
| ----------- | ----------- |
| OpenImageIO | [OpenImageIO project @ Github](https://github.com/OpenImageIO/oiio)
| OpenColorIO | [OpenColorIO project @ Github](https://github.com/AcademySoftwareFoundation/OpenColorIO)
| OpenEXR     | [OpenEXR project @ Github](https://github.com/AcademySoftwareFoundation/openexr)
| 3rdparty    | [3rdparty project containing all dependencies @ Github](https://github.com/mikaelsundell/3rdparty)

Limitations
-------------

Cube LUTs are limited in their precision therefore does not handle the toe of the curve very well.

Project
-------

* GitHub page   
https://github.com/mikaelsundell/logctool
* Issues   
https://github.com/mikaelsundell/logctool/issues


Resources
---------

* ALEXA Log C Curve    
https://github.com/mikaelsundell/utilities/blob/master/whitepapers/arri/11-06-30_Alexa_LogC_Curve.pdf


Copyright
---------

* Arri LogC   
Copyright © 2023 ARRI AG. All rights reserved.

* X-Rite ColorChecker   
Copyright © 2009, X-Rite Incorporated. All rights reserved.

* Roboto font   
https://fonts.google.com/specimen/Roboto   
Designed by Christian Robertson