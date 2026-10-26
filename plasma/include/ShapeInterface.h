// ShapeInterface is the base of MeshInterface, TextInterface and GenericInterface, which provide editing
// operations for their shape types.
#pragma once
#include <list>
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

class NodeInterface;
class DesignSheet;
class BoundingBoxTool;

namespace plasma {
class Texture;
}

// ShapeInterface::m_type: the interface's kind (the constructors' second argument; the tools test it before
// casting to MeshInterface or TextInterface).
enum ShapeInterfaceType {
    INTERFACE_MESH = 1,    // MeshInterface.
    INTERFACE_TEXT = 2,    // TextInterface.
    INTERFACE_GENERIC = 3, // GenericInterface.
};

// The ColorTarget selects the colours used by painting, picking, shifting and brightness, contrast or saturation
// adjustments: mesh fill, stroke, or front and back extrusion colours; text fill, stroke or extrusion colour.
enum ColorTarget {
    COLOR_TARGET_FILL = 0,
    COLOR_TARGET_STROKE = 1,
    COLOR_TARGET_EXTRUSION = 2,
};

// Default implementations shared by the mesh, text and generic shape interfaces.
class ShapeInterface {
public:
    ShapeInterface(NodeInterface *node, ShapeInterfaceType type);
    virtual ~ShapeInterface();
    // Rebuilds the editable mesh from the shape (DesignSheet for the other nodes showing a changed shape).
    virtual void rebuild() {}
    // Before a change: a deformer's mesh binds its children's meshes, an outline-aligned node saves its children's
    // states (MeshInterface).
    virtual void prepareChildren() {}
    virtual void refresh() {}
    // After a change: the bound children follow the mesh, the outline-aligned ones are aligned again (MeshInterface).
    virtual void updateChildren() {}
    virtual void draw() {}
    virtual bool selectAllVertices(bool selected) { return false; }
    virtual bool selectAllEdges(bool selected) { return false; }
    virtual bool selectAllFaces(bool selected) { return false; }
    // Empty in every interface and never called.
    virtual void unused9() {}
    // Adds the offsets from the origin of the points the pointer snaps to (Workspace: vertices, or the text's bounds
    // corners).
    virtual void getSnapOffsets(const plasma::Vector<2, float> &origin, std::list<plasma::Vector<2, float>> &offsets) {}

    // The colour tool: paints the colour module's colour into colour 'colorTarget' (a ColorTarget) under the mouse;
    // Alt picks it instead.
    virtual void paintColor(int colorTarget, bool update) {}

    // The colour under the mouse and where it is on the sheet (the colour tool's pick).
    virtual bool pickColor(plasma::Vector<4, float> &color, plasma::Vector<2, float> &position) { return false; }
    // Adds colour 'colorTarget' if the shape is under the mouse (TextInterface).
    virtual void getColorsUnderMouse(std::list<plasma::Vector<4, float>> &colors, int colorTarget) {}

    // The stroke width tool: the width under the mouse and its position, painting the module's width, scaling the
    // widths into the range, and the range of the widths under the mouse.
    virtual bool pickStrokeWidth(float &width, plasma::Vector<2, float> &position) { return false; }
    virtual void paintStrokeWidth() {}
    virtual void scaleStrokeWidths(float minimum, float maximum) {}
    virtual bool getStrokeWidthRange(float &minimum, float &maximum) { return false; }
    virtual void selectVerticesInBox(BoundingBoxTool *tool, bool select) {}
    virtual void selectVertices() {}
    // Selects the vertices whose extruded position is under the mouse (MeshInterface).
    virtual void selectExtrudedVertices() {}
    virtual void selectEdgesInBox(BoundingBoxTool *tool, bool select) {}
    virtual void selectEdges() {}
    virtual void selectFacesInBox(BoundingBoxTool *tool, bool select) {}
    virtual void selectFaces() {}
    virtual void applyEdgeSelection() {}
    virtual void applyFaceSelection() {}
    // Shifts the colours matching one of the source colours towards the target (ColorModule's colour shift).
    virtual void shiftColors(const std::list<plasma::Vector<4, float>> &source, const plasma::Vector<4, float> &target,
                             float tolerance, int colorTarget, bool update)
    {}

    // The special colour module's brightness, contrast and saturation of colour 'colorTarget' (a ColorTarget); the
    // factor 1 keeps it.
    virtual void adjustBrightness(float factor, int colorTarget, bool update) {}
    virtual void adjustContrast(float factor, int colorTarget, bool update) {}
    virtual void adjustSaturation(float factor, int colorTarget, bool update) {}
    // Moves the extrusion by the workspace's drag offset (MeshInterface; never called).
    virtual void moveExtrusion() {}

    // The vertex, edge and face tools' drags of the selection.
    virtual void moveSelectedVertices(const plasma::Vector<2, float> &drag) {}
    virtual void scaleSelectedVertices(const plasma::Vector<2, float> &pivot, const plasma::Vector<2, float> &scale) {}
    virtual void rotateSelectedVertices(const plasma::Vector<2, float> &pivot, const plasma::Vector<3, float> &rotation)
    {}
    virtual void smoothVertices() {}
    virtual void sharpenVertices() {}
    virtual void bezierVertices() {}
    virtual void roundCornerVertices() {}
    // Clears or sets bit 1 of each vertex's type as "smooth vertices" or "sharpen vertices" (MeshInterface). Currently
    // unused.
    virtual void smoothAllVertices() {}
    virtual void sharpenAllVertices() {}
    virtual void subdivideMesh() {}
    virtual void subdivideContour() {}
    virtual int countSelectedVertices() { return 0; }
    virtual int countSelectedEdges() { return 0; }
    virtual int countSelectedFaces() { return 0; }
    virtual bool deleteVertices() { return false; }
    virtual bool deleteEdges() { return false; }
    virtual bool deleteFaces() { return false; }
    virtual void setSubdivisions(int subdivisions) {}
    virtual int getSubdivisions() { return -1; }

    // Setter and getter pairs shaped like Set/GetSubdivisions: no interface overrides them and nothing calls them.
    virtual void setUnusedValue1(int value) {}
    virtual int getUnusedValue1() { return -1; }
    virtual void setUnusedValue2(int value) {}
    virtual int getUnusedValue2() { return -1; }
    virtual void setUnusedValue3(int value) {}
    virtual int getUnusedValue3() { return -1; }
    virtual bool setVertexPositionsKey() { return false; }
    virtual bool setVertexColorsKey() { return false; }
    virtual bool setStrokeColorsKey() { return false; }
    virtual bool setStrokeWidthsKey() { return false; }
    virtual bool setVertexTextureCoordsKey() { return false; }
    virtual bool setTextKey() { return false; } // "set text key" (TextInterface).

    // The shape's bounds on the sheet, and those of its selected vertices (the defaults are empty boxes).
    virtual void getBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum);
    virtual void getSelectionBounds(plasma::Vector<2, float> *minimum, plasma::Vector<2, float> *maximum);
    virtual void beginModify() {}
    virtual void endModify() {}
    // Records the colours being previewed as one "modify mesh" ("modify text") operation (MeshInterface,
    // TextInterface).
    virtual void recordColorEdit() {}
    // Forgets the state BeginModify kept (DesignSheet::ClearHistory).
    virtual void discardModifyState() {}
    virtual void storePositions() {}
    virtual void restorePositions() {}

    // The special colour module's preview: with true the colours' element 1 becomes a working copy and current, with
    // false element 0 is current again (m_colorPreview records it); Apply copies the working colours back.
    virtual void setColorPreview(bool preview) {}
    virtual void applyColorPreview() {}
    virtual void setTexture(int texture) {}
    virtual plasma::Texture *getTexture() { return 0; }
    virtual double getTextureOpacity() { return 1.0; }
    // Maps the texture over the mesh's vertex bounds (MeshInterface).
    virtual void fitTextureToMesh() {}
    // Moves the point to the nearest vertex within the tool radius (MeshInterface; never called).
    virtual bool snapToVertex(plasma::Vector<2, float> &point) { return false; }
    virtual bool isInsideBox(BoundingBoxTool *tool) { return false; } // NodeTool passes itself.

    // "node:align:outline": spreads the children along the shape's contours (MeshInterface).
    virtual void alignChildrenToOutline() {}
    // Adds the colours the shape uses (TextInterface).
    virtual void getColors(std::list<plasma::Vector<4, float>> &colors) {}
    // "Flatten Transparency" (ColorTool): blends the translucent colours with what is drawn under them.
    virtual void flattenTransparency() {}
    // Refreshes the interface, or without 'refresh' clears its wire frame and forgets the modify state (MeshInterface;
    // never called).
    virtual void refreshOrRelease(bool refresh) {}

    // Temporarily uses the texture sheet's matrix while a tool edits the shape.
    void setMatrix(const plasma::Matrix<float> &matrix);
    void resetMatrix();

    // The node's sheet node, used by TextTool::PointerPressed to check and change selection.
    NodeInterface *getSheetNode() const { return m_node; }

    DesignSheet *m_sheet; // The node's sheet.
    NodeInterface *m_node;
    ShapeInterfaceType m_type;
    // Set by every edit and selection change; DesignSheet then ends the modification (EndModify) and Tool updates the
    // shape.
    bool m_hasChanges;
    // Set with m_hasChanges by the edits; Workspace::Update refreshes the interface (and the shape, with realtime
    // update) and clears it.
    bool m_refreshPending;
    bool m_hasMatrix; // m_matrix replaces the node's.
    plasma::Matrix<float> m_matrix;
    bool m_colorPreview; // SetColorPreview(true) is in effect.
};

// Shifts a colour towards the target according to the source colour nearest to it in hue.
plasma::Vector<4, float> shiftColor(const std::list<plasma::Vector<4, float>> &source,
                                    const plasma::Vector<4, float> &target, float tolerance,
                                    const plasma::Vector<4, float> &color);
