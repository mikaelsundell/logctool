//
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022 - present Mikael Sundell.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <regex>
#include <variant>

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

// boost
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

using namespace boost::property_tree;

// prints
template <typename T>
static void
print_info(std::string param, const T& value = T()) {
    std::cout << "info: " << param << value << std::endl;
}

static void
print_info(std::string param) {
    print_info<std::string>(param);
}

template <typename T>
static void
print_warning(std::string param, const T& value = T()) {
    std::cout << "warning: " << param << value << std::endl;
}

static void
print_warning(std::string param) {
    print_warning<std::string>(param);
}

template <typename T>
static void
print_error(std::string param, const T& value = T()) {
    std::cerr << "error: " << param << value << std::endl;
}

static void
print_error(std::string param) {
    print_error<std::string>(param);
}

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
    std::string dataformat = "float";
    std::string colorspace;
    std::string outputfilename;
    std::string outputfalsecolorcubefile;
    std::string outputstopscubefile;
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
set_width(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.width = Strutil::stoi(argv[1]);
    return 0;
}

static int
set_height(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.height = Strutil::stoi(argv[1]);
    return 0;
}

static int
set_colorspace(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.colorspace = argv[1];
    return 0;
}

static int
set_outfilename(int argc, const char* argv[])
{
    OIIO_DASSERT(argc == 2);
    tool.outputfilename = argv[1];
    return 0;
}

static void
print_help(ArgParse& ap)
{
    ap.print_help();
}

// utils - dates
std::string datetime()
{
    std::time_t now = time(NULL);
    struct tm tm;
    Sysutil::get_local_time(&now, &tm);
    char datetime[20];
    strftime(datetime, 20, "%Y-%m-%d %H:%M:%S", &tm);
    return std::string(datetime);
}

// utils - colors
Imath::Vec3<float> rgb_from_hsv(const Imath::Vec3<float>& hsv) {
    float hue = hsv.x;
    float saturation = hsv.y;
    float value = hsv.z;
    if (hue < std::numeric_limits<float>::epsilon()) {
        return Imath::Vec3<float>(0, 0, 0);
    }
    
    int hi = static_cast<int>(std::floor(hue / 60.0f)) % 6;
    float f = hue / 60.0f - static_cast<float>(hi);
    float p = value * (1.0f - saturation);
    float q = value * (1.0f - f * saturation);
    float t = value * (1.0f - (1.0f - f) * saturation);

    float r, g, b;
    switch (hi) {
        case 0: r = value; g = t; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t; g = p; b = value; break;
        case 5: r = value; g = p; b = q; break;
    }

    return Imath::Vec3<float>(r, g, b);
}

float color_by_gamma(float value, float gamma) {
    return pow(value, gamma);
}

// utils - types
int int_by_10bit(int value)
{
    return (value >> 6); // bit shift by 6 for 10 bit representation
}

std::string str_by_float(float value)
{
    char str[20];
    sprintf(str, "%.2f", value);
    return std::string(str);
}

std::string str_by_percent(float value)
{
    char str[20];
    sprintf(str, "%.0f%%", value * 100);
    return std::string(str);
}

std::string str_by_int(int value)
{
    return std::to_string(value);
}

std::string str_by_10bit(int value)
{
    return std::to_string(int_by_10bit(value)); // 10bit stored 16bit and bitshifted for strings
}


// utils - filesystem
std::string program_path(const std::string& path)
{
    return Filesystem::parent_path(Sysutil::this_program_path()) + path;
}

std::string font_path(const std::string& font)
{
    return Filesystem::parent_path(Sysutil::this_program_path()) + "/fonts/" + font;
}

std::string resources_path(const std::string& resource)
{
    return Filesystem::parent_path(Sysutil::this_program_path()) + "/resources/" + resource;
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

// logc colorspace
struct LogCColorspace
{
    std::string description;
    std::string filename;
};

// main
int 
main( int argc, const char * argv[])
{
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
    
    ap.arg("--ei %d:EI")
      .help("LogC exposure index")
      .action(set_ei);
    
    ap.arg("--dataformat %s:DATAFORMAT")
      .help("LogC format (float, uint10, uint16 and unit32)")
      .action(set_dataformat);
    
    ap.arg("--colorspace %s:COLORSPACE")
      .help("LogC colorspace")
      .action(set_colorspace);
    
    ap.separator("Output flags:");
    ap.arg("--outputfilename %s:OUTFILENAME")
      .help("Output filename of log steps")
      .action(set_outfilename);

    ap.arg("--outputwidth %s:WIDTH")
      .help("Output width of log steps")
      .action(set_width);
    
    ap.arg("--outputheight %s:HEIGHT")
      .help("Output height of log steps")
      .action(set_height);
    
    ap.arg("--outputfalsecolorcubefile %s:FILE", &tool.outputfalsecolorcubefile)
      .help("Optional output false color cube (lut) file");
    
    ap.arg("--outputstopscubefile %s:FILE", &tool.outputstopscubefile)
      .help("Optional output stops cube (lut) file");

    // clang-format on
    if (ap.parse_args(argc, (const char**)argv) < 0) {
        print_error("Could no parse arguments: ", ap.geterror());
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
        print_error("missing parameter: ", "ei");
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (!tool.dataformat.size()) {
        print_error("missing parameter: ", "dataformat");
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (!tool.outputfilename.size()) {
        print_error("missing parameter: ", "outputfilename");
        ap.briefusage();
        ap.abort();
        return EXIT_FAILURE;
    }
    if (argc <= 1) {
        ap.briefusage();
        print_error("For detailed help: logctool --help");
        return EXIT_FAILURE;
    }
    
    // logc program
    print_info("logctool -- a set of utilities for processing logc encoded images");
    
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
        //              800   default gamma
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
    
    // colorspaces
    std::map<std::string, LogCColorspace> colorspaces;
    std::string jsonfile = resources_path("colorspaces.json");
    std::ifstream json(jsonfile);
    if (json.is_open()) {
        ptree pt;
        read_json(jsonfile, pt);
        for (const std::pair<const ptree::key_type, ptree&>& item : pt) {
            std::string name = item.first;
            const ptree data = item.second;
            
            LogCColorspace colorspace {
                resources_path(data.get<std::string>("description", "")),
                resources_path(data.get<std::string>("filename", "")),
            };
            
            if (!Filesystem::exists(colorspace.filename)) {
                print_warning("'filename' does not exist for colorspace: ", colorspace.filename);
                continue;
            }
        
            colorspaces[name] = colorspace;
        }
    } else {
        print_warning("could not open colorspaces file: ", jsonfile);
        ap.abort();
        return EXIT_FAILURE;
    }
    
    if (tool.colorspace.size()) {
        if (!colorspaces.count(tool.colorspace)) {
            print_error("unknown colorspace: ", tool.colorspace);
            ap.abort();
            return EXIT_FAILURE;
        }
    }
    
    // image data
    print_info("image data");
    if (gamma.ei > 0)
    {
        print_info("    ei: ", gamma.ei);
        if (tool.verbose)
        {
            print_info("   cut: ", gamma.cut);
            print_info("     a: ", gamma.a);
            print_info("     b: ", gamma.b);
            print_info("     c: ", gamma.c);
            print_info("     d: ", gamma.d);
            print_info("     e: ", gamma.e);
            print_info("     f: ", gamma.f);
        }
    }
    else
    {
        print_error("unknown ei: ", tool.ei);
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
        is10bit = true; // 10bit stored 16bit and bitshifted in formats like DPX
    }
    else if (tool.dataformat == "uint16") {
        typedesc = TypeDesc::UINT16;
    }
    else if (tool.dataformat == "uint32") {
        typedesc = TypeDesc::UINT32;
    }
    else {
        print_error("unknown data format: ", tool.dataformat);
        ap.abort();
        return EXIT_FAILURE;
    }
    
    // type info
    print_info("format: ", typedesc);
    if (is10bit) {
        print_info(" 10bit: ", "yes");
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
    print_info("signal stops: ", signalsize);
    
    // lut info
    ConstConfigRcPtr config = Config::CreateRaw();
    ConstCPUProcessorRcPtr colorspaceProcessor;
    
    if (tool.colorspace.size()) {
        LogCColorspace colorspace = colorspaces[tool.colorspace];
        FileTransformRcPtr transform = FileTransform::Create();
        transform->setSrc(colorspace.filename.c_str());
        transform->setInterpolation(INTERP_BEST);
        
        ConstProcessorRcPtr processor = config->getProcessor(transform);
        colorspaceProcessor = processor->getDefaultCPUProcessor();
    }

    for(int s=0; s<signalsize; s++) {
        int relativestop = s-8;
        float lin = pow(2, relativestop) * midgray;
        float log = std::min<float>(gamma.lin2log(lin), typelimit);
        
        if (tool.verbose) {
            print_info(" stop:  ", relativestop);
            print_info("   lin: ", lin);
            print_info("   log: ", log);
        }
        
        if (tool.colorspace.size()) {
            float triplet[3] = { log, log, log };
            {
                colorspaceProcessor->applyRGB(triplet);
                log = triplet[0];
            }
            if (tool.verbose) {
                print_info("   lut: ", log);
            }
        }

        if (typedesc.is_floating_point()) {
            memcpy((char*)signaldata + typesize*s, &log, typesize);
            if (relativestop == 0) {
                midvalue = log;
                midlog = log;
            }
            if (tool.verbose) {
                print_info("   value: ", log);
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
                if (is10bit) {
                    print_info("   value: ", str_by_10bit(value));
                } else {
                    print_info("   value: ", str_by_int(value));
                }
            }
        }
    }
    if (tool.outputfilename.size()) {
        
        print_info("image: ", tool.outputfilename);
        
        // image data
        int width = tool.width;
        int height = tool.height;
        int channels = tool.channels;
        int imagesize = width * height * channels;
        
        if (tool.verbose) {
            print_info(" width:    ", width);
            print_info(" height:   ", height);
            print_info(" channels: ", channels);
        }

        void* imagedata = (void*)malloc(typesize * imagesize);
        memset(imagedata, 0, typesize * imagesize);
        
        int stopwidth = floor(width / signalsize);
        void* pixeldata = (void*)malloc(typesize);
        std::map<int, std::pair<int, float>> stops;
        
        for(int y=0; y<height; ++y)
        {
            int stopcount=0;
            float gradientstop=0.0;
            for(int x=0; x<width; ++x)
            {
                int stop = std::min<int>(signalsize - 1, stopcount);
                if (((float)y / height) > 0.5) {
                    
                    float relativestop = (((float)x / width) * (signalsize - 1)) - 8;
                    float lin = pow(2, relativestop) * midgray;
                    float log = gamma.lin2log(lin);
                    if (tool.colorspace.size()) {
                        float triplet[3] = { log, log, log };
                        {
                            colorspaceProcessor->applyRGB(triplet);
                            log = triplet[0];
                        }
                    }
                    
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
                        
                        int stop = std::min<int>(signalsize - 1, stopcount);
                        int relativestop = stop-8;
                        // data
                        memcpy(pixeldata, (char*)signaldata + typesize*stop, typesize);
                        if (stops.find(relativestop) == stops.end()) {
                            if (typedesc.is_floating_point()) {
                                float log = 0.0;
                                memcpy(&log, pixeldata, typesize);
                                stops[relativestop] = std::pair<int, float>(stop * stopwidth + stopwidth/2, log);
                            } else {
                                int value = 0;
                                memcpy(&value, pixeldata, typesize);
                                stops[relativestop] = std::pair<int, float>(stop * stopwidth + stopwidth/2, value);
                            }
                        }
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
            float fillwidth = width * 0.4;
            float fillheight = height * 0.2;
            float fillcolor[3] = { midlog, midlog, midlog };
            
            std::string font = "Roboto-Regular.ttf";
            float fontsmall = height * 0.025;
            float fontmedium = height * 0.04;
            float fontlarge = height * 0.08;
            float fontcolor[] = { 1, 1, 1, 1 };
            
            float xbegin = (width - fillwidth) / 2.0;
            float ybegin = (height - fillheight) / 2.0;

            ImageBufAlgo::fill(imageBuf, fillcolor,
                ROI(
                    xbegin, width - xbegin, ybegin, height - ybegin
                )
            );
            
            for (std::pair<int, std::pair<int, float>> stop : stops) {
                
                int x = stop.second.first;
                float value = stop.second.second;
  
                ImageBufAlgo::render_text(imageBuf,
                    x,
                    height * 0.04,
                    std::to_string(stop.first), // relative stop
                    fontmedium,
                    font_path(font),
                    fontcolor,
                    ImageBufAlgo::TextAlignX::Center,
                    ImageBufAlgo::TextAlignY::Center
                );

                {
                    std::string code, signal;
                    if (typedesc.is_floating_point()) {
                        code = str_by_float(value);
                        signal = str_by_percent(value);
                        
                    } else {
                        if (is10bit) {
                            code = str_by_10bit(value);
                            signal = str_by_percent((float)int_by_10bit(value) / type10bitlimit);
                        } else {
                            code = str_by_int(value);
                            signal = str_by_percent(value / typelimit);
                        }
                    }
                    
                    ImageBufAlgo::render_text(imageBuf,
                        x,
                        height * 0.04 + fontmedium,
                        code,
                        fontsmall,
                        font_path(font),
                        fontcolor,
                        ImageBufAlgo::TextAlignX::Center,
                        ImageBufAlgo::TextAlignY::Center
                    );
                    
                    ImageBufAlgo::render_text(imageBuf,
                        x,
                        height * 0.04 + fontmedium * 2,
                        signal,
                        fontsmall,
                        font_path(font),
                        fontcolor,
                        ImageBufAlgo::TextAlignX::Center,
                        ImageBufAlgo::TextAlignY::Center
                    );
                }
            }
            
            ImageBufAlgo::render_text(imageBuf,
                width / 2.0,
                height / 2.0,
                "LogC Ø:" +
                str_by_float(midgray) +
                " EI:" +
                str_by_int(tool.ei),
                fontlarge,
                font_path(font),
                fontcolor,
                ImageBufAlgo::TextAlignX::Center,
                ImageBufAlgo::TextAlignY::Center
            );
            
            ImageBufAlgo::render_text(imageBuf,
                width * 0.02,
                height - height * 0.04,
                "Logctool " +
                datetime() +
                " " +
                Filesystem::filename(tool.outputfilename) +
                " (" +
                tool.dataformat +
                " " +
                std::to_string(spec.width) +
                "x" +
                std::to_string(spec.height) +
                ")",
                fontsmall,
                font_path(font),
                fontcolor,
                ImageBufAlgo::TextAlignX::Left,
                ImageBufAlgo::TextAlignY::Center
            );
            
            if (tool.colorspace.size()) {
                
                ImageBufAlgo::render_text(imageBuf,
                    width / 2.0,
                    height / 2.0 + fontlarge,
                    "Colorspace: " + tool.colorspace,
                    fontsmall,
                    font_path(font),
                    fontcolor,
                    ImageBufAlgo::TextAlignX::Center,
                    ImageBufAlgo::TextAlignY::Center
                );
            }
        }
        
        print_info("Writing output file: ", tool.outputfilename);
        
        if (!imageBuf.write(tool.outputfilename)) {
            print_error("could not write file: ", imageBuf.geterror());
        }
        
        // output stops cube (LUT) file
        if (tool.outputfalsecolorcubefile.length())
        {
            print_info("Writing output false color cube (lut) file: ", tool.outputfalsecolorcubefile);
            
            int size = 32 + 1;
            int nsize = size * size * size;
            std::vector<float> values;
            
            // table
            std::vector<Imath::Vec4<float>> colors {
                // purple - black clipping
                Imath::Vec4<float>(-6, 250.0f, 0.6f, 0.6f),
                // blue
                Imath::Vec4<float>(-4, 200, 0.6f, 0.6f),
                // gray
                Imath::Vec4<float>(0 , 90.0, 0.1f, 0.5f),
                // pink
                Imath::Vec4<float>(1 , 330.0f, 0.8f, 0.9f),
                // yellow
                Imath::Vec4<float>(2.5 , 50, 0.8f, 0.9f),
                // red - white clipping
                Imath::Vec4<float>(6 , 5.0f, 0.6f, 1.0f),
            };
            
            for (Imath::Vec4<float>& color : colors) {
                float lin = pow(2, color[0]+0.5f) * midgray;
                float log = std::min<float>(gamma.lin2log(lin), 1.0f);
                if (tool.colorspace.size()) {
                    float rgb[3] = { log, log, log };
                    {
                        colorspaceProcessor->applyRGB(rgb);
                        log = rgb[0];
                    }
                }
                color[0] = log;
            }
        
            for (unsigned i = 0; i < nsize; ++i)
            {
                float r = std::max(0.0f, std::min(1.0f, static_cast<float>(i % size) / (size - 1)));
                float g = std::max(0.0f, std::min(1.0f, static_cast<float>((i / size) % size) / (size - 1)));
                float b = std::max(0.0f, std::min(1.0f, static_cast<float>((i / (size * size)) % size) / (size - 1)));
                float y = 0.2126 * r + 0.7152 * g + 0.0722 * b;

                int index = 0;
                for (Imath::Vec4<float>& color : colors) {
                    if (y <= color[0] || index == colors.size() - 1) {
                        Imath::Vec3<float> rgb = rgb_from_hsv(
                            Imath::Vec3<float>(color[1], color[2], color[3])
                        );
                        values.push_back(color_by_gamma(rgb[0], 2.2f));
                        values.push_back(color_by_gamma(rgb[1], 2.2f));
                        values.push_back(color_by_gamma(rgb[2], 2.2f));
                        break;
                    }
                    index++;
                }
            }
            
            std::ofstream outputFile(tool.outputstopscubefile);
            if (outputFile)
            {
                outputFile << "# LogCTool False color LUT" << std::endl;
                outputFile << "#   Input: LogC3 EI: " << tool.ei << std::endl;
                if (tool.colorspace.size()) {
                outputFile << "#        : Colorspace: " << tool.colorspace << std::endl;
                }
                outputFile << "#        : floating point data (range 0.0 - 1.0)" << std::endl;
                outputFile << "#  Output: False color luminance colors" << std::endl;
                outputFile << "#        : floating point data (range 0.0 - 1.0)" << std::endl;
                outputFile << std::endl;
                outputFile << "LUT_3D_SIZE " << size << std::endl;
                outputFile << "DOMAIN_MIN 0.0 0.0 0.0" << std::endl;
                outputFile << "DOMAIN_MAX 1.0 1.0 1.0" << std::endl;
                outputFile << std::endl;
                
                for (unsigned i = 0; i < nsize; ++i) {
                    outputFile << values[i * 3] << " "
                               << values[i * 3 + 1] << " "
                               << values[i * 3 + 2] << std::endl;
                }
                outputFile.close();
                
            } else {
                print_error("could not open output false color cube (lut) file: ", tool.outputfalsecolorcubefile);
            }
            
        }
        
        // output stops cube (LUT) file
        if (tool.outputstopscubefile.length())
        {
            print_info("Writing output stops cube (lut) file: ", tool.outputstopscubefile);
            
            int size = 32 + 1;
            int nsize = size * size * size;
            std::vector<float> values;
            
            // table
            std::vector<Imath::Vec4<float>> colors {
                // blacks
                Imath::Vec4<float>(-8, 90.0f, 0.0f, 0.0f),
                Imath::Vec4<float>(-7, 90.0f, 0.0f, 0.0f),
                // purple - toe
                Imath::Vec4<float>(-6, 270.0f, 0.6f, 0.6f),
                Imath::Vec4<float>(-5, 270.0f, 0.4f, 0.8f),
                // cyan
                Imath::Vec4<float>(-4, 180.0f, 0.6f, 0.6f),
                Imath::Vec4<float>(-3, 180.0f, 0.4f, 0.8f),
                // green
                Imath::Vec4<float>(-2, 90.0f, 0.6f, 0.6f),
                Imath::Vec4<float>(-1, 90.0f, 0.4f, 0.8f),
                // gray
                Imath::Vec4<float>(0 , 90.0, 0.1f, 0.5f),
                // yellow
                Imath::Vec4<float>(1 , 60.0f, 0.8f, 0.9f),
                Imath::Vec4<float>(2 , 60.0f, 0.6f, 1.0f),
                // orange
                Imath::Vec4<float>(3 , 40.0f, 0.8f, 0.9f),
                Imath::Vec4<float>(4 , 40.0f, 0.6f, 1.0f),
                // cerise
                Imath::Vec4<float>(5 , 330.0f, 0.8f, 0.9f),
                Imath::Vec4<float>(6 , 330.0f, 0.6f, 1.0f),
                // pink
                Imath::Vec4<float>(7 , 90.0f, 0.0f, 0.9f),
                Imath::Vec4<float>(8 , 90.0f, 0.0f, 1.0f),
            };
            
            for (Imath::Vec4<float>& color : colors) {
                float lin = pow(2, color[0]+0.5f) * midgray;
                float log = std::min<float>(gamma.lin2log(lin), 1.0f);
                if (tool.colorspace.size()) {
                    float rgb[3] = { log, log, log };
                    {
                        colorspaceProcessor->applyRGB(rgb);
                        log = rgb[0];
                    }
                }
                color[0] = log;
            }
        
            for (unsigned i = 0; i < nsize; ++i)
            {
                float r = std::max(0.0f, std::min(1.0f, static_cast<float>(i % size) / (size - 1)));
                float g = std::max(0.0f, std::min(1.0f, static_cast<float>((i / size) % size) / (size - 1)));
                float b = std::max(0.0f, std::min(1.0f, static_cast<float>((i / (size * size)) % size) / (size - 1)));
                float y = 0.2126 * r + 0.7152 * g + 0.0722 * b;

                int index = 0;
                for (Imath::Vec4<float>& color : colors) {
                    if (y <= color[0] || index == colors.size() - 1) {
                        Imath::Vec3<float> rgb = rgb_from_hsv(
                            Imath::Vec3<float>(color[1], color[2], color[3])
                        );
                        values.push_back(color_by_gamma(rgb[0], 2.2f));
                        values.push_back(color_by_gamma(rgb[1], 2.2f));
                        values.push_back(color_by_gamma(rgb[2], 2.2f));
                        break;
                    }
                    index++;
                }
            }
            
            std::ofstream outputFile(tool.outputstopscubefile);
            if (outputFile)
            {
                outputFile << "# LogCTool Stops LUT" << std::endl;
                outputFile << "#   Input: LogC3 EI: " << tool.ei << std::endl;
                if (tool.colorspace.size()) {
                outputFile << "#        : Colorspace: " << tool.colorspace << std::endl;
                }
                outputFile << "#        : floating point data (range 0.0 - 1.0)" << std::endl;
                outputFile << "#  Output: Stops luminance colors" << std::endl;
                outputFile << "#        : floating point data (range 0.0 - 1.0)" << std::endl;
                outputFile << std::endl;
                outputFile << "LUT_3D_SIZE " << size << std::endl;
                outputFile << "DOMAIN_MIN 0.0 0.0 0.0" << std::endl;
                outputFile << "DOMAIN_MAX 1.0 1.0 1.0" << std::endl;
                outputFile << std::endl;
                
                for (unsigned i = 0; i < nsize; ++i) {
                    outputFile << values[i * 3] << " "
                               << values[i * 3 + 1] << " "
                               << values[i * 3 + 2] << std::endl;
                }
                outputFile.close();
                
            } else {
                print_error("could not open output stops cube (lut) file: ", tool.outputstopscubefile);
            }
        }
    }
    return 0;
}
