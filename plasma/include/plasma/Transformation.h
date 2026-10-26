// plasma::Transformation: a keyable 2D transformation (translation, deformation matrix, rotation, pivot) that the
// engine keeps in a list.
#pragma once
#include <list>
#include <set>
#include "plasma/Keyable.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Vector.h"

class MeshInterface;

namespace svg {
class SVGReader;
}
class Workspace;
class ZoomModule;
class NodeModule;
class Module;
class ToolModule;
class NodeInterface;
class DesignSheet;
class Sheet;
class NodeTool;
class StatusModule;
class TextModule;
class ExportModule;
class InterfaceViewWidget;
class Tool;
class ProceduralTool;
class TextureSheet;

class ClipboardModule;
#ifdef PLASMA_PORTABLE
namespace prism {
class Application;
}
#endif

namespace plasma {

class Node;

class Transformation : public Keyable {
    friend class ::ClipboardModule;
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    const Matrix<float> &getMatrix() const { return m_matrix; }
    const Matrix<float> &getInverseMatrix() const { return m_inverseMatrix; }
#ifdef PLASMA_PORTABLE
    friend class ::prism::Application; // Hosts shared panels and their zoom attributes.
#endif
    Transformation(Engine *engine, const std::wstring &name);
    virtual ~Transformation();

    // A new transformation of the same engine and name with copies of the attributes.
    virtual Transformation *clone();

    // Rebuilds m_matrix and its inverse m_inverseMatrix from the attributes.
    virtual void attributesChanged(int reason);

    int countNodes() const { return m_nodes.size(); }

    // Moves the pivot to the point without moving the node (the translation compensates).
    void setPivot(const Vector<2, float> &pivot);

    // NodeTool's rotations and flips. Rotates the node by the angles about the pivot (in the parent's coordinates);
    // matrix and inverseMatrix are the node's.
    void rotate(const Vector<3, float> &rotation, const Matrix<float> &matrix, const Vector<2, float> &pivot,
                const Matrix<float> &inverseMatrix);

    // Scales the node about the centre (in sheet coordinates); the matrices are the parent's.
    void scale(const Vector<2, float> &center, const Vector<2, float> &scale, const Matrix<float> &parentMatrix,
               const Matrix<float> &parentInverseMatrix);

    // Scales the node about the point (in the parent's coordinates), keeping the point in place (ScalableFont's glyph
    // nodes).
    void scaleAbout(const Vector<2, float> &point, const Vector<2, float> &scale);

protected:
    friend class Widget;                // Moves the node by its translation.
    friend class Node;                  // A node registers itself in m_nodes and reads the matrices.
    friend class Engine;                // DeleteShape/DeleteTransformation/DeleteDisplay erase m_engineEntry.
    friend class ScalableFont;          // The glyph nodes' translation.
    friend class PlasmaFont;            // Places and scales the glyph nodes.
    friend class ::Workspace;           // Resets the interface nodes' matrices.
    friend class ::ZoomModule;          // Shows the root's deformation scale.
    friend class ::NodeInterface;       // SaveState copies the attributes into a NodeState.
    friend class ::TextureSheet;        // Resets the root's deformation scale.
    friend class ::Sheet;               // The grid and rulers read the root's matrices, translation and deformation.
    friend class ::DesignSheet;         // Clear resets the root's translation and deformation.
    friend class ::NodeModule;          // The node panel sets translation, deformation and rotation.
    friend class ::Module;              // Zooms the panels and centres the icons.
    friend class ::ToolModule;          // Scales and centres the current tool's icon.
    friend class ::NodeTool;            // Moves the selected nodes by their translation.
    friend class ::StatusModule;        // Places the tooltip icon.
    friend class ::TextModule;          // Moves the font search icon and edit box.
    friend class ::ExportModule;        // Places the exported instances and renders the page.
    friend class GLEngine;              // Multiplies the model-view matrix by m_matrix.
    friend class ::MeshInterface;       // Places the children along the outline.
    friend class ::Tool;                // Places the icons by their translation and pivot.
    friend class ::ProceduralTool;      // The procedural preview's deformation, rotation and matrix.
    friend class ::InterfaceViewWidget; // The tool radius scales with the root's deformation.
    friend class ::svg::SVGReader;      // The SVG walker premultiplies by m_deformation.
    ContinuousAttribute<Vector<2, float>> m_translation;
    ContinuousAttribute<Matrix<float>> m_deformation;
    ContinuousAttribute<Vector<3, float>> m_rotation;
    ContinuousAttribute<Vector<2, float>> m_pivot;
    std::set<Node *> m_nodes;                            // The nodes that use this transformation.
    Matrix<float> m_matrix;                              // Starts as the identity.
    Matrix<float> m_inverseMatrix;                       // m_matrix inverted; starts as the identity.
    std::list<Transformation *>::iterator m_engineEntry; // This transformation's node in the engine's list.
};

}
