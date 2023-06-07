Readme for logctool
===================

[![License](https://img.shields.io/badge/license-BSD%203--Clause-blue.svg?style=flat-square)](https://github.com/mikaelsundell/logctool/blob/master/README.md)

Introduction
------------

logctool is a set of utilities for processing image data in logc.

Documentation
-------------

Building
--------

The logctool app can be built both from commandline or using optional Xcode `-GXcode`.

```shell
mkdir build
cd build
cmake .. -DCMAKE_MODULE_PATH=<path>/logctool/modules -DCMAKE_PREFIX_PATH=<path>/3rdparty/build/macosx/arm64.debug -GXcode
cmake --build . --config Release -j 8
```

Packaging
---------

The `macdeploy.sh` script will deploy mac bundle to dmg including dependencies.

```shell
./macdeploy.sh -e <path>/logctool -d <path>/dependencies -p <path>/path to deploy
```

Web Resources
-------------

* GitHub page:        https://github.com/mikaelsundell/logctool
* Issues              https://github.com/mikaelsundell/logctool/issues

Copyright
---------

* Roboto font         https://fonts.google.com/specimen/Roboto
                      Designed by Christian Robertson
