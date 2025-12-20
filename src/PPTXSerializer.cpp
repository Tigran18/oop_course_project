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

static std::string xmlEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() * 12 / 10 + 4);
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

    if (data.size() < 24) return false;

    auto be32 = [&](size_t off) -> uint32_t {
        return (uint32_t(data[off]) << 24) |
               (uint32_t(data[off + 1]) << 16) |
               (uint32_t(data[off + 2]) << 8) |
               (uint32_t(data[off + 3]));
    };

    uint32_t chunkLen = be32(8);
    if (data.size() < 8 + 4 + 4 + chunkLen) return false;

    if (!(data[12] == 'I' && data[13] == 'H' && data[14] == 'D' && data[15] == 'R'))
        return false;

    uint32_t W = be32(16);
    uint32_t H = be32(20);
    if (W == 0 || H == 0) return false;

    w = static_cast<int>(W);
    h = static_cast<int>(H);
    return true;
}

static std::string zipReadFile(zip_t* z, const std::string& name) {
    zip_stat_t st;
    if (zip_stat(z, name.c_str(), 0, &st) != 0) return {};
    zip_file_t* f = zip_fopen(z, name.c_str(), 0);
    if (!f) return {};
    std::string buf(st.size, '\0');
    zip_fread(f, buf.data(), st.size);
    zip_fclose(f);
    return buf;
}

static std::string extractFirstTextAT(const std::string& xml) {
    size_t a = xml.find("<a:t>");
    if (a == std::string::npos) return "";
    a += 5;
    size_t b = xml.find("</a:t>", a);
    if (b == std::string::npos) return "";
    return xml.substr(a, b - a);
}

static void extractOffXY(const std::string& xml, int& x, int& y) {
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
        x = std::stoi(xml.substr(xPos, xEnd - xPos)) / 9525;
        y = std::stoi(xml.substr(yPos, yEnd - yPos)) / 9525;
    } catch (...) {
        x = 0;
        y = 0;
    }
}

static void extractExtWH(const std::string& xml, int& w, int& h) {
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
        w = std::stoi(xml.substr(cxPos, cxEnd - cxPos)) / 9525;
        h = std::stoi(xml.substr(cyPos, cyEnd - cyPos)) / 9525;
    } catch (...) {
        w = 0;
        h = 0;
    }
}

static std::string extractCNvPrName(const std::string& block)
{
    std::smatch m;
    static const std::regex re(R"REGEX(<p:cNvPr[^>]*\sname="([^"]*)")REGEX");
    if (std::regex_search(block, m, re) && m.size() >= 2)
        return m[1].str();
    return {};
}


static ShapeKind detectShapeKindFromSp(const std::string& block)
{
    if (block.find("prst=\"ellipse\"") != std::string::npos) return ShapeKind::Ellipse;
    if (block.find("prst=\"rect\"") != std::string::npos) return ShapeKind::Rect;
    return ShapeKind::Text;
}

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

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties")"
          << R"( xmlns:dc="http://purl.org/dc/elements/1.1/")"
          << R"( xmlns:dcterms="http://purl.org/dc/terms/")"
          << R"( xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">)"
          << R"(<dc:title>CLI Presentation</dc:title>)"
          << R"(<dc:creator>SlideShow CLI</dc:creator>)"
          << R"(<cp:lastModifiedBy>SlideShow CLI</cp:lastModifiedBy>)"
          << R"(<dcterms:created xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:created>)"
          << R"(<dcterms:modified xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:modified>)"
          << R"(</cp:coreProperties>)";
        addTextPart(zip, "docProps/core.xml", o.str());
    }

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties")"
          << R"( xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">)"
          << R"(<Application>SlideShow CLI</Application>)"
          << "<Slides>" << totalSlides << "</Slides>"
          << R"(<Notes>0</Notes><HiddenSlides>0</HiddenSlides>)"
          << R"(</Properties>)";
        addTextPart(zip, "docProps/app.xml", o.str());
    }

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<p:presentationPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>)";
        addTextPart(zip, "ppt/presProps.xml", o.str());
    }

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

        o << R"(<p:sldSz cx="9144000" cy="6858000" type="screen4x3"/>)"
          << R"(<p:notesSz cx="6858000" cy="9144000"/>)"
          << R"(<p:defaultTextStyle/>)"
          << R"(</p:presentation>)";
        addTextPart(zip, "ppt/presentation.xml", o.str());
    }

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="CLI Theme">)"
          << R"(<a:themeElements><a:clrScheme name="Office">)"
          << R"(<a:dk1><a:sysClr val="windowText" lastClr="000000"/></a:dk1>)"
          << R"(<a:lt1><a:sysClr val="window" lastClr="FFFFFF"/></a:lt1>)"
          << R"(</a:clrScheme></a:themeElements></a:theme>)";
        addTextPart(zip, "ppt/theme/theme1.xml", o.str());
    }

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<p:sldMaster xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")"
          << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")"
          << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
          << R"(<p:cSld name="CLI Master"><p:spTree>)"
          << R"(<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>)"
          << R"(<p:grpSpPr><a:xfrm/></p:grpSpPr>)"
          << R"(</p:spTree></p:cSld>)"
          << R"(<p:clrMap bg1="lt1" tx1="dk1" bg2="lt1" tx2="dk1" accent1="accent1" accent2="accent2" accent3="accent3" accent4="accent4" accent5="accent5" accent6="accent6" hlink="hlink" folHlink="folHlink"/>)"
          << R"(<p:sldLayoutIdLst><p:sldLayoutId id="1" r:id="rId1"/></p:sldLayoutIdLst>)"
          << R"(<p:txStyles/>)"
          << R"(</p:sldMaster>)";
        addTextPart(zip, "ppt/slideMasters/slideMaster1.xml", o.str());
    }
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
          << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>)"
          << R"(<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="../theme/theme1.xml"/>)"
          << R"(</Relationships>)";
        addTextPart(zip, "ppt/slideMasters/_rels/slideMaster1.xml.rels", o.str());
    }

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<p:sldLayout xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")"
          << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")"
          << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships" type="blank" preserve="1">)"
          << R"(<p:cSld name="Blank"><p:spTree>)"
          << R"(<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>)"
          << R"(<p:grpSpPr><a:xfrm/></p:grpSpPr>)"
          << R"(</p:spTree></p:cSld>)"
          << R"(<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>)"
          << R"(</p:sldLayout>)";
        addTextPart(zip, "ppt/slideLayouts/slideLayout1.xml", o.str());
    }
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
          << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="../slideMasters/slideMaster1.xml"/>)"
          << R"(</Relationships>)";
        addTextPart(zip, "ppt/slideLayouts/_rels/slideLayout1.xml.rels", o.str());
    }

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
           << R"(<p:grpSpPr><a:xfrm/></p:grpSpPr>)";

        sr << R"(<?xml version="1.0" encoding="UTF-8"?>)"
           << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)"
           << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>)";

        int shapeId = 2;
        int localRelId = 2;

        for (const auto& sh : sl->getShapes()) {
            if (sh.isImage()) {
                std::string imgName = "image" + std::to_string(globalImageIndex) + ".png";
                addBinaryPart(zip, "ppt/media/" + imgName, sh.getImageData());

                int pxW = 0, pxH = 0;
                if (!parsePngWH(sh.getImageData(), pxW, pxH)) {
                    pxW = 320;
                    pxH = 240;
                }
                int cx = pxW * 9525;
                int cy = pxH * 9525;

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
                            "<a:off x=\"" << (sh.getX() * 9525) << "\" y=\"" << (sh.getY() * 9525) << "\"/>"
                            "<a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/>"
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
            if (k == ShapeKind::Rect) { geom = "rect"; nvName = "RectShape"; }
            else if (k == ShapeKind::Ellipse) { geom = "ellipse"; nvName = "EllipseShape"; }
            else { geom = "rect"; nvName = "TextBox"; }

            int cx = std::max(10, sh.getW()) * 9525;
            int cy = std::max(10, sh.getH()) * 9525;

            sx << "<p:sp>"
                    "<p:nvSpPr>"
                      "<p:cNvPr id=\"" << shapeId++ << "\" name=\"" << nvName << "\"/>"
                      "<p:cNvSpPr/>"
                      "<p:nvPr/>"
                    "</p:nvSpPr>"
                    "<p:spPr>"
                      "<a:xfrm>"
                        "<a:off x=\"" << (sh.getX() * 9525) << "\" y=\"" << (sh.getY() * 9525) << "\"/>"
                        "<a:ext cx=\"" << cx << "\" cy=\"" << cy << "\"/>"
                      "</a:xfrm>"
                      "<a:prstGeom prst=\"" << geom << "\"><a:avLst/></a:prstGeom>"
                    "</p:spPr>"
                    "<p:txBody>"
                      "<a:bodyPr wrap=\"square\"/>"
                      "<a:lstStyle/>"
                      "<a:p><a:r><a:t>"
               << xmlEscape(sh.getText())
               << "</a:t></a:r></a:p>"
                    "</p:txBody>"
                  "</p:sp>";
        }

        sx << R"(</p:spTree></p:cSld>)"
           << R"(<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>)"
           << R"(</p:sld>)";
        sr << "</Relationships>";

        addTextPart(zip,
                    "ppt/slides/slide" + std::to_string(i + 1) + ".xml",
                    sx.str());
        addTextPart(zip,
                    "ppt/slides/_rels/slide" + std::to_string(i + 1) + ".xml.rels",
                    sr.str());
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

            if (isImage) {
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

                int x, y;
                extractOffXY(block, x, y);

                slide.addShape(Shape("Image", x, y, std::move(data)));
            } else {
                int x, y;
                extractOffXY(block, x, y);

                int w, h;
                extractExtWH(block, w, h);

                std::string t = extractFirstTextAT(block);
                std::string nvName = extractCNvPrName(block);

                ShapeKind k = detectShapeKindFromSp(block);
                if (nvName == "TextBox") k = ShapeKind::Text;

                if (k == ShapeKind::Rect || k == ShapeKind::Ellipse) {
                    if (w <= 0) w = 220;
                    if (h <= 0) h = 80;

                    std::string spec;
                    if (k == ShapeKind::Rect) spec = "rect(" + std::to_string(w) + "," + std::to_string(h) + "):" + t;
                    else spec = "ellipse(" + std::to_string(w) + "," + std::to_string(h) + "):" + t;

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
