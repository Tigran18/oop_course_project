#include "PPTXSerializer.hpp"
#include <zip.h>
#include <sstream>
#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

bool PPTXSerializer::save(const std::vector<SlideShow>& slideshows,
                          const std::vector<std::string>& order,
                          const std::string& outputPath)
{
    std::string path = outputPath;
    if (path.size() < 5 || path.substr(path.size()-5) != ".pptx") path += ".pptx";

    int err = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!zip) return false;

    auto addFile = [&](const std::string& name, const std::string& content) {
        zip_source_t* src = zip_source_buffer(zip, content.c_str(), content.size(), 0);
        if (src) zip_file_add(zip, name.c_str(), src, ZIP_FL_OVERWRITE);
    };

    auto addBinaryFile = [&](const std::string& name, const std::vector<uint8_t>& data) {
        zip_source_t* src = zip_source_buffer(zip, data.data(), data.size(), 0);
        if (src) zip_file_add(zip, name.c_str(), src, ZIP_FL_OVERWRITE);
    };

    // ===== [Content_Types].xml =====
    std::ostringstream types;
    types << R"(<?xml version="1.0" encoding="UTF-8"?>)"
          << R"(<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">)"
          << R"(<Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>)"
          << R"(<Default Extension="xml" ContentType="application/xml"/>)"
          << R"(<Default Extension="png" ContentType="image/png"/>)";

    int totalSlides = 0, totalImages = 0;
    for (const auto& ss : slideshows) {
        totalSlides += ss.getSlides().size();
        for (const auto& slide : ss.getSlides()) {
            for (const auto& shape : slide.getShapes()) {
                if (shape.isImage()) totalImages++;
            }
        }
    }

    for (int i = 0; i < totalSlides; ++i) {
        types << "<Override PartName=\"/ppt/slides/slide" << (i+1) << ".xml\" "
              << "ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.slide+xml\"/>";
    }
    types << "<Override PartName=\"/ppt/presentation.xml\" "
          << "ContentType=\"application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml\"/>";
    types << "</Types>";
    addFile("[Content_Types].xml", types.str());

    // ===== _rels/.rels =====
    const std::string rels_rels = R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>
</Relationships>)";
    addFile("_rels/.rels", rels_rels);

    // ===== ppt/_rels/presentation.xml.rels =====
    std::ostringstream presRels;
    presRels << R"(<?xml version="1.0" encoding="UTF-8"?>)"
             << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";
    int slideRelId = 1;
    for (int i = 0; i < totalSlides; ++i) {
        presRels << "<Relationship Id=\"rId" << slideRelId++ 
                 << "\" Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide\" "
                 << "Target=\"slides/slide" << (i+1) << ".xml\"/>";
    }
    presRels << "</Relationships>";
    addFile("ppt/_rels/presentation.xml.rels", presRels.str());

    // ===== ppt/presentation.xml =====
    std::ostringstream presXml;
    presXml << R"(<?xml version="1.0" encoding="UTF-8"?>)"
            << R"(<p:presentation xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
            << "<p:sldIdLst>";
    slideRelId = 1;
    for (int i = 0; i < totalSlides; ++i) {
        presXml << "<p:sldId id=\"" << (256+i) << "\" r:id=\"rId" << slideRelId++ << "\"/>";
    }
    presXml << "</p:sldIdLst></p:presentation>";
    addFile("ppt/presentation.xml", presXml.str());

    // ===== Slides & slide relationships =====
    int imageRelId = 1;
    int slideIndex = 0;
    for (const auto& ss : slideshows) {
        for (const auto& slide : ss.getSlides()) {
            std::ostringstream slideXml;
            slideXml << R"(<?xml version="1.0" encoding="UTF-8"?>)"
                     << R"(<p:sld xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main" xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships">)"
                     << "<p:cSld><p:spTree>";
            std::ostringstream slideRels;
            slideRels << R"(<?xml version="1.0" encoding="UTF-8"?>)"
                      << R"(<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">)";

            int localImageId = 1;
            for (const auto& shape : slide.getShapes()) {
                if (shape.isImage()) {
                    std::string imgName = "image" + std::to_string(imageRelId) + ".png";
                    addBinaryFile("ppt/media/" + imgName, shape.getImageData());
                    slideXml << "<p:pic><p:blipFill><a:blip r:embed=\"rId" << localImageId << "\"/></p:blipFill></p:pic>";
                    slideRels << "<Relationship Id=\"rId" << localImageId << "\" "
                               << "Type=\"http://schemas.openxmlformats.org/officeDocument/2006/relationships/image\" "
                               << "Target=\"../media/" << imgName << "\"/>";
                    localImageId++;
                    imageRelId++;
                } else {
                    slideXml << "<p:sp><p:txBody><a:p><a:r><a:t>" 
                             << shape.getName() << "</a:t></a:r></a:p></p:txBody></p:sp>";
                }
            }

            slideXml << "</p:spTree></p:cSld></p:sld>";
            slideRels << "</Relationships>";

            addFile("ppt/slides/slide" + std::to_string(slideIndex+1) + ".xml", slideXml.str());
            addFile("ppt/slides/_rels/slide" + std::to_string(slideIndex+1) + ".xml.rels", slideRels.str());

            slideIndex++;
        }
    }

    zip_close(zip);
    return true;
}

bool PPTXSerializer::load(std::vector<SlideShow>& slideshows,
                          std::map<std::string, size_t>& presentationIndex,
                          std::vector<std::string>& presentationOrder,
                          size_t& currentIndex,
                          const std::string& inputFile) {
    return false;
}
