#include "LayerModule.h"
#include "DesignSheet.h"
#include "CreateNodeOperation.h"
#include "NodeTreeWidget.h"
#include "ToolModule.h"
#include "plasma/GenericWidget.h"
#include "plasma/SmoothMeshShape.h"
#include <sstream>

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
LayerModule::LayerModule()
{
    {
        std::wstring name(L"module:layers");
        std::wstring kind(L"sidewidget");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 180.0f, 400.0f, g_workspace.m_scalableNode,
                                                           kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    {
        std::wstring caption(L"Layers");
        m_panel->setCaption(caption, true);
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void LayerModule::build()
{
    createZoomSequences();
    addWidget(L"", L"layers:listbox", L"listbox2", 160, 350, 0);
    plasma::Node *node = m_workspace->m_engine.createNode(0, 0, 0, 0, L"");
    m_layerWidget = new NodeTreeWidget(&m_workspace->m_engine, m_workspace, node);
    getPanel()->getNode()->moveIntoWidget(L"layers:listbox:list", m_layerWidget, true);
    newRow(3);
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"layers:delete", L"button2", 0, 0, 26, 25));
    items.push_back(ModuleWidgetItem(L" ", L"layers:new", L"button2", 26, 0, 26, 25));
    items.push_back(ModuleWidgetItem(L" ", L"layers:front", L"button2", 52, 0, 26, 25));
    items.push_back(ModuleWidgetItem(L" ", L"layers:forward", L"button2", 78, 0, 26, 25));
    items.push_back(ModuleWidgetItem(L" ", L"layers:backward", L"button2", 104, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"layers:back", L"button2", 130, 0, 26, 25));
    addPanel(items, L"layers:controls", 0);
    newRow(2);
    addWidget(L"Selected: 0", L"layers:selected", L"static", 107, 16, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"layers:find", L"button2", 0, 0, 50, 23));
    addUpperPanel(L"", items, (int)(m_panel->viewWidth() * 0.5 - 25.0));
    placeIcon(L"layers:new", L"icon:new:layer");
    placeIcon(L"layers:delete", L"icon:delete");
    placeIcon(L"layers:front", L"icon:node:front");
    placeIcon(L"layers:forward", L"icon:node:forward");
    placeIcon(L"layers:backward", L"icon:node:backward");
    placeIcon(L"layers:back", L"icon:node:back");
    placeIcon(L"layers:find", L"icon:find");
    addZoomBar();
    getPanel()->getNode()->connectWidget(L"layers:new", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &LayerModule::createLayer, true);
    getPanel()->getNode()->connectWidget(L"layers:new", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &LayerModule::createLayer, true);
    getPanel()->getNode()->connectWidget(L"layers:delete", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::deleteSelected, true);
    getPanel()->getNode()->connectWidget(L"layers:front", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::bringToFront, true);
    getPanel()->getNode()->connectWidget(L"layers:forward", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::bringForward, true);
    getPanel()->getNode()->connectWidget(L"layers:forward", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::bringForward, true);
    getPanel()->getNode()->connectWidget(L"layers:backward", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::sendBackward, true);
    getPanel()->getNode()->connectWidget(L"layers:backward", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::sendBackward, true);
    getPanel()->getNode()->connectWidget(L"layers:back", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::sendToBack, true);
    getPanel()->getNode()->connectWidget(L"layers:back", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::sendToBack, true);
    getPanel()->getNode()->connectWidget(L"layers:delete", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         &m_workspace->getToolModule()->m_nodeTool, &NodeTool::deleteSelected, true);
    getPanel()->getNode()->connectWidget(L"layers:find", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &LayerModule::showCurrentNode, true);
    fitHeight(10);

    // The controls stay at the bottom and the list stretches with the panel.
    getPanel()->getNode()->findWidget(L"layers:controls")->m_verticalAlignment = plasma::ALIGN_BOTTOM;
    getPanel()->getNode()->findWidget(L"layers:selected")->m_verticalAlignment = plasma::ALIGN_BOTTOM;
    getPanel()->getNode()->findWidget(L"layers:listbox")->m_flags |= (1 << plasma::WIDGET_ADAPT_HEIGHT);
}

void LayerModule::createLayer()
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Node *node = m_workspace->m_engine.createNode(0, 0, 0, 0, L"Layer");
        plasma::Vector<2, float> pageSize(m_workspace->getOpenDesignSheet()->m_page.size.m_elements[0],
                                          m_workspace->getOpenDesignSheet()->m_page.size.m_elements[1]);
        plasma::SmoothMeshShape *shape = m_workspace->m_engine.createSmoothMeshShape(L"Layer");
        shape->m_vertexPositions.resize(4);
        shape->m_vertexTexCoords.resize(4);
        shape->m_vertexFlags.resize(4, plasma::VERTEX_SHARP);
        shape->m_vertexColors.resize(4);
        shape->m_strokeColors.resize(4);
        shape->m_extrusionFrontColors.resize(4);
        shape->m_extrusionBackColors.resize(4);
        shape->m_vertexPositions.value()[0] = plasma::Vector<2, float>(0.0f, 0.0f);
        shape->m_vertexPositions.value()[1] = plasma::Vector<2, float>(pageSize.m_elements[0], 0.0f);
        shape->m_vertexPositions.value()[2] = plasma::Vector<2, float>(pageSize.m_elements[0], pageSize.m_elements[1]);
        shape->m_vertexPositions.value()[3] = plasma::Vector<2, float>(0.0f, pageSize.m_elements[1]);
        shape->m_vertexTexCoords.value()[0] = plasma::Vector<2, float>(0.0f, 0.0f);
        shape->m_vertexTexCoords.value()[1] = plasma::Vector<2, float>(1.0f, 0.0f);
        shape->m_vertexTexCoords.value()[2] = plasma::Vector<2, float>(1.0f, 1.0f);
        shape->m_vertexTexCoords.value()[3] = plasma::Vector<2, float>(0.0f, 1.0f);
        shape->m_vertexColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.0f);
        shape->m_vertexColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.0f);
        shape->m_vertexColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.0f);
        shape->m_vertexColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.0f);
        shape->m_strokeColors.value()[0] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
        shape->m_strokeColors.value()[1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
        shape->m_strokeColors.value()[2] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
        shape->m_strokeColors.value()[3] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
        shape->m_extrusionFrontColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_extrusionFrontColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_extrusionFrontColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_extrusionFrontColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_extrusionBackColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_extrusionBackColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_extrusionBackColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_extrusionBackColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->setSubdivisions(1, false);
        std::vector<std::vector<int>> faces(1);
        faces[0].resize(4);
        faces[0][0] = 0;
        faces[0][1] = 1;
        faces[0][2] = 2;
        faces[0][3] = 3;
        shape->setFaces(faces, true);
        node->m_flags |= (1 << plasma::NODE_FIXED);
        node->setShape(shape);
        node->m_flags |= (1 << plasma::NODE_LOCKED);
        if (m_workspace->getOpenDesignSheet()->getEditRoot() &&
            m_workspace->getOpenDesignSheet()->getEditRoot()->getParent() ==
                m_workspace->getOpenDesignSheet()->getRoot()) {
            plasma::Node *editRoot = m_workspace->getOpenDesignSheet()->getEditRoot();
            plasma::Node *root = m_workspace->getOpenDesignSheet()->getRoot();
            root->insertChild(node, editRoot->getNextSibling());
        } else {
            m_workspace->getOpenDesignSheet()->getRoot()->addChild(node);
        }
        m_workspace->getOpenDesignSheet()->createSheetNode(node);
        m_workspace->getOpenDesignSheet()->setEditRoot(node, true);
        m_workspace->getOpenDesignSheet()->addOperation(
            new CreateNodeOperation(m_workspace->getOpenDesignSheet(), "create layer", node));
        m_workspace->getOpenDesignSheet()->commit();
        updatePanel();
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void LayerModule::showCurrentNode()
{
    if (m_workspace->getOpenDesignSheet()) {
        if (m_workspace->getOpenDesignSheet()->m_currentNode) {
            plasma::Node *node = m_workspace->getOpenDesignSheet()->m_currentNode->m_parent;
            while (m_workspace->getOpenDesignSheet()->findSheetNode(node)) {
                m_workspace->getOpenDesignSheet()->findSheetNode(node)->m_expanded = true;
                node = node->m_parent;
            }

            // Lay the tree out to find the current node's row, scroll the panel to it and lay out again.
            m_layerWidget->rebuild();
            plasma::Vector<2, float> position(m_layerWidget->getCurrentNodePosition() * -1 +
                                              plasma::Vector<2, int>(100, 0));
            m_layerWidget->getParentWidget()->setViewPosition(
                m_layerWidget->getParentWidget()->getPlacementMatrix().transformDirection(position), true);
            m_layerWidget->rebuild();
        }
    }
}

void LayerModule::updatePanel()
{
    m_layerWidget->rebuild();
    if (m_workspace->getOpenDesignSheet()) {
        std::wstringstream text;
        text << L"Selected: " << m_workspace->getOpenDesignSheet()->countSelectedNodes();
        m_panel->setText(L"layers:selected", text.str(), true);
    }
}

void LayerModule::sheetChanged()
{
    updatePanel();
}

void plasma::MeshShape::setExtruded(bool extruded)
{
    if (extruded)
        m_flags |= (1 << MESH_EXTRUDED);
    else
        m_flags &= ~(1 << MESH_EXTRUDED);
}
