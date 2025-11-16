#include "PPTXSerializer.hpp"
#include "SlideShow.hpp"
#include "Slide.hpp"
#include "Shape.hpp"

#include <zip.h>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <iostream>

// ----------------------------------------------------------------------
// Escape XML text
// ----------------------------------------------------------------------
static std::string xmlEscape(const std::string& s)
{
    std::string out;
    out.reserve(s.size() * 1.2);
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

// ----------------------------------------------------------------------
// IMPORTANT FIX: libzip must *not* free our memory.
// We store all XML strings in 'xmlStorage' so they stay alive
// until zip_close().
// ----------------------------------------------------------------------
static void addTextPart(zip_t* zip,
                        std::vector<std::string>& xmlStorage,
                        const std::string& name,
                        std::string content)
{
    // Store content to keep memory alive until zip_close()
    xmlStorage.push_back(std::move(content));
    std::string& stored = xmlStorage.back();

    zip_source_t* src = zip_source_buffer(
        zip,
        stored.data(),
        stored.size(),
        0      // libzip will NOT free this buffer
    );

    if (!src) return;

    zip_file_add(zip, name.c_str(), src, ZIP_FL_OVERWRITE);
}

// ----------------------------------------------------------------------
// Binary data: vectors already own their buffer, so keep freep=0.
// ----------------------------------------------------------------------
static void addBinaryPart(zip_t* zip,
                          const std::string& name,
                          const std::vector<uint8_t>& data)
{
    if (data.empty()) return;

    zip_source_t* src = zip_source_buffer(
        zip,
        data.data(),
        data.size(),
        0   // do not free; vector owns memory
    );

    if (!src) return;

    zip_file_add(zip, name.c_str(), src, ZIP_FL_OVERWRITE);
}

// ======================================================================
// Save multiple slideshows into one PPTX file
// ======================================================================

bool PPTXSerializer::save(const std::vector<SlideShow>& slideshows,
                          const std::vector<std::string>& order,
                          const std::string& outputPath)
{
    // Create output path with .pptx extension
    std::string path = outputPath;
    if (path.size() < 5 || path.substr(path.size() - 5) != ".pptx")
        path += ".pptx";

    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!zip) {
        return false;
    }

    // Keep all XML string buffers alive till zip_close()
    std::vector<std::string> xmlStorage;
    xmlStorage.reserve(64);

    // ------------------------------------------------------------------
    // Flatten slides across all SlideShows in the specified order.
    // ------------------------------------------------------------------
    std::vector<const Slide*> flatSlides;

    if (!order.empty()) {
        for (const auto& name : order) {
            auto it = std::find_if(
                slideshows.begin(), slideshows.end(),
                [&](const SlideShow& ss) { return ss.getFilename() == name; }
            );
            if (it == slideshows.end()) continue;

            for (const auto& sl : it->getSlides())
                flatSlides.push_back(&sl);
        }
    } else {
        for (const auto& ss : slideshows)
            for (const auto& sl : ss.getSlides())
                flatSlides.push_back(&sl);
    }

    int totalSlides = (int)flatSlides.size();

    // ==================================================================
    // PART A: [Content_Types].xml
    // ==================================================================

    {
        std::ostringstream o;

        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)";
        o << R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">)";

        // defaults
        o << R"(<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>)";
        o << R"(<Default Extension="xml"  ContentType="application/xml"/>)";
        o << R"(<Default Extension="png"  ContentType="image/png"/>)";

        // overrides
        o << R"(<Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>)";
        o << R"(<Override PartName="/docProps/app.xml"  ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>)";
        o << R"(<Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>)";
        o << R"(<Override PartName="/ppt/presProps.xml"   ContentType="application/vnd.openxmlformats-officedocument.presentationml.presProps+xml"/>)";
        o << R"(<Override PartName="/ppt/slideMasters/slideMaster1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideMaster+xml"/>)";
        o << R"(<Override PartName="/ppt/slideLayouts/slideLayout1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slideLayout+xml"/>)";
        o << R"(<Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>)";

        // each slide
        for (int i = 0; i < totalSlides; ++i) {
            o << "<Override PartName=\"/ppt/slides/slide" << (i + 1)
              << ".xml\" ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>";
        }

        o << "</Types>";

        addTextPart(zip, xmlStorage, "[Content_Types].xml", o.str());
    }


    // ==================================================================
    // PART B: Root relationships: _rels/.rels
    // ==================================================================

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        o << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";
        o << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>)";
        o << R"(<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>)";
        o << R"(<Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>)";
        o << R"(</Relationships>)";

        addTextPart(zip, xmlStorage, "_rels/.rels", o.str());
    }


    // ==================================================================
    // PART C: docProps/core.xml
    // ==================================================================

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)";
        o << R"(<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties")";
        o << R"( xmlns:dc="http://purl.org/dc/elements/1.1/")";
        o << R"( xmlns:dcterms="http://purl.org/dc/terms/")";
        o << R"( xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">)";
        o << R"(<dc:title>CLI Presentation</dc:title>)";
        o << R"(<dc:creator>SlideShow CLI</dc:creator>)";
        o << R"(<cp:lastModifiedBy>SlideShow CLI</cp:lastModifiedBy>)";
        o << R"(<dcterms:created xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:created>)";
        o << R"(<dcterms:modified xsi:type="dcterms:W3CDTF">2025-01-01T00:00:00Z</dcterms:modified>)";
        o << R"(</cp:coreProperties>)";

        addTextPart(zip, xmlStorage, "docProps/core.xml", o.str());
    }


    // ==================================================================
    // PART D: docProps/app.xml
    // ==================================================================

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)";
        o << R"(<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties")";
        o << R"( xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">)";
        o << R"(<Application>SlideShow CLI</Application>)";
        o << "<Slides>" << totalSlides << "</Slides>";
        o << R"(<Notes>0</Notes>)";
        o << R"(<HiddenSlides>0</HiddenSlides>)";
        o << R"(</Properties>)";

        addTextPart(zip, xmlStorage, "docProps/app.xml", o.str());
    }


    // ==================================================================
    // PART E: ppt/presProps.xml
    // ==================================================================

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)";
        o << R"(<p:presentationPr xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"/>)";

        addTextPart(zip, xmlStorage, "ppt/presProps.xml", o.str());
    }

    // ---- END OF PART 2 ----
    // Next: presentation.xml.rels, slide master, layout, theme, slides

    // Continue in part 3...
    // ==================================================================
    // PART F: ppt/_rels/presentation.xml.rels
    // Maps:
    //   rId1 -> slideMaster1.xml
    //   rId2 -> presProps.xml
    //   rId3.. -> slides
    // ==================================================================

    {
        std::ostringstream o;
        o << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        o << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";

        o << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="slideMasters/slideMaster1.xml"/>)";
        o << R"(<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/presProps" Target="presProps.xml"/>)";

        int relId = 3;
        for (int i = 0; i < totalSlides; ++i) {
            o << "<Relationship Id=\"rId" << relId++ << "\" "
              << "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" "
              << "Target=\"slides/slide" << (i + 1) << ".xml\"/>";
        }

        o << "</Relationships>";

        addTextPart(zip, xmlStorage, "ppt/_rels/presentation.xml.rels", o.str());
    }


    // ==================================================================
    // PART G: ppt/presentation.xml
    // ==================================================================

    {
        std::ostringstream o;

        o << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        o << R"(<p:presentation xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")";
        o << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)";

        // slide master reference
        o << R"(<p:sldMasterIdLst>)";
        o << R"(<p:sldMasterId id="2147483648" r:id="rId1"/>)";
        o << R"(</p:sldMasterIdLst>)";

        // slide list
        if (totalSlides > 0) {
            o << "<p:sldIdLst>";
            int id = 256;
            int slideRel = 3;
            for (int i = 0; i < totalSlides; ++i) {
                o << "<p:sldId id=\"" << id++ << "\" r:id=\"rId" << slideRel++ << "\"/>";
            }
            o << "</p:sldIdLst>";
        }

        // slide size
        o << R"(<p:sldSz cx="9144000" cy="6858000" type="screen4x3"/>)";
        o << R"(<p:notesSz cx="6858000" cy="9144000"/>)";
        o << R"(<p:defaultTextStyle/>)";
        o << R"(</p:presentation>)";

        addTextPart(zip, xmlStorage, "ppt/presentation.xml", o.str());
    }


    // ==================================================================
    // PART H: ppt/theme/theme1.xml
    // ==================================================================

    {
        std::ostringstream o;

        o << R"(<?xml version="1.0" encoding="UTF-8" standalone="yes"?>)";
        o << R"(<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="CLI Theme">)";
        o << R"(<a:themeElements>)";
        o << R"(<a:clrScheme name="Office">)";
        o << R"(<a:dk1><a:sysClr val="windowText" lastClr="000000"/></a:dk1>)";
        o << R"(<a:lt1><a:sysClr val="window" lastClr="FFFFFF"/></a:lt1>)";
        o << R"(</a:clrScheme>)";
        o << R"(</a:themeElements>)";
        o << R"(</a:theme>)";

        addTextPart(zip, xmlStorage, "ppt/theme/theme1.xml", o.str());
    }


    // ==================================================================
    // PART I: slideMaster1.xml
    // ==================================================================

    {
        std::ostringstream o;

        o << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        o << R"(<p:sldMaster xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")";
        o << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")";
        o << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)";

        o << R"(<p:cSld name="CLI Master">)";
        o << R"(<p:spTree>)";

        o << R"(<p:nvGrpSpPr>)";
        o << R"(<p:cNvPr id="1" name=""/>)";
        o << R"(<p:cNvGrpSpPr/>)";
        o << R"(<p:nvPr/>)";
        o << R"(</p:nvGrpSpPr>)";

        o << R"(<p:grpSpPr><a:xfrm/></p:grpSpPr>)";

        o << R"(</p:spTree>)";
        o << R"(</p:cSld>)";

        o << R"(<p:clrMap bg1="lt1" tx1="dk1" bg2="lt1" tx2="dk1" )"
             R"(accent1="accent1" accent2="accent2" accent3="accent3" )"
             R"(accent4="accent4" accent5="accent5" accent6="accent6" )"
             R"(hlink="hlink" folHlink="folHlink"/>)";

        o << R"(<p:sldLayoutIdLst><p:sldLayoutId id="1" r:id="rId1"/></p:sldLayoutIdLst>)";
        o << R"(<p:txStyles/>)";

        o << R"(</p:sldMaster>)";

        addTextPart(zip, xmlStorage, "ppt/slideMasters/slideMaster1.xml", o.str());
    }


    // ==================================================================
    // PART J: slideMaster1.xml.rels
    // ==================================================================

    {
        std::ostringstream o;

        o << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        o << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";
        o << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>)";
        o << R"(<Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="../theme/theme1.xml"/>)";
        o << R"(</Relationships>)";

        addTextPart(zip, xmlStorage, "ppt/slideMasters/_rels/slideMaster1.xml.rels", o.str());
    }


    // ==================================================================
    // PART K: slideLayout1.xml
    // ==================================================================

    {
        std::ostringstream o;

        o << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        o << R"(<p:sldLayout xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")";
        o << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")";
        o << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships")";
        o << R"( type="blank" preserve="1">)";

        o << R"(<p:cSld name="Blank">)";
        o << R"(<p:spTree>)";

        o << R"(<p:nvGrpSpPr>)";
        o << R"(<p:cNvPr id="1" name=""/>)";
        o << R"(<p:cNvGrpSpPr/>)";
        o << R"(<p:nvPr/>)";
        o << R"(</p:nvGrpSpPr>)";

        o << R"(<p:grpSpPr><a:xfrm/></p:grpSpPr>)";

        o << R"(</p:spTree>)";
        o << R"(</p:cSld>)";

        o << R"(<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>)";

        o << R"(</p:sldLayout>)";

        addTextPart(zip, xmlStorage, "ppt/slideLayouts/slideLayout1.xml", o.str());
    }


    // ==================================================================
    // PART L: slideLayout1.xml.rels
    // ==================================================================

    {
        std::ostringstream o;

        o << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        o << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";
        o << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideMaster" Target="../slideMasters/slideMaster1.xml"/>)";
        o << R"(</Relationships>)";

        addTextPart(zip, xmlStorage, "ppt/slideLayouts/_rels/slideLayout1.xml.rels", o.str());
    }

    // ---- END OF PART 3 ----
    // Next: slides and closing zip
    // ==================================================================
    // PART M: Slides + slide rels + images
    // ==================================================================

    int globalImageIndex = 1;

    for (int i = 0; i < totalSlides; ++i) {
        const Slide* slide = flatSlides[i];

        // ---------------------------
        // slideN.xml content
        // ---------------------------
        std::ostringstream sXml;

        sXml << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        sXml << R"(<p:sld xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main")";
        sXml << R"( xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main")";
        sXml << R"( xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)";

        sXml << R"(<p:cSld><p:spTree>)";

        // Required group shape
        sXml << R"(<p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>)";
        sXml << R"(<p:grpSpPr><a:xfrm/></p:grpSpPr>)";

        int shapeId = 2;      // shape ID counter
        int localRelId = 2;   // rId2.. for images in this slide

        // ----------------------
        // Slide relationships
        // ----------------------
        std::ostringstream sRels;
        sRels << R"(<?xml version="1.0" encoding="UTF-8"?>)";
        sRels << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";
        sRels << R"(<Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slideLayout" Target="../slideLayouts/slideLayout1.xml"/>)";

        // -------------------------
        // SHAPES IN THE SLIDE
        // -------------------------

        for (const auto& sh : slide->getShapes()) {

            // -------------------------
            // IMAGE SHAPE
            // -------------------------
            if (sh.isImage()) {
                // Save image
                std::string imgName = "image" + std::to_string(globalImageIndex) + ".png";

                addBinaryPart(zip,
                              "ppt/media/" + imgName,
                              sh.getImageData());

                // Add picture element
                sXml << "<p:pic>";
                sXml << "<p:nvPicPr>"
                        "<p:cNvPr id=\"" << shapeId++ << "\" name=\"" << xmlEscape(sh.getName()) << "\"/>"
                        "<p:cNvPicPr/>"
                        "<p:nvPr/>"
                     "</p:nvPicPr>";

                sXml << "<p:blipFill>"
                        "<a:blip r:embed=\"rId" << localRelId << "\"/>"
                        "<a:stretch><a:fillRect/></a:stretch>"
                     "</p:blipFill>";

                sXml << "<p:spPr>"
                        "<a:xfrm>"
                            "<a:off x=\"" << (sh.getX() * 9525) << "\" y=\"" << (sh.getY() * 9525) << "\"/>"
                            "<a:ext cx=\"914400\" cy=\"685800\"/>"
                        "</a:xfrm>"
                        "<a:prstGeom prst=\"rect\"><a:avLst/></a:prstGeom>"
                     "</p:spPr>";

                sXml << "</p:pic>";

                // Add slide relationship for image
                sRels << "<Relationship Id=\"rId" << localRelId
                      << "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
                      << "Target=\"../media/" << imgName << "\"/>";

                ++localRelId;
                ++globalImageIndex;
            }

            // -------------------------
            // TEXT SHAPE
            // -------------------------
            else {
                sXml << "<p:sp>";
                sXml << "<p:nvSpPr>"
                        "<p:cNvPr id=\"" << shapeId++ << "\" name=\"TextBox\"/>"
                        "<p:cNvSpPr/>"
                        "<p:nvPr/>"
                     "</p:nvSpPr>";

                sXml << "<p:spPr/>";

                sXml << "<p:txBody>"
                        "<a:bodyPr/><a:lstStyle/>"
                        "<a:p><a:r><a:t>"
                     << xmlEscape(sh.getName()) <<
                        "</a:t></a:r></a:p>"
                     "</p:txBody>";

                sXml << "</p:sp>";
            }
        }

        // Finish slide
        sXml << R"(</p:spTree></p:cSld>)";
        sXml << R"(<p:clrMapOvr><a:masterClrMapping/></p:clrMapOvr>)";
        sXml << R"(</p:sld>)";

        // Finish slide rels
        sRels << "</Relationships>";

        // Write slide XML
        addTextPart(zip,
                    xmlStorage,
                    "ppt/slides/slide" + std::to_string(i + 1) + ".xml",
                    sXml.str());

        // Write slide rels
        addTextPart(zip,
                    xmlStorage,
                    "ppt/slides/_rels/slide" + std::to_string(i + 1) + ".xml.rels",
                    sRels.str());
    }

    // ==================================================================
    // CLOSE ZIP
    // ==================================================================

    zip_close(zip);
    return true;
}


// ======================================================================
// load() not implemented (matches your original behavior)
// ======================================================================

bool PPTXSerializer::load(std::vector<SlideShow>&,
                          std::map<std::string, size_t>&,
                          std::vector<std::string>&,
                          size_t&,
                          const std::string&)
{
    return false;
}

