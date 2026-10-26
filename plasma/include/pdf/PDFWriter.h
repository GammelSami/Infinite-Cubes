// PDFWriter exports a plasma scene as PDF 1.5. ExportPDFModule constructs it with the workspace, path and options,
// calls Write and lets its destructor close the file. BeginObject numbers objects from 1 and keeps their file
// offsets for the cross-reference table. Resource maps cache object numbers by scene object.
#pragma once
#include <fstream>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include "../../vendor/lcms2-2.0/include/lcms2.h"
#include "plasma/Engine.h"
#include "plasma/Vector.h"

class Workspace;
class Image;

namespace plasma {
class Drawing;
class Node;
class ScalableFont;
class Shape;
class TextShape;
class Transformation;
}

// Export options (PDFWriter::m_options), set by ExportPDFModule from its check boxes ("export:pdf:rasterize:blur",
// ":textures", ":all", "export:pdf:embedfonts", "export:pdf:multipage") and the colour management module's state.
enum PDFExportOption {
    kPDFRasterizeBlur = 0x01, // Blurred nodes become images.
    kPDFRasterizeTextures = 0x02,
    kPDFRasterizeAll = 0x08, // Every node becomes an image.
    kPDFEmbedFonts = 0x10,
    kPDFOutputCMYK = 0x20, // The colour management module has an output profile.
    kPDFMultipage = 0x40   // One page per child of the root.
};

// Flate-compresses a content stream (zlib's compress) into out.
void compress(std::vector<char> &out, const std::stringstream &content);

class PDFWriter {
public:
    PDFWriter(Workspace *workspace, const std::wstring &path, int options);

    bool write(plasma::Node *root, const plasma::Engine::Page *page);
    // Renders the rasterized nodes' and textured drawings' images up front (ExportModule::RenderImage).
    void prepareRasterizedNodes(plasma::Node *node);

    // Starts a new indirect object ("<n> 0 obj") and returns its number.
    int beginObject();
    void endObject();

    // The graphics state ("/GS<n> gs") for a text shape (its fill, or its stroke when stroke is set), a mesh drawing
    // or a node and drawing pair.
    void writeGraphicsState(plasma::Node *node, std::ostream &out, const plasma::Drawing *drawing, plasma::Shape *shape,
                            bool stroke);
    // Draws the matching group ("q /Gr<n> Do Q"); nothing for a drawing with fewer than three path vertices.
    void writeGroupReference(plasma::Node *node, std::ostream &out, const plasma::Drawing *drawing,
                             plasma::Shape *shape, bool stroke);
    // The group of a node with a blend effect: DISPLAY_MODULATION -> Multiply and ColorDodge, DISPLAY_SCREEN ->
    // Difference, else plain.
    void writeBlendedGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node, plasma::Shape *shape,
                           bool stroke);
    void writeGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node, plasma::Shape *shape,
                    bool stroke);
    void writeMultiplyColorDodgeGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node,
                                      plasma::Shape *shape, bool stroke);
    // A Difference ExtGState and an ExtGState with the corresponding luminosity soft mask over the bounds; returns the
    // latter.
    int writeDifferenceState(plasma::Node *node, const plasma::Vector<2, float> &minimum,
                             const plasma::Vector<2, float> &maximum);
    void writeDifferenceGroup(std::ostream &out, const plasma::Drawing *drawing, plasma::Node *node,
                              plasma::Shape *shape, bool stroke);

    // The node form and a textured drawing rendered as an image.
    void writeNodeGroup(plasma::Node *node);
    void writeDrawingImage(plasma::Node *node, const plasma::Drawing *drawing);

    // Rasterized nodes are written as images, with their subtrees.
    bool isRasterized(plasma::Node *node);

    // Counts the shown texts of the subtree whose fonts are drawn as meshes (m_fontCount, "Creating font meshes").
    void countFontMeshes(plasma::Node *node);

    // Writes the shadings and soft masks of the shown meshes and texts of the subtree, advancing the progress bar per
    // node.
    void writeShadings(plasma::Node *node);
    // The free-form (type 4) shadings of a mesh drawing: colour, alpha, and the soft mask group that combines them.
    void writeColorShading(plasma::Drawing *drawing);
    void writeAlphaShading(plasma::Drawing *drawing);
    void writeSoftMask(plasma::Drawing *drawing);
    // A text's fill or stroke resources.
    void writeTextResources(plasma::TextShape *text, bool stroke);
    // The text's glyphs as a text object ("BT... TJ"), outlined for the stroke.
    void writeText(std::ostream &out, plasma::TextShape *text, bool stroke);
    // The font's TrueType font, descriptor and widths objects, once per font.
    int writeFont(plasma::ScalableFont *font);
    // Converts the shown texts in mesh fonts to temporary glyph nodes ("Creating font meshes").
    void createFontMeshes(plasma::Node *node);

    // Lets the engine delete the nodes the writer created.
    void destroyTemporaryNodes();

    // Converts one RGB colour, or a buffer of 8-bit RGB pixels, to CMYK with the transforms Write creates from the
    // output profile.
    plasma::Vector<4, float> toCMYK(const plasma::Vector<4, float> &rgb);
    void toCMYK(std::vector<unsigned char> &cmyk, const std::vector<unsigned char> &rgb);

    Workspace *m_workspace;
    int m_unused04; // Uninitialized and unused.
    std::ofstream m_file;
    std::vector<std::streampos> m_objectOffsets; // File offset of object n at [n - 1].
    plasma::Engine::Page m_page;                 // The sheet's page (Write copies it).
    plasma::Node *m_currentRoot;
    std::map<const plasma::Drawing *, int> m_colorShadings;  // "/Sh<n>": a mesh drawing's colours.
    std::map<const plasma::Drawing *, int> m_alphaShadings;  // "/Sh<n>": its alpha (DeviceGray).
    std::map<plasma::Shape *, int> m_textFillGroups;         // "/Gr<n> Do".
    std::map<plasma::Shape *, int> m_textFillGraphicsStates; // "/GS<n> gs".
    std::map<plasma::Shape *, int> m_textStrokeGroups;
    std::map<plasma::Shape *, int> m_textStrokeGraphicsStates;
    std::map<std::pair<plasma::Node *, const plasma::Drawing *>, int> m_drawingImageGroups;
    std::map<std::pair<plasma::Node *, const plasma::Drawing *>, int> m_drawingImageGraphicsStates;
    std::map<const plasma::Drawing *, int> m_drawingGroups;
    std::map<const plasma::Drawing *, int> m_drawingGraphicsStates;
    std::list<int> m_graphicsStateObjects;       // ExtGState objects.
    std::map<plasma::Node *, int> m_nodeForms;   // Each written node's form XObject ("/Gr<n> Do").
    std::map<plasma::Node *, int> m_childGroups; // "/Gr<n> Do": a clipping node's children as one isolated group.
    std::list<int> m_groupObjects;               // Form XObjects (groups).
    std::map<plasma::ScalableFont *, int> m_fontObjects; // "/F<n>": the font object per font.
    std::list<plasma::Node *> m_temporaryNodes;
    std::map<plasma::TextShape *, plasma::Node *> m_fontMeshes; // The glyph nodes made per mesh-font text.
    int m_nodeCount;                                            // Shown nodes, for the progress bar.
    int m_fontCount;                                            // Texts drawn with font meshes.
    std::wstring m_path;
    std::map<plasma::Node *, Image *> m_rasterizedNodes; // Rasterized subtrees.

    // The textured drawings' images (PrepareRasterizedNodes renders them, WriteDrawingImage places them).
    std::map<std::pair<plasma::Node *, const plasma::Drawing *>, Image *> m_rasterizedDrawings;
    int m_options;                  // PDFExportOption bits.
    std::string m_colorSpace;       // "/DeviceRGB" or "[/ICCBased <n> 0 R]".
    cmsHTRANSFORM m_rgbToCMYKFloat; // TYPE_RGB_FLT -> TYPE_CMYK_FLT.
    cmsHTRANSFORM m_rgbToCMYK8;     // TYPE_RGB_8 -> TYPE_CMYK_8.
};

// Accesses the transformation's local matrix.
const plasma::Matrix<float> &getTransformationMatrix(const plasma::Transformation *transformation);
