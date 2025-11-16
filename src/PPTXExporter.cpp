// PPTXExporter.cpp
#include "../include/PPTXExporter.hpp"
#include "../include/SlideShow.hpp"
#include "../include/Shape.hpp"
#include "../include/Color.hpp"
#include <zip.h>
#include <filesystem>
#include <sstream>

namespace fs = std::filesystem;

bool PPTXExporter::exportToRealPPTX(const std::vector<SlideShow>& /*slideshows*/,
                                    const std::vector<std::string>& /*order*/,
                                    const std::string& outputPath) {
    // Build output filename (ensure .pptx)
    std::string out = outputPath;
    if (out.size() < 5 || out.substr(out.size() - 5) != ".pptx") out += ".pptx";

    // Open zip for writing (create/truncate)
    int zipErr = 0;
    zip_t* zip = zip_open(out.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &zipErr);
    if (!zip) {
        err() << "Failed to create .pptx (zip_open error " << zipErr << ")\n";
        return false;
    }

    auto addString = [&](const std::string& name, const std::string& contents) {
        zip_source_t* src = zip_source_buffer(zip, contents.data(), contents.size(), 0);
        if (!src) return false;
        // overwrite existing or add new
        zip_int64_t idx = zip_name_locate(zip, name.c_str(), 0);
        if (idx < 0) zip_file_add(zip, name.c_str(), src, ZIP_FL_ENC_UTF_8);
        else zip_file_add(zip, name.c_str(), src, ZIP_FL_OVERWRITE);
        return true;
    };

    // 1) [Content_Types].xml
    const std::string content_types =
R"(<?xml version="1.0" encoding="UTF-8"?>
<Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types">
  <Default Extension="rels" ContentType="application/vnd.openxmlformats-package.relationships+xml"/>
  <Default Extension="xml" ContentType="application/xml"/>
  <Override PartName="/ppt/presentation.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.presentation.main+xml"/>
  <Override PartName="/ppt/slides/slide1.xml" ContentType="application/vnd.openxmlformats-officedocument.presentationml.slide+xml"/>
  <Override PartName="/ppt/theme/theme1.xml" ContentType="application/vnd.openxmlformats-officedocument.theme+xml"/>
  <Override PartName="/docProps/core.xml" ContentType="application/vnd.openxmlformats-package.core-properties+xml"/>
  <Override PartName="/docProps/app.xml" ContentType="application/vnd.openxmlformats-officedocument.extended-properties+xml"/>
</Types>)";
    addString("[Content_Types].xml", content_types);

    // 2) _rels/.rels
    const std::string root_rels =
R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/officeDocument" Target="ppt/presentation.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties" Target="docProps/core.xml"/>
  <Relationship Id="rId3" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/extended-properties" Target="docProps/app.xml"/>
</Relationships>)";
    addString("_rels/.rels", root_rels);

    // 3) docProps/core.xml
    const std::string core_xml =
R"(<?xml version="1.0" encoding="UTF-8"?>
<cp:coreProperties xmlns:cp="http://schemas.openxmlformats.org/package/2006/metadata/core-properties"
                   xmlns:dc="http://purl.org/dc/elements/1.1/"
                   xmlns:dcterms="http://purl.org/dc/terms/"
                   xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <dc:title>Generated PPTX</dc:title>
  <dc:creator>SlideShow CLI</dc:creator>
</cp:coreProperties>)";
    addString("docProps/core.xml", core_xml);

    // 4) docProps/app.xml
    const std::string app_xml =
R"(<?xml version="1.0" encoding="UTF-8"?>
<Properties xmlns="http://schemas.openxmlformats.org/officeDocument/2006/extended-properties"
            xmlns:vt="http://schemas.openxmlformats.org/officeDocument/2006/docPropsVTypes">
  <Application>SlideShow CLI</Application>
</Properties>)";
    addString("docProps/app.xml", app_xml);

    // 5) ppt/presentation.xml
    const std::string presentation_xml =
R"(<?xml version="1.0" encoding="UTF-8"?>
<p:presentation xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
                xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
                xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:sldIdLst>
    <p:sldId id="256" r:id="rId1"/>
  </p:sldIdLst>
  <p:defaultTextStyle/>
</p:presentation>)";
    addString("ppt/presentation.xml", presentation_xml);

    // 6) ppt/_rels/presentation.xml.rels
    const std::string presentation_rels =
R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
  <Relationship Id="rId1" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/slide" Target="slides/slide1.xml"/>
  <Relationship Id="rId2" Type="http://schemas.openxmlformats.org/officeDocument/2006/relationships/theme" Target="theme/theme1.xml"/>
</Relationships>)";
    addString("ppt/_rels/presentation.xml.rels", presentation_rels);

    // 7) ppt/theme/theme1.xml (minimal theme)
    const std::string theme_xml =
R"(<?xml version="1.0" encoding="UTF-8"?>
<a:theme xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main" name="Office Theme">
  <a:themeElements>
    <a:clrScheme name="Office">
      <a:dk1><a:sysClr val="windowText" lastClr="000000"/></a:dk1>
      <a:lt1><a:sysClr val="window" lastClr="FFFFFF"/></a:lt1>
      <a:dk2><a:srgbClr val="1F497D"/></a:dk2>
      <a:lt2><a:srgbClr val="EEECE1"/></a:lt2>
      <a:accent1><a:srgbClr val="4F81BD"/></a:accent1>
    </a:clrScheme>
  </a:themeElements>
</a:theme>)";
    addString("ppt/theme/theme1.xml", theme_xml);

    // 8) ppt/slides/slide1.xml (Hello World)
    const std::string slide1_xml =
R"(<?xml version="1.0" encoding="UTF-8"?>
<p:sld xmlns:a="http://schemas.openxmlformats.org/drawingml/2006/main"
       xmlns:r="http://schemas.openxmlformats.org/officeDocument/2006/relationships"
       xmlns:p="http://schemas.openxmlformats.org/presentationml/2006/main">
  <p:cSld>
    <p:spTree>
      <p:nvGrpSpPr>
        <p:cNvPr id="1" name=""/>
        <p:cNvGrpSpPr/>
        <p:nvPr/>
      </p:nvGrpSpPr>
      <p:grpSpPr>
        <a:xfrm>
          <a:off x="0" y="0"/>
          <a:ext cx="0" cy="0"/>
        </a:xfrm>
      </p:grpSpPr>

      <p:sp>
        <p:nvSpPr>
          <p:cNvPr id="2" name="Title 1"/>
          <p:cNvSpPr/>
          <p:nvPr/>
        </p:nvSpPr>
        <p:spPr/>
        <p:txBody>
          <a:bodyPr/>
          <a:lstStyle/>
          <a:p>
            <a:r>
              <a:t>Hello World</a:t>
            </a:r>
          </a:p>
        </p:txBody>
      </p:sp>

    </p:spTree>
  </p:cSld>
</p:sld>)";
    addString("ppt/slides/slide1.xml", slide1_xml);

    // 9) ppt/slides/_rels/slide1.xml.rels (empty relationships for slide)
    const std::string slide1_rels =
R"(<?xml version="1.0" encoding="UTF-8"?>
<Relationships xmlns="http://schemas.openxmlformats.org/package/2006/relationships">
</Relationships>)";
    addString("ppt/slides/_rels/slide1.xml.rels", slide1_rels);

    // finalize
    if (zip_close(zip) < 0) {
        err() << "Failed to finalize .pptx: " << zip_strerror(zip) << "\n";
        return false;
    }

    success() << "Exported minimal PPTX: " << out << "\n";
    return true;
}
