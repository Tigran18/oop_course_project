#include "../include/PPTXExporter.hpp"
#include "../include/SlideShow.hpp"
#include "../include/Shape.hpp"
#include "../include/Functions.hpp"
#include "../include/Color.hpp"
#include <zip.h>
#include <sstream>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

bool PPTXExporter::exportToRealPPTX(const std::vector<SlideShow>& slideshows,
                                    const std::vector<std::string>& order,
                                    const std::string& outputPath) {
    std::string path = outputPath;
    if (!utils::endsWith(path, ".pptx")) {
        path += ".pptx";
    }
    int zipErr = 0;
    zip_t* zip = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zipErr);
    if (!zip) {
        err() << "Failed to create .pptx file\n";
        return false;
    }
    auto addFile = [zip](const char* name, const std::string& content) {
        zip_source_t* src = zip_source_buffer(zip, content.c_str(), content.size(), 0);
        if (src) {
            zip_file_add(zip, name, src, ZIP_FL_OVERWRITE);
        }
    };
    (addFile)("[Content_Types].xml",
        R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Default Extension="png" ContentType="image/png"/>
  <Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>
  <Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>
  <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-officedocument.core-properties+xml"/>
</Types>)");

    (addFile)("_rels/.rels",
        R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>
</Relationships>)");

    (addFile)("docProps/core.xml",
        R"(<?xml version="1.0" encoding="UTF-8"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties">
  <dc:creator>SlideShow CLI</dc:creator>
</cp:coreProperties>)");

    (addFile)("docProps/app.xml",
        R"(<?xml version="1.0" encoding="UTF-8"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties">
  <Application>SlideShow CLI</Application>
</Properties>)");

    std::ostringstream pres;
    pres << R"(<?xml version="1.0" encoding="UTF-8"?>
<p:presentation xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:sldIdLst>)";

    std::map<std::string, int> slideIdMap;
    int slideId = 256;
    for (size_t i = 0; i < order.size(); ++i) {
        const auto& name = order[i];
        auto it = std::find_if(slideshows.begin(), slideshows.end(),
            [&name](const SlideShow& ss) { return ss.getFilename() == name; });
        if (it != slideshows.end()) {
            slideIdMap[name] = slideId++;
            pres << "<p:sldId id=\"" << slideIdMap[name] << "\" r:id=\"rId" << (i + 1) << "\"/>";
        }
    }
    pres << R"(</p:sldIdLst>
</p:presentation>)";
    (addFile)("ppt/presentation.xml", pres.str());

    int relId = 1;
    for (size_t i = 0; i < order.size(); ++i) {
        const auto& name = order[i];
        auto it = std::find_if(slideshows.begin(), slideshows.end(),
            [&name](const SlideShow& ss) { return ss.getFilename() == name; });
        if (it == slideshows.end()) continue;

        const auto& ss = *it;
        std::ostringstream slideXml;
        slideXml << R"(<?xml version="1.0" encoding="UTF-8"?>
<p:sld xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main"
       xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main">
  <p:cSld>
    <p:spTree>
      <p:nvGrpSpPr><p:cNvPr id="1" name=""/><p:cNvGrpSpPr/><p:nvPr/></p:nvGrpSpPr>
      <p:grpSpPr/>
)";

        for (const auto& slide : ss.getSlides()) {
            for (const auto& shape : slide.getShapes()) {
                if (shape.isImage()) {
                    std::string imgName = "image" + std::to_string(relId) + ".png";
                    zip_source_t* src = zip_source_buffer(zip,
                        shape.getImageData().data(),
                        shape.getImageData().size(), 0);
                    if (src) {
                        zip_file_add(zip, ("ppt/media/" + imgName).c_str(), src, ZIP_FL_OVERWRITE);
                    }

                    slideXml << "<p:pic>"
                             << "<p:blipFill><a:blip r:embed=\"rId" << relId << "\"/></p:blipFill>"
                             << "<p:xfrm><a:off x=\"" << (shape.getX() * 9525) << "\" y=\"" << (shape.getY() * 9525) << "\"/>"
                             << "<a:ext cx=\"2000000\" cy=\"2000000\"/></p:xfrm>"
                             << "</p:pic>";
                    relId++;
                } else {
                    slideXml << "<p:sp>"
                             << "<p:txBody><a:p><a:r><a:t>" << shape.getName() << "</a:t></a:r></a:p></p:txBody>"
                             << "<p:xfrm><a:off x=\"" << (shape.getX() * 9525) << "\" y=\"" << (shape.getY() * 9525) << "\"/>"
                             << "<a:ext cx=\"1000000\" cy=\"500000\"/></p:xfrm>"
                             << "</p:sp>";
                }
            }
        }

        slideXml << R"(    </p:spTree>
  </p:cSld>
</p:sld>)";

        (addFile)(("ppt/slides/slide" + std::to_string(i + 1) + ".xml").c_str(), slideXml.str());
    }

    zip_close(zip);
    success() << "Exported: " << path << " (open in PowerPoint!)\n";
    return true;
}
