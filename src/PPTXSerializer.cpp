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

// ===============================
// Helpers
// ===============================

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

// give libzip its own heap buffer (libzip frees it)
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

// ===============================
// SAVE
// ===============================

bool PPTXSerializer::save(const std::vector<SlideShow>& slideshows,
                          const std::vector<std::string>& order,
                          const std::string& outputFile)
{
    // normalize extension
    std::string path = outputFile;
    if (path.size() < 5 || path.substr(path.size() - 5) != ".pptx")
        path += ".pptx";

    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!zip) {
        return false;
    }

    // ---- flatten slides in order ----
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

    // ========================================================
    // [Content_Types].xml
    // ========================================================
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

    // ========================================================
    // _rels/.rels
    // ========================================================
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

    // ========================================================
    // docProps/core.xml
    // ========================================================
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

    // ========================================================
    // docProps/app.xml
    // ========================================================
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

    // ========================================================
    // ppt/presProps.xml
    // ========================================================
    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)"
          << R"(<p:presentationPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>)";
        addTextPart(zip, "ppt/presProps.xml", o.str());
    }

    // ========================================================
    // ppt/_rels/presentation.xml.rels
    // ========================================================
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

    // ========================================================
    // ppt/presentation.xml
    // ========================================================
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

    // ========================================================
    // theme
    // ========================================================
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

    // ========================================================
    // slideMaster + rels
    // ========================================================
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

    // ========================================================
    // slideLayout + rels
    // ========================================================
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

    // ========================================================
    // Slides + rels + images
    // ========================================================
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
                          "<a:ext cx=\"914400\" cy=\"685800\"/>"
                        "</a:xfrm>"
                        "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
                      "</p:spPr>"
                    "</p:pic>";

                sr << "<Relationship Id=\"rId" << localRelId
                   << "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
                   << "Target=\"../media/" << imgName << "\"/>";

                ++localRelId;
                ++globalImageIndex;
            } else {
                sx << "<p:sp>"
                      "<p:nvSpPr>"
                        "<p:cNvPr id=\"" << shapeId++ << "\" name=\"TextBox\"/>"
                        "<p:cNvSpPr/>"
                        "<p:nvPr/>"
                      "</p:nvSpPr>"
                      "<p:spPr/>"
                      "<p:txBody>"
                        "<a:bodyPr/><a:lstStyle/>"
                        "<a:p><a:r><a:t>"
                   << xmlEscape(sh.getName())
                   << "</a:t></a:r></a:p>"
                      "</p:txBody>"
                    "</p:sp>";
            }
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

// ===============================
// LOAD (minimal, for our own PPTX)
// ===============================

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

// extract text from first <a:t>...</a:t>
static std::string extractText(const std::string& xml) {
    size_t a = xml.find("<a:t>");
    if (a == std::string::npos) return "";
    a += 5;
    size_t b = xml.find("</a:t>", a);
    if (b == std::string::npos) return "";
    return xml.substr(a, b - a);
}

// extract coordinates from <a:off x="..." y="..."/>
static void extractOff(const std::string& xml, int& x, int& y) {
    x = 0;
    y = 0;

    size_t p = xml.find("<a:off ");
    if (p == std::string::npos) return;

    size_t xPos = xml.find("x=\"", p);
    size_t yPos = xml.find("y=\"", p);
    if (xPos == std::string::npos || yPos == std::string::npos) return;

    xPos += 3; // skip x="
    yPos += 3; // skip y="

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

    // --------- helper to read a ZIP file into string ----------
    auto readZipText = [&](const std::string& name) -> std::string {
        zip_stat_t st;
        if (zip_stat(zip, name.c_str(), 0, &st) != 0) return {};
        zip_file_t* f = zip_fopen(zip, name.c_str(), 0);
        if (!f) return {};
        std::string out(st.size, '\0');
        zip_fread(f, out.data(), st.size);
        zip_fclose(f);
        return out;
    };

    // --------- helper: extract first <a:t>...</a:t> ----------
    auto extractText = [&](const std::string& xml) -> std::string {
        size_t a = xml.find("<a:t>");
        if (a == std::string::npos) return "";
        a += 5;
        size_t b = xml.find("</a:t>", a);
        if (b == std::string::npos) return "";
        return xml.substr(a, b - a);
    };

    // --------- helper: extract coordinates ----------
    auto extractXY = [&](const std::string& xml, int& x, int& y) {
        x = y = 0;
        size_t p = xml.find("<a:off ");
        if (p == std::string::npos) return;
        size_t xpos = xml.find("x=\"", p);
        size_t ypos = xml.find("y=\"", p);
        if (xpos == std::string::npos || ypos == std::string::npos) return;
        xpos += 3;
        ypos += 3;
        size_t xe = xml.find('"', xpos);
        size_t ye = xml.find('"', ypos);
        if (xe == std::string::npos || ye == std::string::npos) return;
        x = std::stoi(xml.substr(xpos, xe - xpos)) / 9525;
        y = std::stoi(xml.substr(ypos, ye - ypos)) / 9525;
    };

    // -------- read slide1.xml, slide2.xml, ... --------
    int slideNum = 1;
    while (true) {
        std::string slidePath =
            "ppt/slides/slide" + std::to_string(slideNum) + ".xml";

        zip_stat_t st;
        if (zip_stat(zip, slidePath.c_str(), 0, &st) != 0)
            break; // no more slides

        std::string xml = readZipText(slidePath);
        if (xml.empty()) break;

        Slide slide;

        size_t pos = 0;
        while (true) {
            size_t tpos = xml.find("<p:sp>", pos);
            size_t ipos = xml.find("<p:pic>", pos);

            if (tpos == std::string::npos && ipos == std::string::npos)
                break;

            // choose nearest tag
            bool isImage = false;
            size_t start = 0;
            if (ipos != std::string::npos &&
                (tpos == std::string::npos || ipos < tpos)) {
                isImage = true;
                start = ipos;
            } else {
                isImage = false;
                start = tpos;
            }

            size_t endTag = isImage
                ? xml.find("</p:pic>", start)
                : xml.find("</p:sp>", start);

            if (endTag == std::string::npos)
                break;

            std::string block = xml.substr(start, endTag - start);

            if (isImage) {
                // ---- find embed rId="rIdX"
                size_t ridp = block.find("r:embed=\"");
                if (ridp == std::string::npos) {
                    pos = endTag;
                    continue;
                }
                ridp += 9;
                size_t rend = block.find('"', ridp);
                if (rend == std::string::npos) {
                    pos = endTag;
                    continue;
                }
                std::string rid = block.substr(ridp, rend - ridp);

                // read slide rels
                std::string relFile =
                    "ppt/slides/_rels/slide" + std::to_string(slideNum) + ".xml.rels";
                std::string relXml = readZipText(relFile);

                // find image file
                std::string key = "Id=\"" + rid + "\"";
                size_t posKey = relXml.find(key);
                if (posKey == std::string::npos) {
                    pos = endTag;
                    continue;
                }
                size_t targ = relXml.find("Target=\"../media/", posKey);
                if (targ == std::string::npos) {
                    pos = endTag;
                    continue;
                }
                targ += strlen("Target=\"../media/");
                size_t tend = relXml.find('"', targ);
                if (tend == std::string::npos) {
                    pos = endTag;
                    continue;
                }
                std::string imageFile = relXml.substr(targ, tend - targ);

                // read image bytes
                std::string zipImgPath = "ppt/media/" + imageFile;
                zip_stat_t ist;
                if (zip_stat(zip, zipImgPath.c_str(), 0, &ist) != 0) {
                    pos = endTag;
                    continue;
                }

                zip_file_t* f = zip_fopen(zip, zipImgPath.c_str(), 0);
                std::vector<uint8_t> data(ist.size);
                zip_fread(f, data.data(), ist.size);
                zip_fclose(f);

                int x, y;
                extractXY(block, x, y);

                slide.addShape(Shape("Image", x, y, std::move(data)));
            }
            else {
                std::string text = extractText(block);
                if (!text.empty()) {
                    int x, y;
                    extractXY(block, x, y);
                    slide.addShape(Shape(text, x, y));
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
