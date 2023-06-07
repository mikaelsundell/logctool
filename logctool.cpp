//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>

// imath
#include <Imath/ImathMatrix.h>
#include <Imath/ImathVec.h>

// openimageio
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/argparse.h>
#include <OpenImageIO/filesystem.h>
#include <OpenImageIO/sysutil.h>

#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

using namespace OIIO;

// opencolorio
#include <OpenColorIO/OpenColorIO.h>
using namespace OpenColorIO_v2_2dev;

// json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// logc tool
struct LogCTool
{
    bool help = false;
    bool verbose = false;
    int ei = 800;
    int width = 1024;
    int height = 512;
    int channels = 3;
    float midgray = 0.18;
    std::string dataformat;
    std::string convertlut;
    std::string outfilename;
    std::string outfalsecolor;
    std::string outzones;
    // Example
    // logctool -ei 800
    //          --format 10bit
    //          --convertlut "arri2bm6k.cube"
    //          --outfilename "ei800.dpx"
    //          --outfalsecolor "ei800_falsecolor.cube"
    int code = EXIT_SUCCESS;
};

static LogCTool tool;

static int
set_dataformat(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.dataformat = argv[1];
    return 0;
}

static int
set_ei(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.ei = Strutil::stoi(argv[1]);
    return 0;
}

static int
set_outfilename(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.outfilename = argv[1];
    return 0;
}

static void
print_help(ArgParse& ap)
{
    ap.print_help();
}

// logc gamma
struct LogCGamma
{
    int ei;
    float cut;
    float a;
    float b;
    float c;
    float d;
    float e;
    float f;
    float lin2log(float lin)
    {
        return ((lin > cut) ? c * log10(a * lin + b) + d : e * lin + f);
    }
    float log2lin(float log)
    {
        return ((log > e * cut + f) ? (pow(10, (log - d) / c) - b) / a : (log - f) / e);
    }
};



// TODO:
//  compute 1D signal data instead of 3 components RGB with equal values
//  compute gammas for EI300 => EI12000


// main
int 
main( int argc, const char * argv[])
{
    const double vmin = channel_traits<float>::min_value();
    const double vmax = channel_traits<float>::max_value();
    
    
    
    // Helpful for debugging to make sure that any crashes dump a stack
    // trace.
    Sysutil::setup_crash_stacktrace("stdout");

    Filesystem::convert_native_arguments(argc, (const char**)argv);
    ArgParse ap;

    ap.intro("logctool -- a set of utilities for processing logc encoded images\n");
    ap.usage("logctool [options] filename...")
      .add_help(false)
      .exit_on_error(true);
    
    ap.separator("General flags:");
    ap.arg("--help", &tool.help)
      .help("Print help message");
    
    ap.arg("-v", &tool.verbose)
      .help("Verbose status messages");
    
    ap.arg("-ei %d:EI")
      .help("LogC exposure index")
      .action(set_ei);
    
    ap.arg("-dataformat %s:DATAFORMAT")
      .help("LogC format (float, uint10, uint16 and unit32)")
      .action(set_dataformat);
    
    ap.separator("Output flags:");
    ap.arg("--convertlut", &tool.convertlut)
      .help("Optional convert lut to other log format");

    ap.arg("--outfilename %s:OUTFILENAME")
      .help("Optional filename of output log steps")
      .action(set_outfilename);
    
    ap.arg("--outfalsecolor", &tool.outfalsecolor)
      .help("Optional filename of output falsecolor lut");
    
    ap.arg("--outzones", &tool.outfalsecolor)
      .help("Optional filename of output zones lut");

    // clang-format on
    if (ap.parse_args(argc, (const char**)argv) < 0) {
        std::cerr << "error: " << ap.geterror() << std::endl;
        print_help(ap);
        ap.abort();
        return EXIT_FAILURE;
    }
    if (ap["help"].get<int>()) {
        print_help(ap);
        ap.abort();
        return EXIT_SUCCESS;
    }
    if (!tool.ei) {
        std::cerr << "error: must have ei parameter\n";
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (!tool.dataformat.size()) {
        std::cerr << "error: must have data format parameter\n";
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (argc <= 1) {
        ap.briefusage();
        std::cout << "\nFor detailed help: logctool --help\n";
        return EXIT_FAILURE;
    }
    
    // logc program
    std::cout << "logctool -- a set of utilities for processing logc encoded images" << std::endl;
    
    // logc midgray
    float midgray = 0.18f;
    float midvalue = 0.0;
    float midlog = 0.0;
    
    // logc gammas
    LogCGamma gamma;
    std::vector<LogCGamma> gammas =
    {
        //              ei    cut       a         b         c         d         e         f
        LogCGamma() = { 160,  0.005561, 5.555556, 0.080216, 0.269036, 0.381991, 5.842037, 0.092778 },
        LogCGamma() = { 200,  0.006208, 5.555556, 0.076621, 0.266007, 0.382478, 5.776265, 0.092782 },
        LogCGamma() = { 250,  0.006871, 5.555556, 0.072941, 0.262978, 0.382966, 5.710494, 0.092786 },
        LogCGamma() = { 320,  0.007622, 5.555556, 0.068768, 0.259627, 0.383508, 5.637732, 0.092791 },
        LogCGamma() = { 400,  0.008318, 5.555556, 0.064901, 0.256598, 0.383999, 5.571960, 0.092795 },
        LogCGamma() = { 500,  0.009031, 5.555556, 0.060939, 0.253569, 0.384493, 5.506188, 0.092800 },
        LogCGamma() = { 640,  0.009840, 5.555556, 0.056443, 0.250219, 0.385040, 5.433426, 0.092805 },
        LogCGamma() = { 800,  0.010591, 5.555556, 0.052272, 0.247190, 0.385537, 5.367655, 0.092809 },
        LogCGamma() = { 1000, 0.011361, 5.555556, 0.047996, 0.244161, 0.386036, 5.301883, 0.092814 },
        LogCGamma() = { 1280, 0.012235, 5.555556, 0.043137, 0.240810, 0.386590, 5.229121, 0.092819 },
        LogCGamma() = { 1600, 0.013047, 5.555556, 0.038625, 0.237781, 0.387093, 5.163350, 0.092824 }
    };
    for(LogCGamma logcgamma : gammas)
    {
        if (tool.ei == logcgamma.ei) {
            gamma = logcgamma;
            break;
        }
    }
    
    if (gamma.ei > 0)
    {
        std::cout << "info: ei:   " << gamma.ei << std::endl;
        if (tool.verbose)
        {
            std::cout << " cut:   " << gamma.cut << "\n"
                      << " a:     " << gamma.a << "\n"
                      << " b:     " << gamma.b << "\n"
                      << " c:     " << gamma.c << "\n"
                      << " d:     " << gamma.d << "\n"
                      << " e:     " << gamma.e << "\n"
                      << " f:     " << gamma.f << "\n";
        }
    }
    else
    {
        std::cerr << "error: unknown ei: " << tool.ei << std::endl;
        ap.abort();
        return EXIT_FAILURE;
    }
    
    // image format
    TypeDesc typedesc = TypeDesc::UNKNOWN;
    bool is10bit = false;
    
    if (tool.dataformat == "float") {
        typedesc = TypeDesc::FLOAT;
    }
    else if (tool.dataformat == "uint8") {
        typedesc = TypeDesc::UINT8;
    }
    else if (tool.dataformat == "uint10") {
        typedesc = TypeDesc::UINT16;
        is10bit = true;
    }
    else if (tool.dataformat == "uint16") {
        typedesc = TypeDesc::UINT16;
    }
    else if (tool.dataformat == "uint32") {
        typedesc = TypeDesc::UINT32;
    }
    else {
        std::cerr << "error: unknown data format: " << tool.dataformat << std::endl;
        ap.abort();
        return EXIT_FAILURE;
    }
    
    // type info
    std::cout << "info: data format: " << typedesc << std::endl;
    if (is10bit) {
        std::cout << " 10bit: " << "yes" << std::endl;
    }
    
    // type data
    int typesize = typedesc.size();
    int typelimit = pow(2, typesize*8) - 1;
    int type10bitlimit = pow(2, 10) - 1;
    
    // signal data
    int signalsize = 17;
    void* signaldata = (void*)malloc(typesize * signalsize);
    memset(signaldata, 0, typesize * signalsize);

    // signal info
    std::cout << "info: signal: " << signalsize << " stops" << std::endl;
    
    for(int s=0; s<signalsize; s++)
    {
        int relativestop = s-8;
        float lin = pow(2, relativestop) * midgray;
        float log = std::min<float>(gamma.lin2log(lin), typelimit);
        
        if (tool.verbose)
        {
            std::cout << " stop:  " << s << "\n"
                      << "   lin: " << lin << "\n"
                      << "   log: " << log << "\n";
        }
        
        if (typedesc.is_floating_point()) {
            memcpy((char*)signaldata + typesize*s, &log, typesize);
            if (relativestop == 0) {
                midvalue = log;
                midlog = log;
            }
            if (tool.verbose) {
                std::cout << "   value:" << log << std::endl;
            }
        } else {
            int value = round(typelimit * log);
            memcpy((char*)signaldata + typesize*s, &value, typesize);
            if (relativestop == 0) {
                if (is10bit) {
                    midvalue = round(type10bitlimit * log);
                } else {
                    midvalue = value;
                }
                midlog = log;
            }
            if (tool.verbose) {
                std::cout << " value: " << value << std::endl;
            }
        }
    }
    if (tool.outfilename.size()) {
        
        // signal info
        std::cout << "info: image: " << tool.outfilename << std::endl;
        
        // image data
        int width = tool.width;
        int height = tool.height;
        int channels = tool.channels;
        int imagesize = width * height * channels;
        
        if (tool.verbose)
        {
            std::cout << " width:  " << width << "\n"
                      << "height:  " << height << "\n"
                      << "channels:" << channels << "\n";
        }

        void* imagedata = (void*)malloc(typesize * imagesize);
        memset(imagedata, 0, typesize * imagesize);
        
        int stopwidth = floor(width / signalsize);
        void* pixeldata = (void*)malloc(typesize);
        
        for(int y=0; y<height; ++y)
        {
            int stopcount=0;
            float gradientstop=0.0;
            for(int x=0; x<width; ++x)
            {
                int stop = std::min<int>(signalsize - 1, stopcount);
                if (((float)y / height) > 0.5)
                {
                    float relativestop = (((float)x / width) * (signalsize - 1)) - 8;
                    float lin = pow(2, relativestop) * midgray;
                    float log = gamma.lin2log(lin);
                    
                    if (typedesc.is_floating_point()) {
                        memcpy(pixeldata, &log, typesize);
                    } else {
                        int value = round(typelimit * log);
                        memcpy(pixeldata, &value, typesize);
                    }
                }
                else
                {
                    if (x % stopwidth == 0) {
                        int stop = std::min(signalsize - 1, stopcount);
                        // data
                        memcpy(pixeldata, (char*)signaldata + typesize*stop, typesize);
                        stopcount++;
                    }
                }
                for(int c=0; c<channels; c++) {
                    int offset = channels * (width * y + x);
                    memcpy((char*)imagedata + typesize*offset + typesize*c, pixeldata, typesize);
                }
            }
        }
        
        // image algo
        ImageSpec spec (width, height, channels, typedesc);
        if (is10bit) {
            spec.attribute("oiio:BitsPerSample", 10);
        }
        
        ImageBuf imageBuf = ImageBuf(spec, imagedata);
        {
            float fillwidth = width * 0.3;
            float fillheight = height * 0.2;
            
            float pink[3] = { midlog, midlog, midlog };

            float xbegin = (width - fillwidth) / 2.0;
            float ybegin = (height - fillheight) / 2.0;
            float color[] = { 1, 1, 1, 1 };
            

            ImageBufAlgo::fill(imageBuf, pink,
                ROI(
                    xbegin, width - xbegin, ybegin, height - ybegin
                )
            );
            
            std::cerr << "imageBuf: " << imageBuf.geterror() << std::endl;
        }
        
        if (!imageBuf.write(tool.outfilename)) {
            std::cerr << "error: " << imageBuf.geterror() << std::endl;
        }
    }
    return 0;
}
