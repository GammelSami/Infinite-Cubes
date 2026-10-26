// ExportModule.cpp: the "module:export" toolbar. It exports the page, the drawing, the selected area or the
// selected objects ("export:area") as an image at a resolution given in pixels or DPI, with 1x, 4x, 9x or 16x
// antialiasing, and prints.
#pragma once
#include "ProceduralShapeModule.h" // Shared module helpers.
#include "DesignSheet.h"
#include "plasma/Transformation.h"

// FreeImage's FREE_IMAGE_FORMAT values RenderImageToFile writes.
enum ImageFileFormat {
    kFormatUnknown = -1,
    kFormatBMP = 0,
    kFormatICO = 1,
    kFormatJPEG = 2,
    kFormatPNG = 13,
    kFormatPPM = 14,
    kFormatTarga = 17,
    kFormatTIFF = 18,
    kFormatXPM = 23,
    kFormatGIF = 25
};

class Image;
namespace plasma {
class Drawing;
class GLRenderSurface;
}

class ExportModule : public Module {
public:
    ExportModule();

    virtual void build();
    virtual void sheetChanged();

    // "export:area": the area's list entry was chosen.
    void chooseArea(plasma::Widget *sender);
    // The exported area in page units (zero without an open sheet; an empty area gives zeros). Also used by the
    // renderer.
    void getExportBounds(plasma::Vector<2, float> &min, plasma::Vector<2, float> &max);
    plasma::Vector<2, float> getExportSize();

    // Shows the export size in pixels and the DPI.
    void updateWidgets();

    // The width and height in pixels set the page's DPI.
    void editWidth();
    void dragWidth(plasma::Widget *sender);
    void stepWidth(plasma::Widget *sender);
    void editHeight();
    void dragHeight(plasma::Widget *sender);
    void stepHeight(plasma::Widget *sender);
    void editDPI();
    void dragDPI(plasma::Widget *sender);
    void stepDPI(plasma::Widget *sender);
    void choosePixelPreview();

    // "export:export": asks for the file name and renders the area into it.
    void exportFile();

    // "export:print": asks first above 10000 pixels a side; not in the demo.
    void print();
    // Renders the area between the bounds to an image file ("Rendering "; callers Export and::Print).
    bool renderImageToFile(const std::wstring &path, const plasma::Vector<2, float> &boundsMin,
                           const plasma::Vector<2, float> &boundsMax);
    // Renders the node (or only the mask drawing) between the bounds into a new downsampled image, 0 for an empty area
    // (the PDF writer's rasterized nodes).
    Image *renderImage(plasma::Node *node, const plasma::Vector<2, float> &minimum,
                       const plasma::Vector<2, float> &maximum, plasma::Drawing *mask);
    // Renders the node or mask drawing tile by tile into the image for PDF rasterization.
    void renderTiles(plasma::Node *node, Image *image, const plasma::Matrix<float> &matrix,
                     plasma::GLRenderSurface *surface, int tilesX, int tilesY, float scale,
                     const plasma::Vector<2, float> &origin, plasma::Drawing *mask);
    // "export:nodes": instances of the selected nodes under parent.
    void instanceSelectedNodes(plasma::Node *parent, plasma::Node *node);

    // "export:aa:1".."export:aa:4": 1x ("-"), 4x, 9x, 16x.
    void chooseAntialiasing1();
    void chooseAntialiasing4();
    void chooseAntialiasing9();
    void chooseAntialiasing16();

    int m_unused44;                       // The constructor skips it; no access.
    int m_antialiasing;                   // 1..4: the "export:aa:N" button (samples N*N).
    bool m_pixelPreview;                  // "export:pixelpreview".
    plasma::Vector<2, float> m_boundsMin; // The area last shown by UpdateWidgets.
    plasma::Vector<2, float> m_boundsMax;

private:
    // The page settings changed: the root transformation's flag makes the view render the page again
    // (Keyable::m_changed).
    void pageChanged() { m_workspace->getOpenDesignSheet()->m_root->m_transformation->m_changed = true; }
};

// Reduces an antialiased rendering by factor in each direction.
Image *__stdcall DownsampleImage(Image *image, bool alpha, int factor);

// Converts a byte string to std::wstring, character by character.
std::wstring widenString(const std::string &text);
