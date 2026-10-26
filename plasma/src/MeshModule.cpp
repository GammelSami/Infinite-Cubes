#include "MeshModule.h"
#include <stdlib.h>
#include <sstream>
#include "MeshOperation.h"
#include "DesignSheet.h"
#include "MeshInterface.h"
#include "main.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h" // The edit box filters.
#include "plasma/SmoothMeshShape.h"

// The "mesh:subdivisions" field's range is 0..6.
static const int MAX_SUBDIVISIONS = 6;

static const int DefaultMeshSubdivisionCount = 4;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
MeshModule::MeshModule()
    : m_shapeMode(SHAPE_MODE_MESH),
      m_subdivisions(DefaultMeshSubdivisionCount),
      m_strokePattern(plasma::STROKE_PATTERN_SOLID),
      m_extruded(false),
      m_strokeJoint(plasma::STROKE_JOINT_ROUND),
      m_strokeCap(plasma::STROKE_CAP_ROUND),
      m_strokeDash(1.0f),
      m_strokeGap(2.0f),
      m_strokeAlignment(plasma::STROKE_ALIGNMENT_NORMAL)
{
    {
        std::wstring name(L"module:mesh");
        std::wstring kind(L"sidewidget");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 230.0f, 140.0f, g_workspace.m_scalableNode,
                                                           kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Mesh", true);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void MeshModule::build()
{
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"mesh:mesh", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:mesh:stroke", L"radiobutton2", 30, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:contour", L"radiobutton2", 60, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:contour:stroke", L"radiobutton2", 90, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke", L"radiobutton2", 120, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:open", L"radiobutton2", 150, 0, 30, 30));
    addPanel(items, L"", 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:extruded", L"checkbutton2", 0, 0, 30, 30));
    addPanel(items, L"", 3);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:dashed", L"checkbutton2", 0, 0, 30, 40));
    addPanel(items, L"", 0);

    // The gap row goes below the dash row, right of the dashed button.
    plasma::Vector<2, int> cursor = m_cursor;
    addWidget(L"Dash:", L"", L"static", 33, 20, 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:dash", L"editbox", 0, 0, 30, 20));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:dash:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 30, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"mesh:stroke:dash", &plasma::g_unsignedFloatFilter, true);
    m_cursor.m_elements[0] = cursor.m_elements[0];
    m_cursor.m_elements[1] = cursor.m_elements[1] + 22;
    addWidget(L"Gap:", L"", L"static", 33, 20, 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:gap", L"editbox", 0, 0, 30, 20));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:gap:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 30, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"mesh:stroke:gap", &plasma::g_unsignedFloatFilter, true);
    newRow(3);
    addWidget(L"Subdivisions:", L"", L"static", 66, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:subdivisions", L"editbox", 0, 0, 30, 20));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:subdivisions:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 30, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"mesh:subdivisions", &plasma::g_unsignedIntegerFilter, true);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:joints:round", L"radiobutton3", 0, 0, 27, 32));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:joints:sharp", L"radiobutton3", 0, 32, 27, 33));
    addPanelAt(items, L"", 130, 63);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:caps:none", L"radiobutton3", 0, 0, 30, 22));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:caps:round", L"radiobutton3", 0, 22, 30, 21));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:caps:square", L"radiobutton3", 0, 43, 30, 22));
    addPanelAt(items, L"", 160, 63);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:alignment:normal", L"radiobutton3", 0, 0, 30, 22));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:alignment:outer", L"radiobutton3", 0, 22, 30, 21));
    items.push_back(ModuleWidgetItem(L" ", L"mesh:stroke:alignment:inner", L"radiobutton3", 0, 43, 30, 22));
    addPanelAt(items, L"", 193, 63);
    fitWidth(10);
    addZoomBar();
    addApplyBar(L"mesh", 0);
    getPanel()->getNode()->connectWidget(L"mesh:apply", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &MeshModule::apply,
                                         true);
    getPanel()->getNode()->connectWidget(L"mesh:pick", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &MeshModule::pick,
                                         true);
    m_panel->setChecked(L"mesh:apply:auto", true, false, true);
    m_panel->setChecked(L"mesh:pick:auto", true, false, true);
    getPanel()->getNode()->connectWidget(L"mesh:mesh", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setMeshMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:mesh:stroke", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setMeshStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:contour", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setContourMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:contour:stroke", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setContourStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:open", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setOpenStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:mesh", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyMeshMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:mesh:stroke", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyMeshStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:contour", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyContourMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:contour:stroke", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyContourStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:open", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyOpenStrokeMode, true);
    getPanel()->getNode()->connectWidget(L"mesh:subdivisions", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &MeshModule::editSubdivisions, true);
    getPanel()->getNode()->connectWidget(L"mesh:subdivisions:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &MeshModule::dragSubdivisions, true);
    getPanel()->getNode()->connectWidget(L"mesh:subdivisions:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &MeshModule::editSubdivisions, true);
    getPanel()->getNode()->connectWidget(L"mesh:subdivisions:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::stepSubdivisions, true);
    getPanel()->getNode()->connectWidget(L"mesh:subdivisions:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::stepSubdivisions, true);
    getPanel()->getNode()->connectWidget(L"mesh:extruded", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setExtruded, true);
    getPanel()->getNode()->connectWidget(L"mesh:extruded", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyExtruded, true);
    getPanel()->getNode()->connectWidget(L"mesh:joints:round", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeJoint, true);
    getPanel()->getNode()->connectWidget(L"mesh:joints:round", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyStrokeJoint, true);
    getPanel()->getNode()->connectWidget(L"mesh:joints:sharp", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeJoint, true);
    getPanel()->getNode()->connectWidget(L"mesh:joints:sharp", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyStrokeJoint, true);
    getPanel()->getNode()->connectWidget(L"mesh:caps:none", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeCap, true);
    getPanel()->getNode()->connectWidget(L"mesh:caps:none", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyStrokeCap, true);
    getPanel()->getNode()->connectWidget(L"mesh:caps:round", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeCap, true);
    getPanel()->getNode()->connectWidget(L"mesh:caps:round", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyStrokeCap, true);
    getPanel()->getNode()->connectWidget(L"mesh:caps:square", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeCap, true);
    getPanel()->getNode()->connectWidget(L"mesh:caps:square", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyStrokeCap, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:alignment:normal", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeAlignment, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:alignment:normal", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &MeshModule::applyStrokeAlignment, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:alignment:inner", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeAlignment, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:alignment:inner", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &MeshModule::applyStrokeAlignment, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:alignment:outer", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokeAlignment, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:alignment:outer", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &MeshModule::applyStrokeAlignment, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:dashed", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::setStrokePattern, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:dashed", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::applyStrokePattern, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:dash", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &MeshModule::editStrokeDash, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:dash:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &MeshModule::dragStrokeDash, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:dash:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &MeshModule::editStrokeDash, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:dash:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::stepStrokeDash, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:dash:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::stepStrokeDash, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:gap", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &MeshModule::editStrokeGap, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:gap:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &MeshModule::dragStrokeGap, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:gap:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &MeshModule::editStrokeGap, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:gap:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &MeshModule::stepStrokeGap, true);
    getPanel()->getNode()->connectWidget(L"mesh:stroke:gap:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &MeshModule::stepStrokeGap, true);
    std::wstringstream stream;
    stream << m_subdivisions;
    getPanel()->getNode()->setText(L"mesh:subdivisions", stream.str(), true);
    placeIcon(L"mesh:mesh", L"icon:mesh:mesh");
    placeIcon(L"mesh:contour", L"icon:mesh:contour");
    placeIcon(L"mesh:mesh:stroke", L"icon:mesh:mesh:stroke");
    placeIcon(L"mesh:contour:stroke", L"icon:mesh:contour:stroke");
    placeIcon(L"mesh:stroke", L"icon:mesh:stroke");
    placeIcon(L"mesh:stroke:open", L"icon:mesh:stroke:open");
    placeIcon(L"mesh:extruded", L"icon:colormode:extrusion");
    placeIcon(L"mesh:stroke:dashed", L"icon:mesh:dashed");
    placeIcon(L"mesh:joints:sharp", L"icon:mesh:joints:sharp");
    placeIcon(L"mesh:joints:round", L"icon:mesh:joints:round");
    placeIcon(L"mesh:caps:none", L"icon:mesh:caps:none");
    placeIcon(L"mesh:caps:round", L"icon:mesh:caps:round");
    placeIcon(L"mesh:caps:square", L"icon:mesh:caps:square");
    placeIcon(L"mesh:stroke:alignment:normal", L"icon:mesh:alignment:normal");
    placeIcon(L"mesh:stroke:alignment:inner", L"icon:mesh:alignment:inner");
    placeIcon(L"mesh:stroke:alignment:outer", L"icon:mesh:alignment:outer");
    updatePanel();
    createZoomSequences();
}

void MeshModule::pick()
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        readShape(m_workspace->getOpenDesignSheet()->m_currentNode);
        updatePanel();
    }
}

MeshState::MeshState()
    : m_vertexPositions("", std::vector<plasma::Vector<2, float>>()),
      m_vertexTexCoords("", std::vector<plasma::Vector<2, float>>()),
      m_vertexColors("", std::vector<plasma::Vector<4, float>>()),
      m_strokeColors("", std::vector<plasma::Vector<4, float>>()),
      m_extrusionFrontColors("", std::vector<plasma::Vector<4, float>>()),
      m_extrusionBackColors("", std::vector<plasma::Vector<4, float>>()),
      m_strokeWidths("", std::vector<float>()),

      // The vector and matrix initial values are uninitialised copies.
      m_textureTranslation("", plasma::Vector<2, float>()),
      m_texturePivot("", plasma::Vector<2, float>()),
      m_textureRotation("", plasma::Vector<3, float>()),
      m_textureDeformation("", plasma::Matrix<float>()),
      m_textureOpacity("", 0.0f),
      m_textureBrightness("", 0.0f),
      m_textureContrast("", 0.0f),
      m_textureSaturation("", 0.0f),
      m_strokeTextureOpacity("", 0.0f),
      m_strokeTextureBrightness("", 0.0f),
      m_strokeTextureContrast("", 0.0f),
      m_strokeTextureSaturation("", 0.0f),
      m_strokeTextureStretch("", 0.0f),
      m_extrusionMatrix("", plasma::Matrix<float>()),
      m_texture("", 0),
      m_strokeTexture("", 0)
{}

void MeshModule::apply()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
    while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
        if (it->first->isShown() && it->second->m_selected) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                MeshState before;
                mesh->captureState(before);
                setShapeModeFlags(mesh->m_mesh.m_shape);
                bool extruded = m_extruded;
                plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
                if (extruded)
                    shape->m_flags |= (1 << plasma::MESH_EXTRUDED);
                else
                    shape->m_flags &= ~(1 << plasma::MESH_EXTRUDED);
                mesh->m_mesh.m_shape->setStrokeJointType(m_strokeJoint);
                mesh->m_mesh.m_shape->setStrokeCapType(m_strokeCap);
                mesh->m_mesh.m_shape->setStrokeAlignment(m_strokeAlignment);
                mesh->m_mesh.m_shape->setSubdivisions(m_subdivisions, true);
                mesh->m_mesh.m_shape->setStrokePattern(m_strokePattern);
                MeshState after;
                mesh->captureState(after);

                // Recorded as "set extruded".
                m_workspace->getOpenDesignSheet()->addOperation(
                    new MeshOperation(m_workspace->getOpenDesignSheet(), "set extruded", mesh, before, after));
                mesh->refresh();
            }
        }
        ++it;
    }
    m_workspace->redraw();
    m_workspace->getOpenDesignSheet()->commit();
}

void MeshModule::applyShapeMode()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
    while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
        if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                MeshState before;
                mesh->captureState(before);
                if (setShapeModeFlags(mesh->m_mesh.m_shape)) {
                    MeshState after;
                    mesh->captureState(after);
                    m_workspace->getOpenDesignSheet()->addOperation(
                        new MeshOperation(m_workspace->getOpenDesignSheet(), "change shape mode", mesh, before, after));
                    mesh->refresh();
                }
            }
        }
        ++it;
    }
    m_workspace->redraw();
    m_workspace->getOpenDesignSheet()->commit();
}

bool MeshModule::setShapeModeFlags(plasma::SmoothMeshShape *shape)
{
    bool contour = bool(shape->m_flags & (1 << plasma::MESH_CONTOUR));
    bool stroke = shape->isStroked();
    bool mesh = shape->isFilled();
    bool closed = !shape->isOpen();
    switch (m_shapeMode) {
    case SHAPE_MODE_MESH:
        shape->m_flags &= ~((1 << plasma::MESH_CONTOUR) | (1 << plasma::MESH_STROKE) | (1 << plasma::MESH_NO_FILL));
        break;
    case SHAPE_MODE_MESH_STROKE:
        shape->m_flags = (shape->m_flags & ~((1 << plasma::MESH_CONTOUR) | (1 << plasma::MESH_NO_FILL))) |
                         (1 << plasma::MESH_STROKE);
        break;
    case SHAPE_MODE_CONTOUR:
        shape->m_flags = (shape->m_flags & ~((1 << plasma::MESH_STROKE) | (1 << plasma::MESH_NO_FILL))) |
                         (1 << plasma::MESH_CONTOUR);
        break;
    case SHAPE_MODE_CONTOUR_STROKE:
        shape->m_flags =
            (shape->m_flags & ~(1 << plasma::MESH_NO_FILL)) | (1 << plasma::MESH_CONTOUR) | (1 << plasma::MESH_STROKE);
        break;
    case SHAPE_MODE_STROKE:
        shape->m_flags =
            (shape->m_flags & ~(1 << plasma::MESH_CONTOUR)) | (1 << plasma::MESH_STROKE) | (1 << plasma::MESH_NO_FILL);
    case SHAPE_MODE_OPEN_STROKE:
        shape->m_flags =
            (shape->m_flags & ~(1 << plasma::MESH_CONTOUR)) | (1 << plasma::MESH_STROKE) | (1 << plasma::MESH_NO_FILL);
        break;
    }
    if (m_shapeMode == SHAPE_MODE_OPEN_STROKE)
        shape->m_flags |= (1 << plasma::MESH_OPEN);
    else
        shape->m_flags &= ~(1 << plasma::MESH_OPEN);
    bool changed = bool(shape->m_flags & (1 << plasma::MESH_CONTOUR)) != contour || shape->isFilled() != mesh ||
                   shape->isStroked() != stroke || !shape->isOpen() != closed;
    if (changed) {
        shape->updateSubdivision();
        shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    return changed;
}

void MeshModule::setExtruded()
{
    m_extruded = m_panel->isChecked(L"mesh:extruded", true);
    if (m_workspace->getOpenDesignSheet()) {
        if (m_panel->isChecked(L"mesh:apply:auto", true)) {
            for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                     m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                 it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                    ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                    if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                        MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                        if (m_extruded != bool(mesh->m_mesh.m_shape->m_flags & (1 << plasma::MESH_EXTRUDED))) {
                            MeshState before;
                            mesh->captureState(before);
                            mesh->m_mesh.m_shape->setExtruded(m_extruded);
                            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                            MeshState after;
                            mesh->captureState(after);
                            m_workspace->getOpenDesignSheet()->addOperation(new MeshOperation(
                                m_workspace->getOpenDesignSheet(), "set extruded", mesh, before, after));
                            mesh->refresh();
                        }
                    }
                }
            }
            m_workspace->getOpenDesignSheet()->commit();
        }
    }
}

void MeshModule::applyExtruded()
{
    m_extruded = m_panel->isChecked(L"mesh:extruded", true);
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
    while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
        if (it->first->isShown() && it->second->m_selected) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                if (m_extruded != bool(mesh->m_mesh.m_shape->m_flags & (1 << plasma::MESH_EXTRUDED))) {
                    MeshState before;
                    mesh->captureState(before);
                    bool extruded = m_extruded;
                    plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
                    if (extruded)
                        shape->m_flags |= (1 << plasma::MESH_EXTRUDED);
                    else
                        shape->m_flags &= ~(1 << plasma::MESH_EXTRUDED);
                    mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    MeshState after;
                    mesh->captureState(after);
                    m_workspace->getOpenDesignSheet()->addOperation(
                        new MeshOperation(m_workspace->getOpenDesignSheet(), "set extruded", mesh, before, after));
                    mesh->refresh();
                }
            }
        }
        ++it;
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void MeshModule::setStrokeJoint()
{
    if (m_panel->isChecked(L"mesh:joints:round", true))
        m_strokeJoint = plasma::STROKE_JOINT_ROUND;
    else
        m_strokeJoint = plasma::STROKE_JOINT_SHARP;
    if (m_workspace->getOpenDesignSheet()) {
        if (m_panel->isChecked(L"mesh:apply:auto", true)) {
            for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                     m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                 it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                    ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                    if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                        MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                        if (m_strokeJoint != mesh->m_mesh.m_shape->getStrokeJointType()) {
                            MeshState before;
                            mesh->captureState(before);
                            mesh->m_mesh.m_shape->setStrokeJointType(m_strokeJoint);
                            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                            MeshState after;
                            mesh->captureState(after);
                            m_workspace->getOpenDesignSheet()->addOperation(new MeshOperation(
                                m_workspace->getOpenDesignSheet(), "set stroke joints", mesh, before, after));
                        }
                    }
                }
            }
            m_workspace->getOpenDesignSheet()->commit();
        }
    }
}

void MeshModule::setStrokeCap()
{
    if (m_panel->isChecked(L"mesh:caps:none", true))
        m_strokeCap = plasma::STROKE_CAP_NONE;
    else if (m_panel->isChecked(L"mesh:caps:round", true))
        m_strokeCap = plasma::STROKE_CAP_ROUND;
    else
        m_strokeCap = plasma::STROKE_CAP_SQUARE;
    if (m_workspace->getOpenDesignSheet()) {
        if (m_panel->isChecked(L"mesh:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
            for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                     m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                 it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                    ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                    if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                        MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                        if (m_strokeCap != mesh->m_mesh.m_shape->getStrokeCapType()) {
                            MeshState before;
                            mesh->captureState(before);
                            mesh->m_mesh.m_shape->setStrokeCapType(m_strokeCap);
                            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                            MeshState after;
                            mesh->captureState(after);
                            m_workspace->getOpenDesignSheet()->addOperation(new MeshOperation(
                                m_workspace->getOpenDesignSheet(), "set stroke caps", mesh, before, after));
                        }
                    }
                }
            }
            m_workspace->getOpenDesignSheet()->commit();
        }
    }
}

void MeshModule::setStrokeAlignment()
{
    if (m_panel->isChecked(L"mesh:stroke:alignment:inner", true))
        m_strokeAlignment = plasma::STROKE_ALIGNMENT_INNER;
    else if (m_panel->isChecked(L"mesh:stroke:alignment:outer", true))
        m_strokeAlignment = plasma::STROKE_ALIGNMENT_OUTER;
    else
        m_strokeAlignment = plasma::STROKE_ALIGNMENT_NORMAL;
    if (m_workspace->getOpenDesignSheet()) {
        if (m_panel->isChecked(L"mesh:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
            for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                     m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                 it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                    ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                    if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                        MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                        int alignment = mesh->m_mesh.m_shape->m_strokeAlignment;
                        if (m_strokeAlignment != alignment) {
                            MeshState before;
                            mesh->captureState(before);
                            mesh->m_mesh.m_shape->setStrokeAlignment(m_strokeAlignment);
                            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                            MeshState after;
                            mesh->captureState(after);
                            m_workspace->getOpenDesignSheet()->addOperation(new MeshOperation(
                                m_workspace->getOpenDesignSheet(), "set stroke alignment", mesh, before, after));
                        }
                    }
                }
            }
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        }
    }
}

void MeshModule::applyStrokeAlignment()
{
    if (m_panel->isChecked(L"mesh:stroke:alignment:inner", true))
        m_strokeAlignment = plasma::STROKE_ALIGNMENT_INNER;
    else if (m_panel->isChecked(L"mesh:stroke:alignment:outer", true))
        m_strokeAlignment = plasma::STROKE_ALIGNMENT_OUTER;
    else
        m_strokeAlignment = plasma::STROKE_ALIGNMENT_NORMAL;
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
    while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
        if (it->first->isShown() && it->second->m_selected) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                int currentStrokeAlignment = mesh->m_mesh.m_shape->m_strokeAlignment;
                if (m_strokeAlignment != currentStrokeAlignment) {
                    MeshState before;
                    mesh->captureState(before);
                    mesh->m_mesh.m_shape->setStrokeAlignment(m_strokeAlignment);
                    mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    MeshState after;
                    mesh->captureState(after);
                    m_workspace->getOpenDesignSheet()->addOperation(new MeshOperation(
                        m_workspace->getOpenDesignSheet(), "set stroke alignment", mesh, before, after));
                }
            }
        }
        ++it;
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->redraw();
}

void MeshModule::applyStrokeJoint()
{
    if (m_panel->isChecked(L"mesh:joints:round", true))
        m_strokeJoint = plasma::STROKE_JOINT_ROUND;
    else
        m_strokeJoint = plasma::STROKE_JOINT_SHARP;
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
    while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
        if (it->first->isShown() && it->second->m_selected) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                int currentStrokeJoint = mesh->m_mesh.m_shape->getStrokeJointType();
                if (m_strokeJoint != currentStrokeJoint) {
                    MeshState before;
                    mesh->captureState(before);
                    mesh->m_mesh.m_shape->setStrokeJointType(m_strokeJoint);
                    mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    MeshState after;
                    mesh->captureState(after);
                    m_workspace->getOpenDesignSheet()->addOperation(
                        new MeshOperation(m_workspace->getOpenDesignSheet(), "set stroke joints", mesh, before, after));
                }
            }
        }
        ++it;
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void MeshModule::applyStrokeCap()
{
    if (m_panel->isChecked(L"mesh:caps:none", true))
        m_strokeCap = plasma::STROKE_CAP_NONE;
    else if (m_panel->isChecked(L"mesh:caps:round", true))
        m_strokeCap = plasma::STROKE_CAP_ROUND;
    else
        m_strokeCap = plasma::STROKE_CAP_SQUARE;
    if (m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (it->first->isShown() && it->second->m_selected) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                    MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                    int currentStrokeCap = mesh->m_mesh.m_shape->getStrokeCapType();
                    if (m_strokeCap != currentStrokeCap) {
                        MeshState before;
                        mesh->captureState(before);
                        mesh->m_mesh.m_shape->setStrokeCapType(m_strokeCap);
                        mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        MeshState after;
                        mesh->captureState(after);
                        m_workspace->getOpenDesignSheet()->addOperation(new MeshOperation(
                            m_workspace->getOpenDesignSheet(), "set stroke caps", mesh, before, after));
                    }
                }
            }
            ++it;
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void MeshModule::setContourMode()
{
    m_shapeMode = SHAPE_MODE_CONTOUR;
    if (m_panel->isChecked(L"mesh:apply:auto", true))
        applyShapeMode();
}

void MeshModule::setMeshMode()
{
    m_shapeMode = SHAPE_MODE_MESH;
    if (m_panel->isChecked(L"mesh:apply:auto", true))
        applyShapeMode();
}

void MeshModule::setContourStrokeMode()
{
    m_shapeMode = SHAPE_MODE_CONTOUR_STROKE;
    if (m_panel->isChecked(L"mesh:apply:auto", true))
        applyShapeMode();
}

void MeshModule::setMeshStrokeMode()
{
    m_shapeMode = SHAPE_MODE_MESH_STROKE;
    if (m_panel->isChecked(L"mesh:apply:auto", true))
        applyShapeMode();
}

void MeshModule::setStrokeMode()
{
    m_shapeMode = SHAPE_MODE_STROKE;
    if (m_panel->isChecked(L"mesh:apply:auto", true))
        applyShapeMode();
}

void MeshModule::setOpenStrokeMode()
{
    m_shapeMode = SHAPE_MODE_OPEN_STROKE;
    if (m_panel->isChecked(L"mesh:apply:auto", true))
        applyShapeMode();
}

void MeshModule::applyContourMode()
{
    m_shapeMode = SHAPE_MODE_CONTOUR;
    applyShapeMode();
}

void MeshModule::applyMeshMode()
{
    m_shapeMode = SHAPE_MODE_MESH;
    applyShapeMode();
}

void MeshModule::applyContourStrokeMode()
{
    m_shapeMode = SHAPE_MODE_CONTOUR_STROKE;
    applyShapeMode();
}

void MeshModule::applyMeshStrokeMode()
{
    m_shapeMode = SHAPE_MODE_MESH_STROKE;
    applyShapeMode();
}

void MeshModule::applyStrokeMode()
{
    m_shapeMode = SHAPE_MODE_STROKE;
    applyShapeMode();
}

void MeshModule::applyOpenStrokeMode()
{
    m_shapeMode = SHAPE_MODE_OPEN_STROKE;
    applyShapeMode();
}

void MeshModule::currentNodeChanged()
{
    if (!m_workspace->getOpenDesignSheet() || !m_workspace->getOpenDesignSheet()->getCurrentNode())
        return;

    // Fixed nodes (Node flag bit 11) do not contribute automatic settings.
    if (m_workspace->getOpenDesignSheet()->getCurrentNode()->isFixed())
        return;
    if (!m_panel->isChecked(L"mesh:pick:auto", true))
        return;
    readShape(m_workspace->getOpenDesignSheet()->getCurrentNode());
    updatePanel();
}

void MeshModule::readShape(plasma::Node *node)
{
    if (!node || !node->m_shape || node->m_shape->getType() != plasma::SHAPE_SMOOTH_MESH)
        return;
    plasma::SmoothMeshShape *shape = static_cast<plasma::SmoothMeshShape *>(node->m_shape);
    m_subdivisions = shape->m_subdivisions;
    m_strokePattern = shape->m_strokePattern;
    m_strokeDash = shape->m_strokeDash;
    m_strokeGap = shape->m_strokeGap;
    m_extruded = (shape->m_flags & (1 << plasma::MESH_EXTRUDED)) != 0;
    m_strokeCap = shape->m_strokeCapType;
    m_strokeAlignment = shape->m_strokeAlignment;
    m_strokeJoint = shape->m_strokeJointType;
    if (!shape->isClosed())
        m_shapeMode = SHAPE_MODE_OPEN_STROKE;
    else if (!shape->isFilled())
        m_shapeMode = SHAPE_MODE_STROKE;
    else {
        bool solid = !shape->isContour();
        if (shape->isStroked())
            m_shapeMode = SHAPE_MODE_CONTOUR_STROKE | solid;
        else
            m_shapeMode = solid;
    }
}

void MeshModule::updatePanel()
{
    std::wstringstream text;
    text << m_subdivisions;
    m_panel->setText(L"mesh:subdivisions", text.str(), true);
    text.str(L"");
    text << m_strokeDash;
    m_panel->setText(L"mesh:stroke:dash", text.str(), true);
    text.str(L"");
    text << m_strokeGap;
    m_panel->setText(L"mesh:stroke:gap", text.str(), true);
    switch (m_shapeMode) {
    case SHAPE_MODE_MESH:
        m_panel->setChecked(L"mesh:mesh", true, false, true);
        break;
    case SHAPE_MODE_MESH_STROKE:
        m_panel->setChecked(L"mesh:mesh:stroke", true, false, true);
        break;
    case SHAPE_MODE_CONTOUR:
        m_panel->setChecked(L"mesh:contour", true, false, true);
        break;
    case SHAPE_MODE_CONTOUR_STROKE:
        m_panel->setChecked(L"mesh:contour:stroke", true, false, true);
        break;
    case SHAPE_MODE_STROKE:
        m_panel->setChecked(L"mesh:stroke", true, false, true);
        break;
    case SHAPE_MODE_OPEN_STROKE:
        m_panel->setChecked(L"mesh:stroke:open", true, false, true);
        break;
    }
    getPanel()->getNode()->setChecked(L"mesh:extruded", m_extruded, false, true);
    getPanel()->getNode()->setChecked(L"mesh:stroke:dashed", m_strokePattern == plasma::STROKE_PATTERN_DASHED, false,
                                      true);
    switch (m_strokeJoint) {
    case plasma::STROKE_JOINT_ROUND:
        m_panel->setChecked(L"mesh:joints:round", true, false, true);
        break;
    case plasma::STROKE_JOINT_SHARP:
        m_panel->setChecked(L"mesh:joints:sharp", true, false, true);
        break;
    }
    switch (m_strokeCap) {
    case plasma::STROKE_CAP_NONE:
        m_panel->setChecked(L"mesh:caps:none", true, false, true);
        break;
    case plasma::STROKE_CAP_ROUND:
        m_panel->setChecked(L"mesh:caps:round", true, false, true);
        break;
    case plasma::STROKE_CAP_SQUARE:
        m_panel->setChecked(L"mesh:caps:square", true, false, true);
        break;
    }
    switch (m_strokeAlignment) {
    case plasma::STROKE_ALIGNMENT_INNER:
        m_panel->setChecked(L"mesh:stroke:alignment:inner", true, false, true);
        break;
    case plasma::STROKE_ALIGNMENT_NORMAL:
        m_panel->setChecked(L"mesh:stroke:alignment:normal", true, false, true);
        break;
    case plasma::STROKE_ALIGNMENT_OUTER:
        m_panel->setChecked(L"mesh:stroke:alignment:outer", true, false, true);
        break;
    }
}

void MeshModule::showSelectedValues()
{
    std::list<plasma::Node *> nodes;
    m_workspace->getSelectedNodes(nodes);

    // The most subdivisions among the selected smooth meshes.
    int subdivisions = -1;
    for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        plasma::Shape *shape = (*it)->m_shape;
        if (shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH) {
            int shapeSubdivisions = static_cast<plasma::SmoothMeshShape *>(shape)->m_subdivisions;
            if (shapeSubdivisions > subdivisions)
                subdivisions = shapeSubdivisions;
        }
    }
    if (subdivisions >= 0) {
        std::wstringstream text;
        text << subdivisions;
        m_panel->setText(L"mesh:subdivisions", text.str(), true);
    }
}

void MeshModule::editSubdivisions()
{
    m_subdivisions = atoi(narrowString(getPanel()->getNode()->getText(L"mesh:subdivisions", true)).c_str());
    if (m_subdivisions < 0)
        m_subdivisions = 0;
    if (m_subdivisions > MAX_SUBDIVISIONS)
        m_subdivisions = MAX_SUBDIVISIONS;
    std::wstringstream text;
    text << m_subdivisions;
    m_panel->setText(L"mesh:subdivisions", text.str(), true);
    if (m_panel->isChecked(L"mesh:apply:auto", true)) {
        m_workspace->beginModifySelected();
        applySubdivisions();
        m_workspace->endModifySelected();
    }
}

void MeshModule::dragSubdivisions(plasma::Widget *widget)
{
    if (isDragging(widget)) {
        updateDrag();
        m_subdivisions = (int)(m_dragPosition * 0.1f + m_dragStart);
        if (m_subdivisions < 0) {
            m_subdivisions = 0;
            beginDrag(0.0f);
        }
        if (m_subdivisions > MAX_SUBDIVISIONS) {
            m_subdivisions = MAX_SUBDIVISIONS;
            beginDrag((float)MAX_SUBDIVISIONS);
        }
        std::wstringstream text;
        text << m_subdivisions;
        m_panel->setText(L"mesh:subdivisions", text.str(), true);

        // Read and ignored.
        m_panel->isChecked(L"mesh:apply:auto", true);
    }
}

void MeshModule::stepSubdivisions(plasma::Widget *widget)
{
    m_subdivisions += pressDragButton(widget);
    if (m_subdivisions > MAX_SUBDIVISIONS)
        m_subdivisions = MAX_SUBDIVISIONS;
    if (m_subdivisions < 0)
        m_subdivisions = 0;
    beginDrag((float)m_subdivisions);
    std::wstringstream text;
    text << m_subdivisions;
    m_panel->setText(L"mesh:subdivisions", text.str(), true);

    // Read and ignored.
    m_panel->isChecked(L"mesh:apply:auto", true);
}

void MeshModule::applySubdivisions()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    for (std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
                if (shape->m_subdivisions != m_subdivisions) {
                    shape->setSubdivisions(m_subdivisions, true);
                    mesh->refresh();
                    mesh->m_hasChanges = true;
                }
            }
        }
    }
    m_workspace->redraw();
}

void MeshModule::setStrokePattern()
{
    m_strokePattern =
        m_panel->isChecked(L"mesh:stroke:dashed", true) ? plasma::STROKE_PATTERN_DASHED : plasma::STROKE_PATTERN_SOLID;
    if (m_workspace->getOpenDesignSheet()) {
        if (m_panel->isChecked(L"mesh:apply:auto", true)) {
            for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                     m_workspace->getOpenDesignSheet()->sheetNodesBegin();
                 it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                    ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                    if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                        MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                        if (m_strokePattern != mesh->m_mesh.m_shape->getStrokePattern()) {
                            MeshState before;
                            mesh->captureState(before);
                            mesh->m_mesh.m_shape->setStrokePattern(m_strokePattern);
                            mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                            MeshState after;
                            mesh->captureState(after);

                            // Recorded as "set extruded".
                            m_workspace->getOpenDesignSheet()->addOperation(new MeshOperation(
                                m_workspace->getOpenDesignSheet(), "set extruded", mesh, before, after));
                        }
                    }
                }
            }
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        }
    }
}

void MeshModule::applyStrokePattern()
{
    m_strokePattern =
        m_panel->isChecked(L"mesh:stroke:dashed", true) ? plasma::STROKE_PATTERN_DASHED : plasma::STROKE_PATTERN_SOLID;
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
    while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
        if (it->first->isShown() && it->second->m_selected) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                int currentStrokePattern = mesh->m_mesh.m_shape->getStrokePattern();
                if (m_strokePattern != currentStrokePattern) {
                    MeshState before;
                    mesh->captureState(before);
                    mesh->m_mesh.m_shape->setStrokePattern(m_strokePattern);
                    mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    MeshState after;
                    mesh->captureState(after);

                    // Recorded as "set extruded".
                    m_workspace->getOpenDesignSheet()->addOperation(
                        new MeshOperation(m_workspace->getOpenDesignSheet(), "set extruded", mesh, before, after));
                }
            }
        }
        ++it;
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void MeshModule::editStrokeDash()
{
    m_strokeDash = (float)atof(narrowString(getPanel()->getNode()->getText(L"mesh:stroke:dash", true)).c_str());
    if (m_strokeDash < 0.0f)
        m_strokeDash = 0.0f;
    std::wstringstream text;
    text << m_strokeDash;
    m_panel->setText(L"mesh:stroke:dash", text.str(), true);
    if (m_panel->isChecked(L"mesh:apply:auto", true)) {
        m_workspace->beginModifySelected();
        applyStrokeDash();
        m_workspace->endModifySelected();
    }
}

void MeshModule::dragStrokeDash(plasma::Widget *widget)
{
    if (isDragging(widget)) {
        updateDrag();
        m_strokeDash = (int)(m_dragPosition * 0.25f) + m_dragStart;
        if (m_strokeDash < 0.0f) {
            m_strokeDash = 0.0f;
            beginDrag(0.0f);
        }
        std::wstringstream text;
        text << m_strokeDash;
        m_panel->setText(L"mesh:stroke:dash", text.str(), true);

        // Read and ignored.
        m_panel->isChecked(L"mesh:apply:auto", true);
    }
}

void MeshModule::stepStrokeDash(plasma::Widget *widget)
{
    m_strokeDash = pressDragButton(widget) + m_strokeDash;
    if (m_strokeDash < 0.0f)
        m_strokeDash = 0.0f;
    beginDrag(m_strokeDash);
    std::wstringstream text;
    text << m_strokeDash;
    m_panel->setText(L"mesh:stroke:dash", text.str(), true);

    // Read and ignored.
    m_panel->isChecked(L"mesh:apply:auto", true);
}

void MeshModule::editStrokeGap()
{
    m_strokeGap = (float)atof(narrowString(getPanel()->getNode()->getText(L"mesh:stroke:gap", true)).c_str());
    if (m_strokeGap < 0.0f)
        m_strokeGap = 0.0f;
    std::wstringstream text;
    text << m_strokeGap;
    m_panel->setText(L"mesh:stroke:gap", text.str(), true);
    if (m_panel->isChecked(L"mesh:apply:auto", true)) {
        m_workspace->beginModifySelected();
        applyStrokeGap();
        m_workspace->endModifySelected();
    }
}

void MeshModule::dragStrokeGap(plasma::Widget *widget)
{
    if (isDragging(widget)) {
        updateDrag();
        m_strokeGap = (int)(m_dragPosition * 0.25f) + m_dragStart;
        if (m_strokeGap < 0.0f) {
            m_strokeGap = 0.0f;
            beginDrag(0.0f);
        }
        std::wstringstream text;
        text << m_strokeGap;
        m_panel->setText(L"mesh:stroke:gap", text.str(), true);

        // Read and ignored.
        m_panel->isChecked(L"mesh:apply:auto", true);
    }
}

void MeshModule::stepStrokeGap(plasma::Widget *widget)
{
    m_strokeGap = pressDragButton(widget) + m_strokeGap;
    if (m_strokeGap < 0.0f)
        m_strokeGap = 0.0f;
    beginDrag(m_strokeGap);
    std::wstringstream text;
    text << m_strokeGap;
    m_panel->setText(L"mesh:stroke:gap", text.str(), true);

    // Read and ignored.
    m_panel->isChecked(L"mesh:apply:auto", true);
}

void MeshModule::applyStrokeDash()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    for (std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
                if (shape->m_strokeDash != m_strokeDash) {
                    shape->m_strokeDash = m_strokeDash;
                    mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    mesh->m_hasChanges = true;
                }
            }
        }
    }
    m_workspace->redraw();
}

void MeshModule::applyStrokeGap()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    for (std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
            ShapeInterface *shapeInterface = it->second->m_shapeInterface;
            if (shapeInterface && shapeInterface->m_type == INTERFACE_MESH) {
                MeshInterface *mesh = static_cast<MeshInterface *>(shapeInterface);
                plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
                if (shape->m_strokeGap != m_strokeGap) {
                    shape->m_strokeGap = m_strokeGap;
                    mesh->m_mesh.m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    mesh->m_hasChanges = true;
                }
            }
        }
    }
    m_workspace->redraw();
}
