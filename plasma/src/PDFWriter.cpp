#include "plasma/FilePath.h"
// PDF export builds content streams in string streams and stores them Flate-compressed with zlib. The writer emits
// the document, images and node forms.
#include "pdf/PDFWriter.h"

#include "main.h"
#include "ProgressModule.h"
#include "Workspace.h"
#include "ColorManagementModule.h"
#include "plasma/Display.h"
#include "plasma/Drawing.h"
#include "plasma/Node.h"
#include "plasma/ScalableFont.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/TextShape.h"
#include <zlib.h>
#include "plasma/Engine.h"
#include "plasma/FontEngine.h"
#include "plasma/PLXStreamWriter.h"
#include "Image.h"
#include <iomanip>

// FreeType 2.4.1 functions the writer calls (as ScalableFont.cpp declares them).
#ifndef PLASMA_PORTABLE
extern "C" {
int FT_Load_Char(FT_Face face, unsigned long character, long flags);
unsigned FT_Get_Char_Index(FT_Face face, unsigned long character);
int FT_Get_Kerning(FT_Face face, unsigned left, unsigned right, unsigned mode, FT_Vector *kerning);
unsigned long FT_Get_First_Char(FT_Face face, unsigned *glyphIndex);
}
#endif

// A font descriptor's /Flags bit for a symbolic font (PDF 1.5, 5.7.1).
enum {
    PDF_FONT_SYMBOLIC = 4
};

// Flate-compresses a content stream into out, sized by zlib's worst-case bound for compress (0.1% + 12 bytes,
// generously rounded up).
void compress(std::vector<char> &out, const std::stringstream &content)
{
    std::string contentBytes = content.str();
    uLong size = contentBytes.size();
    uLongf compressedSize = (uLongf)(size * 1.1) + 12;
    out.resize(compressedSize);
    compress((Bytef *)&out[0], &compressedSize, (const Bytef *)contentBytes.c_str(), size);
    out.resize(compressedSize);
}

PDFWriter::PDFWriter(Workspace *workspace, const std::wstring &path, int options)
    : m_workspace(workspace),
      m_file(PLASMA_FILE_PATH(path), std::ios::binary),
      m_page(1000.0f, 1000.0f, 72.0f),
      m_path(path),
      m_options(options),
      m_rgbToCMYKFloat(0),
      m_rgbToCMYK8(0)
{}

int PDFWriter::beginObject()
{
    m_objectOffsets.push_back(m_file.tellp());
    m_file << m_objectOffsets.size() << " 0 obj" << std::endl;
    return m_objectOffsets.size();
}

void PDFWriter::endObject()
{
    m_file << "endobj" << std::endl;
}

bool PDFWriter::write(plasma::Node *root, const plasma::Engine::Page *page)
{
    if (m_file.rdstate())
        return false;
    m_currentRoot = root;
    m_page = *page;
    m_fontCount = 0;
    countFontMeshes(root);
    if (m_fontCount) {
        m_workspace->getProgressModule()->setText(L"Creating font meshes ");
        m_workspace->getProgressModule()->setProgress(0.0f);
        pumpMessages();
        createFontMeshes(root);
        m_workspace->getProgressModule()->setProgress(1.0f);
    }
    m_nodeCount = 0;
    prepareRasterizedNodes(root);
    int separator = (int)m_path.rfind(L'/');
    int backslash = (int)m_path.rfind(L'\\');
    if (separator <= backslash)
        separator = backslash;
    std::wstring progress(L"Writing ");
    progress += m_path.substr(separator + 1, m_path.size() - separator - 1);
    m_workspace->getProgressModule()->setText(progress);
    m_workspace->getProgressModule()->setProgress(0.0f);
    pumpMessages();
    m_file << "%PDF-1.5" << std::endl;
    m_file << std::fixed;
    m_colorSpace = "/DeviceRGB";
    cmsHPROFILE srgb = cmsCreate_sRGBProfile();
    if (m_options & kPDFOutputCMYK) {
        m_rgbToCMYKFloat = cmsCreateTransform(srgb, TYPE_RGB_FLT, m_workspace->m_colorManagementModule->m_profile,
                                              TYPE_CMYK_FLT, m_workspace->m_colorManagementModule->m_intent, 0);
        m_rgbToCMYK8 = cmsCreateTransform(srgb, TYPE_RGB_8, m_workspace->m_colorManagementModule->m_profile,
                                          TYPE_CMYK_8, m_workspace->m_colorManagementModule->m_intent, 0);
        cmsUInt32Number size = 0;
        cmsSaveProfileToMem(m_workspace->m_colorManagementModule->m_profile, 0, &size);
        std::vector<char> profile;
        profile.resize(size);
        cmsSaveProfileToMem(m_workspace->m_colorManagementModule->m_profile, &profile[0], &size);
        int profileObject = beginObject();
        m_file << "<</N 4" << std::endl;
        m_file << "/Length " << size << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&profile[0], size);
        m_file << "\nendstream" << std::endl;
        endObject();
        int colorSpace = beginObject();
        m_file << "[/ICCBased " << profileObject << " 0 R]" << std::endl;
        endObject();
        std::stringstream reference;
        reference << colorSpace << " 0 R" << std::endl;
        m_colorSpace = reference.str();
    }
    if (!(m_options & kPDFRasterizeAll)) {
        for (std::list<plasma::Node *>::iterator it = root->m_children.begin(); it != root->m_children.end(); ++it)
            writeShadings(*it);
    }
    if (m_options & kPDFMultipage) {
        for (std::list<plasma::Node *>::iterator it = root->m_children.begin(); it != root->m_children.end(); ++it) {
            m_currentRoot = *it;
            writeNodeGroup(*it);
        }
    } else {
        writeNodeGroup(root);
    }
    m_currentRoot = root;
    int transparencyGroup = beginObject();
    m_file << "<</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    endObject();
    int opaqueState = beginObject();
    m_file << "<</AIS false/BM/Normal/CA 1.0/OP false/OPM 1/SA true/SMask/None/Type/ExtGState/ca 1.0/op false>>"
           << std::endl;
    endObject();
    plasma::Matrix<float> matrix;
    matrix.identity();
    matrix.translate(plasma::Vector<2, float>(0.0f, m_page.size.m_elements[1]));
    matrix.scale(1.0f, -1.0f);
    bool empty = true;
    plasma::Vector<2, float> minimum, maximum;
    root->extendBounds(minimum, maximum, matrix, empty, true);

    // The code reads uninitialized ArtBox bounds when an empty root supplies none.
    std::list<plasma::Node *> pages;
    if (m_options & kPDFMultipage)
        pages = root->getChildren();
    else
        pages.push_back(root);
    int resources = beginObject();
    m_file << "<< " << std::endl;
    m_file << "/ProcSet[/PDF/Text]" << std::endl;
    if (m_options & kPDFOutputCMYK)
        m_file << "/ColorSpace<</CS0 " << m_colorSpace << ">>" << std::endl;
    m_file << "/ExtGState <<" << std::endl;
    for (std::map<const plasma::Drawing *, int>::iterator it = m_drawingGraphicsStates.begin();
         it != m_drawingGraphicsStates.end(); ++it)
        m_file << "/GS" << it->second << " " << it->second << " 0 R" << std::endl;
    for (std::list<int>::iterator it = m_graphicsStateObjects.begin(); it != m_graphicsStateObjects.end(); ++it)
        m_file << "/GS" << *it << " " << *it << " 0 R" << std::endl;
    m_file << "/GSOpaque " << opaqueState << " 0 R" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "/Shading <<" << std::endl;
    for (std::map<const plasma::Drawing *, int>::iterator it = m_colorShadings.begin(); it != m_colorShadings.end();
         ++it)
        m_file << "/Sh" << it->second << " " << it->second << " 0 R" << std::endl;
    for (std::map<const plasma::Drawing *, int>::iterator it = m_alphaShadings.begin(); it != m_alphaShadings.end();
         ++it)
        m_file << "/Sh" << it->second << " " << it->second << " 0 R" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "/XObject <<" << std::endl;
    for (std::map<plasma::Node *, int>::iterator it = m_nodeForms.begin(); it != m_nodeForms.end(); ++it)
        m_file << "/Gr" << it->second << " " << it->second << " 0 R" << std::endl;
    for (std::map<plasma::Node *, int>::iterator it = m_childGroups.begin(); it != m_childGroups.end(); ++it)
        m_file << "/Gr" << it->second << " " << it->second << " 0 R" << std::endl;
    for (std::list<int>::iterator it = m_groupObjects.begin(); it != m_groupObjects.end(); ++it)
        m_file << "/Gr" << *it << " " << *it << " 0 R" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "/Font <<" << std::endl;
    for (std::map<plasma::ScalableFont *, int>::iterator it = m_fontObjects.begin(); it != m_fontObjects.end(); ++it)
        m_file << "/F" << it->second << " " << it->second << " 0 R" << std::endl;
    m_file << ">>" << std::endl;
    m_file << ">>" << std::endl;
    endObject();
    std::list<int> pageObjects;
    int parent = m_objectOffsets.size() + pages.size() + 1;
    for (std::list<plasma::Node *>::iterator it = pages.begin(); it != pages.end(); ++it) {
        int pageObject = beginObject();
        pageObjects.push_back(pageObject);
        m_file << "<</Type /Page" << std::endl;
        m_file << "/MediaBox[0 0 " << page->size.m_elements[0] << " " << page->size.m_elements[1] << " ]" << std::endl;
        m_file << "/BleedBox[0 0 " << page->size.m_elements[0] << " " << page->size.m_elements[1] << " ]" << std::endl;
        m_file << "/TrimBox[0 0 " << page->size.m_elements[0] << " " << page->size.m_elements[1] << " ]" << std::endl;
        m_file << "/ArtBox[ " << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0]
               << " " << maximum.m_elements[1] << " ]" << std::endl;
        m_file << "/Contents " << m_nodeForms[*it] << " 0 R" << std::endl;
        m_file << "/Group " << transparencyGroup << " 0 R" << std::endl;
        m_file << "/Parent " << parent << " 0 R" << std::endl;
        m_file << "/Resources " << resources << " 0 R" << std::endl;
        m_file << ">>" << std::endl;
        endObject();
    }
    int pageTree = beginObject();
    m_file << "<< /Type /Pages" << std::endl;
    m_file << "/Count " << pages.size() << std::endl;
    m_file << "/Kids [ ";
    for (std::list<int>::iterator it = pageObjects.begin(); it != pageObjects.end(); ++it)
        m_file << *it << " 0 R ";
    m_file << "]" << std::endl;
    m_file << ">>" << std::endl;
    endObject();
    int catalog = beginObject();
    m_file << "<< /Type /Catalog" << std::endl;
    m_file << "/Pages " << pageTree << " 0 R" << std::endl;
    m_file << ">>" << std::endl;
    endObject();
    std::streampos xref = m_file.tellp();
    m_file << "xref" << std::endl;
    m_file << "0 " << m_objectOffsets.size() + 1 << std::endl;
    m_file.fill('0');
    m_file << "0000000000 65535 f \n";
    for (int i = 0; i < (int)m_objectOffsets.size(); i++)
        m_file << std::setw(10) << (std::streamoff)m_objectOffsets[i] << " 00000 n \n";
    m_file << "trailer" << std::endl;
    m_file << "<< /Size " << m_objectOffsets.size() + 1 << std::endl;
    m_file << "/Root " << catalog << " 0 R" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "startxref" << std::endl;
    m_file << (std::streamoff)xref << std::endl;
    m_file << "%%EOF" << std::endl;
    for (std::map<plasma::Node *, Image *>::iterator it = m_rasterizedNodes.begin(); it != m_rasterizedNodes.end();
         ++it)
        delete it->second;
    if (m_rgbToCMYKFloat) {
        cmsDeleteTransform(m_rgbToCMYKFloat);
        m_rgbToCMYKFloat = 0;
    }
    if (m_rgbToCMYK8) {
        cmsDeleteTransform(m_rgbToCMYK8);
        m_rgbToCMYK8 = 0;
    }
    cmsCloseProfile(srgb);
    destroyTemporaryNodes();
    m_workspace->getProgressModule()->setProgress(1.0f);
    pumpMessages();
    return true;
}

// A drawing has its own graphics state, per node when available; otherwise it uses the shape's state, with the
// stroke selected when requested.
void PDFWriter::writeGraphicsState(plasma::Node *node, std::ostream &out, const plasma::Drawing *drawing,
                                   plasma::Shape *shape, bool stroke)
{
    if (drawing) {
        if (m_drawingImageGraphicsStates.find(std::make_pair(node, drawing)) != m_drawingImageGraphicsStates.end())
            out << "/GS" << m_drawingImageGraphicsStates[std::make_pair(node, drawing)] << " gs" << std::endl;
        else
            out << "/GS" << m_drawingGraphicsStates[drawing] << " gs" << std::endl;
    } else if (stroke) {
        out << "/GS" << m_textStrokeGraphicsStates[shape] << " gs" << std::endl;
    } else {
        out << "/GS" << m_textFillGraphicsStates[shape] << " gs" << std::endl;
    }
}

// Draws a drawing's group, per node when available, or otherwise the shape's group. Drawings without an area draw
// nothing.
void PDFWriter::writeGroupReference(plasma::Node *node, std::ostream &out, const plasma::Drawing *drawing,
                                    plasma::Shape *shape, bool stroke)
{
    if (drawing && drawing->m_pathVertices.size() < 3)
        return;
    out << "q" << std::endl;
    if (drawing) {
        if (m_drawingImageGroups.find(std::make_pair(node, drawing)) != m_drawingImageGroups.end())
            out << "/Gr" << m_drawingImageGroups[std::make_pair(node, drawing)] << " Do" << std::endl;
        else
            out << "/Gr" << m_drawingGroups[drawing] << " Do" << std::endl;
    } else if (stroke) {
        out << "/Gr" << m_textStrokeGroups[shape] << " Do" << std::endl;
    } else {
        out << "/Gr" << m_textFillGroups[shape] << " Do" << std::endl;
    }
    out << "Q" << std::endl;
}

// The group of a node with a blend effect: DISPLAY_MODULATION or DISPLAY_SCREEN.
void PDFWriter::writeBlendedGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node,
                                  plasma::Shape *shape, bool stroke)
{
    if (node->m_display->m_flags & (1 << plasma::DISPLAY_MODULATION))
        writeMultiplyColorDodgeGroup(out, drawing, node, shape, stroke);
    else if (node->m_display->m_flags & (1 << plasma::DISPLAY_SCREEN))
        writeDifferenceGroup(out, drawing, node, shape, stroke);
    else
        writeGroup(out, drawing, node, shape, stroke);
}

// A transparency group over the drawing's or shape's bounds. It draws the shape or drawing group and the node
// group through the graphics state.
void PDFWriter::writeGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node, plasma::Shape *shape,
                           bool stroke)
{
    out << "q" << std::endl;
    plasma::Vector<2, float> minimum = drawing ? drawing->m_minimum : shape->getMinimum();
    plasma::Vector<2, float> maximum = drawing ? drawing->m_maximum : shape->getMaximum();
    std::stringstream content;
    content << std::fixed;
    content << "q" << std::endl;
    writeGroupReference(node, content, drawing, shape, stroke);
    content << "/Gr" << m_childGroups[node] << " Do" << std::endl;
    content << "Q" << std::endl;
    std::vector<char> compressed;
    compress(compressed, content);
    int group = beginObject();
    m_groupObjects.push_back(group);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();
    writeGraphicsState(node, out, drawing, shape, stroke);
    out << "/Gr" << group << " Do" << std::endl;
    out << "Q" << std::endl;
}

// The Multiply and ColorDodge composite: the node's group is combined with grey backdrops through Multiply,
// ColorDodge and the Difference state of WriteDifferenceState, in five nested groups.
void PDFWriter::writeMultiplyColorDodgeGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node,
                                             plasma::Shape *shape, bool stroke)
{
    plasma::Vector<2, float> minimum = drawing ? drawing->m_minimum : shape->getMinimum();
    plasma::Vector<2, float> maximum = drawing ? drawing->m_maximum : shape->getMaximum();
    int difference = writeDifferenceState(node, minimum, maximum);
    int multiply = beginObject();
    m_graphicsStateObjects.push_back(multiply);
    m_file << "<</AIS false/BM/Multiply/CA 1.0/OP false/OPM 1/SA true/SMask/None/Type/ExtGState/ca 1.0/op false>>"
           << std::endl;
    endObject();
    int colorDodge = beginObject();
    m_graphicsStateObjects.push_back(colorDodge);
    m_file << "<</AIS false/BM/ColorDodge/CA 1.0/OP false/OPM 1/SA true/SMask/None/Type/ExtGState/ca 1.0/op false>>"
           << std::endl;
    endObject();

    // White, then black through the Difference state, over the bounds.
    std::stringstream content;
    content << std::fixed;
    content << "q" << std::endl;
    content << "q" << std::endl;
    content << "/GSOpaque gs" << std::endl;
    content << "1 g" << std::endl;
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re" << std::endl;
    content << "f" << std::endl;
    content << "Q" << std::endl;
    content << "q" << std::endl;
    content << "/GS" << difference << " gs" << std::endl;
    content << "0 g" << std::endl;
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re" << std::endl;
    content << "f" << std::endl;
    content << "Q" << std::endl;
    content << "Q" << std::endl;
    std::vector<char> compressed;
    compress(compressed, content);
    int backdrop = beginObject();
    m_groupObjects.push_back(backdrop);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    // The node's group over black.
    content.str("");
    content << "q" << std::endl;
    content << "/GSOpaque gs" << std::endl;
    content << "q" << std::endl;
    content << "0 g" << std::endl;
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re" << std::endl;
    content << "f" << std::endl;
    content << "Q" << std::endl;
    content << "/Gr" << m_childGroups[node] << " Do" << std::endl;
    content << "Q" << std::endl;
    compress(compressed, content);
    int nodeOverBlack = beginObject();
    m_groupObjects.push_back(nodeOverBlack);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    // ColorDodge of the backdrop over it.
    content.str("");
    content << "q" << std::endl;
    content << "/Gr" << nodeOverBlack << " Do" << std::endl;
    content << "/GS" << colorDodge << " gs" << std::endl;
    content << "/Gr" << backdrop << " Do" << std::endl;
    content << "Q" << std::endl;
    compress(compressed, content);
    int dodged = beginObject();
    m_groupObjects.push_back(dodged);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    // The shape's (or drawing's) group, multiplied by the result.
    content.str("");
    content << "q" << std::endl;
    content << "/GSOpaque gs" << std::endl;
    writeGroupReference(node, content, drawing, shape, stroke);
    content << "/GS" << multiply << " gs" << std::endl;
    content << "/Gr" << dodged << " Do" << std::endl;
    content << "Q" << std::endl;
    compress(compressed, content);
    int multiplied = beginObject();
    m_groupObjects.push_back(multiplied);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    // Through the Difference state's soft mask.
    content.str("");
    content << "q" << std::endl;
    content << "/GS" << difference << " gs" << std::endl;
    content << "/Gr" << multiplied << " Do" << std::endl;
    content << "Q" << std::endl;
    compress(compressed, content);
    int group = beginObject();
    m_groupObjects.push_back(group);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    out << "q" << std::endl;
    writeGraphicsState(node, out, drawing, shape, stroke);
    out << "/Gr" << group << " Do" << std::endl;
    out << "Q" << std::endl;
}

// Creates a Difference ExtGState and a state with a luminosity soft mask. The mask draws the node group over
// black, then over white through Difference. Returns the latter state's object number.
int PDFWriter::writeDifferenceState(plasma::Node *node, const plasma::Vector<2, float> &minimum,
                                    const plasma::Vector<2, float> &maximum)
{
    int difference = beginObject();
    m_graphicsStateObjects.push_back(difference);
    m_file << "<</AIS false/BM/Difference/CA 1.0/OP false/OPM 1/SA true/SMask/None/Type/ExtGState/ca 1.0/op false>>"
           << std::endl;
    endObject();
    std::stringstream content;
    std::vector<char> compressed;
    content << std::fixed;
    content << "q" << std::endl;
    content << "0 g" << std::endl;
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re f" << std::endl;
    content << "/Gr" << m_childGroups[node] << " Do" << std::endl;
    content << "Q" << std::endl;
    compress(compressed, content);
    int blackGroup = beginObject();
    m_groupObjects.push_back(blackGroup);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS/DeviceGray/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    content.str("");
    content << "q" << std::endl;
    content << "1 g" << std::endl;
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re f" << std::endl;
    content << "/Gr" << m_childGroups[node] << " Do" << std::endl;
    content << "/GS" << difference << " gs" << std::endl;
    content << "/Gr" << blackGroup << " Do" << std::endl;
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re f" << std::endl;
    content << "Q" << std::endl;
    compress(compressed, content);
    int whiteGroup = beginObject();
    m_groupObjects.push_back(whiteGroup);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS/DeviceGray/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    int mask = beginObject();
    m_file << "<</G " << whiteGroup << " 0 R/S/Luminosity/Type/Mask>>" << std::endl;
    endObject();
    int state = beginObject();
    m_graphicsStateObjects.push_back(state);
    m_file << "<</AIS false/BM/Normal/CA 1.0/OP false/OPM 1/SA true/SMask " << mask
           << " 0 R/Type/ExtGState/ca 1.0/op false>>" << std::endl;
    endObject();
    return state;
}

// The group drawn through the Difference state's soft mask.
void PDFWriter::writeDifferenceGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node,
                                     plasma::Shape *shape, bool stroke)
{
    plasma::Vector<2, float> minimum = drawing ? drawing->m_minimum : shape->getMinimum();
    plasma::Vector<2, float> maximum = drawing ? drawing->m_maximum : shape->getMaximum();
    int state = writeDifferenceState(node, minimum, maximum);
    std::stringstream content;
    content << std::fixed;
    content.str("");
    content << "q" << std::endl;
    content << "/GS" << state << " gs" << std::endl;
    writeGroupReference(node, content, drawing, shape, stroke);
    content << "Q" << std::endl;
    std::vector<char> compressed;
    compress(compressed, content);
    int group = beginObject();
    m_groupObjects.push_back(group);
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();
    out << "q" << std::endl;
    writeGraphicsState(node, out, drawing, shape, stroke);
    out << "/Gr" << group << " Do" << std::endl;
    out << "Q" << std::endl;
}

void PDFWriter::writeDrawingImage(plasma::Node *node, const plasma::Drawing *drawing)
{
    Image *image = m_rasterizedDrawings[std::make_pair(node, drawing)];
    if (!image || image->m_width < 1 || image->m_height < 1)
        return;

    plasma::Matrix<float> placement;
    placement.identity();
    plasma::Matrix<float> matrix = m_currentRoot->m_inverseMatrix;
    matrix *= node->m_matrix;
    placement *= matrix;
    placement.invert();
    matrix = m_currentRoot->m_inverseMatrix;
    bool empty = true;
    matrix *= node->m_matrix;
    plasma::Vector<2, float> minimum, maximum;
    drawing->getTransformedBounds(matrix, minimum, maximum, empty);

    // The code uses the uninitialised bounds when the drawing is empty.
    std::vector<unsigned char> rgb;
    rgb.resize(3 * image->m_width * image->m_height);
    std::vector<unsigned char> alpha;
    alpha.resize(image->m_width * image->m_height);
    for (int y = 0; y < image->m_height; y++) {
        for (int x = 0; x < image->m_width; x++) {
            int offset = 3 * (x + image->m_width * (image->m_height - y - 1));
            rgb[offset] = image->m_rows[y][4 * x];
            rgb[offset + 1] = image->m_rows[y][4 * x + 1];
            rgb[offset + 2] = image->m_rows[y][4 * x + 2];
            alpha[x + image->m_width * (image->m_height - y - 1)] = image->m_rows[y][4 * x + 3];
        }
    }
    std::vector<char> compressedColor;
    if (m_options & kPDFOutputCMYK) {
        std::vector<unsigned char> cmyk;
        toCMYK(cmyk, rgb);
        plasma::compressPLX(compressedColor, cmyk);
    } else {
        plasma::compressPLX(compressedColor, rgb);
    }
    std::vector<char> compressedAlpha;
    plasma::compressPLX(compressedAlpha, alpha);

    int alphaImage = beginObject();
    m_groupObjects.push_back(alphaImage);
    m_file << "<</Type/XObject" << std::endl;
    m_file << "/Subtype/Image" << std::endl;
    m_file << "/ColorSpace/DeviceGray" << std::endl;
    m_file << "/BitsPerComponent 8" << std::endl;
    m_file << "/Width " << image->m_width << std::endl;
    m_file << "/Height " << image->m_height << std::endl;
    m_file << "/Length " << compressedAlpha.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressedAlpha[0], compressedAlpha.size());
    m_file << "\nendstream" << std::endl;
    endObject();

    std::stringstream content;
    content << std::fixed;
    content << "q" << std::endl;
    float width = maximum.m_elements[0] - minimum.m_elements[0];
    float height = maximum.m_elements[1] - minimum.m_elements[1];
    content << width << " 0 0 " << height << " " << minimum.m_elements[0] << " " << minimum.m_elements[1] << " cm"
            << std::endl;
    content << "/Gr" << alphaImage << " Do" << std::endl;
    content << "Q" << std::endl;
    std::vector<char> compressed;
    compress(compressed, content);
    int alphaGroup = beginObject();
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << " ]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [";
    m_file << placement.m_elements[0];
    m_file << " ";
    m_file << placement.m_elements[1];
    m_file << " ";
    m_file << placement.m_elements[4];
    m_file << " ";
    m_file << placement.m_elements[5];
    m_file << " ";
    m_file << placement.m_elements[12];
    m_file << " ";
    m_file << placement.m_elements[13] << "]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    int mask = beginObject();
    m_file << "<</G " << alphaGroup << " 0 R/S/Luminosity/Type/Mask>>" << std::endl;
    endObject();
    int state = beginObject();
    m_graphicsStateObjects.push_back(state);
    m_drawingImageGraphicsStates[std::make_pair(node, drawing)] = state;
    m_file << "<</AIS false/BM/Normal/CA 1.0/OP false/OPM 1/SA true/SMask " << mask
           << " 0 R/Type/ExtGState/ca 1.0/op false>>" << std::endl;
    endObject();

    int colorImage = beginObject();
    m_groupObjects.push_back(colorImage);
    m_file << "<</Type/XObject" << std::endl;
    m_file << "/Subtype/Image" << std::endl;
    m_file << "/ColorSpace " << m_colorSpace << std::endl;
    m_file << "/BitsPerComponent 8" << std::endl;
    m_file << "/Width " << image->m_width << std::endl;
    m_file << "/Height " << image->m_height << std::endl;
    m_file << "/Length " << compressedColor.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressedColor[0], compressedColor.size());
    m_file << "\nendstream" << std::endl;
    endObject();

    content.str("");
    content << std::fixed;
    content << "q" << std::endl;
    content << width << " 0 0 " << height << " " << minimum.m_elements[0] << " " << minimum.m_elements[1] << " cm"
            << std::endl;
    content << "/Gr" << colorImage << " Do" << std::endl;
    content << "Q" << std::endl;
    compress(compressed, content);
    int group = beginObject();
    m_groupObjects.push_back(group);
    m_drawingImageGroups[std::make_pair(node, drawing)] = group;
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << " ]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [";
    m_file << placement.m_elements[0];
    m_file << " ";
    m_file << placement.m_elements[1];
    m_file << " ";
    m_file << placement.m_elements[4];
    m_file << " ";
    m_file << placement.m_elements[5];
    m_file << " ";
    m_file << placement.m_elements[12];
    m_file << " ";
    m_file << placement.m_elements[13] << "]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();
}

void PDFWriter::writeNodeGroup(plasma::Node *node)
{
    if (!node->isShown() || !node->display()->m_visibility.value())
        return;
    if (m_rasterizedNodes.find(node) != m_rasterizedNodes.end()) {
        Image *image = m_rasterizedNodes[node];
        if (image->m_width < 1 || image->m_height < 1)
            return;
        std::vector<unsigned char> rgb;
        rgb.resize(3 * image->m_width * image->m_height);
        std::vector<unsigned char> alpha;
        alpha.resize(image->m_width * image->m_height);
        for (int y = 0; y < image->m_height; y++) {
            for (int x = 0; x < image->m_width; x++) {
                int offset = 3 * (x + image->m_width * (image->m_height - y - 1));
                rgb[offset] = image->m_rows[y][4 * x];
                rgb[offset + 1] = image->m_rows[y][4 * x + 1];
                rgb[offset + 2] = image->m_rows[y][4 * x + 2];
                alpha[x + image->m_width * (image->m_height - y - 1)] = image->m_rows[y][4 * x + 3];
            }
        }
        std::vector<char> compressedColor;
        if (m_options & kPDFOutputCMYK) {
            std::vector<unsigned char> cmyk;
            toCMYK(cmyk, rgb);
            plasma::compressPLX(compressedColor, cmyk);
        } else {
            plasma::compressPLX(compressedColor, rgb);
        }
        std::vector<char> compressedAlpha;
        plasma::compressPLX(compressedAlpha, alpha);

        int alphaImage = beginObject();
        m_groupObjects.push_back(alphaImage);
        m_file << "<</Type/XObject" << std::endl;
        m_file << "/Subtype/Image" << std::endl;
        m_file << "/ColorSpace/DeviceGray" << std::endl;
        m_file << "/BitsPerComponent 8" << std::endl;
        m_file << "/Width " << image->m_width << std::endl;
        m_file << "/Height " << image->m_height << std::endl;
        m_file << "/Length " << compressedAlpha.size() << std::endl;
        m_file << "/Filter/FlateDecode" << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&compressedAlpha[0], compressedAlpha.size());
        m_file << "\nendstream" << std::endl;
        endObject();

        int colorImage = beginObject();
        m_groupObjects.push_back(colorImage);
        m_file << "<</Type/XObject" << std::endl;
        m_file << "/Subtype/Image" << std::endl;
        m_file << "/ColorSpace " << m_colorSpace << std::endl;
        m_file << "/BitsPerComponent 8" << std::endl;
        m_file << "/SMask " << alphaImage << " 0 R" << std::endl;
        m_file << "/Width " << image->m_width << std::endl;
        m_file << "/Height " << image->m_height << std::endl;
        m_file << "/Length " << compressedColor.size() << std::endl;
        m_file << "/Filter/FlateDecode" << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&compressedColor[0], compressedColor.size());
        m_file << "\nendstream" << std::endl;
        endObject();

        bool empty = true;
        plasma::Matrix<float> matrix = m_currentRoot->m_inverseMatrix;
        matrix *= node->m_matrix;
        plasma::Vector<2, float> minimum, maximum;
        if (isRasterized(node))
            node->extendBounds(minimum, maximum, matrix, empty, true);
        else
            node->m_shape->getTransformedBounds(minimum, maximum, matrix, empty);
        std::stringstream content;
        content << std::fixed;
        content << "q" << std::endl;
        if (node == m_currentRoot) {
            content << "1 0 0 -1 0 " << m_page.size.m_elements[1] << " cm" << std::endl;
            content << m_page.color.m_elements[0] << " " << m_page.color.m_elements[1] << " "
                    << m_page.color.m_elements[2] << " rg" << std::endl;
            content << "0 0 " << m_page.size.m_elements[0] << " " << m_page.size.m_elements[1] << " re f" << std::endl;
        }
        float width = maximum.m_elements[0] - minimum.m_elements[0];
        float height = maximum.m_elements[1] - minimum.m_elements[1];
        content << width << " 0 0 " << height << " " << minimum.m_elements[0] << " " << minimum.m_elements[1] << " cm"
                << std::endl;
        content << "/Gr" << colorImage << " Do" << std::endl;
        content << "Q" << std::endl;
        std::vector<char> compressed;
        compress(compressed, content);
        plasma::Matrix<float> placement;
        placement.identity();
        matrix = m_currentRoot->m_inverseMatrix;
        matrix *= node->m_parent->m_matrix;
        placement *= matrix;
        placement.invert();
        empty = true;
        matrix = m_currentRoot->m_inverseMatrix;
        matrix *= node->m_matrix;
        node->extendBounds(minimum, maximum, matrix, empty, true);
        m_nodeForms[node] = beginObject();
        m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0]
               << " " << maximum.m_elements[1] << " ]" << std::endl;
        m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
        m_file << "/Length " << compressed.size() << std::endl;
        m_file << "/Filter/FlateDecode" << std::endl;
        m_file << "/Matrix [";
        m_file << placement.m_elements[0];
        m_file << " ";
        m_file << placement.m_elements[1];
        m_file << " ";
        m_file << placement.m_elements[4];
        m_file << " ";
        m_file << placement.m_elements[5];
        m_file << " ";
        m_file << placement.m_elements[12];
        m_file << " ";
        m_file << placement.m_elements[13] << "]" << std::endl;
        m_file << "/Subtype/Form" << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&compressed[0], compressed.size());
        m_file << "endstream" << std::endl;
        endObject();
        m_workspace->getProgressModule()->advance((float)(0.5 / m_nodeCount));
        pumpMessages();
        return;
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        writeNodeGroup(*it);
    plasma::Matrix<float> identity;
    identity.identity();
    bool empty = true;
    plasma::Vector<2, float> minimum, maximum;
    node->extendBounds(minimum, maximum, identity, empty, true);
    if (node->display()->m_clipping.value()) {
        std::stringstream children;
        children << std::fixed;
        for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it) {
            if ((*it)->isShown() && (*it)->display()->m_visibility.value() &&
                m_nodeForms.find(*it) != m_nodeForms.end()) {
                children << "q" << std::endl;
                children << "/Gr" << m_nodeForms[*it] << " Do" << std::endl;
                children << "Q" << std::endl;
            }
        }
        std::vector<char> compressed;
        compress(compressed, children);
        m_childGroups[node] = beginObject();
        m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0]
               << " " << maximum.m_elements[1] << "]" << std::endl;
        m_file << "/Group <</CS " << m_colorSpace << "/I true/K false/S/Transparency>>" << std::endl;
        m_file << "/Length " << compressed.size() << std::endl;
        m_file << "/Filter/FlateDecode" << std::endl;
        m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
        m_file << "/Subtype/Form" << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&compressed[0], compressed.size());
        m_file << "endstream" << std::endl;
        endObject();
    }
    std::stringstream content;
    content << std::fixed;
    content << "q" << std::endl;
    if (node == m_currentRoot) {
        content << "1 0 0 -1 0 " << m_page.size.m_elements[1] << " cm" << std::endl;
        content << m_page.color.m_elements[0] << " " << m_page.color.m_elements[1] << " " << m_page.color.m_elements[2]
                << " rg" << std::endl;
        content << "0 0 " << m_page.size.m_elements[0] << " " << m_page.size.m_elements[1] << " re f" << std::endl;
    }
    plasma::Shape *shape = node->m_shape;
    plasma::SmoothMeshShape *mesh =
        shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH ? static_cast<plasma::SmoothMeshShape *>(shape) : 0;
    if (mesh) {
        if (!(mesh->m_flags & (1 << plasma::MESH_NO_FILL)) && mesh->m_fillDrawing && mesh->m_fillDrawing->m_texture)
            writeDrawingImage(node, mesh->m_fillDrawing);
        if ((mesh->m_flags & (1 << plasma::MESH_STROKE)) && mesh->m_strokeDrawing && mesh->m_strokeDrawing->m_texture)
            writeDrawingImage(node, mesh->m_strokeDrawing);
        if (node->display()->m_clipping.value()) {
            if ((mesh->isExtruded()) && mesh->m_extrusionDrawing)
                writeBlendedGroup(content, mesh->m_extrusionDrawing, node, 0, false);
            if (!(mesh->m_flags & (1 << plasma::MESH_NO_FILL)) && mesh->m_fillDrawing)
                writeBlendedGroup(content, mesh->m_fillDrawing, node, 0, false);
            if ((mesh->m_flags & (1 << plasma::MESH_STROKE)) && mesh->m_strokeDrawing)
                writeBlendedGroup(content, mesh->m_strokeDrawing, node, 0, false);
        } else {
            if ((mesh->isExtruded()) && mesh->m_extrusionDrawing) {
                content << "q" << std::endl;
                writeGraphicsState(node, content, mesh->m_extrusionDrawing, 0, false);
                writeGroupReference(node, content, mesh->m_extrusionDrawing, 0, false);
                content << "Q" << std::endl;
            }
            if ((!(mesh->m_flags & (1 << plasma::MESH_NO_FILL))) && mesh->m_fillDrawing) {
                content << "q" << std::endl;
                writeGraphicsState(node, content, mesh->m_fillDrawing, 0, false);
                writeGroupReference(node, content, mesh->m_fillDrawing, 0, false);
                content << "Q" << std::endl;
            }
            if ((mesh->m_flags & (1 << plasma::MESH_STROKE)) && mesh->m_strokeDrawing) {
                content << "q" << std::endl;
                writeGraphicsState(node, content, mesh->m_strokeDrawing, 0, false);
                writeGroupReference(node, content, mesh->m_strokeDrawing, 0, false);
                content << "Q" << std::endl;
            }
        }
    }
    shape = node->m_shape;
    if (shape && shape->getType() == plasma::SHAPE_TEXT) {
        plasma::TextShape *text = static_cast<plasma::TextShape *>(shape);
        if (text->m_font && !text->m_font->m_plasmaRoot) {
            if (node->display()->m_clipping.value()) {
                writeBlendedGroup(content, 0, node, shape, false);
                if (text->m_strokeRadius > 0.0)
                    writeBlendedGroup(content, 0, node, shape, true);
            } else {
                content << "q" << std::endl;
                writeGraphicsState(node, content, 0, shape, false);
                writeGroupReference(node, content, 0, shape, false);
                content << "Q" << std::endl;
                if (text->m_strokeRadius > 0.0) {
                    content << "q" << std::endl;
                    writeGraphicsState(node, content, 0, shape, true);
                    writeGroupReference(node, content, 0, shape, true);
                    content << "Q" << std::endl;
                }
            }
        }
    }
    if (!node->display()->m_clipping.value()) {
        for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it) {
            if ((*it)->isShown() && (*it)->display()->m_visibility.value() &&
                m_nodeForms.find(*it) != m_nodeForms.end()) {
                content << "q" << std::endl;
                content << "/Gr" << m_nodeForms[*it] << " Do" << std::endl;
                content << "Q" << std::endl;
            }
        }
    }
    content << "Q" << std::endl;
    plasma::Matrix<float> placement;
    if (node == m_currentRoot) {
        placement.m_elements[0] = 1.0f;
        placement.m_elements[1] = 0.0f;
        placement.m_elements[4] = 0.0f;
        placement.m_elements[12] = 0.0f;
        placement.m_elements[13] = 0.0f;
        placement.m_elements[5] = 1.0f;
    } else {
        placement = getTransformationMatrix(node->m_transformation);
    }
    std::vector<char> compressed;
    compress(compressed, content);
    m_nodeForms[node] = beginObject();
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Matrix [";
    m_file << placement.m_elements[0];
    m_file << " ";
    m_file << placement.m_elements[1];
    m_file << " ";
    m_file << placement.m_elements[4];
    m_file << " ";
    m_file << placement.m_elements[5];
    m_file << " ";
    m_file << placement.m_elements[12];
    m_file << " ";
    m_file << placement.m_elements[13] << "]" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();
    m_workspace->getProgressModule()->advance((float)(0.5 / m_nodeCount));
    pumpMessages();
}

// Writes the colour shading of its triangles once per drawing with an area.
void PDFWriter::writeColorShading(plasma::Drawing *drawing)
{
    if (m_colorShadings.find(drawing) != m_colorShadings.end())
        return;
    if (drawing->m_pathVertices.size() < 3)
        return;
    plasma::Vector<2, float> minimum = drawing->m_minimum;
    plasma::Vector<2, float> maximum = drawing->m_maximum;
    std::stringstream content;
    for (int i = drawing->m_pathIndices.size() - 1; i >= 0; i--) {
        int index = drawing->m_pathIndices[i];
        const plasma::Vector<2, float> &vertex = drawing->m_pathVertices[index];
        const plasma::Vector<4, float> &color = drawing->m_pathColors[index];
        unsigned x = (unsigned)((vertex.m_elements[0] - minimum.m_elements[0]) /
                                (maximum.m_elements[0] - minimum.m_elements[0]) * 4294967294.0);
        unsigned y = (unsigned)((vertex.m_elements[1] - minimum.m_elements[1]) /
                                (maximum.m_elements[1] - minimum.m_elements[1]) * 4294967294.0);
        content.put(0);
        for (unsigned byte = 1; byte <= 4; byte++)
            content.put((char)((x >> (8 * (4 - byte))) & 0xff));
        for (unsigned byte = 1; byte <= 4; byte++)
            content.put((char)((y >> (8 * (4 - byte))) & 0xff));
        if (m_options & kPDFOutputCMYK) {
            // The transform gives percentages.
            plasma::Vector<4, float> cmyk = toCMYK(color);
            for (unsigned c = 0; c < 4; c++)
                cmyk.m_elements[c] *= 0.01f;
            unsigned cyan = (unsigned)(cmyk.m_elements[0] * 4294967294.0);
            unsigned magenta = (unsigned)(cmyk.m_elements[1] * 4294967294.0);
            unsigned yellow = (unsigned)(cmyk.m_elements[2] * 4294967294.0);
            unsigned black = (unsigned)(cmyk.m_elements[3] * 4294967294.0);
            content.put((char)(cyan >> 24));
            content.put((char)(magenta >> 24));
            content.put((char)(yellow >> 24));
            content.put((char)(black >> 24));
        } else {
            unsigned red = (unsigned)(color.m_elements[0] * 4294967294.0);
            unsigned green = (unsigned)(color.m_elements[1] * 4294967294.0);
            unsigned blue = (unsigned)(color.m_elements[2] * 4294967294.0);
            content.put((char)(red >> 24));
            content.put((char)(green >> 24));
            content.put((char)(blue >> 24));
        }
    }
    std::vector<char> compressed;
    compress(compressed, content);
    m_colorShadings[drawing] = beginObject();
    m_file << "<</ShadingType 4" << std::endl;
    m_file << "/ColorSpace " << m_colorSpace << std::endl;
    if (m_options & kPDFOutputCMYK)
        m_file << "/Decode [" << minimum.m_elements[0] << " " << maximum.m_elements[0] << " " << minimum.m_elements[1]
               << " " << maximum.m_elements[1] << " 0 1 0 1 0 1 0 1]" << std::endl;
    else
        m_file << "/Decode [" << minimum.m_elements[0] << " " << maximum.m_elements[0] << " " << minimum.m_elements[1]
               << " " << maximum.m_elements[1] << " 0 1 0 1 0 1]" << std::endl;
    m_file << "/BitsPerCoordinate 32" << std::endl;
    m_file << "/BitsPerComponent 8" << std::endl;
    m_file << "/BitsPerFlag 8" << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();
}

// Writes the drawing's alpha as a grey shading of its triangles. A drawing with an area is written again even if
// it already has a shading; the condition uses || rather than the colour shading's &&.
void PDFWriter::writeAlphaShading(plasma::Drawing *drawing)
{
    if (m_alphaShadings.find(drawing) == m_alphaShadings.end() || drawing->m_pathVertices.size() >= 3) {
        plasma::Vector<2, float> minimum = drawing->m_minimum;
        plasma::Vector<2, float> maximum = drawing->m_maximum;
        std::stringstream content;
        for (int i = drawing->m_pathIndices.size() - 1; i >= 0; i--) {
            int index = drawing->m_pathIndices[i];
            const plasma::Vector<2, float> &vertex = drawing->m_pathVertices[index];
            const plasma::Vector<4, float> &color = drawing->m_pathColors[index];
            unsigned x = (unsigned)((vertex.m_elements[0] - minimum.m_elements[0]) /
                                    (maximum.m_elements[0] - minimum.m_elements[0]) * 4294967294.0);
            unsigned y = (unsigned)((vertex.m_elements[1] - minimum.m_elements[1]) /
                                    (maximum.m_elements[1] - minimum.m_elements[1]) * 4294967294.0);
            unsigned alpha = (unsigned)(color.m_elements[3] * 4294967294.0);
            content.put(0);
            for (unsigned byte = 1; byte <= 4; byte++)
                content.put((char)((x >> (8 * (4 - byte))) & 0xff));
            for (unsigned byte = 1; byte <= 4; byte++)
                content.put((char)((y >> (8 * (4 - byte))) & 0xff));
            content.put((char)(alpha >> 24));
        }
        std::vector<char> compressed;
        compress(compressed, content);
        m_alphaShadings[drawing] = beginObject();
        m_file << "<</ShadingType 4" << std::endl;
        m_file << "/ColorSpace/DeviceGray" << std::endl;
        m_file << "/Decode [" << minimum.m_elements[0] << " " << maximum.m_elements[0] << " " << minimum.m_elements[1]
               << " " << maximum.m_elements[1] << " 0 1]" << std::endl;
        m_file << "/BitsPerCoordinate 32" << std::endl;
        m_file << "/BitsPerComponent 8" << std::endl;
        m_file << "/BitsPerFlag 8" << std::endl;
        m_file << "/Filter/FlateDecode" << std::endl;
        m_file << "/Length " << compressed.size() << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&compressed[0], compressed.size());
        m_file << "endstream" << std::endl;
        endObject();
    }
}

// Writes the drawing's luminosity soft mask from the alpha shading, its /SMask graphics state, and the group
// drawing the colour shading clipped to contours. The cache condition uses the same || as the alpha shading.
void PDFWriter::writeSoftMask(plasma::Drawing *drawing)
{
    if (m_drawingGraphicsStates.find(drawing) == m_drawingGraphicsStates.end() || drawing->m_pathVertices.size() >= 3) {
        plasma::Vector<2, float> minimum = drawing->m_minimum;
        plasma::Vector<2, float> maximum = drawing->m_maximum;
        int group = beginObject();
        m_file << "<</CS/DeviceGray/I false/K false/S/Transparency/Type/Group>>" << std::endl;
        endObject();

        std::stringstream content;
        content << std::fixed;
        content << "q" << std::endl;
        content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
                << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
                << " re" << std::endl;
        content << "W n" << std::endl;
        content << "0 g" << std::endl;
        content << "/GSOpaque gs" << std::endl;
        content << "BX /Sh" << m_alphaShadings[drawing] << " sh EX" << std::endl;
        content << "Q" << std::endl;
        std::vector<char> compressed;
        compress(compressed, content);
        int form = beginObject();
        m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0]
               << " " << maximum.m_elements[1] << "]" << std::endl;
        m_file << "/Group " << group << " 0 R" << std::endl;
        m_file << "/Length " << compressed.size() << std::endl;
        m_file << "/Filter/FlateDecode" << std::endl;
        m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
        m_file << "/Subtype/Form" << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&compressed[0], compressed.size());
        m_file << "endstream" << std::endl;
        endObject();

        int mask = beginObject();
        m_file << "<</G " << form << " 0 R/S/Luminosity/Type/Mask>>" << std::endl;
        endObject();

        m_drawingGraphicsStates[drawing] = beginObject();
        m_file << "<</AIS false/BM/Normal/CA 1.0/OP false/OPM 1/SA true/SMask " << mask
               << " 0 R/Type/ExtGState/ca 1.0/op false>>" << std::endl;
        endObject();

        std::stringstream clipped;
        clipped << std::fixed;
        clipped << "q" << std::endl;
        if (drawing) {
            for (int c = 0; c < (int)drawing->m_contours.size(); c++) {
                for (int i = 0; i < (int)drawing->m_contours[c].size(); i++) {
                    const plasma::Vector<2, float> &vertex = drawing->m_pathVertices[drawing->m_contours[c][i]];
                    clipped << vertex.m_elements[0] << " " << vertex.m_elements[1] << (i == 0 ? " m " : " l ");
                }
            }
            clipped << "h W n" << std::endl;
            clipped << "BX /Sh" << m_colorShadings[drawing] << " sh EX" << std::endl;
        }
        clipped << "Q" << std::endl;
        compress(compressed, clipped);
        int object = beginObject();
        m_groupObjects.push_back(object);
        m_drawingGroups[drawing] = object;
        m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0]
               << " " << maximum.m_elements[1] << "]" << std::endl;
        m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
        m_file << "/Length " << compressed.size() << std::endl;
        m_file << "/Filter/FlateDecode" << std::endl;
        m_file << "/Matrix [1.0 0.0 0.0 1.0 0.0 0.0]" << std::endl;
        m_file << "/Subtype/Form" << std::endl;
        m_file << ">>" << std::endl;
        m_file << "stream" << std::endl;
        m_file.write(&compressed[0], compressed.size());
        m_file << "endstream" << std::endl;
        endObject();
    }
}

// The text as a PDF text object in its embedded TrueType font: one TJ array per line, glyph codes in hex with the
// font's kerning (in thousandths of the text size) between them; "1 Tr" strokes the outlines at twice the stroke
// radius. The lines are placed like the engine places them (TextShape flags: bit 0 centred, bit 1 right-aligned,
// bit 4 wrapped).
void PDFWriter::writeText(std::ostream &out, plasma::TextShape *text, bool stroke)
{
    int font = writeFont(text->m_font);
    out << "1 0 0 -1 0 0 cm" << std::endl;
    out << "BT" << std::endl;
    out << "/F" << font << " " << text->m_pixelSize << " Tf" << std::endl;
    if (stroke) {
        out << "1 Tr" << std::endl;
        out << text->m_strokeRadius + text->m_strokeRadius << " w" << std::endl;
        out << "1 j" << std::endl;
    } else {
        out << "0 Tr" << std::endl;
    }
    out << text->m_spacing << " Tc" << std::endl;
    plasma::Matrix<float> identity;
    identity.identity();
    text->getEngine()->getFontEngine()->lock();
    text->m_font->prepare(identity, text->m_pixelSize, text->m_strokeRadius, text->m_spacing, text->m_lineSpacing,
                          false);
    std::wstring string(text->m_string.value());
    if (text->m_flags & (1 << plasma::TEXT_WRAP))
        text->m_font->wrapText(string, text->m_wrapWidth);

    // The text's bounds unaligned, aligned, and the first line's.
    plasma::Vector<2, float> lineMinimum;
    plasma::Vector<2, float> maximum;
    plasma::Vector<2, float> start;
    plasma::Vector<2, float> lineMaximum;
    text->m_font->getBounds(string, lineMinimum, maximum, 0, -1, false);
    text->m_font->getBounds(string, start, maximum, text->m_flags, -1, false);
    start.m_elements[0] -= lineMinimum.m_elements[0];
    start.m_elements[1] -= lineMinimum.m_elements[1];
    text->m_font->getBounds(string, lineMinimum, lineMaximum, text->m_flags, 0, false);
    float x = start.m_elements[0];
    if (text->m_flags & (1 << plasma::TEXT_CENTER))
        x = start.m_elements[0] -
            (start.m_elements[0] + (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0])) *
                0.5;
    else if (text->m_flags & (1 << plasma::TEXT_RIGHT))
        x = start.m_elements[0] -
            (start.m_elements[0] + (lineMaximum.m_elements[0] - lineMinimum.m_elements[0] - maximum.m_elements[0]));
    out << x << " " << -start.m_elements[1] << " Td" << std::endl;
    int length = string.size();
    out << "[";
    for (int i = 0; i < length; i++) {
        if (string[i] == L'\n') {
            // A new line: move by the difference of the two lines' starts.
            float previousX = x;
            x = start.m_elements[0];
            text->m_font->getBounds(string, lineMinimum, lineMaximum, text->m_flags, i + 1, false);
            if (text->m_flags & (1 << plasma::TEXT_CENTER))
                x = start.m_elements[0] - (start.m_elements[0] + (lineMaximum.m_elements[0] -
                                                                  lineMinimum.m_elements[0] - maximum.m_elements[0])) *
                                              0.5;
            else if (text->m_flags & (1 << plasma::TEXT_RIGHT))
                x = start.m_elements[0] - (start.m_elements[0] + (lineMaximum.m_elements[0] -
                                                                  lineMinimum.m_elements[0] - maximum.m_elements[0]));
            out << "] TJ " << x - previousX << " " << -(text->m_pixelSize + text->m_lineSpacing) << " Td ["
                << std::endl;
        } else {
            out << "<" << std::setw(2) << std::setfill('0') << std::hex << (int)(unsigned char)string[i] << ">";
            if (i < length - 1) {
                FT_Vector kerning;
                plasma::ScalableFont *glyphFont = text->m_font;
                unsigned next = FT_Get_Char_Index(glyphFont->m_face, string[i + 1]);
                unsigned current = FT_Get_Char_Index(glyphFont->m_face, string[i]);
                FT_Get_Kerning(glyphFont->m_face, current, next, FT_KERNING_UNSCALED, &kerning);
                out << (float)(-kerning.x * 1000.0 / text->m_font->m_face->units_per_EM);
            }
        }
    }
    out << "] TJ" << std::endl;
    out << "ET" << std::endl;
    text->getEngine()->getFontEngine()->unlock();
}

// A text's fill (or stroke) as a group filling its bounds with the colour, drawn through a luminosity soft mask of
// the glyphs in the colour's alpha. The groups and states are keyed by the text shape (the stroke maps).
void PDFWriter::writeTextResources(plasma::TextShape *text, bool stroke)
{
    plasma::Vector<2, float> minimum = text->getMinimum();
    plasma::Vector<2, float> maximum = text->getMaximum();
    std::stringstream content;
    content << std::fixed;
    content << "q" << std::endl;
    if (m_options & kPDFOutputCMYK) {
        // The transform gives percentages.
        plasma::Vector<4, float> cmyk = toCMYK((stroke ? text->m_strokeColor : text->m_color).value());
        for (unsigned c = 0; c < 4; c++)
            cmyk.m_elements[c] *= 0.01f;
        content << "/CS0 cs" << std::endl;
        content << cmyk.m_elements[0] << " " << cmyk.m_elements[1] << " " << cmyk.m_elements[2] << " "
                << cmyk.m_elements[3] << " sc" << std::endl;
    } else if (stroke) {
        const plasma::Vector<4, float> &color = text->m_strokeColor.value();
        content << color.m_elements[0] << " " << color.m_elements[1] << " " << color.m_elements[2] << " rg"
                << std::endl;
    } else {
        const plasma::Vector<4, float> &color = text->m_color.value();
        content << color.m_elements[0] << " " << color.m_elements[1] << " " << color.m_elements[2] << " rg"
                << std::endl;
    }
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re f" << std::endl;
    content << "Q" << std::endl;
    std::vector<char> compressed;
    compress(compressed, content);
    int group = beginObject();
    m_groupObjects.push_back(group);
    if (stroke)
        m_textStrokeGroups[text] = group;
    else
        m_textFillGroups[text] = group;
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS " << m_colorSpace << "/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    // The mask: the glyphs in the colour's alpha.
    content.str("");
    content << "q" << std::endl;
    if (stroke)
        content << text->m_strokeColor.value().m_elements[3] << " G" << std::endl;
    else
        content << text->m_color.value().m_elements[3] << " g" << std::endl;
    content << minimum.m_elements[0] << " " << minimum.m_elements[1] << " "
            << maximum.m_elements[0] - minimum.m_elements[0] << " " << maximum.m_elements[1] - minimum.m_elements[1]
            << " re W n" << std::endl;
    writeText(content, text, stroke);
    content << "Q" << std::endl;
    compress(compressed, content);
    int form = beginObject();
    m_file << "<</BBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0] << " "
           << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Group <</CS/DeviceGray/I false/K false/S/Transparency>>" << std::endl;
    m_file << "/Length " << compressed.size() << std::endl;
    m_file << "/Filter/FlateDecode" << std::endl;
    m_file << "/Subtype/Form" << std::endl;
    m_file << ">>" << std::endl;
    m_file << "stream" << std::endl;
    m_file.write(&compressed[0], compressed.size());
    m_file << "endstream" << std::endl;
    endObject();

    int mask = beginObject();
    m_file << "<</G " << form << " 0 R/S/Luminosity/Type/Mask>>" << std::endl;
    endObject();

    int state = beginObject();
    m_graphicsStateObjects.push_back(state);
    if (stroke)
        m_textStrokeGraphicsStates[text] = state;
    else
        m_textFillGraphicsStates[text] = state;
    m_file << "<</AIS false/BM/Normal/CA 1.0/OP false/OPM 1/SA true/SMask " << mask
           << " 0 R/Type/ExtGState/ca 1.0/op false>>" << std::endl;
    endObject();
}

void PDFWriter::writeShadings(plasma::Node *node)
{
    if (!node->isShown() || !node->m_display->m_visibility.value())
        return;
    plasma::Shape *shape = node->m_shape;
    if (shape && shape->m_type == plasma::SHAPE_SMOOTH_MESH) {
        plasma::SmoothMeshShape *mesh = static_cast<plasma::SmoothMeshShape *>(shape);
        if (mesh->drawsExtrusion() && mesh->m_extrusionDrawing) {
            writeColorShading(mesh->m_extrusionDrawing);
            writeAlphaShading(mesh->m_extrusionDrawing);
            writeSoftMask(mesh->m_extrusionDrawing);
        }
        if (mesh->drawsFill() && mesh->m_fillDrawing) {
            writeColorShading(mesh->m_fillDrawing);
            writeAlphaShading(mesh->m_fillDrawing);
            writeSoftMask(mesh->m_fillDrawing);
        }
        if (mesh->drawsStroke() && mesh->m_strokeDrawing) {
            writeColorShading(mesh->m_strokeDrawing);
            writeAlphaShading(mesh->m_strokeDrawing);
            writeSoftMask(mesh->m_strokeDrawing);
        }
    }
    shape = node->m_shape;
    if (shape && shape->m_type == plasma::SHAPE_TEXT) {
        plasma::TextShape *text = static_cast<plasma::TextShape *>(shape);

        // Texts in mesh fonts are written with the meshes.
        if (text->m_font && !text->m_font->m_plasmaRoot) {
            writeTextResources(text, false);
            if (text->m_strokeRadius > 0.0f)
                writeTextResources(text, true);
        }
    }

    // The shadings take the first half of the progress bar.
    m_workspace->m_progressModule->advance(0.5 / m_nodeCount);
    pumpMessages();
    if (isRasterized(node))
        return;
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        writeShadings(*it);
}

bool PDFWriter::isRasterized(plasma::Node *node)
{
    // Display flag bit 0: the blur is enabled.
    return (m_options & kPDFRasterizeAll) ||
           ((m_options & kPDFRasterizeBlur) && (node->m_display->m_flags & (1 << plasma::DISPLAY_BLUR)) &&
            node->m_display->m_blurRadius.value() > 0.0f);
}

// Writes each font's TrueType font object, descriptor and advances for characters 0..255 in thousandths of the em.
// Embeds the font file when kPDFEmbedFonts is set. Returns the font object number.
int PDFWriter::writeFont(plasma::ScalableFont *font)
{
    if (m_fontObjects.find(font) != m_fontObjects.end())
        return m_fontObjects[font];
    int fontFile = 0;
    font->m_engine->getFontEngine()->lock();
    if (m_options & kPDFEmbedFonts) {
        const std::wstring &path = font->m_path;
        std::ifstream file(PLASMA_FILE_PATH(path), std::ios::binary);
        file.seekg(0, std::ios::end);
        const std::streampos length = file.tellg();
        file.seekg(0, std::ios::beg);
        fontFile = beginObject();
        m_file << "<</Length1 " << (std::streamoff)length << ">>" << std::endl;
        m_file << "stream" << std::endl;
        char c;
        while (file && file.get(c))
            m_file.put(c);
        m_file << "\nendstream" << std::endl;
        endObject();
    }
    plasma::Vector<2, float> minimum, maximum;
    minimum = font->m_minimum * 1000.0f;
    maximum = font->m_maximum * 1000.0f;
    int descriptor = beginObject();
    m_file << "<</Type/FontDescriptor" << std::endl;
    m_file << "/FontName/" << narrowString(font->m_postscriptName) << std::endl;
    if (m_options & kPDFEmbedFonts)
        m_file << "/FontFile2 " << fontFile << " 0 R" << std::endl;
    m_file << "/FontBBox[" << minimum.m_elements[0] << " " << minimum.m_elements[1] << " " << maximum.m_elements[0]
           << " " << maximum.m_elements[1] << "]" << std::endl;
    m_file << "/Flags " << PDF_FONT_SYMBOLIC << std::endl;
    m_file << ">>" << std::endl;
    endObject();
    unsigned glyphIndex;
    FT_Get_First_Char(font->m_face, &glyphIndex); // The result is unused.
    int widths = beginObject();
    m_file << "[";
    for (int c = 0; c < 256; c++) {
        FT_Load_Char(font->m_face, c, FT_LOAD_NO_SCALE);
        FT_GlyphSlot glyph = font->m_face->glyph;
        float width = (float)((double)glyph->metrics.horiAdvance / font->m_face->units_per_EM * 1000.0);
        m_file << " " << width;
    }
    m_file << " ]" << std::endl;
    endObject();
    int object = beginObject();
    m_fontObjects[font] = object;
    m_file << "<</Type/Font" << std::endl;
    m_file << "/Subtype/TrueType" << std::endl;
    m_file << "/BaseFont/" << narrowString(font->m_postscriptName) << std::endl;
    m_file << "/FontDescriptor " << descriptor << " 0 R" << std::endl;
    m_file << "/Widths " << widths << " 0 R" << std::endl;
    m_file << "/FirstChar " << 0UL << std::endl;
    m_file << "/LastChar " << 255UL << std::endl;
    m_file << ">>" << std::endl;
    endObject();
    font->m_engine->getFontEngine()->unlock();
    return object;
}

void PDFWriter::countFontMeshes(plasma::Node *node)
{
    if (!node->isShown() || !node->m_display->m_visibility.value())
        return;
    plasma::Shape *shape = node->m_shape;
    if (shape && shape->m_type == plasma::SHAPE_TEXT) {
        plasma::ScalableFont *font = static_cast<plasma::TextShape *>(shape)->m_font;
        if (font && font->m_plasmaRoot)
            m_fontCount++;
    }
    if (isRasterized(node))
        return;
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        countFontMeshes(*it);
}

// Texts in mesh fonts (".plasma" fonts, m_plasmaRoot) are written as nodes: the text converted to glyph nodes once
// per text shape (a hidden temporary node under the text's node), then instanced for each further node showing the
// same shape ("Creating font meshes": the progress bar advances per text, m_fontCount from CountFontMeshes).
void PDFWriter::createFontMeshes(plasma::Node *node)
{
    if (!node->isShown() || !node->m_display->m_visibility.value())
        return;
    plasma::Shape *shape = node->m_shape;
    plasma::TextShape *text =
        shape && shape->getType() == plasma::SHAPE_TEXT ? static_cast<plasma::TextShape *>(shape) : 0;
    if (text && text->m_font && text->m_font->m_plasmaRoot) {
        if (m_fontMeshes[text]) {
            plasma::Node *copy = m_fontMeshes[text]->instance(0);
            node->insertChild(node->m_children.begin(), copy);
            m_temporaryNodes.push_back(copy);
        } else {
            plasma::Node *meshes = m_workspace->m_engine.createNode(0, 0, 0, 0);
            node->insertChild(node->m_children.begin(), meshes);
            std::wstring string(text->m_string.value());
            text->getEngine()->getFontEngine()->lock();
            if (text->m_flags & (1 << plasma::TEXT_WRAP)) {
                plasma::Matrix<float> identity;
                identity.identity();
                text->m_font->prepare(identity, text->m_pixelSize, text->m_strokeRadius, text->m_spacing,
                                      text->m_lineSpacing, false);
                text->m_font->wrapText(string, text->m_wrapWidth);
            }
            text->m_font->convertTextToNodes(meshes, string, text->m_pixelSize, text->m_strokeRadius, text->m_spacing,
                                             text->m_lineSpacing, text->m_color.value().m_elements,
                                             text->m_strokeColor.value().m_elements,
                                             text->m_extrusionColor.value().m_elements, text->m_flags);
            text->getEngine()->getFontEngine()->unlock();
            meshes->animate(0);
            meshes->m_flags |= (1 << plasma::NODE_INTERNAL);
            m_temporaryNodes.push_back(meshes);
            m_fontMeshes[text] = meshes;
        }
        m_workspace->m_progressModule->advance((float)(1.0 / m_fontCount));
        pumpMessages();
    }
    if (isRasterized(node))
        return;
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        createFontMeshes(*it);
}

void PDFWriter::destroyTemporaryNodes()
{
    for (std::list<plasma::Node *>::iterator it = m_temporaryNodes.begin(); it != m_temporaryNodes.end(); ++it)
        m_workspace->m_engine.destroyNode(*it);
}

plasma::Vector<4, float> PDFWriter::toCMYK(const plasma::Vector<4, float> &rgb)
{
    plasma::Vector<4, float> cmyk;
    cmsDoTransform(m_rgbToCMYKFloat, &rgb, &cmyk, 1);
    return cmyk;
}

// Converts three-byte RGB input pixels to four-byte CMYK output pixels.
void PDFWriter::toCMYK(std::vector<unsigned char> &cmyk, const std::vector<unsigned char> &rgb)
{
    cmyk.resize(rgb.size() / 3 * 4);
    cmsDoTransform(m_rgbToCMYK8, &rgb[0], &cmyk[0], rgb.size() / 3);
}
