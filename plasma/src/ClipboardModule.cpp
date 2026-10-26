#include "ClipboardModule.h"
#include "DesignSheet.h"
#include "ToolModule.h"
#include "plasma/GenericWidget.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ClipboardModule::ClipboardModule()
{
    {
        std::wstring name(L"module:clipboard");
        std::wstring kind(L"toolbar");
        m_panel =
            g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 80.0f, 35.0f, g_workspace.m_scalableNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ClipboardModule::build()
{
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"clipboard:cut", L"button2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"clipboard:copy", L"button2", 25, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"clipboard:paste", L"button2", 50, 0, 25, 25));
    addPanelAt(items, L"", 3, 3);
    placeIcon(L"clipboard:cut", L"icon:cut");
    placeIcon(L"clipboard:copy", L"icon:copy");
    placeIcon(L"clipboard:paste", L"icon:paste");
    getPanel()->getNode()->connectWidget(L"clipboard:cut", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ClipboardModule::cut, true);
    getPanel()->getNode()->connectWidget(L"clipboard:cut", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ClipboardModule::cut, true);
    getPanel()->getNode()->connectWidget(L"clipboard:copy", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ClipboardModule::copy, true);
    getPanel()->getNode()->connectWidget(L"clipboard:copy", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ClipboardModule::copy, true);
    getPanel()->getNode()->connectWidget(L"clipboard:paste", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ClipboardModule::paste, true);
    getPanel()->getNode()->connectWidget(L"clipboard:paste", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ClipboardModule::paste, true);
    createZoomSequences();
}

void ClipboardModule::cut()
{
    if (m_workspace->getOpenDesignSheet()) {
        copyNodes(0);
        m_workspace->getOpenDesignSheet()->deleteSelectedNodes(0);
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void ClipboardModule::copy()
{
    if (m_workspace->getOpenDesignSheet()) {
        copyNodes(0);
        m_workspace->m_toolModule->m_stampTool.reset();
    }
}

void ClipboardModule::paste()
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Matrix<float> matrix;
        matrix.identity();
        m_workspace->resetTools();
        pasteNodes(false, matrix, true);
        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
        m_workspace->updateModules();
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->markDesignSheetChanged();
        m_workspace->refreshPointer();
        m_workspace->selectionChanged();
    }
}

void ClipboardModule::duplicate()
{
    if (m_workspace->getOpenDesignSheet()) {
        copyNodes(0);
        plasma::Matrix<float> matrix;
        matrix.identity();
        m_workspace->resetTools();
        pasteNodes(false, matrix, true);
        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
        m_workspace->updateModules();
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->markDesignSheetChanged();
        m_workspace->refreshPointer();
        m_workspace->selectionChanged();
    }
}

void ClipboardModule::pasteInstance()
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Matrix<float> matrix;
        matrix.identity();
        m_workspace->resetTools();
        pasteNodes(true, matrix, true);
        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
        m_workspace->updateModules();
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->markDesignSheetChanged();
        m_workspace->refreshPointer();
        m_workspace->selectionChanged();
    }
}

void ClipboardModule::duplicateInstance()
{
    if (m_workspace->getOpenDesignSheet()) {
        copyNodes(0);
        plasma::Matrix<float> matrix;
        matrix.identity();
        m_workspace->resetTools();
        pasteNodes(true, matrix, true);
        m_workspace->getOpenDesignSheet()->updateCurrentNode(0);
        m_workspace->updateModules();
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->markDesignSheetChanged();
        m_workspace->refreshPointer();
        m_workspace->selectionChanged();
    }
}

void cloneNodeObjects(plasma::Node *node, std::map<void *, void *> &shapes, std::map<void *, void *> &transformations,
                      std::map<void *, void *> &displays)
{
    if (node->m_shape) {
        plasma::Shape *shape = static_cast<plasma::Shape *>(shapes[node->m_shape]);
        if (!shape) {
            shape = node->m_shape->clone();
            shapes[node->m_shape] = shape;
        }
        node->setShape(shape);
    }
    if (node->m_transformation) {
        plasma::Transformation *transformation =
            static_cast<plasma::Transformation *>(transformations[node->m_transformation]);
        if (!transformation) {
            transformation = node->m_transformation->clone();
            transformations[node->m_transformation] = transformation;
        }
        node->setTransformation(transformation);
    }
    if (node->m_display) {
        plasma::Display *display = static_cast<plasma::Display *>(displays[node->m_display]);
        if (!display) {
            display = node->m_display->clone();
            displays[node->m_display] = display;
        }
        node->setDisplay(display);
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        cloneNodeObjects(*it, shapes, transformations, displays);
    }
}

plasma::Vector<4, float> hsvToRgb(const plasma::Vector<4, float> &hsv)
{
    float saturation = hsv.m_elements[1] * 0.01f;
    float value = 0.01f * hsv.m_elements[2];
    double hue = hsv.m_elements[0] / 60.0f;

    // The conversion is unchecked for out-of-range values and NaNs.
    int sector = (int)hue;
    float fraction = hue - (double)sector;
    float p = (1.0 - saturation) * value;
    float q = (1.0 - fraction * saturation) * value;
    float t = (1.0 - saturation * (1.0 - fraction)) * value;
    switch (sector % 6) {
    case 0:
        return plasma::Vector<4, float>(value, t, p, hsv.m_elements[3]);
    case 1:
        return plasma::Vector<4, float>(q, value, p, hsv.m_elements[3]);
    case 2:
        return plasma::Vector<4, float>(p, value, t, hsv.m_elements[3]);
    case 3:
        return plasma::Vector<4, float>(p, q, value, hsv.m_elements[3]);
    case 4:
        return plasma::Vector<4, float>(t, p, value, hsv.m_elements[3]);
    case 5:
        return plasma::Vector<4, float>(value, p, q, hsv.m_elements[3]);
    default:
        return plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, hsv.m_elements[3]);
    }
}
