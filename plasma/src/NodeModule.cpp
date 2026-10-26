#include "NodeModule.h"
#include "FormatModule.h" // The edit box filters.
#include "DesignSheet.h"
#include "Operations.h"
#include "TextModule.h"
#include "TextureModule.h"
#include "MeshModule.h"
#include "main.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "plasma/Shape.h"
#include "plasma/Transformation.h"
#include <cmath>
#include <cstdlib>
#include <sstream>

using plasma::Node;

typedef std::map<Node *, NodeInterface *>::iterator SheetNodeIterator;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
NodeModule::NodeModule()
{
    {
        std::wstring name(L"module:node");
        std::wstring kind(L"bottombar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 180.0f, 30.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Object", true);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void NodeModule::build()
{
    m_margin.m_elements[0] = 10;
    m_cursor.m_elements[1] += 5;
    addWidget(L"Name:", L"", L"static", 40, 20, 0);
    addWidget(L"", L"node:name", L"editbox", 200, 20, 0);
    m_cursor.m_elements[1] -= 5;
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"node:fixed", L"checkbutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:visible", L"checkbutton2", 30, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:snapable", L"checkbutton2", 60, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:nodeformer", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:deformer", L"radiobutton2", 30, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:align:outline", L"radiobutton2", 60, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:align:outline:rotation", L"radiobutton2", 90, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:adapt", L"radiobutton2", 120, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:noclipping", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:clipping", L"radiobutton2", 30, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:modulation", L"radiobutton2", 60, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:screen", L"radiobutton2", 90, 0, 30, 30));
    addPanel(items, L"", 3);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:blur", L"checkbutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:blur:edit", L"editbox", 30, 0, 50, 30));
    items.push_back(ModuleWidgetItem(L" ", L"node:blur:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 80, 0,
                                     20, 30));
    addPanel(items, L"", 3);
    placeIcon(L"node:visible", L"icon:visible");
    placeIcon(L"node:fixed", L"icon:fixed");
    placeIcon(L"node:noclipping", L"icon:noclipping");
    placeIcon(L"node:clipping", L"icon:clipping");
    placeIcon(L"node:modulation", L"icon:modulation");
    placeIcon(L"node:screen", L"icon:screen");
    placeIcon(L"node:snapable", L"icon:snapable");
    placeIcon(L"node:nodeformer", L"icon:cancel");
    placeIcon(L"node:deformer", L"icon:deformer");
    placeIcon(L"node:align:outline", L"icon:align:outline");
    placeIcon(L"node:align:outline:rotation", L"icon:align:outline:rotation");
    placeIcon(L"node:adapt", L"icon:adapt");
    placeIcon(L"node:blur", L"icon:blur");
    m_cursor.m_elements[1] += 5;
    addWidget(L"px", L"text:unit", L"static", 10, 20, 0);
    m_cursor.m_elements[1] -= 5;
    getPanel()->getNode()->setEditFilter(L"node:blur:edit", &plasma::g_unsignedFloatFilter, true);
    newRow(3);
    addWidget(L"Position:", L"", L"static", 45, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:translation:x", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"node:translation:x:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:translation:y", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"node:translation:y:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 2);
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);
    getPanel()->getNode()->setEditFilter(L"node:translation:x", &plasma::g_floatFilter, true);
    getPanel()->getNode()->setEditFilter(L"node:translation:y", &plasma::g_floatFilter, true);
    addWidget(L"Scale:", L"", L"static", 32, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:scale:x", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"node:scale:x:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:scale:y", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"node:scale:y:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 2);
    getPanel()->getNode()->setEditFilter(L"node:scale:x", &plasma::g_floatFilter, true);
    getPanel()->getNode()->setEditFilter(L"node:scale:y", &plasma::g_floatFilter, true);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    addWidget(L"Shear:", L"", L"static", 35, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:shear:x", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"node:shear:x:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:shear:y", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"node:shear:y:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 2);
    getPanel()->getNode()->setEditFilter(L"node:shear:x", &plasma::g_floatFilter, true);
    getPanel()->getNode()->setEditFilter(L"node:shear:y", &plasma::g_floatFilter, true);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    addWidget(L"Angle:", L"", L"static", 35, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:rotation", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"node:rotation:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    addWidget(L"\u00b0", L"", L"static", 10, 20, 0);
    getPanel()->getNode()->setEditFilter(L"node:rotation", &plasma::g_floatFilter, true);
    fitWidth(10);
    fitHeight(5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"node:module:mesh", L"checkbutton2", 0, 0, 50, 23));
    items.push_back(ModuleWidgetItem(L" ", L"node:module:text", L"checkbutton2", 50, 0, 50, 23));
    items.push_back(ModuleWidgetItem(L" ", L"node:module:texture", L"checkbutton2", 100, 0, 50, 23));
    addUpperPanel(L"modulebar", items, (int)(m_panel->viewWidth() * 0.5 - 75.0));
    getPanel()->getNode()->setChecked(L"modulebar", true, false, true);
    placeIcon(L"node:module:mesh", L"icon:module:mesh");
    placeIcon(L"node:module:text", L"icon:module:text");
    placeIcon(L"node:module:texture", L"icon:wrap:none");
    addZoomBar();
    getPanel()->getNode()->connectWidget(L"node:module:mesh", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleMeshModule, true);
    getPanel()->getNode()->connectWidget(L"node:module:mesh", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleMeshModule, true);
    getPanel()->getNode()->connectWidget(L"node:module:text", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleTextModule, true);
    getPanel()->getNode()->connectWidget(L"node:module:text", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleTextModule, true);
    getPanel()->getNode()->connectWidget(L"node:module:texture", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleTextureModule, true);
    getPanel()->getNode()->connectWidget(L"node:module:texture", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleTextureModule, true);
    getPanel()->getNode()->connectWidget(L"node:name", plasma::WIDGET_EVENT_FOCUS_LOST, this, &NodeModule::editName,
                                         true);
    getPanel()->getNode()->connectWidget(L"node:visible", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleVisible, true);
    getPanel()->getNode()->connectWidget(L"node:visible", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleVisible, true);
    getPanel()->getNode()->connectWidget(L"node:fixed", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleFixed, true);
    getPanel()->getNode()->connectWidget(L"node:fixed", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleFixed, true);
    getPanel()->getNode()->connectWidget(L"node:noclipping", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setNoClipping, true);
    getPanel()->getNode()->connectWidget(L"node:clipping", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setClipping, true);
    getPanel()->getNode()->connectWidget(L"node:modulation", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setModulation, true);
    getPanel()->getNode()->connectWidget(L"node:screen", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setScreen, true);
    getPanel()->getNode()->connectWidget(L"node:active", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleActive, true);
    getPanel()->getNode()->connectWidget(L"node:active", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleActive, true);
    getPanel()->getNode()->connectWidget(L"node:snapable", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleSnapable, true);
    getPanel()->getNode()->connectWidget(L"node:snapable", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleSnapable, true);
    getPanel()->getNode()->connectWidget(L"node:nodeformer", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setNoDeformer, true);
    getPanel()->getNode()->connectWidget(L"node:deformer", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setDeformer, true);
    getPanel()->getNode()->connectWidget(L"node:align:outline", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setAlignOutline, true);
    getPanel()->getNode()->connectWidget(L"node:align:outline:rotation", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setAlignOutlineRotation, true);
    getPanel()->getNode()->connectWidget(L"node:adapt", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::setAdapt, true);
    getPanel()->getNode()->connectWidget(L"node:generic", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleSource, true);
    getPanel()->getNode()->connectWidget(L"node:generic", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleSource, true);
    getPanel()->getNode()->connectWidget(L"node:blur", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::toggleBlur, true);
    getPanel()->getNode()->connectWidget(L"node:blur", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::toggleBlur, true);
    getPanel()->getNode()->connectWidget(L"node:blur:edit", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editBlur, true);
    getPanel()->getNode()->connectWidget(L"node:blur:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragBlur, true);
    getPanel()->getNode()->connectWidget(L"node:blur:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:blur:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepBlur, true);
    getPanel()->getNode()->connectWidget(L"node:blur:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepBlur, true);
    getPanel()->getNode()->connectWidget(L"node:translation:x", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editTranslationX, true);
    getPanel()->getNode()->connectWidget(L"node:translation:x:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragTranslationX, true);
    getPanel()->getNode()->connectWidget(L"node:translation:x:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:translation:x:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepTranslationX, true);
    getPanel()->getNode()->connectWidget(L"node:translation:x:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepTranslationX, true);
    getPanel()->getNode()->connectWidget(L"node:translation:y", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editTranslationY, true);
    getPanel()->getNode()->connectWidget(L"node:translation:y:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragTranslationY, true);
    getPanel()->getNode()->connectWidget(L"node:translation:y:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:translation:y:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepTranslationY, true);
    getPanel()->getNode()->connectWidget(L"node:translation:y:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepTranslationY, true);
    getPanel()->getNode()->connectWidget(L"node:scale:x", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editScaleX, true);
    getPanel()->getNode()->connectWidget(L"node:scale:x:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragScaleX, true);
    getPanel()->getNode()->connectWidget(L"node:scale:x:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:scale:x:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepScaleX, true);
    getPanel()->getNode()->connectWidget(L"node:scale:x:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepScaleX, true);
    getPanel()->getNode()->connectWidget(L"node:scale:y", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editScaleY, true);
    getPanel()->getNode()->connectWidget(L"node:scale:y:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragScaleY, true);
    getPanel()->getNode()->connectWidget(L"node:scale:y:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:scale:y:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepScaleY, true);
    getPanel()->getNode()->connectWidget(L"node:scale:y:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepScaleY, true);
    getPanel()->getNode()->connectWidget(L"node:shear:x", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editShearX, true);
    getPanel()->getNode()->connectWidget(L"node:shear:x:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragShearX, true);
    getPanel()->getNode()->connectWidget(L"node:shear:x:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:shear:x:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepShearX, true);
    getPanel()->getNode()->connectWidget(L"node:shear:x:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepShearX, true);
    getPanel()->getNode()->connectWidget(L"node:shear:y", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editShearY, true);
    getPanel()->getNode()->connectWidget(L"node:shear:y:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragShearY, true);
    getPanel()->getNode()->connectWidget(L"node:shear:y:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:shear:y:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepShearY, true);
    getPanel()->getNode()->connectWidget(L"node:shear:y:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepShearY, true);
    getPanel()->getNode()->connectWidget(L"node:rotation", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &NodeModule::editRotation, true);
    getPanel()->getNode()->connectWidget(L"node:rotation:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &NodeModule::dragRotation, true);
    getPanel()->getNode()->connectWidget(L"node:rotation:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &NodeModule::endDrag, true);
    getPanel()->getNode()->connectWidget(L"node:rotation:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &NodeModule::stepRotation, true);
    getPanel()->getNode()->connectWidget(L"node:rotation:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &NodeModule::stepRotation, true);
    createZoomSequences();
    updatePanel();
}

void NodeModule::endDrag()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->redraw();
    }
}
// Shows the current node's settings, or blanks the values and disables the panel (but its zoom and module bars)
// without a current node.
void NodeModule::updatePanel()
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->getCurrentNode()) {
        for (std::list<Node *>::iterator it = getPanel()->getNode()->m_children.begin();
             it != getPanel()->getNode()->m_children.end(); ++it) {
            if ((*it)->getName() != L"zoombar" && (*it)->getName() != L"modulebar")
                (*it)->setEnabled(true);
        }
        getPanel()->getNode()->setText(L"node:name", m_workspace->getOpenDesignSheet()->getCurrentNode()->getName(),
                                       true);
        getPanel()->getNode()->setChecked(L"node:fixed", m_workspace->getOpenDesignSheet()->getCurrentNode()->isFixed(),
                                          false, true);
        getPanel()->getNode()->setChecked(
            L"node:visible", m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_visibility.value() != 0,
            false, true);
        getPanel()->getNode()->setChecked(
            L"node:noclipping", m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_clipping.value() == 0,
            true, true);
        bool clipping =
            m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_clipping.value() != 0 &&
            !(m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_flags &
              (1 << plasma::DISPLAY_MODULATION)) &&
            !(m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_flags & (1 << plasma::DISPLAY_SCREEN));
        getPanel()->getNode()->setChecked(L"node:clipping", clipping, true, true);
        getPanel()->getNode()->setChecked(L"node:modulation",
                                          (m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_flags &
                                           (1 << plasma::DISPLAY_MODULATION)) != 0,
                                          true, true);
        getPanel()->getNode()->setChecked(L"node:screen",
                                          (m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_flags &
                                           (1 << plasma::DISPLAY_SCREEN)) != 0,
                                          true, true);
        getPanel()->getNode()->setChecked(
            L"node:active", m_workspace->getOpenDesignSheet()->getCurrentNode()->isHitTestable(), false, true);
        getPanel()->getNode()->setChecked(
            L"node:snapable", m_workspace->getOpenDesignSheet()->getCurrentNode()->isSnapable(), false, true);
        bool noDeformer = !m_workspace->getOpenDesignSheet()->getCurrentNode()->isDeformer() &&
                          !m_workspace->getOpenDesignSheet()->getCurrentNode()->alignsToOutline() &&
                          !m_workspace->getOpenDesignSheet()->getCurrentNode()->alignsRotationToOutline() &&
                          !m_workspace->getOpenDesignSheet()->getCurrentNode()->fitsShapeToChildren();
        getPanel()->getNode()->setChecked(L"node:nodeformer", noDeformer, true, true);
        getPanel()->getNode()->setChecked(
            L"node:deformer", m_workspace->getOpenDesignSheet()->getCurrentNode()->isDeformer(), true, true);
        bool alignOutline = m_workspace->getOpenDesignSheet()->getCurrentNode()->alignsToOutline() &&
                            !m_workspace->getOpenDesignSheet()->getCurrentNode()->alignsRotationToOutline();
        getPanel()->getNode()->setChecked(L"node:align:outline", alignOutline, true, true);
        getPanel()->getNode()->setChecked(
            L"node:align:outline:rotation",
            m_workspace->getOpenDesignSheet()->getCurrentNode()->alignsRotationToOutline(), true, true);
        getPanel()->getNode()->setChecked(
            L"node:adapt", m_workspace->getOpenDesignSheet()->getCurrentNode()->fitsShapeToChildren(), true, true);
        getPanel()->getNode()->setChecked(L"node:generic",
                                          m_workspace->getOpenDesignSheet()->getCurrentNode()->isSource(), false, true);
        getPanel()->getNode()->setChecked(L"node:blur",
                                          (m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_flags &
                                           (1 << plasma::DISPLAY_BLUR)) != 0,
                                          false, true);

        // The blur radius scaled by the node's mean scale on the sheet.
        std::wstringstream text;
        Node *currentNode = m_workspace->getOpenDesignSheet()->getCurrentNode();
        plasma::Matrix<float> matrix = m_workspace->getOpenDesignSheet()->getNodeMatrix(currentNode);
        plasma::Vector<2, float> xAxis(matrix.m_elements[0], matrix.m_elements[4]);
        plasma::Vector<2, float> yAxis(matrix.m_elements[1], matrix.m_elements[5]);
        float scaleX = std::sqrt(xAxis[0] * xAxis[0] + xAxis[1] * xAxis[1]);
        float scaleY = std::sqrt(yAxis[0] * yAxis[0] + yAxis[1] * yAxis[1]);
        float blur = m_workspace->getOpenDesignSheet()->getCurrentNode()->display()->m_blurRadius.value() *
                     ((scaleX + scaleY) * 0.5);
        text << blur;
        getPanel()->getNode()->setText(L"node:blur:edit", text.str(), true);
        showTransformation();
    } else {
        setNodeText(getPanel()->getNode(), L"node:name", "");
        setNodeText(getPanel()->getNode(), L"node:translation:x", "");
        setNodeText(getPanel()->getNode(), L"node:translation:y", "");
        setNodeText(getPanel()->getNode(), L"node:scale:x", "");
        setNodeText(getPanel()->getNode(), L"node:scale:y", "");
        setNodeText(getPanel()->getNode(), L"node:shear:x", "");
        setNodeText(getPanel()->getNode(), L"node:shear:y", "");
        setNodeText(getPanel()->getNode(), L"node:rotation", "");
        setNodeText(getPanel()->getNode(), L"node:blur:edit", "");
        for (std::list<Node *>::iterator it = getPanel()->getNode()->m_children.begin();
             it != getPanel()->getNode()->m_children.end(); ++it) {
            if ((*it)->getName() != L"zoombar" && (*it)->getName() != L"modulebar") {
                (*it)->setChecked(false, true);
                (*it)->setEnabled(false);
            }
        }
    }
}

void NodeModule::showTransformation()
{
    if (!m_workspace->getOpenDesignSheet() || !m_workspace->getOpenDesignSheet()->getCurrentNode())
        return;
    std::wstringstream text;

    // The position is the translation plus the pivot.
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(
        (m_workspace->getOpenDesignSheet()->getCurrentNode()->m_transformation->m_translation.value() +
         m_workspace->getOpenDesignSheet()->getCurrentNode()->m_transformation->m_pivot.value())[0]));
    m_panel->setText(L"node:translation:x", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(
        (m_workspace->getOpenDesignSheet()->getCurrentNode()->transformation()->m_translation.value() +
         m_workspace->getOpenDesignSheet()->getCurrentNode()->transformation()->m_pivot.value())[1]));
    m_panel->setText(L"node:translation:y", text.str(), true);
    text.str(L"");

    // Scale and shear in percent from the deformation matrix.
    float value = (float)(m_workspace->getOpenDesignSheet()
                              ->getCurrentNode()
                              ->m_transformation->m_deformation.value()
                              .m_elements[0] *
                          100.0);
    text << roundToHundredths(value);
    m_panel->setText(L"node:scale:x", text.str(), true);
    text.str(L"");
    value = (float)(m_workspace->getOpenDesignSheet()
                        ->getCurrentNode()
                        ->m_transformation->m_deformation.value()
                        .m_elements[5] *
                    100.0);
    text << roundToHundredths(value);
    m_panel->setText(L"node:scale:y", text.str(), true);
    value = (float)(m_workspace->getOpenDesignSheet()
                        ->getCurrentNode()
                        ->m_transformation->m_deformation.value()
                        .m_elements[4] *
                    -100.0);
    value = roundToHundredths(value);
    if (value == 0.0f)
        value = 0.0f; // No "-0".
    text.str(L"");
    text << value;
    m_panel->setText(L"node:shear:x", text.str(), true);
    text.str(L"");
    value = (float)(m_workspace->getOpenDesignSheet()
                        ->getCurrentNode()
                        ->m_transformation->m_deformation.value()
                        .m_elements[1] *
                    100.0);
    text << roundToHundredths(value);
    m_panel->setText(L"node:shear:y", text.str(), true);
    text.str(L"");
    text << roundToHundredths(
        m_workspace->getOpenDesignSheet()->getCurrentNode()->m_transformation->m_rotation.value().m_elements[2]);
    m_panel->setText(L"node:rotation", text.str(), true);

    // The blur radius in display units, scaled by the node's mean scale on the sheet.
    std::wstringstream blurText;
    Node *currentNode = m_workspace->getOpenDesignSheet()->getCurrentNode();
    plasma::Matrix<float> matrix = m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix * currentNode->m_matrix;
    value = matrix.m_elements[0] * matrix.m_elements[0] + matrix.m_elements[4] * matrix.m_elements[4];
    float scaleX = std::sqrt(value);
    value = matrix.m_elements[1] * matrix.m_elements[1] + matrix.m_elements[5] * matrix.m_elements[5];
    float scaleY = std::sqrt(value);
    float blur = m_workspace->getOpenDesignSheet()->getCurrentNode()->m_display->m_blurRadius.value() *
                 ((scaleX + scaleY) * 0.5);
    blurText << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(blur));
    m_panel->setText(L"node:blur:edit", blurText.str(), true);
}

void NodeModule::showSelectedValues()
{
    // The selection's name (the last node's) and its shared flags.
    std::wstring name;
    bool visible = true;
    bool clipping = false;
    bool active = true;
    bool hasSourceNode = false;
    std::list<plasma::Node *> nodes;
    m_workspace->getSelectedNodes(nodes);
    for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
        name = (*it)->getName();
        plasma::Display *display = (*it)->m_display;
        if (!display->m_visibility.value())
            visible = false;
        if (display->m_clipping.value())
            clipping = true;
        if (!(*it)->isHitTestable())
            active = false;
        if ((*it)->isSource())
            hasSourceNode = true;
    }
    m_panel->setText(L"node:name", name, true);
    m_panel->setChecked(L"node:visible", visible, false, true);
    m_panel->setChecked(L"node:clipping", clipping, false, true);
    m_panel->setChecked(L"node:active", active, false, true);
    m_panel->setChecked(L"node:generic", hasSourceNode, false, true);
}

void NodeModule::editName()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    std::wstring name = getPanel()->getNode()->getText(L"node:name", true);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->first->setName(name);
            if (it->first->m_shape)
                it->first->m_shape->setName(name);
            if (it->first->m_widget)
                it->first->m_widget->setName(name);
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->selectionChanged();
}

void NodeModule::toggleVisible()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bool visible = m_panel->isChecked(L"node:visible", true);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->first->m_display->m_visibility.value() = visible ? 1 : 0;
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->markDesignSheetChanged();
    m_workspace->selectionChanged();
}

void NodeModule::setNoClipping()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->first->m_display->m_clipping.value() = 0;
            it->first->m_display->m_flags &= ~((1 << plasma::DISPLAY_MODULATION) | (1 << plasma::DISPLAY_SCREEN));
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->markDesignSheetChanged();
    m_workspace->selectionChanged();
}

void NodeModule::setClipping()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->first->m_display->m_clipping.value() = 1;
            it->first->m_display->m_flags &= ~((1 << plasma::DISPLAY_MODULATION) | (1 << plasma::DISPLAY_SCREEN));
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->markDesignSheetChanged();
    m_workspace->selectionChanged();
}

void NodeModule::toggleActive()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bool active = m_panel->isChecked(L"node:active", true);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            if (active)
                it->first->m_flags &= ~(1 << plasma::NODE_INACTIVE);
            else
                it->first->m_flags |= (1 << plasma::NODE_INACTIVE);
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::toggleSource()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bool sourceEnabled = m_panel->isChecked(L"node:generic", true);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            if (sourceEnabled)
                it->first->m_flags |= (1 << plasma::NODE_GENERIC);
            else
                it->first->m_flags &= ~(1 << plasma::NODE_GENERIC);
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::toggleSnapable()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bool snapable = m_panel->isChecked(L"node:snapable", true);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            if (snapable)
                it->first->m_flags |= (1 << plasma::NODE_SNAPABLE);
            else
                it->first->m_flags &= ~(1 << plasma::NODE_SNAPABLE);
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->markDesignSheetChanged();
}

void NodeModule::setNoDeformer()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            NodeState before;
            it->second->saveState(before);
            it->first->m_flags &= ~(1 << plasma::NODE_ALIGN_OUTLINE);
            it->first->m_flags &= ~(1 << plasma::NODE_ALIGN_OUTLINE_ROTATION);
            it->first->m_flags &= ~(1 << plasma::NODE_DEFORMER);
            it->first->m_flags &= ~(1 << plasma::NODE_ADAPT);
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeOperation(m_workspace->getOpenDesignSheet(), "set no deformer", it->second, before, after));
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::setDeformer()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            NodeState before;
            it->second->saveState(before);
            it->first->m_flags &= ~(1 << plasma::NODE_ALIGN_OUTLINE);
            it->first->m_flags &= ~(1 << plasma::NODE_ALIGN_OUTLINE_ROTATION);
            it->first->m_flags |= (1 << plasma::NODE_DEFORMER);
            it->first->m_flags &= ~(1 << plasma::NODE_ADAPT);
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeOperation(m_workspace->getOpenDesignSheet(), "set deformer", it->second, before, after));
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::toggleBlur()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bool blur = m_panel->isChecked(L"node:blur", true);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            if (blur)
                it->first->m_display->m_flags |= (1 << plasma::DISPLAY_BLUR);
            else
                it->first->m_display->m_flags &= ~(1 << plasma::DISPLAY_BLUR);
            it->second->recordModification();
        }
    }
    m_workspace->markDesignSheetChanged();
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->redraw();
}

void NodeModule::setModulation()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->first->m_display->m_clipping.value() = 1;
            it->first->m_display->m_flags &= ~((1 << plasma::DISPLAY_MODULATION) | (1 << plasma::DISPLAY_SCREEN));
            it->first->m_display->m_flags |= (1 << plasma::DISPLAY_MODULATION);
            it->second->recordModification();
        }
    }
    m_workspace->markDesignSheetChanged();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::setScreen()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->first->m_display->m_clipping.value() = 1;
            it->first->m_display->m_flags &= ~((1 << plasma::DISPLAY_MODULATION) | (1 << plasma::DISPLAY_SCREEN));
            it->first->m_display->m_flags |= (1 << plasma::DISPLAY_SCREEN);
            it->second->recordModification();
        }
    }
    m_workspace->markDesignSheetChanged();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::setAlignOutline()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface) {
            NodeState before;
            it->second->saveState(before);
            it->first->m_flags |= (1 << plasma::NODE_ALIGN_OUTLINE);
            it->first->m_flags &= ~(1 << plasma::NODE_ALIGN_OUTLINE_ROTATION);
            it->first->m_flags &= ~(1 << plasma::NODE_DEFORMER);
            it->first->m_flags &= ~(1 << plasma::NODE_ADAPT);
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeOperation(m_workspace->getOpenDesignSheet(), "align outline", it->second, before, after));
            it->second->m_shapeInterface->prepareChildren();
            it->second->m_shapeInterface->alignChildrenToOutline();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::setAlignOutlineRotation()
{
    if (!m_workspace->getOpenDesignSheet())
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface) {
            NodeState before;
            it->second->saveState(before);
            it->first->m_flags |= (1 << plasma::NODE_ALIGN_OUTLINE);
            it->first->m_flags |= (1 << plasma::NODE_ALIGN_OUTLINE_ROTATION);
            it->first->m_flags &= ~(1 << plasma::NODE_DEFORMER);
            it->first->m_flags &= ~(1 << plasma::NODE_ADAPT);
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(new NodeOperation(
                m_workspace->getOpenDesignSheet(), "align outline rotation", it->second, before, after));
            it->second->m_shapeInterface->prepareChildren();
            it->second->m_shapeInterface->alignChildrenToOutline();
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::setAdapt()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    for (SheetNodeIterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface) {
            NodeState before;
            it->second->saveState(before);
            it->first->m_flags &= ~(1 << plasma::NODE_ALIGN_OUTLINE);
            it->first->m_flags &= ~(1 << plasma::NODE_ALIGN_OUTLINE_ROTATION);
            it->first->m_flags &= ~(1 << plasma::NODE_DEFORMER);
            it->first->m_flags |= (1 << plasma::NODE_ADAPT);
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeOperation(m_workspace->getOpenDesignSheet(), "adapt to children", it->second, before, after));
            if (it->first->getShape()) {
                it->second->getShapeInterface()->beginModify();
                it->first->getShape()->m_changed = true;
                it->first->animate(0);
                it->second->getShapeInterface()->endModify();
                it->second->getShapeInterface()->refresh();
            }
        }
    }
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::toggleFixed()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    bool fixed = m_panel->isChecked(L"node:fixed", true);
    for (SheetNodeIterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface) {
            NodeState before;
            it->second->saveState(before);
            if (fixed)
                it->first->m_flags |= (1 << plasma::NODE_FIXED);
            else
                it->first->m_flags &= ~(1 << plasma::NODE_FIXED);
            NodeState after;
            it->second->saveState(after);
            m_workspace->getOpenDesignSheet()->addOperation(
                new NodeOperation(m_workspace->getOpenDesignSheet(), "set fixed", it->second, before, after));
        }
    }
    if (fixed)
        m_workspace->updateModules();
    m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    m_workspace->getOpenDesignSheet()->commit();
}

void NodeModule::editBlur()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float blur = (float)atof(narrowString(getPanel()->getNode()->getText(L"node:blur:edit", true)).c_str());
    if (blur < 0.0f)
        blur = 0.0f;
    std::wstringstream text;
    text << blur;
    m_panel->setText(L"node:blur:edit", text.str(), true);
    blur = m_workspace->toDesignSheetInternalUnits(blur);
    for (SheetNodeIterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            const plasma::Matrix<float> &nodeMatrix = it->first->m_matrix;
            plasma::Matrix<float> matrix = m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix * nodeMatrix;

            // The blur radius divided by the node's mean scale on the sheet.
            plasma::Display *display = it->first->m_display;
            double x0 = matrix.m_elements[0];
            double x1 = matrix.m_elements[4];
            float y0 = matrix.m_elements[1];
            float y1 = matrix.m_elements[5];
            float scaleY = std::sqrt(y1 * y1 + y0 * y0);
            display->m_blurRadius.value() = blur / ((scaleY + std::sqrt((float)(x0 * x0 + x1 * x1))) * 0.5);
            it->first->m_display->m_changed = true;
            it->second->recordModification();
        }
    }
    m_workspace->getOpenDesignSheet()->commit();
    m_workspace->markDesignSheetChanged();
}

void NodeModule::dragBlur(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        updateDrag();
        float blur = m_dragStart + m_dragPosition;
        if (blur < 0.0f)
            blur = 0.0f;
        std::wstringstream text;
        text << blur;
        m_panel->setText(L"node:blur:edit", text.str(), true);
        blur = m_workspace->toDesignSheetInternalUnits(blur);
        std::list<Node *> nodes;
        m_workspace->getSelectedNodes(nodes);
        for (std::list<Node *>::iterator node = nodes.begin(); node != nodes.end(); ++node) {
            const plasma::Matrix<float> &nodeMatrix = (*node)->m_matrix;
            plasma::Matrix<float> matrix = m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix * nodeMatrix;

            // The blur radius divided by the node's mean scale on the sheet.
            double x0 = matrix.m_elements[0];
            double x1 = matrix.m_elements[4];
            float scaleX = std::sqrt((float)(x0 * x0 + x1 * x1));
            float adjustedRadius = blur / ((scaleX + std::sqrt(matrix.m_elements[1] * matrix.m_elements[1] +
                                                               matrix.m_elements[5] * matrix.m_elements[5])) *
                                           0.5);
            plasma::Display *display = (*node)->m_display;
            display->m_blurRadius.value() = adjustedRadius;
            display->m_changed = true;
        }
        m_workspace->markDesignSheetChanged();
    }
}

void NodeModule::stepBlur(plasma::Widget *sender)
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float blur = (float)atof(narrowString(getPanel()->getNode()->getText(L"node:blur:edit", true)).c_str());
    blur = pressDragButton(sender) + blur;
    if (blur < 0.0f)
        blur = 0.0f;
    beginDrag(blur);
    std::wstringstream text;
    text << blur;
    m_panel->setText(L"node:blur:edit", text.str(), true);
    blur = m_workspace->toDesignSheetInternalUnits(blur);
    for (SheetNodeIterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
         it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->second->m_modificationPending = true;
            const plasma::Matrix<float> &nodeMatrix = it->first->m_matrix;
            plasma::Matrix<float> matrix = m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix * nodeMatrix;

            // The blur radius divided by the node's mean scale on the sheet.
            plasma::Display *display = it->first->m_display;
            double x0 = matrix.m_elements[0];
            double x1 = matrix.m_elements[4];
            float y0 = matrix.m_elements[1];
            float y1 = matrix.m_elements[5];
            float scaleY = std::sqrt(y1 * y1 + y0 * y0);
            display->m_blurRadius.value() = blur / ((scaleY + std::sqrt((float)(x0 * x0 + x1 * x1))) * 0.5);
        }
    }
    m_workspace->markDesignSheetChanged();
}

void NodeModule::editTranslationX()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyTranslationX();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    }
}

void NodeModule::dragTranslationX(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode && isDragging(sender)) {
        updateDrag();
        float value = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:translation:x", text.str(), true);
        applyTranslationX();
    }
}

void NodeModule::stepTranslationX(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"node:translation:x", true)) >> value;
        value = pressDragButton(sender) + value;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:translation:x", text.str(), true);
        applyTranslationX();
        beginDrag(value);
    }
}

void NodeModule::editTranslationY()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyTranslationY();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    }
}

void NodeModule::dragTranslationY(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode && isDragging(sender)) {
        updateDrag();
        float value = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:translation:y", text.str(), true);
        applyTranslationY();
    }
}

void NodeModule::stepTranslationY(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"node:translation:y", true)) >> value;
        value = pressDragButton(sender) + value;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:translation:y", text.str(), true);
        applyTranslationY();
        beginDrag(value);
    }
}

void NodeModule::editScaleX()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyScaleX();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
        showTransformation();
    }
}

void NodeModule::dragScaleX(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        updateDrag();
        float value = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:scale:x", text.str(), true);
        applyScaleX();
        showTransformation();
    }
}

void NodeModule::stepScaleX(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"node:scale:x", true)) >> value;
        value = pressDragButton(sender) + value;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:scale:x", text.str(), true);
        applyScaleX();
        showTransformation();
        beginDrag(value);
    }
}

void NodeModule::editScaleY()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyScaleY();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
        showTransformation();
    }
}

void NodeModule::dragScaleY(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        updateDrag();
        float value = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:scale:y", text.str(), true);
        applyScaleY();
        showTransformation();
    }
}

void NodeModule::stepScaleY(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"node:scale:y", true)) >> value;
        value = pressDragButton(sender) + value;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:scale:y", text.str(), true);
        applyScaleY();
        showTransformation();
        beginDrag(value);
    }
}

void NodeModule::editShearX()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyShearX();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
        showTransformation();
    }
}

void NodeModule::dragShearX(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        updateDrag();
        float value = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:shear:x", text.str(), true);
        applyShearX();
        showTransformation();
    }
}

void NodeModule::stepShearX(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"node:shear:x", true)) >> value;
        value = pressDragButton(sender) + value;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:shear:x", text.str(), true);
        applyShearX();
        showTransformation();
        beginDrag(value);
    }
}

void NodeModule::editShearY()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyShearY();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
        showTransformation();
    }
}

void NodeModule::dragShearY(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        updateDrag();
        float value = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:shear:y", text.str(), true);
        applyShearY();
        showTransformation();
    }
}

void NodeModule::stepShearY(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"node:shear:y", true)) >> value;
        value = pressDragButton(sender) + value;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:shear:y", text.str(), true);
        applyShearY();
        showTransformation();
        beginDrag(value);
    }
}

void NodeModule::editRotation()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyRotation();
        m_workspace->getOpenDesignSheet()->recordModifiedNodes();
    }
}

void NodeModule::dragRotation(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && isDragging(sender)) {
        updateDrag();
        float value = m_dragStart + m_dragPosition;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:rotation", text.str(), true);
        applyRotation();
    }
}

void NodeModule::stepRotation(plasma::Widget *sender)
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        float value; // The code leaves this uninitialized if extraction is skipped.
        std::wstringstream(getPanel()->getNode()->getText(L"node:rotation", true)) >> value;
        value = pressDragButton(sender) + value;
        std::wstringstream text;
        text << value;
        m_panel->setText(L"node:rotation", text.str(), true);
        applyRotation();
        beginDrag(value);
    }
}

void NodeModule::applyTranslationX()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float value; // The code leaves this uninitialized if extraction is skipped.
    std::wstringstream(getPanel()->getNode()->getText(L"node:translation:x", true)) >> value;
    value = m_workspace->toDesignSheetInternalUnits(value);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            plasma::Transformation *transformation = it->first->m_transformation;
            transformation->m_translation.value().m_elements[0] = value - transformation->m_pivot.value().m_elements[0];
            it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_modificationPending = true;
        }
    }
}

void NodeModule::applyTranslationY()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float value; // The code leaves this uninitialized if extraction is skipped.
    std::wstringstream(getPanel()->getNode()->getText(L"node:translation:y", true)) >> value;
    value = m_workspace->toDesignSheetInternalUnits(value);

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            plasma::Transformation *transformation = it->first->m_transformation;
            transformation->m_translation.value().m_elements[1] = value - transformation->m_pivot.value().m_elements[1];
            it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_modificationPending = true;
        }
    }
}

void NodeModule::applyScaleX()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float value; // The code leaves this uninitialized if extraction is skipped.
    std::wstringstream(getPanel()->getNode()->getText(L"node:scale:x", true)) >> value;
    value *= 0.01f;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->first->m_transformation->m_deformation.value().m_elements[0] = value;
            it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_modificationPending = true;
        }
    }
    m_workspace->m_textModule->currentNodeChanged();
}

void NodeModule::applyScaleY()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float value; // The code leaves this uninitialized if extraction is skipped.
    std::wstringstream(getPanel()->getNode()->getText(L"node:scale:y", true)) >> value;
    value *= 0.01f;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->first->m_transformation->m_deformation.value().m_elements[5] = value;
            it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_modificationPending = true;
        }
    }
    m_workspace->m_textModule->currentNodeChanged();
}

void NodeModule::applyShearX()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float value; // The code leaves this uninitialized if extraction is skipped.
    std::wstringstream(getPanel()->getNode()->getText(L"node:shear:x", true)) >> value;
    value *= -0.01f;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->first->m_transformation->m_deformation.value().m_elements[4] = value;
            it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_modificationPending = true;
        }
    }
    m_workspace->m_textModule->currentNodeChanged();
}

void NodeModule::applyShearY()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float value; // The code leaves this uninitialized if extraction is skipped.
    std::wstringstream(getPanel()->getNode()->getText(L"node:shear:y", true)) >> value;
    value *= 0.01f;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->first->m_transformation->m_deformation.value().m_elements[1] = value;
            it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_modificationPending = true;
        }
    }
    m_workspace->m_textModule->currentNodeChanged();
}

void NodeModule::applyRotation()
{
    if (!m_workspace->getOpenDesignSheet())
        return;
    float value; // The code leaves this uninitialized if extraction is skipped.
    std::wstringstream(getPanel()->getNode()->getText(L"node:rotation", true)) >> value;

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    SheetNodeIterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected) {
            it->first->m_transformation->m_rotation.value().m_elements[2] = value;
            it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            it->second->m_modificationPending = true;
        }
    }
}

void NodeModule::currentNodeChanged()
{
    updatePanel();
}

void NodeModule::unitChanged()
{
    updatePanel();
}

void NodeModule::toggleMeshModule()
{
    m_workspace->getMeshModule()->setFadingIn(m_panel->getNode()->isChecked(L"node:module:mesh", true));
}

void NodeModule::toggleTextModule()
{
    m_workspace->getTextModule()->setFadingIn(m_panel->getNode()->isChecked(L"node:module:text", true));
}

void NodeModule::toggleTextureModule()
{
    m_workspace->getTextureModule()->setFadingIn(m_panel->getNode()->isChecked(L"node:module:texture", true));
}
