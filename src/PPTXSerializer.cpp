#include "PPTXSerializer.hpp"
#include "SlideShow.hpp"
#include "Slide.hpp"
#include "Shape.hpp"

#include <zip.h>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <regex>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace {
// Canvas space used by the Qt editor.
constexpr int kCanvasW = 960;
constexpr int kCanvasH = 540;

// PowerPoint slide size (EMU). 13.333" x 7.5" (16:9)
// 1 inch = 914400 EMU.
constexpr long long kSlideCx = 12192000;
constexpr long long kSlideCy = 6858000;

static std::string xmlEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() * 12 / 10 + 8);
    for (char c : s) {
        switch (c) {
        case '&':  out += "&amp;";  break;
        case '<':  out += "&lt;";   break;
        case '>':  out += "&gt;";   break;
        case '"':  out += "&quot;"; break;
        case '\'': out += "&apos;"; break;
        default:   out += c;        break;
        }
    }
    return out;
}

static void addTextPart(zip_t* zip,
                        const std::string& name,
                        const std::string& content)
{
    zip_uint64_t len = static_cast<zip_uint64_t>(content.size());
    char* buf = static_cast<char*>(std::malloc(len));
    if (!buf) return;
    std::memcpy(buf, content.data(), len);

    zip_source_t* src = zip_source_buffer(zip, buf, len, 1);
    if (!src) {
        std::free(buf);
        return;
    }
    zip_file_add(zip, name.c_str(), src, ZIP_FL_OVERWRITE);
}

static void addBinaryPart(zip_t* zip,
                          const std::string& name,
                          const std::vector<uint8_t>& data)
{
    if (data.empty()) return;

    zip_uint64_t len = static_cast<zip_uint64_t>(data.size());
    uint8_t* buf = static_cast<uint8_t*>(std::malloc(len));
    if (!buf) return;
    std::memcpy(buf, data.data(), len);

    zip_source_t* src = zip_source_buffer(zip, buf, len, 1);
    if (!src) {
        std::free(buf);
        return;
    }
    zip_file_add(zip, name.c_str(), src, ZIP_FL_OVERWRITE);
}

static bool parsePngWH(const std::vector<uint8_t>& data, int& w, int& h)
{
    w = 0; h = 0;
    if (data.size() < 24) return false;

    const uint8_t sig[8] = { 137,80,78,71,13,10,26,10 };
    if (std::memcmp(data.data(), sig, 8) != 0) return false;

    auto be32 = [&](size_t off) -> uint32_t {
        return (uint32_t(data[off]) << 24) |
               (uint32_t(data[off + 1]) << 16) |
               (uint32_t(data[off + 2]) << 8) |
               (uint32_t(data[off + 3]));
    };

    // IHDR is the first chunk for PNG.
    if (!(data[12] == 'I' && data[13] == 'H' && data[14] == 'D' && data[15] == 'R'))
        return false;

    uint32_t W = be32(16);
    uint32_t H = be32(20);
    if (W == 0 || H == 0) return false;

    w = static_cast<int>(W);
    h = static_cast<int>(H);
    return true;
}

static std::string zipReadFile(zip_t* z, const std::string& name)
{
    zip_stat_t st;
    if (zip_stat(z, name.c_str(), 0, &st) != 0) return {};
    zip_file_t* f = zip_fopen(z, name.c_str(), 0);
    if (!f) return {};
    std::string buf(st.size, '\0');
    zip_fread(f, buf.data(), st.size);
    zip_fclose(f);
    return buf;
}

static bool parsePresentationSlideSize(const std::string& presXml, long long& cx, long long& cy)
{
    // Very small and robust: just search cx="..." and cy="..." inside <p:sldSz ...>
    cx = kSlideCx;
    cy = kSlideCy;

    size_t p = presXml.find("<p:sldSz");
    if (p == std::string::npos) return false;

    size_t cxPos = presXml.find("cx=\"", p);
    size_t cyPos = presXml.find("cy=\"", p);
    if (cxPos == std::string::npos || cyPos == std::string::npos) return false;

    cxPos += 4;
    cyPos += 4;

    size_t cxEnd = presXml.find('"', cxPos);
    size_t cyEnd = presXml.find('"', cyPos);
    if (cxEnd == std::string::npos || cyEnd == std::string::npos) return false;

    try {
        cx = std::stoll(presXml.substr(cxPos, cxEnd - cxPos));
        cy = std::stoll(presXml.substr(cyPos, cyEnd - cyPos));
        return (cx > 0 && cy > 0);
    } catch (...) {
        cx = kSlideCx;
        cy = kSlideCy;
        return false;
    }
}

static void extractOffXYEmu(const std::string& xml, long long& x, long long& y)
{
    x = 0;
    y = 0;

    size_t p = xml.find("<a:off ");
    if (p == std::string::npos) return;

    size_t xPos = xml.find("x=\"", p);
    size_t yPos = xml.find("y=\"", p);
    if (xPos == std::string::npos || yPos == std::string::npos) return;

    xPos += 3;
    yPos += 3;

    size_t xEnd = xml.find('"', xPos);
    size_t yEnd = xml.find('"', yPos);
    if (xEnd == std::string::npos || yEnd == std::string::npos) return;

    try {
        x = std::stoll(xml.substr(xPos, xEnd - xPos));
        y = std::stoll(xml.substr(yPos, yEnd - yPos));
    } catch (...) {
        x = 0;
        y = 0;
    }
}

static void extractExtWHEmu(const std::string& xml, long long& w, long long& h)
{
    w = 0;
    h = 0;

    size_t p = xml.find("<a:ext ");
    if (p == std::string::npos) return;

    size_t cxPos = xml.find("cx=\"", p);
    size_t cyPos = xml.find("cy=\"", p);
    if (cxPos == std::string::npos || cyPos == std::string::npos) return;

    cxPos += 4;
    cyPos += 4;

    size_t cxEnd = xml.find('"', cxPos);
    size_t cyEnd = xml.find('"', cyPos);
    if (cxEnd == std::string::npos || cyEnd == std::string::npos) return;

    try {
        w = std::stoll(xml.substr(cxPos, cxEnd - cxPos));
        h = std::stoll(xml.substr(cyPos, cyEnd - cyPos));
    } catch (...) {
        w = 0;
        h = 0;
    }
}

static std::string extractFirstTextAT(const std::string& xml)
{
    size_t a = xml.find("<a:t>");
    if (a == std::string::npos) return "";
    a += 5;
    size_t b = xml.find("</a:t>", a);
    if (b == std::string::npos) return "";
    return xml.substr(a, b - a);
}

static std::string extractCNvPrName(const std::string& block)
{
    std::smatch m;
    static const std::regex re(R"REGEX(<p:cNvPr[^>]*\sname=\"([^\"]*)\")REGEX");
    if (std::regex_search(block, m, re) && m.size() >= 2)
        return m[1].str();
    return {};
}

static ShapeKind detectShapeKindFromSp(const std::string& block)
{
    if (block.find("prst=\"ellipse\"") != std::string::npos) return ShapeKind::Ellipse;
    if (block.find("prst=\"rect\"") != std::string::npos) return ShapeKind::Rect;
    // Many text boxes are also rect geometry, but have txBox="1" on cNvSpPr.
    if (block.find("txBox=\"1\"") != std::string::npos) return ShapeKind::Text;
    return ShapeKind::Text;
}

static long long pxToEmuX(int px, long long slideCx)
{
    // px in [0..kCanvasW]
    return (slideCx * static_cast<long long>(px)) / kCanvasW;
}

static long long pxToEmuY(int px, long long slideCy)
{
    // px in [0..kCanvasH]
    return (slideCy * static_cast<long long>(px)) / kCanvasH;
}

static int emuToPxX(long long emu, long long slideCx)
{
    return static_cast<int>((emu * kCanvasW + slideCx / 2) / slideCx);
}

static int emuToPxY(long long emu, long long slideCy)
{
    return static_cast<int>((emu * kCanvasH + slideCy / 2) / slideCy);
}

static std::string theme1Xml()
{
    // A small but complete theme that PowerPoint accepts without repair.
    // (clrScheme includes accents/hlinks; fontScheme + fmtScheme are present.)
    std::ostringstream o;
    o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
      << R"(<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="SlideShow Theme">)"
      << R"(<a:themeElements>)"
      << R"(<a:clrScheme name="Office">)"
      << R"(<a:dk1><a:sysClr val="windowText" lastClr="000000"/></a:dk1>)"
      << R"(<a:lt1><a:sysClr val="window" lastClr="FFFFFF"/></a:lt1>)"
      << R"(<a:dk2><a:srgbClr val="1F497D"/></a:dk2>)"
      << R"(<a:lt2><a:srgbClr val="EEECE1"/></a:lt2>)"
      << R"(<a:accent1><a:srgbClr val="4F81BD"/></a:accent1>)"
      << R"(<a:accent2><a:srgbClr val="C0504D"/></a:accent2>)"
      << R"(<a:accent3><a:srgbClr val="9BBB59"/></a:accent3>)"
      << R"(<a:accent4><a:srgbClr val="8064A2"/></a:accent4>)"
      << R"(<a:accent5><a:srgbClr val="4BACC6"/></a:accent5>)"
      << R"(<a:accent6><a:srgbClr val="F79646"/></a:accent6>)"
      << R"(<a:hlink><a:srgbClr val="0000FF"/></a:hlink>)"
      << R"(<a:folHlink><a:srgbClr val="800080"/></a:folHlink>)"
      << R"(</a:clrScheme>)"

      << R"(<a:fontScheme name="Office">)"
      << R"(<a:majorFont><a:latin typeface="Calibri"/><a:ea typeface=""/><a:cs typeface=""/></a:majorFont>)"
      << R"(<a:minorFont><a:latin typeface="Calibri"/><a:ea typeface=""/><a:cs typeface=""/></a:minorFont>)"
      << R"(</a:fontScheme>)"

      << R"(<a:fmtScheme name="Office">)"
      << R"(<a:fillStyleLst>)"
      << R"(<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>)"
      << R"(<a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:schemeClr val="phClr"/></a:gs></a:gsLst><a:lin ang="5400000" scaled="0"/></a:gradFill>)"
      << R"(<a:solidFill><a:srgbClr val="FFFFFF"/></a:solidFill>)"
      << R"(</a:fillStyleLst>)"

      << R"(<a:lnStyleLst>)"
      << R"(<a:ln w="9525" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/></a:ln>)"
      << R"(<a:ln w="12700" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/></a:ln>)"
      << R"(<a:ln w="19050" cap="flat" cmpd="sng" algn="ctr"><a:solidFill><a:schemeClr val="phClr"/></a:solidFill><a:prstDash val="solid"/></a:ln>)"
      << R"(</a:lnStyleLst>)"

      << R"(<a:effectStyleLst>)"
      << R"(<a:effectStyle><a:effectLst/></a:effectStyle>)"
      << R"(<a:effectStyle><a:effectLst/></a:effectStyle>)"
      << R"(<a:effectStyle><a:effectLst/></a:effectStyle>)"
      << R"(</a:effectStyleLst>)"

      << R"(<a:bgFillStyleLst>)"
      << R"(<a:solidFill><a:schemeClr val="phClr"/></a:solidFill>)"
      << R"(<a:solidFill><a:srgbClr val="FFFFFF"/></a:solidFill>)"
      << R"(<a:gradFill rotWithShape="1"><a:gsLst><a:gs pos="0"><a:srgbClr val="FFFFFF"/></a:gs></a:gsLst><a:lin ang="5400000" scaled="0"/></a:gradFill>)"
      << R"(</a:bgFillStyleLst>)"
      << R"(</a:fmtScheme>)"

      << R"(</a:themeElements>)"
      << R"(<a:objectDefaults/>)"
      << R"(<a:extraClrSchemeLst/>)"
      << R"(</a:theme>)";

    return o.str();
}

static std::string grpSpPrXfrm(long long cx, long long cy)
{
    std::ostringstream o;
    o << "<p:grpSpPr><a:xfrm>"
      << "<a:off x=\"0\" y=\"0\"/>"
      << "<a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/>"
      << "<a:chOff x=\"0\" y=\"0\"/>"
      << "<a:chExt cx=\"" << cx << "\" cy=\"" << cy << "\"/>"
      << "</a:xfrm></p:grpSpPr>";
    return o.str();
}

} // namespace

bool PPTXSerializer::save(const std::vector<SlideShow>& slideshows,
                          const std::vector<std::string>& order,
                          const std::string& outputFile)
{
    std::string path = outputFile;
    if (path.size() < 5 || path.substr(path.size() - 5) != ".pptx")
        path += ".pptx";

    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!zip) return false;

    // Flatten slides in presentation order.
    std::vector<const Slide*> flatSlides;
    if (!order.empty()) {
        for (const auto& name : order) {
            auto it = std::find_if(slideshows.begin(), slideshows.end(),
                [&](const SlideShow& ss) { return ss.getFilename() == name; });
            if (it == slideshows.end()) continue;
            for (const auto& sl : it->getSlides())
                flatSlides.push_back(&sl);
        }
    } else {
        for (const auto& ss : slideshows)
            for (const auto& sl : ss.getSlides())
                flatSlides.push_back(&sl);
    }

    const int totalSlides = static_cast<int>(flatSlides.size());

    // [Content_Types].xml
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">)"
          << R"(<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>)"
          << R"(<Default Extension="xml"  ContentType="application/xml"/>)"
          << R"(<Default Extension="png"  ContentType="image/png"/>)"
          << R"(<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>)"
          << R"(<Override PartName="/docProps/app.xml"  ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>)"
          << R"(<Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>)"
          << R"(<Override PartName="/ppt/presProps.xml"   ContentType="application/vnd.openxmlformats-officedocument.presentationml.presProps+xml"/>)"
          << R"(<Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>)"
          << R"(<Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>)"
          << R"(<Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>)";

        for (int i = 0; i < totalSlides; ++i) {
            o << "<Override PartName=\"/ppt/slides/slide" << (i + 1)
              << ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>";
        }

        o << "</Types>";
        addTextPart(zip, "[Content_Types].xml", o.str());
    }

    // _rels/.rels
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
          << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>)"
          << R"(<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>)"
          << R"(<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>)"
          << R"(</Relationships>)";
        addTextPart(zip, "_rels/.rels", o.str());
    }

    // docProps/core.xml
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties")"
          << R"( xmlns:dc="http://purl.org/dc/elements/1.1/")"
          << R"( xmlns:dcterms="http://purl.org/dc/terms/")"
          << R"( xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">)"
          << R"(<dc:title>SlideShow Export</dc:title>)"
          << R"(<dc:creator>SlideShow</dc:creator>)"
          << R"(<cp:lastModifiedBy>SlideShow</cp:lastModifiedBy>)"
          << R"(<dcterms:created xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:created>)"
          << R"(<dcterms:modified xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:modified>)"
          << R"(</cp:coreProperties>)";
        addTextPart(zip, "docProps/core.xml", o.str());
    }

    // docProps/app.xml
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties")"
          << R"( xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">)"
          << R"(<Application>SlideShow</Application>)"
          << "<Slides>" << totalSlides << "</Slides>"
          << R"(<Notes>0</Notes><HiddenSlides>0</HiddenSlides>)"
          << R"(</Properties>)";
        addTextPart(zip, "docProps/app.xml", o.str());
    }

    // ppt/presProps.xml
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<p:presentationPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>)";
        addTextPart(zip, "ppt/presProps.xml", o.str());
    }

    // ppt/_rels/presentation.xml.rels
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
          << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="slideMasters/slideMaster1.xml"/>)"
          << R"(<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps" Target="presProps.xml"/>)";

        int relId = 3;
        for (int i = 0; i < totalSlides; ++i) {
            o << "<Relationship Id=\"rId" << relId++ << "\" "
              << "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" "
              << "Target=\"slides/slide" << (i + 1) << ".xml\"/>";
        }

        o << "</Relationships>";
        addTextPart(zip, "ppt/_rels/presentation.xml.rels", o.str());
    }

    // ppt/presentation.xml
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<p:presentation xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")"
          << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
          << R"(<p:sldMasterIdLst><p:sldMasterId id="2147483648" r:id="rId1"/></p:sldMasterIdLst>)";

        if (totalSlides > 0) {
            o << "<p:sldIdLst>";
            int id = 256;
            int slideRel = 3;
            for (int i = 0; i < totalSlides; ++i) {
                o << "<p:sldId id=\"" << id++ << "\" r:id=\"rId" << slideRel++ << "\"/>";
            }
            o << "</p:sldIdLst>";
        }

        o << "<p:sldSz cx=\"" << kSlideCx << "\" cy=\"" << kSlideCy << "\" type=\"screen16x9\"/>"
          << R"(<p:notesSz cx="6858000" cy="9144000"/>)"
          << R"(<p:defaultTextStyle/>)"
          << R"(</p:presentation>)";

        addTextPart(zip, "ppt/presentation.xml", o.str());
    }

    // ppt/theme/theme1.xml
    addTextPart(zip, "ppt/theme/theme1.xml", theme1Xml());

    // ppt/slideMasters/slideMaster1.xml
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<p:sldMaster xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")"
          << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")"
          << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
          << R"(<p:cSld name="Master"><p:spTree>)"
          << R"(<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>)"
          << grpSpPrXfrm(kSlideCx, kSlideCy)
          << R"(</p:spTree></p:cSld>)"
          << R"(<p:clrMap bg1="lt1" tx1="dk1" bg2="lt2" tx2="dk2" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/>)"
          << R"(<p:sldLayoutIdLst><p:sldLayoutId id="1" r:id="rId1"/></p:sldLayoutIdLst>)"
          << R"(<p:txStyles/>)"
          << R"(</p:sldMaster>)";

        addTextPart(zip, "ppt/slideMasters/slideMaster1.xml", o.str());
    }

    // ppt/slideMasters/_rels/slideMaster1.xml.rels
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
          << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>)"
          << R"(<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="../theme/theme1.xml"/>)"
          << R"(</Relationships>)";
        addTextPart(zip, "ppt/slideMasters/_rels/slideMaster1.xml.rels", o.str());
    }

    // ppt/slideLayouts/slideLayout1.xml
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<p:sldLayout xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")"
          << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")"
          << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" type="blank" preserve="1">)"
          << R"(<p:cSld name="Blank"><p:spTree>)"
          << R"(<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>)"
          << grpSpPrXfrm(kSlideCx, kSlideCy)
          << R"(</p:spTree></p:cSld>)"
          << R"(<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>)"
          << R"(</p:sldLayout>)";

        addTextPart(zip, "ppt/slideLayouts/slideLayout1.xml", o.str());
    }

    // ppt/slideLayouts/_rels/slideLayout1.xml.rels
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
          << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="../slideMasters/slideMaster1.xml"/>)"
          << R"(</Relationships>)";
        addTextPart(zip, "ppt/slideLayouts/_rels/slideLayout1.xml.rels", o.str());
    }

    // Slides + images
    int globalImageIndex = 1;

    for (int i = 0; i < totalSlides; ++i) {
        const Slide* sl = flatSlides[i];

        std::ostringstream sx;
        std::ostringstream sr;

        sx << R"(<?xml version="1.0" encoding="UTF-8"?>)"
           << R"(<p:sld xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")"
           << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")"
           << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
           << R"(<p:cSld><p:spTree>)"
           << R"(<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>)"
           << grpSpPrXfrm(kSlideCx, kSlideCy);

        sr << R"(<?xml version="1.0" encoding="UTF-8"?>)"
           << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
           << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>)";

        int shapeId = 2;
        int localRelId = 2; // rId1 is layout

        for (const auto& sh : sl->getShapes()) {
            const int xPx = sh.getX();
            const int yPx = sh.getY();

            const long long xEmu = pxToEmuX(xPx, kSlideCx);
            const long long yEmu = pxToEmuY(yPx, kSlideCy);

            if (sh.isImage()) {
                std::string imgName = "image" + std::to_string(globalImageIndex) + ".png";
                addBinaryPart(zip, "ppt/media/" + imgName, sh.getImageData());

                int pngW = 0, pngH = 0;
                if (!parsePngWH(sh.getImageData(), pngW, pngH)) {
                    pngW = 320;
                    pngH = 240;
                }

                int drawW = (sh.getW() > 1) ? sh.getW() : pngW;
                int drawH = (sh.getH() > 1) ? sh.getH() : pngH;

                // If the image is absurdly large, scale it down to fit canvas.
                if (drawW > kCanvasW || drawH > kCanvasH) {
                    const double sxF = double(kCanvasW) / double(std::max(1, drawW));
                    const double syF = double(kCanvasH) / double(std::max(1, drawH));
                    const double f = std::min(sxF, syF);
                    drawW = std::max(1, int(std::round(drawW * f)));
                    drawH = std::max(1, int(std::round(drawH * f)));
                }

                const long long cxEmu = pxToEmuX(drawW, kSlideCx);
                const long long cyEmu = pxToEmuY(drawH, kSlideCy);

                sx << "<p:pic>"
                      "<p:nvPicPr>"
                        "<p:cNvPr id=\"" << shapeId++ << "\" name=\"" << xmlEscape(sh.getName()) << "\"/>"
                        "<p:cNvPicPr/>"
                        "<p:nvPr/>"
                      "</p:nvPicPr>"
                      "<p:blipFill>"
                        "<a:blip r:embed=\"rId" << localRelId << "\"/>"
                        "<a:stretch><a:fillRect/></a:stretch>"
                      "</p:blipFill>"
                      "<p:spPr>"
                        "<a:xfrm>"
                          "<a:off x=\"" << xEmu << "\" y=\"" << yEmu << "\"/>"
                          "<a:ext cx=\"" << cxEmu << "\" cy=\"" << cyEmu << "\"/>"
                        "</a:xfrm>"
                        "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
                      "</p:spPr>"
                    "</p:pic>";

                sr << "<Relationship Id=\"rId" << localRelId
                   << "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
                   << "Target=\"../media/" << imgName << "\"/>";

                ++localRelId;
                ++globalImageIndex;
                continue;
            }

            ShapeKind k = sh.kind();

            std::string geom = "rect";
            std::string nvName = "TextBox";
            bool txBox = true;

            if (k == ShapeKind::Rect) {
                geom = "rect";
                nvName = "RectShape";
                txBox = false;
            } else if (k == ShapeKind::Ellipse) {
                geom = "ellipse";
                nvName = "EllipseShape";
                txBox = false;
            } else {
                geom = "rect";
                nvName = "TextBox";
                txBox = true;
            }

            const int wPx = std::max(10, sh.getW());
            const int hPx = std::max(10, sh.getH());

            const long long cxEmu = pxToEmuX(wPx, kSlideCx);
            const long long cyEmu = pxToEmuY(hPx, kSlideCy);

            sx << "<p:sp>"
                  "<p:nvSpPr>"
                    "<p:cNvPr id=\"" << shapeId++ << "\" name=\"" << nvName << "\"/>";

            if (txBox) sx << "<p:cNvSpPr txBox=\"1\"/>";
            else sx << "<p:cNvSpPr/>";

            sx <<   "<p:nvPr/>"
                  "</p:nvSpPr>"
                  "<p:spPr>"
                    "<a:xfrm>"
                      "<a:off x=\"" << xEmu << "\" y=\"" << yEmu << "\"/>"
                      "<a:ext cx=\"" << cxEmu << "\" cy=\"" << cyEmu << "\"/>"
                    "</a:xfrm>"
                    "<a:prstGeom prst=\"" << geom << "\"><a:avLst/></a:prstGeom>"
                  "</p:spPr>"
                  "<p:txBody>"
                    "<a:bodyPr wrap=\"square\"/>"
                    "<a:lstStyle/>"
                    "<a:p><a:r><a:t>" << xmlEscape(sh.getText()) << "</a:t></a:r></a:p>"
                  "</p:txBody>"
                "</p:sp>";
        }

        sx << R"(</p:spTree></p:cSld>)"
           << R"(<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>)"
           << R"(</p:sld>)";

        sr << "</Relationships>";

        addTextPart(zip, "ppt/slides/slide" + std::to_string(i + 1) + ".xml", sx.str());
        addTextPart(zip, "ppt/slides/_rels/slide" + std::to_string(i + 1) + ".xml.rels", sr.str());
    }

    zip_close(zip);
    return true;
}

bool PPTXSerializer::load(std::vector<SlideShow>& slideshows,
                          std::map<std::string, size_t>& presentationIndex,
                          std::vector<std::string>& presentationOrder,
                          size_t& currentIndex,
                          const std::string& inputFile)
{
    int err = 0;
    zip_t* zip = zip_open(inputFile.c_str(), ZIP_RDONLY, &err);
    if (!zip) return false;

    slideshows.clear();
    presentationIndex.clear();
    presentationOrder.clear();

    slideshows.emplace_back(inputFile);
    SlideShow& ss = slideshows.back();
    presentationOrder.push_back(inputFile);
    presentationIndex[inputFile] = 0;
    currentIndex = 0;

    // Read slide size from ppt/presentation.xml for proper scaling.
    long long slideCx = kSlideCx;
    long long slideCy = kSlideCy;
    {
        std::string presXml = zipReadFile(zip, "ppt/presentation.xml");
        if (!presXml.empty()) {
            parsePresentationSlideSize(presXml, slideCx, slideCy);
        }
        if (slideCx <= 0) slideCx = kSlideCx;
        if (slideCy <= 0) slideCy = kSlideCy;
    }

    int slideNum = 1;
    while (true) {
        std::string slidePath = "ppt/slides/slide" + std::to_string(slideNum) + ".xml";

        zip_stat_t st;
        if (zip_stat(zip, slidePath.c_str(), 0, &st) != 0) break;

        std::string xml = zipReadFile(zip, slidePath);
        if (xml.empty()) break;

        Slide slide;

        std::string relFile = "ppt/slides/_rels/slide" + std::to_string(slideNum) + ".xml.rels";
        std::string relXml = zipReadFile(zip, relFile);

        size_t pos = 0;
        while (true) {
            size_t spPos = xml.find("<p:sp>", pos);
            size_t picPos = xml.find("<p:pic>", pos);
            if (spPos == std::string::npos && picPos == std::string::npos) break;

            bool isImage = false;
            size_t start = 0;

            if (picPos != std::string::npos && (spPos == std::string::npos || picPos < spPos)) {
                isImage = true;
                start = picPos;
            } else {
                isImage = false;
                start = spPos;
            }

            size_t endTag = isImage ? xml.find("</p:pic>", start) : xml.find("</p:sp>", start);
            if (endTag == std::string::npos) break;

            std::string block = xml.substr(start, endTag - start);

            long long xEmu = 0, yEmu = 0;
            long long wEmu = 0, hEmu = 0;
            extractOffXYEmu(block, xEmu, yEmu);
            extractExtWHEmu(block, wEmu, hEmu);

            const int x = emuToPxX(xEmu, slideCx);
            const int y = emuToPxY(yEmu, slideCy);
            const int w = (wEmu > 0) ? emuToPxX(wEmu, slideCx) : 0;
            const int h = (hEmu > 0) ? emuToPxY(hEmu, slideCy) : 0;

            if (isImage) {
                // Map rId -> ppt/media/file
                size_t ridp = block.find("r:embed=\"");
                if (ridp == std::string::npos) { pos = endTag; continue; }
                ridp += 9;
                size_t rend = block.find('"', ridp);
                if (rend == std::string::npos) { pos = endTag; continue; }
                std::string rid = block.substr(ridp, rend - ridp);

                std::string key = "Id=\"" + rid + "\"";
                size_t posKey = relXml.find(key);
                if (posKey == std::string::npos) { pos = endTag; continue; }

                size_t targ = relXml.find("Target=\"../media/", posKey);
                if (targ == std::string::npos) { pos = endTag; continue; }
                targ += std::strlen("Target=\"../media/");
                size_t tend = relXml.find('"', targ);
                if (tend == std::string::npos) { pos = endTag; continue; }
                std::string imageFile = relXml.substr(targ, tend - targ);

                std::string zipImgPath = "ppt/media/" + imageFile;
                zip_stat_t ist;
                if (zip_stat(zip, zipImgPath.c_str(), 0, &ist) != 0) { pos = endTag; continue; }

                zip_file_t* f = zip_fopen(zip, zipImgPath.c_str(), 0);
                if (!f) { pos = endTag; continue; }

                std::vector<uint8_t> data(ist.size);
                zip_fread(f, data.data(), ist.size);
                zip_fclose(f);

                Shape img("Image", x, y, std::move(data));
                if (w > 0) img.setW(w);
                if (h > 0) img.setH(h);
                slide.addShape(std::move(img));
            } else {
                std::string t = extractFirstTextAT(block);
                std::string nvName = extractCNvPrName(block);

                ShapeKind k = detectShapeKindFromSp(block);
                if (nvName == "TextBox") k = ShapeKind::Text;

                if (k == ShapeKind::Rect || k == ShapeKind::Ellipse) {
                    int W = (w > 0) ? w : 220;
                    int H = (h > 0) ? h : 80;

                    std::string spec;
                    if (k == ShapeKind::Rect) spec = "rect(" + std::to_string(W) + "," + std::to_string(H) + "):" + t;
                    else spec = "ellipse(" + std::to_string(W) + "," + std::to_string(H) + "):" + t;

                    slide.addShape(Shape(spec, x, y));
                } else {
                    if (t.empty()) {
                        pos = endTag;
                        continue;
                    }

                    Shape s(t, x, y);
                    if (w > 0) s.setW(w);
                    if (h > 0) s.setH(h);
                    slide.addShape(std::move(s));
                }
            }

            pos = endTag;
        }

        ss.getSlides().push_back(std::move(slide));
        slideNum++;
    }

    zip_close(zip);
    return true;
}
