// MeshModule: the editor's mesh panel ("module:mesh", caption "Mesh"). It keeps the settings it applies to the
// selected mesh shapes: the shape mode (the "mesh:contour", "mesh:mesh"... radio buttons), the subdivisions,
// extrusion, the stroke's joints, caps, alignment and dash pattern. Changing a setting records a MeshOperation
// ("set extruded", "change shape mode", "set stroke joints", "set stroke caps", "set stroke alignment") holding a
// MeshState of the shape before and after; subdivisions, dash and gap go through
// BeginModifySelected/EndModifySelected.
#pragma once
#include <list>
#include <string>
#include <vector>
#include "Module.h"
#include "Operation.h"
#include "plasma/ContinuousArrayAttribute.h"
#include "plasma/ContinuousAttribute.h"
#include "plasma/DiscreteAttribute.h"
#include "plasma/Matrix.h"
#include "plasma/Node.h"
#include "plasma/Widget.h"

namespace plasma {
class Node;
class SmoothMeshShape;
}

class MeshInterface;

// A copy of a mesh shape's editable state: its arrays, attributes and SmoothMeshShape settings, captured before
// and after an edit. Copy and assignment operate member by member.
class MeshState {
public:
    MeshState();

    plasma::ContinuousArrayAttribute<plasma::Vector<2, float>> m_vertexPositions; // MeshShape.
    plasma::ContinuousArrayAttribute<plasma::Vector<2, float>> m_vertexTexCoords;
    plasma::ContinuousArrayAttribute<plasma::Vector<4, float>> m_vertexColors;
    plasma::ContinuousArrayAttribute<plasma::Vector<4, float>> m_strokeColors;
    plasma::ContinuousArrayAttribute<plasma::Vector<4, float>> m_extrusionFrontColors;
    plasma::ContinuousArrayAttribute<plasma::Vector<4, float>> m_extrusionBackColors;
    plasma::ContinuousArrayAttribute<float> m_strokeWidths;
    plasma::ContinuousAttribute<plasma::Vector<2, float>> m_textureTranslation;
    plasma::ContinuousAttribute<plasma::Vector<2, float>> m_texturePivot;
    plasma::ContinuousAttribute<plasma::Vector<3, float>> m_textureRotation;
    plasma::ContinuousAttribute<plasma::Matrix<float>> m_textureDeformation;
    plasma::ContinuousAttribute<float> m_textureOpacity;
    plasma::ContinuousAttribute<float> m_textureBrightness;
    plasma::ContinuousAttribute<float> m_textureContrast;
    plasma::ContinuousAttribute<float> m_textureSaturation;
    plasma::ContinuousAttribute<float> m_strokeTextureOpacity;
    plasma::ContinuousAttribute<float> m_strokeTextureBrightness;
    plasma::ContinuousAttribute<float> m_strokeTextureContrast;
    plasma::ContinuousAttribute<float> m_strokeTextureSaturation;
    plasma::ContinuousAttribute<float> m_strokeTextureStretch;
    plasma::ContinuousAttribute<plasma::Matrix<float>> m_extrusionMatrix;
    plasma::DiscreteAttribute<int> m_texture;
    plasma::DiscreteAttribute<int> m_strokeTexture;
    std::vector<std::vector<int>> m_faces; // MeshShape::m_faces.
    std::vector<int> m_vertexFlags;        // MeshShape::m_vertexFlags.
    std::vector<float> m_vertexParameters; // MeshShape::m_vertexParameters.
    int m_subdivisions;                    // SmoothMeshShape::m_subdivisions.
    std::list<int> m_drawnVertexIndices;   // The in-progress DrawFaceTool's vertices, as vertex indices.

    // MeshShape::m_flags, bit by bit.
    bool m_stroke;         // MESH_STROKE set.
    bool m_closed;         // MESH_OPEN clear.
    bool m_filled;         // MESH_NO_FILL clear.
    bool m_contour;        // MESH_CONTOUR set.
    bool m_extruded;       // MESH_EXTRUDED set.
    int m_strokeCapType;   // MeshShape::m_strokeCapType.
    int m_strokeJointType; // MeshShape::m_strokeJointType.
    int m_strokePattern;   // SmoothMeshShape::m_strokePattern.
    float m_strokeDash;
    float m_strokeGap;
    int m_drawnFaceIndex; // The in-progress DrawFaceTool's face, as a face index; -1 when none.
};

class MeshModule : public Module {
public:
    // The shape mode radio buttons, in m_shapeMode order.
    enum ShapeMode {
        SHAPE_MODE_CONTOUR,        // "mesh:contour".
        SHAPE_MODE_MESH,           // "mesh:mesh".
        SHAPE_MODE_CONTOUR_STROKE, // "mesh:contour:stroke".
        SHAPE_MODE_MESH_STROKE,    // "mesh:mesh:stroke".
        SHAPE_MODE_STROKE,         // "mesh:stroke".
        SHAPE_MODE_OPEN_STROKE     // "mesh:stroke:open".
    };

    MeshModule();

    virtual void build();
    virtual void updatePanel(); // Shows the mesh settings.

    // Shows the most subdivisions among the selected smooth meshes.
    virtual void showSelectedValues();
    virtual void currentNodeChanged(); // "mesh:pick:auto".

    // "mesh:apply": the panel's settings to every selected mesh shape.
    void apply();

    // "mesh:pick": the panel takes the current shape's settings.
    void pick();

    // The shape mode to every selected mesh shape.
    void applyShapeMode();

    // Sets the shape's flags for the shape mode; true when they changed.
    bool setShapeModeFlags(plasma::SmoothMeshShape *shape);

    // The settings of the node's shape, if it is a smooth mesh.
    void readShape(plasma::Node *node);

    // The shape mode radio buttons (event 2): the mode, applied when "mesh:apply:auto" is checked.
    void setContourMode();
    void setMeshMode();
    void setContourStrokeMode();
    void setMeshStrokeMode();
    void setStrokeMode();
    void setOpenStrokeMode();

    // The same buttons' event 4: the mode, applied.
    void applyContourMode();
    void applyMeshMode();
    void applyContourStrokeMode();
    void applyMeshStrokeMode();
    void applyStrokeMode();
    void applyOpenStrokeMode();

    // The check boxes and radio buttons of the stroke settings: event 2 reads the setting and applies it when
    // "mesh:apply:auto" is checked, event 4 reads it and applies it.
    void setExtruded(); // "mesh:extruded".
    void applyExtruded();
    void setStrokeJoint();     // "mesh:joints:round", ":sharp".
    void setStrokeCap();       // "mesh:caps:none", ":round", ":square".
    void setStrokeAlignment(); // "mesh:stroke:alignment:*".
    void applyStrokeAlignment();
    void applyStrokeJoint();
    void applyStrokeCap();

    // The number fields: typed (event 19, and event 3 of the drag button), dragged (event 12) and stepped by the drag
    // button's ends (events 2, 4).
    void editSubdivisions();                       // "mesh:subdivisions".
    void dragSubdivisions(plasma::Widget *widget); // "mesh:subdivisions:drag".
    void stepSubdivisions(plasma::Widget *widget);
    void applySubdivisions();
    void setStrokePattern(); // "mesh:stroke:dashed".
    void applyStrokePattern();
    void editStrokeDash(); // "mesh:stroke:dash".
    void dragStrokeDash(plasma::Widget *widget);
    void stepStrokeDash(plasma::Widget *widget);
    void editStrokeGap(); // "mesh:stroke:gap".
    void dragStrokeGap(plasma::Widget *widget);
    void stepStrokeGap(plasma::Widget *widget);
    void applyStrokeDash();
    void applyStrokeGap();

    int m_shapeMode;       // A ShapeMode.
    int m_subdivisions;    // "mesh:subdivisions".
    int m_strokePattern;   // SmoothMeshShape::m_strokePattern ("mesh:stroke:dashed").
    bool m_extruded;       // "mesh:extruded": MESH_EXTRUDED.
    int m_strokeJoint;     // "mesh:joints:round", "mesh:joints:sharp".
    int m_strokeCap;       // "mesh:caps:none", "mesh:caps:round", "mesh:caps:square".
    float m_strokeDash;    // "mesh:stroke:dash".
    float m_strokeGap;     // "mesh:stroke:gap".
    int m_strokeAlignment; // "mesh:stroke:alignment:normal", ":outer", ":inner".
};
