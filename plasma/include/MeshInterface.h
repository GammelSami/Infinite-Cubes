// MeshInterface edits a smooth mesh shape. A NodeInterface whose shape is a SmoothMeshShape owns one. It keeps an
// EditableMesh over the shape's attributes, the wireframe it draws, and the state recorded by "modify mesh"
// operations.
#pragma once
#include <vector>
#include "EditableMesh.h"
#include "WireFrame.h"
#include "Module.h"
#include "Operation.h"
#include "Tool.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"
#include "ShapeInterface.h"
#include "BoundingBoxTool.h"
#include "MeshOperation.h"

class NodeInterface;
class DesignSheet;

namespace plasma {
class Drawing;
class Node;
class SmoothMeshShape;
class Texture;
}

// A vertex bound to a face of another mesh's finest subdivision level: its position is the weighted sum of the
// face's four corners (FollowShape).
struct MeshBinding {
    enum {
        CORNER_COUNT = 4
    };
    float m_cornerWeights[CORNER_COUNT];
    int m_sourceFace;   // In the source's subdivision level.
    int m_targetVertex; // This mesh's vertex index.
};

class MeshInterface : public ShapeInterface {
public:
    MeshInterface(NodeInterface *node, plasma::SmoothMeshShape *shape);
    virtual ~MeshInterface();
    virtual void rebuild();
    virtual void prepareChildren();
    virtual void refresh();
    virtual void updateChildren();
    virtual void draw();
    virtual bool selectAllVertices(bool);
    virtual bool selectAllEdges(bool);
    virtual bool selectAllFaces(bool);
    virtual void getSnapOffsets(const plasma::Vector<2, float> &origin, std::list<plasma::Vector<2, float>> &offsets);
    virtual void paintColor(int colorTarget, bool update);
    virtual bool pickColor(plasma::Vector<4, float> &color, plasma::Vector<2, float> &position);
    virtual void getColorsUnderMouse(std::list<plasma::Vector<4, float>> &colors, int colorTarget);
    virtual bool pickStrokeWidth(float &width, plasma::Vector<2, float> &position);
    virtual void paintStrokeWidth();
    virtual void scaleStrokeWidths(float, float);
    virtual bool getStrokeWidthRange(float &minimum, float &maximum);
    virtual void selectVerticesInBox(BoundingBoxTool *, bool);
    virtual void selectVertices();
    virtual void selectExtrudedVertices();
    virtual void selectEdgesInBox(BoundingBoxTool *, bool);
    virtual void selectEdges();
    virtual void selectFacesInBox(BoundingBoxTool *, bool);
    virtual void selectFaces();
    virtual void applyEdgeSelection();
    virtual void applyFaceSelection();
    virtual void shiftColors(const std::list<plasma::Vector<4, float>> &source, const plasma::Vector<4, float> &target,
                             float tolerance, int colorTarget, bool update);
    virtual void adjustBrightness(float, int, bool);
    virtual void adjustContrast(float, int, bool);
    virtual void adjustSaturation(float, int, bool);
    virtual void moveExtrusion();
    virtual void moveSelectedVertices(const plasma::Vector<2, float> &);
    virtual void scaleSelectedVertices(const plasma::Vector<2, float> &pivot, const plasma::Vector<2, float> &scale);
    virtual void rotateSelectedVertices(const plasma::Vector<2, float> &pivot,
                                        const plasma::Vector<3, float> &rotation);
    virtual void smoothVertices();
    virtual void sharpenVertices();
    virtual void bezierVertices();
    virtual void roundCornerVertices();
    virtual void smoothAllVertices();
    virtual void sharpenAllVertices();
    virtual void subdivideMesh();
    virtual void subdivideContour();
    virtual int countSelectedVertices();
    virtual int countSelectedEdges();
    virtual int countSelectedFaces();
    virtual bool deleteVertices();
    virtual bool deleteEdges();
    virtual bool deleteFaces();
    virtual void setSubdivisions(int);
    virtual int getSubdivisions();
    virtual bool setVertexPositionsKey();
    virtual bool setVertexColorsKey();
    virtual bool setStrokeColorsKey();
    virtual bool setStrokeWidthsKey();
    virtual bool setVertexTextureCoordsKey();
    virtual void getBounds(plasma::Vector<2, float> *, plasma::Vector<2, float> *);
    virtual void getSelectionBounds(plasma::Vector<2, float> *, plasma::Vector<2, float> *);
    virtual void beginModify();
    virtual void endModify();
    virtual void recordColorEdit();
    virtual void discardModifyState();
    virtual void storePositions();
    virtual void restorePositions();
    virtual void setColorPreview(bool);
    virtual void applyColorPreview();
    virtual void setTexture(int);
    virtual plasma::Texture *getTexture();
    virtual double getTextureOpacity();
    virtual void fitTextureToMesh();
    virtual bool snapToVertex(plasma::Vector<2, float> &point);
    virtual bool isInsideBox(BoundingBoxTool *tool);
    virtual void alignChildrenToOutline();
    virtual void getColors(std::list<plasma::Vector<4, float>> &colors);
    virtual void flattenTransparency();
    virtual void refreshOrRelease(bool);
    virtual void fitTextureToBox(const plasma::Vector<2, float> &minimum, const plasma::Vector<2, float> &maximum);

    // The vertex's index in the shape's arrays.
    int &vertexIndex(MeshVertex *vertex) { return m_mesh.vertexIndex(vertex); }
    void setVertexSelected(MeshVertex *vertex, bool selected);
    void setEdgeSelected(MeshHalfEdge *edge, bool selected);
    void setFaceSelected(MeshFace *face, bool selected);
    bool isVertexSelected(MeshVertex *vertex);

    // An edge counts as selected when either half is.
    bool isEdgeSelected(MeshHalfEdge *edge);
    bool isFaceSelected(MeshFace *face);
    MeshVertex *addVertex(const plasma::Vector<2, float> &position, const plasma::Vector<2, float> &textureCoordinate,
                          const plasma::Vector<4, float> &vertexColor, const plasma::Vector<4, float> &strokeColor,
                          const plasma::Vector<4, float> &extrusionFrontColor,
                          const plasma::Vector<4, float> &extrusionBackColor, float width);
    MeshFace *addFace(const std::list<MeshVertex *> &vertices);
    void removeFace(MeshFace *face);
    plasma::Vector<2, float> getVertexPosition(MeshVertex *vertex);
    void getMatrix(plasma::Matrix<float> &matrix);
    // The edges on the sheet as point pairs.
    void getEdgeSegments(std::vector<plasma::Vector<2, float>> &segments, bool unselectedOnly);
    void storeTextureTransform();
    void restoreTextureTransform();

    // Copies the shape's editable state (and the face being drawn) into the state.
    void captureState(MeshState &state);
    // Puts a captured state back.
    void restoreState(const MeshState &state);
    void addBorder();
    // True when the mouse is inside one of the face's fan triangles or within the pick radius of one of its edges.
    bool isFaceUnderMouse(MeshFace *face);
    std::vector<bool> getVertexSelection();
    std::vector<bool> getEdgeSelection();
    std::vector<bool> getFaceSelection();
    plasma::Vector<2, float> deform(const plasma::Vector<2, float> &point);
    plasma::Vector<2, float> undeform(const plasma::Vector<2, float> &point);
    void storeExtrusionMatrix();
    void restoreExtrusionMatrix();
    // Binds each vertex to the face of the source's finest subdivision level that contains it (m_bindings).
    void bindToShape(plasma::SmoothMeshShape *source, const plasma::Matrix<float> &matrix);
    void followShape(plasma::SmoothMeshShape *source, const plasma::Matrix<float> &matrix);
    // True when every path vertex of the drawing, deformed and transformed by the node, lies in the tool's box.
    static bool isDrawingInBox(plasma::Drawing *drawing, BoundingBoxTool *tool, plasma::Node *node);
    // The topmost node drawn before 'self' whose smooth mesh shape is under the point (on the sheet), searching the
    // subtree of 'node'; reachedSelf is set on reaching 'self'.
    static plasma::Node *findNodeUnder(const plasma::Vector<2, float> &point, plasma::Node *node, plasma::Node *self,
                                       bool &reachedSelf);
    // The colour the drawing's triangles interpolate at the point; false when no triangle contains it.
    static bool getDrawingColor(plasma::Drawing *drawing, const plasma::Vector<2, float> &point,
                                plasma::Vector<4, float> &color);

    // Colours the wireframe's points and collects the hovered edges or faces for highlighting (Render).
    void updateWireFrameColors();
    // Edit menu "Create Inner Edges" (Workspace) for a selected mesh.
    void createInnerEdges();
    // VertexTool's "Apply Deformation to Vertices" and "Apply Rotation and Deformation to Vertices".
    void applyDeformation();
    void applyRotationAndDeformation();
    // Appends the source mesh's vertices (with their attributes and animation keys) and faces to this mesh
    // (Workspace::MergeMeshes, DrawFaceTool); reverse flips the faces' winding, update compacts the shape afterwards,
    // and *vertex, a source vertex, becomes its copy.
    void mergeMesh(EditableMesh *source, MeshVertex **vertex, bool reverse, bool update);

    // Constructor: bindings, wireframe, editable mesh, brush hit and saved state.
    std::vector<MeshBinding> m_bindings; // The vertices bound to another mesh (FollowShape).
    WireFrame m_wireFrame;
    EditableMesh m_mesh;
    bool m_brushHit;   // A color/width brush hit a vertex (including Alt picks); never read.
    MeshState m_state; // The state before the current modification.

    // Transform snapshots saved by StoreTextureTransform/StoreExtrusionMatrix.
    plasma::Vector<2, float> m_textureTranslation;
    plasma::Vector<2, float> m_texturePivot;
    plasma::Vector<3, float> m_textureRotation;
    plasma::Matrix<float> m_textureDeformation;
    plasma::Matrix<float> m_extrusionMatrix;
    std::vector<float> m_softSelectionWeights; // Indexed by the vertex index attribute.
};

// Also used by the mesh and curve shapes.
bool isPointInTriangle(const plasma::Vector<2, float> *corners, const plasma::Vector<2, float> &point);

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
