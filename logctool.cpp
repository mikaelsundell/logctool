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
using namespace OpenColorIO_v2_3;

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
    bool transforms = false;
    int ei = 800;
    int width = 1024;
    int height = 512;
    int channels = 3;
    float midgray = 0.18;
    std::string dataformat = "float";
    std::string transform;
    std::string outputtype = "stepchart";
    std::string outputfilename;
    std::string outputfalsecolorcubefile;
    std::string outputstopscubefile;
    bool outputlinear = false;
    bool outputnolabels = false;
    int code = EXIT_SUCCESS;
};

static LogCTool tool;

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

// utils - colorspaces
Imath::Vec3<float> mult_from_matrix(const Imath::Vec3<float>& src, const Imath::Matrix33<float>& matrix) {
    return src * matrix.transposed(); // imath row-order from column-order convention
}

Imath::Vec3<float> ciexyzd50_from_lab(const Imath::Vec3<float>& src) {
    const double Xn = 0.9642;
    const double Yn = 1.00000;
    const double Zn = 0.8251;
    double fy = (src.x + 16.0) / 116.0;
    double fx = fy + (src.y / 500.0);
    double fz = fy - (src.z / 200.0);
    double fx3 = std::pow(fx, 3.0);
    double fz3 = std::pow(fz, 3.0);
    float X = (fx3 > 0.008856) ? fx3 : ((fx - 16.0 / 116.0) / 7.787);
    float Y = (src.x > 8.0) ? std::pow(((src.x + 16.0) / 116.0), 3.0) : (src.x / 903.3);
    float Z = (fz3 > 0.008856) ? fz3 : ((fz - 16.0 / 116.0) / 7.787);
    X *= Xn;
    Y *= Yn;
    Z *= Zn;
    return Imath::Vec3<float>(X, Y, Z);
}

Imath::Vec3<float> ciexyzd65_from_D50(const Imath::Vec3<float>& src) {
    
    // http://www.brucelindbloom.com/index.html?Eqn_ChromAdapt.html
    Imath::Matrix33<float> matrix(
      0.9555766f, -0.0230393f, 0.0631636f,
      -0.0282895f, 1.0099416f, 0.0210077f,
      0.0122982f, -0.020483f, 1.3299098f);
    return mult_from_matrix(src, matrix);
}

// logc3 colorspace
struct LogC3Colorspace
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
    Imath::Vec3<float> ciexyz_awg(Imath::Vec3<float> color)
    {
        Imath::Matrix33<float> matrix(
            1.789066f, -0.482534f, -0.200076f,
            -0.639849f, 1.396400f, 0.194432f,
            -0.041532f, 0.082335f, 0.878868f);
        return mult_from_matrix(color, matrix);
    }
    Imath::Vec3<float> wide_awg(Imath::Vec3<float> color)
    {
        Imath::Matrix33<float> matrix(
            0.638008f, 0.214704f, 0.097744f,
            0.291954f, 0.823841f, -0.115795f,
            0.002798f, -0.067034f, 1.153294f);
        return mult_from_matrix(color, matrix);
    }
};

// lut transform
struct LutTransform
{
    std::string description;
    std::string filename;
};

// colorchecker patch
struct ColorCheckerPatch
{
    int no;
    std::string number;
    float sRGB_r;
    float sRGB_g;
    float sRGB_b;
    float cieLabd50_l;
    float cieLabd50_a;
    float cieLabd50_b;
    float munsell_hue;
    float munsell_value;
    float munsell_chroma;
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
    
    ap.arg("--transforms", &tool.transforms)
      .help("List all transforms");
    
    ap.arg("--ei %d:EI", &tool.ei)
      .help("LogC exposure index");
    
    ap.arg("--dataformat %s:DATAFORMAT", &tool.dataformat)
      .help("LogC format (default: float, uint8, uint10, uint16 and uint32)");
    
    ap.arg("--transform %s:TRANSFORM", &tool.transform)
      .help("LUT transform");
    
    ap.separator("Output flags:");
    ap.arg("--outputtype %s:OUTTYPE", &tool.outputtype)
      .help("Output type (default: stepchart, colorchecker)");
    
    ap.arg("--outputfilename %s:OUTFILENAME", &tool.outputfilename)
      .help("Output filename of log steps");

    ap.arg("--outputwidth %d:WIDTH", &tool.width)
      .help("Output width of log steps");
    
    ap.arg("--outputheight %d:HEIGHT", &tool.height)
      .help("Output height of log steps");
    
    ap.arg("--outputlinear", &tool.outputlinear)
      .help("Output linear steps");
    
    ap.arg("--outputnolabels", &tool.outputnolabels)
      .help("Output no labels");
    
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
    
    if (!tool.transforms) {
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
        if (!tool.outputtype.size()) {
            print_error("missing parameter: ", "outputtype");
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
    }
    
    // logc program
    print_info("logctool -- a set of utilities for processing logc encoded images");
    
    // colorspaces
    std::map<std::string, LutTransform> transforms;
    std::string jsonfile = resources_path("logctool.json");
    std::ifstream json(jsonfile);
    if (json.is_open()) {
        ptree pt;
        read_json(jsonfile, pt);
        for (const std::pair<const ptree::key_type, ptree&>& item : pt) {
            std::string name = item.first;
            const ptree data = item.second;
            LutTransform transform {
                resources_path(data.get<std::string>("description", "")),
                resources_path(data.get<std::string>("filename", "")),
            };

            if (!Filesystem::exists(transform.filename)) {
                print_warning("'filename' does not exist for transform: ", transform.filename);
                continue;
            }
            transforms[name] = transform;
        }
    } else {
        print_error("could not open transforms file: ", jsonfile);
        ap.abort();
        return EXIT_FAILURE;
    }
    
    if (tool.transforms) {
        print_info("Transforms:");
        for (const std::pair<std::string, LutTransform>& pair : transforms) {
            print_info("    ", pair.first);
        }
        return EXIT_SUCCESS;
    }
    
    if (tool.transform.size()) {
        if (!transforms.count(tool.transform)) {
            print_error("unknown transform: ", tool.transform);
            ap.abort();
            return EXIT_FAILURE;
        }
    }
    
    // logc midgray
    float midgray = 0.18f;
    float midlog = 0.0;
    
    // logc colorspace
    LogC3Colorspace colorspace;
    std::vector<LogC3Colorspace> colorspaces =
    {
        //              ei    cut       a         b         c         d         e         f
        LogC3Colorspace() = { 160,  0.005561, 5.555556, 0.080216, 0.269036, 0.381991, 5.842037, 0.092778 },
        LogC3Colorspace() = { 200,  0.006208, 5.555556, 0.076621, 0.266007, 0.382478, 5.776265, 0.092782 },
        LogC3Colorspace() = { 250,  0.006871, 5.555556, 0.072941, 0.262978, 0.382966, 5.710494, 0.092786 },
        LogC3Colorspace() = { 320,  0.007622, 5.555556, 0.068768, 0.259627, 0.383508, 5.637732, 0.092791 },
        LogC3Colorspace() = { 400,  0.008318, 5.555556, 0.064901, 0.256598, 0.383999, 5.571960, 0.092795 },
        LogC3Colorspace() = { 500,  0.009031, 5.555556, 0.060939, 0.253569, 0.384493, 5.506188, 0.092800 },
        LogC3Colorspace() = { 640,  0.009840, 5.555556, 0.056443, 0.250219, 0.385040, 5.433426, 0.092805 },
        //              800   default gamma
        LogC3Colorspace() = { 800,  0.010591, 5.555556, 0.052272, 0.247190, 0.385537, 5.367655, 0.092809 },
        LogC3Colorspace() = { 1000, 0.011361, 5.555556, 0.047996, 0.244161, 0.386036, 5.301883, 0.092814 },
        LogC3Colorspace() = { 1280, 0.012235, 5.555556, 0.043137, 0.240810, 0.386590, 5.229121, 0.092819 },
        LogC3Colorspace() = { 1600, 0.013047, 5.555556, 0.038625, 0.237781, 0.387093, 5.163350, 0.092824 }
    };
    for(LogC3Colorspace logccolorspace : colorspaces) {
        if (tool.ei == logccolorspace.ei) {
            colorspace = logccolorspace;
            break;
        }
    }
    
    // image data
    print_info("image data");
    if (colorspace.ei > 0) {
        print_info("ei: ", colorspace.ei);
        if (tool.verbose) {
            print_info("cut: ", colorspace.cut);
            print_info("a: ", colorspace.a);
            print_info("b: ", colorspace.b);
            print_info("c: ", colorspace.c);
            print_info("d: ", colorspace.d);
            print_info("e: ", colorspace.e);
            print_info("f: ", colorspace.f);
        }
    }
    else {
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
    
    // LUT info
    ConstConfigRcPtr config = Config::CreateRaw();
    ConstCPUProcessorRcPtr transformProcessor;
    
    if (tool.transform.size()) {
        LutTransform transform = transforms[tool.transform];
        FileTransformRcPtr filetransform = FileTransform::Create();
        filetransform->setSrc(transform.filename.c_str());
        filetransform->setInterpolation(INTERP_BEST);
        ConstProcessorRcPtr processor = config->getProcessor(filetransform);
        transformProcessor = processor->getDefaultCPUProcessor();
    }
    print_info("filename: ", tool.outputfilename);
    print_info("format: ", typedesc);
    if (is10bit) {
        print_info(" 10bit: ", "yes");
    }
    
    // transform
    if (!tool.transform.empty()) {
        print_info("transform: ", tool.transform);
    }
    
    // image data
    int width = tool.width;
    int height = tool.height;
    int channels = tool.channels;
    int imagesize = width * height * channels;
    
    if (tool.verbose) {
        print_info(" width: ", width);
        print_info(" height: ", height);
        print_info(" channels: ", channels);
    }
    
    int typesize = typedesc.size();
    int typelimit = pow(2, typesize*8) - 1;
    int type10bitlimit = pow(2, 10) - 1;
    
    if (tool.verbose) {
        print_info(" typesize: ", typesize);
        print_info(" typelimit: ", typelimit);
        print_info(" type10bitlimit: ", type10bitlimit);
    }

    if (tool.outputtype == "stepchart") {
        print_info("image: stepchart");

        // signal
        int signalsize = 17;
        void* signaldata = (void*)malloc(typesize * signalsize);
        memset(signaldata, 0, typesize * signalsize);
        
        print_info("signal stops: ", signalsize);
        for(int s=0; s<signalsize; s++) {
            int relstop = s-8;
            float lin = pow(2, relstop) * midgray;
            float log = 0.0;
            
            if (tool.outputlinear) {
                log = lin;
            } else {
                log = std::min<float>(colorspace.lin2log(lin), typelimit);
            }
            
            if (tool.verbose) {
                print_info(" stop:  ", relstop);
                print_info("   lin: ", lin);
                print_info("   log: ", log);
            }
            
            if (tool.transform.size()) {
                float rgb[3] = { log, log, log };
                transformProcessor->applyRGB(rgb);
                log = rgb[0];
                if (tool.verbose) {
                    print_info("   lut: ", log);
                }
            }

            if (typedesc.is_floating_point()) {
                memcpy((char*)signaldata + typesize*s, &log, typesize);
                if (relstop == 0) {
                    midlog = log;
                }
                if (tool.verbose) {
                    print_info("   value: ", log);
                }
            }
            else {
                int value = round(typelimit * log);
                memcpy((char*)signaldata + typesize*s, &value, typesize);
                if (relstop == 0) {
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
        // output image
        {
            void* imagedata = malloc(typesize * imagesize);
            memset(imagedata, 0, typesize * imagesize);
            
            // image algo
            ImageSpec spec (width, height, channels, typedesc);
            if (is10bit) {
                spec.attribute("oiio:BitsPerSample", 10);
            }
            ImageBuf imageBuf = ImageBuf(spec, imagedata);
            
            int stopwidth = floor(width / signalsize);
            void* pixeldata = (void*)malloc(typesize);
            std::map<int, std::pair<int, float>> stops;
            
            for(int y=0; y<height; ++y) {
                int stopcount=0;
                for(int x=0; x<width; ++x) {
                    int stop = std::min<int>(signalsize - 1, stopcount);
                    if (((float)y / height) > 0.5) {
                        float relstop = (((float)x / width) * (signalsize - 1)) - 8;
                        float lin = pow(2, relstop) * midgray;
                        float log = 0.0;
                        if (tool.outputlinear) {
                            log = lin;
                        } else {
                            log = colorspace.lin2log(lin);;
                        }
                        if (tool.transform.size()) {
                            float rgb[3] = { log, log, log };
                            transformProcessor->applyRGB(rgb);
                            log = rgb[0];
                        }
                        if (typedesc.is_floating_point()) {
                            memcpy(pixeldata, &log, typesize);
                        } else {
                            int value = round(typelimit * log);
                            memcpy(pixeldata, &value, typesize);
                        }
                    }
                    else {
                        if (x % stopwidth == 0) {
                            int stop = std::min<int>(signalsize - 1, stopcount);
                            int relativestop = stop-8;
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
            // labels
            {
                if (!tool.outputnolabels) {
                    float fillwidth = width * 0.4;
                    float fillheight = height * 0.2;
                    float fillcolor[3] = { midlog, midlog, midlog };
                    
                    std::string font = "Roboto.ttf";
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
                            std::to_string(stop.first),
                            fontmedium,
                            font_path(font),
                            fontcolor,
                            ImageBufAlgo::TextAlignX::Center,
                            ImageBufAlgo::TextAlignY::Center
                        );
                        // code and signal
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
                            ImageBufAlgo::render_text(
                                imageBuf,
                                x,
                                height * 0.04 + fontmedium,
                                code,
                                fontsmall,
                                font_path(font),
                                fontcolor,
                                ImageBufAlgo::TextAlignX::Center,
                                ImageBufAlgo::TextAlignY::Center
                            );
                            ImageBufAlgo::render_text(
                                imageBuf,
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
                        "LogC3 Ø:" +
                        str_by_float(midgray) +
                        " EI:" +
                        str_by_int(tool.ei),
                        fontlarge,
                        font_path(font),
                        fontcolor,
                        ImageBufAlgo::TextAlignX::Center,
                        ImageBufAlgo::TextAlignY::Center
                    );
                    
                    std::string logctool =
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
                        ")";

                    if (!tool.transform.empty()) {
                        logctool += " - transform: " + tool.transform;
                    }
                    
                    ImageBufAlgo::render_text(
                        imageBuf,
                        width * 0.02,
                        height - height * 0.04,
                        logctool,
                        fontsmall,
                        font_path(font),
                        fontcolor,
                        ImageBufAlgo::TextAlignX::Left,
                        ImageBufAlgo::TextAlignY::Center
                    );
                    
                    ImageBufAlgo::render_text(
                        imageBuf,
                        width - (width * 0.02),
                        height - height * 0.04,
                        "stepchart",
                        fontsmall,
                        font_path(font),
                        fontcolor,
                        ImageBufAlgo::TextAlignX::Right,
                        ImageBufAlgo::TextAlignY::Center
                    );
                    
                    if (tool.transform.size()) {
                        ImageBufAlgo::render_text(
                            imageBuf,
                            width / 2.0,
                            height / 2.0 + fontlarge,
                            "Transform: " + tool.transform,
                            fontmedium * 0.8,
                            font_path(font),
                            fontcolor,
                            ImageBufAlgo::TextAlignX::Center,
                            ImageBufAlgo::TextAlignY::Center
                        );
                    }
                }
            }
            print_info("writing output file: ", tool.outputfilename);
            
            if (!imageBuf.write(tool.outputfilename)) {
                print_error("could not write file: ", imageBuf.geterror());
            }
        }
        
    }
    else if (tool.outputtype == "colorchecker") {
        print_info("type: colorchecker");
        
        // colorchecker
        std::vector<ColorCheckerPatch> patches;
        std::string jsonfile = resources_path("colorchecker.json");
        std::ifstream json(jsonfile);
        if (json.is_open()) {
            ptree pt;
            read_json(jsonfile, pt);
            for (const std::pair<const ptree::key_type, ptree&>& item : pt) {
                int no = std::stoi(item.first);
                const ptree& data = item.second;
                ColorCheckerPatch patch;
                patch.no = no;
                patch.number = data.get<std::string>("number", "");
                patch.sRGB_r = data.get<float>("sRGB.R", 0.0f);
                patch.sRGB_g = data.get<float>("sRGB.G", 0.0f);
                patch.sRGB_b = data.get<float>("sRGB.B", 0.0f);
                patch.cieLabd50_l = data.get<float>("CIE L*a*b*.L*", 0.0f);
                patch.cieLabd50_a = data.get<float>("CIE L*a*b*.a*", 0.0f);
                patch.cieLabd50_b = data.get<float>("CIE L*a*b*.b*", 0.0f);
                std::string munsell_hue = data.get<std::string>("Munsell Notation.Hue", "");
                patch.munsell_hue = 0.0f;
                if (!munsell_hue.empty()) {
                    size_t space_pos = munsell_hue.find(' ');
                    if (space_pos != std::string::npos) {
                        std::string hue_numeric = munsell_hue.substr(0, space_pos);
                        try {
                            patch.munsell_hue = std::stof(hue_numeric);
                        } catch (...) {
                            patch.munsell_hue = 0.0f;
                        }
                    }
                }
                patch.munsell_value = data.get<float>("Munsell Notation.Value", 0.0f);
                patch.munsell_chroma = data.get<float>("Munsell Notation.Chroma", 0.0f);
                patches.push_back(patch);
            }
        }
        else {
            print_error("could not open colorpatches file: ", jsonfile);
            ap.abort();
            return EXIT_FAILURE;
        }
        
        if (patches.size() != 24) {
            print_error("could not match colorpatches 4 rows x 6 colums = 24, is now: ", patches.size());
            ap.abort();
            return EXIT_FAILURE;
        }
        
        // output image
        {
            void* imagedata = (void*)malloc(typesize * imagesize);
            memset(imagedata, 0, typesize * imagesize);
            
            // image algo
            ImageSpec spec (width, height, channels, typedesc);
            if (is10bit) {
                spec.attribute("oiio:BitsPerSample", 10);
            }
            ImageBuf imageBuf = ImageBuf(spec, imagedata);
            
            // colorchecker patches
            {
                float spacing = width * 0.02;
                int colorswidth = width * 0.8;
                int patchrows = 4;
                int patchcols = 6;
                int patchwidth = (colorswidth - (patchcols + 1) * spacing) / patchcols;
                int patchheight = ((height - height * 0.05) - (patchrows + 1) * spacing) / patchrows;

                for (int row = 0; row < patchrows; ++row) {
                    for (int col = 0; col < patchcols; ++col) {
                        int no = (row * patchcols) + col;
                        ColorCheckerPatch& patch = patches[no];
                        float d50_l = patch.cieLabd50_l;
                        float d50_a = patch.cieLabd50_a;
                        float d50_b = patch.cieLabd50_b;
                        
                        Imath::Vec3<float> xyz =
                            ciexyzd65_from_D50(ciexyzd50_from_lab(Imath::Vec3<float>(d50_l, d50_a, d50_b)));

                        Imath::Vec3<float> awg = colorspace.ciexyz_awg(xyz);
                        Imath::Vec3<float> log = Imath::Vec3<float>(
                            colorspace.lin2log(awg.x), colorspace.lin2log(awg.y), colorspace.lin2log(awg.z)
                        );
                        
                        if (tool.transform.size()) {
                            float rgb[3] = { log.x, log.y, log.z };
                            transformProcessor->applyRGB(rgb);
                            log.x = rgb[0];
                            log.y = rgb[1];
                            log.z = rgb[2];
                        }
                        void* pixeldata = malloc(typesize * channels);
                        if (typedesc.is_floating_point()) {
                            float pixel[3] = { log.x, log.y, log.z };
                            memcpy(pixeldata, pixel, typesize * channels);
                        } else {
                            float colors[3] = { log.x, log.y, log.z };
                            for (int i = 0; i < 3; ++i) {
                                int value = round(colors[i] * typelimit);
                                memcpy((char*)pixeldata + i * typesize, &value, typesize);
                            }
                        }
                        int xwidth = col * (patchwidth + spacing) + spacing;
                        int yheight = (col + 1) * (patchwidth + spacing);
                        
                        for (int y = row * (patchheight + spacing) + spacing; y < (row + 1) * (patchheight + spacing); ++y) {
                            for (int x = col * (patchwidth + spacing) + spacing; x < (col + 1) * (patchwidth + spacing); ++x) {
                                int offset = (y * width + x) * channels;
                                for (int c = 0; c < channels; ++c) {
                                    memcpy((char*)imagedata + typesize * (offset + c), (char*)pixeldata + typesize * c, typesize);
                                }
                            }
                        }
                        // labels
                        {
                            if (!tool.outputnolabels) {
                                std::string font = "Roboto.ttf";
                                float sizecode = height * 0.015;
                                float sizelabel = height * 0.025;
                                float fontcolor[] = { 1, 1, 1, 1 };
                                
                                std::string code;
                                if (typedesc.is_floating_point()) {
                                    code = str_by_float(log.x) + ", " + str_by_float(log.y) + ", " + str_by_float(log.z);
                                } else {
                                    int x = round(typelimit * log.x);
                                    int y = round(typelimit * log.y);
                                    int z = round(typelimit * log.z);
                                    if (is10bit) {
                                        code = str_by_10bit(x) + ", " + str_by_10bit(y) + ", " + str_by_10bit(z);
                                    } else {
                                        code = str_by_int(x) + ", " + str_by_int(y) + ", " + str_by_int(z);
                                    }
                                }
                                int x = col * (patchwidth + spacing) + spacing + (patchwidth / 2);
                                int y = row * (patchheight + spacing) + spacing;
              
                                ImageBufAlgo::render_text(
                                    imageBuf,
                                    x,
                                    y + (patchheight * 0.75),
                                    patch.number,
                                                          sizecode,
                                    font_path(font),
                                    fontcolor,
                                    ImageBufAlgo::TextAlignX::Center,
                                    ImageBufAlgo::TextAlignY::Center
                                );
                                
                                ImageBufAlgo::render_text(
                                    imageBuf,
                                    x,
                                    y + (patchheight * 0.9),
                                    code,
                                    sizelabel,
                                    font_path(font),
                                    fontcolor,
                                    ImageBufAlgo::TextAlignX::Center,
                                    ImageBufAlgo::TextAlignY::Center
                                );
                            }
                        }
                        free(pixeldata);
                    }
                }
                // reference patches
                {
                    int referencex = colorswidth;
                    int referenceheight = ((height - height * 0.05) - (2 + 1) * spacing) / 2;
                    for (int i = 0; i < 2; ++i) {
                        int no;
                        if (i == 0) {
                            no = 18; // white (0.05")
                        } else {
                            no = 23; // black (1.50")
                        }
                        ColorCheckerPatch& patch = patches[no];
                        float d50_l = patch.cieLabd50_l;
                        float d50_a = patch.cieLabd50_a;
                        float d50_b = patch.cieLabd50_b;
                        
                        Imath::Vec3<float> xyz =
                            ciexyzd65_from_D50(ciexyzd50_from_lab(Imath::Vec3<float>(d50_l, d50_a, d50_b)));

                        Imath::Vec3<float> awg = colorspace.ciexyz_awg(xyz);
                        Imath::Vec3<float> log = Imath::Vec3<float>(
                            colorspace.lin2log(awg.x), colorspace.lin2log(awg.y), colorspace.lin2log(awg.z)
                        );
                        
                        if (tool.transform.size()) {
                            float rgb[3] = { log.x, log.y, log.z };
                            transformProcessor->applyRGB(rgb);
                            log.x = rgb[0];
                            log.y = rgb[1];
                            log.z = rgb[2];
                        }
                        void* pixeldata = malloc(typesize * channels);
                        if (typedesc.is_floating_point()) {
                            float pixel[3] = { log.x, log.y, log.z };
                            memcpy(pixeldata, pixel, typesize * channels);
                        } else {
                            float colors[3] = { log.x, log.y, log.z };
                            for (int i = 0; i < 3; ++i) {
                                int value = round(colors[i] * typelimit);
                                memcpy((char*)pixeldata + i * typesize, &value, typesize);
                            }
                        }
                        for (int y = i * (referenceheight + spacing) + spacing; y < ((i + 1) * (referenceheight + spacing)) - 1; ++y) {
                            for (int x = referencex; x < width - spacing; ++x) {
                                int offset = (y * width + x) * channels;
                                for (int c = 0; c < channels; ++c) {
                                    memcpy((char*)imagedata + typesize * (offset + c), (char*)pixeldata + typesize * c, typesize);
                                }
                            }
                        }
                        // labels
                        {
                            if (!tool.outputnolabels) {
                            std::string font = "Roboto.ttf";
                            float sizecode = height * 0.015;
                            float sizelabel = height * 0.025;
                            float fontcolor[] = { 1, 1, 1, 1 };
                            
                            std::string code;
                            if (typedesc.is_floating_point()) {
                                code = str_by_float(log.x) + ", " + str_by_float(log.y) + ", " + str_by_float(log.z);
                            } else {
                                int x = round(typelimit * log.x);
                                int y = round(typelimit * log.y);
                                int z = round(typelimit * log.z);
                                if (is10bit) {
                                    code = str_by_10bit(x) + ", " + str_by_10bit(y) + ", " + str_by_10bit(z);
                                } else {
                                    code = str_by_int(x) + ", " + str_by_int(y) + ", " + str_by_int(z);
                                }
                            }
                            int x = referencex + ((width - referencex - spacing) / 2);
                            int y = i * (referenceheight + spacing) + spacing;
          
                            ImageBufAlgo::render_text(
                                imageBuf,
                                x,
                                y + (referenceheight * 0.48),
                                patch.number,
                                sizecode,
                                font_path(font),
                                fontcolor,
                                ImageBufAlgo::TextAlignX::Center,
                                ImageBufAlgo::TextAlignY::Center
                            );
                            
                            ImageBufAlgo::render_text(
                                imageBuf,
                                x,
                                y + (referenceheight * 0.55),
                                code,
                                sizelabel,
                                font_path(font),
                                fontcolor,
                                ImageBufAlgo::TextAlignX::Center,
                                ImageBufAlgo::TextAlignY::Center
                            );
                        }
                        }
                        free(pixeldata);
                    }
                }
                // labels
                {
                    if (!tool.outputnolabels) {
                        std::string font = "Roboto.ttf";
                        float fontsmall = height * 0.025;
                        float fontmedium = height * 0.04;
                        float fontlarge = height * 0.08;
                        float fontcolor[] = { 1, 1, 1, 1 };
                        
                        std::string logctool =
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
                            ")";

                        if (!tool.transform.empty()) {
                            logctool += " - transform: " + tool.transform;
                        }
                        
                        ImageBufAlgo::render_text(
                            imageBuf,
                            width * 0.02,
                            height - height * 0.04,
                            logctool,
                            fontsmall,
                            font_path(font),
                            fontcolor,
                            ImageBufAlgo::TextAlignX::Left,
                            ImageBufAlgo::TextAlignY::Center
                        );

                        ImageBufAlgo::render_text(
                            imageBuf,
                            width - (width * 0.02),
                            height - height * 0.04,
                            "colorchecker",
                            fontsmall,
                            font_path(font),
                            fontcolor,
                            ImageBufAlgo::TextAlignX::Right,
                            ImageBufAlgo::TextAlignY::Center
                        );
                    }
                }
            }
            print_info("writing output file: ", tool.outputfilename);
            
            if (!imageBuf.write(tool.outputfilename)) {
                print_error("could not write file: ", imageBuf.geterror());
            }
        }
    } else {
        print_error("unknown output type: ", tool.outputtype);
        ap.abort();
        return EXIT_FAILURE;
    }
    
    // output stops cube (LUT) file
    if (tool.outputfalsecolorcubefile.length()) {
        print_info("writing output false color cube (lut) file: ", tool.outputfalsecolorcubefile);
        
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
            float log = std::min<float>(colorspace.lin2log(lin), 1.0f);
            if (tool.transform.size()) {
                float rgb[3] = { log, log, log };
                transformProcessor->applyRGB(rgb);
                log = rgb[0];
            }
            color[0] = log;
        }
    
        for (unsigned i = 0; i < nsize; ++i) {
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
        if (outputFile) {
            outputFile << "# LogCTool False color LUT" << std::endl;
            outputFile << "#   Input: LogC3 EI: " << tool.ei << std::endl;
            if (tool.transform.size()) {
            outputFile << "#        : Transform: " << tool.transform << std::endl;
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
    if (tool.outputstopscubefile.length()) {
        print_info("writing output stops cube (lut) file: ", tool.outputstopscubefile);
        
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
            float log = std::min<float>(colorspace.lin2log(lin), 1.0f);
            if (tool.transform.size()) {
                float rgb[3] = { log, log, log };
                transformProcessor->applyRGB(rgb);
                log = rgb[0];
            }
            color[0] = log;
        }
    
        for (unsigned i = 0; i < nsize; ++i) {
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
        if (outputFile) {
            outputFile << "# LogCTool Stops LUT" << std::endl;
            outputFile << "#   Input: LogC3 EI: " << tool.ei << std::endl;
            if (tool.transform.size()) {
            outputFile << "#        : Transform: " << tool.transform << std::endl;
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
    return 0;
}
