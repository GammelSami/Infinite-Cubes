#include "ProceduralShapeModule.h"
#include "ToolModule.h"
#include "FormatModule.h" // The edit box filters.
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "ColorModule.h"
#include "ColorManagementModule.h"
#include "MeshModule.h"
#include "StrokeWidthModule.h"
#include "plasma/Drawing.h"
#include "plasma/SmoothMeshShape.h"
#include <cmath>
#include <cstdlib>
#include <sstream>

static const double kPi = 3.141592653589793;

static const int StarPresetPointCount = 5;
static const int SpiralPresetSampleCount = 40;
static const int RectangularControlCornerCount = 4;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
ProceduralShapeModule::ProceduralShapeModule()
{
    m_width = 100.0f;
    m_height = 100.0f;
    m_roundness = 20.0f;
    m_innerRoundness = 20.0f;
    m_angle = 0.0f;
    m_corners = RectangularControlCornerCount;
    m_innerAngle = 0.0f;
    m_cornerStyle = CORNERS_SOFT;
    m_innerCornerStyle = CORNERS_SOFT;
    m_innerRadius = 0.5f;
    m_kind = KIND_RECTANGULAR;
    m_shapeBoundary = false;
    {
        std::wstring name(L"module:procedural");
        std::wstring kind(L"bar");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 70.0f, 140.0f, 120.0f, 100.0f,
                                                           g_workspace.m_guiLeftNode, kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Shape", true);
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void ProceduralShapeModule::build()
{
    m_cursor.m_elements[0] = 30;
    m_cursor.m_elements[1] = 25;
    m_margin.m_elements[0] = 30;
    std::list<ModuleWidgetItem> items;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:preset:circle", L"button2", 0, 0, 36, 36));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:preset:rectangle", L"button2", 36, 0, 36, 36));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:preset:triangle", L"button2", 72, 0, 36, 36));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:preset:star", L"button2", 108, 0, 36, 36));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:preset:spiral", L"button2", 144, 0, 36, 36));
    addPanel(items, L"", 0);
    placeIcon(L"procedural:preset:circle", L"icon:circle");
    placeIcon(L"procedural:preset:star", L"icon:star");
    placeIcon(L"procedural:preset:triangle", L"icon:triangle");
    placeIcon(L"procedural:preset:rectangle", L"icon:rectangle");
    placeIcon(L"procedural:preset:spiral", L"icon:spiral");
    newRow(3);
    addWidget(L"Width:", L"", L"static", 60, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:width", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:width:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:width", &plasma::g_floatFilter, true);
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);

    // The link button spans both rows without moving the bottom edge.
    float bottom = m_extent.m_elements[1];
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:link:widthheight", L"checkbutton2", 0, 0, 25, 43));
    addPanel(items, L"", 5);
    placeIcon(L"procedural:link:widthheight", L"icon:link");
    m_extent.m_elements[1] = bottom;
    newRow(3);
    addWidget(L"Height:", L"", L"static", 60, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:height", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:height:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:height", &plasma::g_floatFilter, true);
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);
    newRow(3);
    addWidget(L"Angle:", L"", L"static", 60, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:angle", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:angle:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:angle", &plasma::g_floatFilter, true);
    addWidget(L"\u00b0", L"", L"static", 20, 20, 0);
    newRow(3);
    addWidget(L"Corners:", L"", L"static", 60, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:corners", L"editbox", 0, 0, 50, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:corners:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 50, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:corners", &plasma::g_unsignedIntegerFilter, true);
    newRow(3);
    addWidget(L"Bounds:", L"", L"static", 60, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:boundary:control", L"radiobutton2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:boundary:shape", L"radiobutton2", 25, 0, 25, 25));
    addPanel(items, L"", 0);
    placeIcon(L"procedural:boundary:control", L"icon:bounds:control");
    placeIcon(L"procedural:boundary:shape", L"icon:bounds:shape");
    newRow(3);
    addWidget(L"Curvature:", L"", L"static", 60, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:corners:sharp", L"radiobutton2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:corners:soft", L"radiobutton2", 25, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:corners:round", L"radiobutton2", 50, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:corners:parametric", L"radiobutton2", 75, 0, 25, 25));
    addPanel(items, L"", 0);
    placeIcon(L"procedural:corners:soft", L"icon:corner:soft");
    placeIcon(L"procedural:corners:sharp", L"icon:corner:sharp");
    placeIcon(L"procedural:corners:round", L"icon:corner:round");
    placeIcon(L"procedural:corners:parametric", L"icon:corner:parametric");
    newRow(3);
    addWidget(L"Roundness:", L"", L"static", 60, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:roundness", L"editbox", 0, 0, 70, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:roundness:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 70, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:roundness", &plasma::g_unsignedFloatFilter, true);
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);
    newRow(3);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Rectangular", L"procedural:rectangular", L"radiobutton3", 0, 0, 180, 25));
    items.push_back(ModuleWidgetItem(L"Radial", L"procedural:radial", L"radiobutton3", 0, 25, 180, 25));
    items.push_back(ModuleWidgetItem(L"Spiral", L"procedural:spiral", L"radiobutton3", 0, 50, 180, 25));
    items.push_back(ModuleWidgetItem(L"Star", L"procedural:star", L"radiobutton3", 0, 75, 180, 25));
    items.push_back(ModuleWidgetItem(L"", L" ", L" ", 0, 55, 0, 150));
    addPanel(items, L"", 0);
    m_cursor.m_elements[1] -= 100;
    m_extent.m_elements[1] -= 100;
    newRow(3);
    addWidget(L"Inner curvature:", L"", L"static", 90, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:corners:sharp", L"radiobutton2", 0, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:corners:soft", L"radiobutton2", 25, 0, 25, 25));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:corners:parametric", L"radiobutton2", 50, 0, 25, 25));
    addPanel(items, L"", 0);
    placeIcon(L"procedural:inner:corners:soft", L"icon:corner:soft");
    placeIcon(L"procedural:inner:corners:sharp", L"icon:corner:sharp");
    placeIcon(L"procedural:inner:corners:round", L"icon:corner:round");
    placeIcon(L"procedural:inner:corners:parametric", L"icon:corner:parametric");
    newRow(3);
    addWidget(L"Inner radius:", L"", L"static", 90, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:radius", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:radius:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:inner:radius", &plasma::g_unsignedFloatFilter, true);
    addWidget(L"%", L"", L"static", 20, 20, 0);
    newRow(3);
    addWidget(L"Inner angle:", L"", L"static", 90, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:angle", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:angle:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:inner:angle", &plasma::g_floatFilter, true);
    addWidget(L"\u00b0", L"", L"static", 20, 20, 0);
    newRow(3);
    addWidget(L"Inner roundness:", L"", L"static", 90, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:roundness", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"procedural:inner:roundness:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"procedural:inner:roundness", &plasma::g_unsignedFloatFilter, true);
    addWidget(L"px", L"text:unit", L"static", 20, 20, 0);
    newRow(4);
    items.clear();
    items.push_back(ModuleWidgetItem(L"Reset", L"procedural:reset", L"button2", 0, 0, 180, 30));
    addPanel(items, L"", 0);
    fitWidth(10);
    fitHeight(10);
    addZoomBar();
    getPanel()->getNode()->connectWidget(L"procedural:corners", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editCorners, true);
    getPanel()->getNode()->connectWidget(L"procedural:corners:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragCorners, true);
    getPanel()->getNode()->connectWidget(L"procedural:corners:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepCorners, true);
    getPanel()->getNode()->connectWidget(L"procedural:corners:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::stepCorners, true);
    getPanel()->getNode()->connectWidget(L"procedural:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editWidth, true);
    getPanel()->getNode()->connectWidget(L"procedural:width:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragWidth, true);
    getPanel()->getNode()->connectWidget(L"procedural:width:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"procedural:width:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::stepWidth, true);
    getPanel()->getNode()->connectWidget(L"procedural:height", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editHeight, true);
    getPanel()->getNode()->connectWidget(L"procedural:height:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragHeight, true);
    getPanel()->getNode()->connectWidget(L"procedural:height:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepHeight, true);
    getPanel()->getNode()->connectWidget(L"procedural:height:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::stepHeight, true);
    getPanel()->getNode()->connectWidget(L"procedural:angle", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:angle:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:angle:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:angle:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::stepAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:roundness", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:roundness:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:roundness:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:roundness:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &ProceduralShapeModule::stepRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:angle", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editInnerAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:angle:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragInnerAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:angle:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepInnerAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:angle:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &ProceduralShapeModule::stepInnerAngle, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:roundness", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editInnerRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:roundness:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragInnerRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:roundness:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepInnerRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:roundness:drag",
                                         plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::stepInnerRoundness, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:radius", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &ProceduralShapeModule::editInnerRadius, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:radius:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &ProceduralShapeModule::dragInnerRadius, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:radius:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::stepInnerRadius, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:radius:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &ProceduralShapeModule::stepInnerRadius, true);
    getPanel()->getNode()->connectWidget(L"procedural:spiral", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseSpiral, true);
    getPanel()->getNode()->connectWidget(L"procedural:star", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseStar, true);
    getPanel()->getNode()->connectWidget(L"procedural:star", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::chooseStar, true);
    getPanel()->getNode()->connectWidget(L"procedural:radial", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseRadial, true);
    getPanel()->getNode()->connectWidget(L"procedural:radial", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::chooseRadial, true);
    getPanel()->getNode()->connectWidget(L"procedural:rectangular", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseRectangular, true);
    getPanel()->getNode()->connectWidget(L"procedural:rectangular", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &ProceduralShapeModule::chooseRectangular, true);
    getPanel()->getNode()->connectWidget(L"procedural:corners:soft", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseCornersSoft, true);
    getPanel()->getNode()->connectWidget(L"procedural:corners:sharp", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseCornersSharp, true);
    getPanel()->getNode()->connectWidget(L"procedural:corners:round", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseCornersRound, true);
    getPanel()->getNode()->connectWidget(L"procedural:corners:parametric", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseCornersParametric, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:corners:soft", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseInnerCornersSoft, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:corners:sharp", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseInnerCornersSharp, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:corners:round", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseInnerCornersRound, true);
    getPanel()->getNode()->connectWidget(L"procedural:inner:corners:parametric", plasma::WIDGET_EVENT_POINTER_PRESSED,
                                         this, &ProceduralShapeModule::chooseInnerCornersParametric, true);
    getPanel()->getNode()->connectWidget(L"procedural:boundary:control", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseControlBoundary, true);
    getPanel()->getNode()->connectWidget(L"procedural:boundary:shape", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::chooseShapeBoundary, true);
    getPanel()->getNode()->connectWidget(L"procedural:preset:star", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::presetStar, true);
    getPanel()->getNode()->connectWidget(L"procedural:preset:circle", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::presetCircle, true);
    getPanel()->getNode()->connectWidget(L"procedural:preset:rectangle", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::presetRectangle, true);
    getPanel()->getNode()->connectWidget(L"procedural:preset:triangle", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::presetTriangle, true);
    getPanel()->getNode()->connectWidget(L"procedural:preset:spiral", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::presetSpiral, true);
    getPanel()->getNode()->connectWidget(L"procedural:link:widthheight", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::linkWidthHeight, true);
    getPanel()->getNode()->connectWidget(L"procedural:link:widthheight", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED,
                                         this, &ProceduralShapeModule::linkWidthHeight, true);
    getPanel()->getNode()->connectWidget(L"procedural:reset", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &ProceduralShapeModule::reset, true);
    updateWidgets();
    createZoomSequences();
}

void ProceduralShapeModule::updateWidgets()
{
    if (m_corners < 3)
        m_corners = 3;
    std::wstringstream text;
    text << m_corners;
    getPanel()->getNode()->setText(L"procedural:corners", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_width));
    getPanel()->getNode()->setText(L"procedural:width", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_height));
    getPanel()->getNode()->setText(L"procedural:height", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_angle);
    getPanel()->getNode()->setText(L"procedural:angle", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_roundness));
    getPanel()->getNode()->setText(L"procedural:roundness", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_innerAngle);
    getPanel()->getNode()->setText(L"procedural:inner:angle", text.str(), true);
    text.str(L"");
    text << roundToHundredths((float)(m_innerRadius * 100.0));
    getPanel()->getNode()->setText(L"procedural:inner:radius", text.str(), true);
    text.str(L"");
    text << roundToHundredths(m_workspace->fromDesignSheetInternalUnits(m_innerRoundness));
    getPanel()->getNode()->setText(L"procedural:inner:roundness", text.str(), true);

    getPanel()->getNode()->setChecked(L"procedural:boundary:shape", m_shapeBoundary, false, true);
    getPanel()->getNode()->setChecked(L"procedural:boundary:control", !m_shapeBoundary, false, true);
    switch (m_cornerStyle) {
    case CORNERS_SOFT:
        getPanel()->getNode()->setChecked(L"procedural:corners:soft", true, false, true);
        break;
    case CORNERS_SHARP:
        getPanel()->getNode()->setChecked(L"procedural:corners:sharp", true, false, true);
        break;
    case CORNERS_ROUND:
        getPanel()->getNode()->setChecked(L"procedural:corners:round", true, false, true);
        break;
    case CORNERS_PARAMETRIC:
        getPanel()->getNode()->setChecked(L"procedural:corners:parametric", true, false, true);
        break;
    }
    switch (m_innerCornerStyle) {
    case CORNERS_SOFT:
        getPanel()->getNode()->setChecked(L"procedural:inner:corners:soft", true, false, true);
        break;
    case CORNERS_SHARP:
        getPanel()->getNode()->setChecked(L"procedural:inner:corners:sharp", true, false, true);
        break;
    case CORNERS_ROUND:
        getPanel()->getNode()->setChecked(L"procedural:inner:corners:round", true, false, true);
        break;
    case CORNERS_PARAMETRIC:
        getPanel()->getNode()->setChecked(L"procedural:inner:corners:parametric", true, false, true);
        break;
    }

    // The kind's radio button is only pressed when it is not already.
    switch (m_kind) {
    case KIND_RECTANGULAR:
        if (!getPanel()->getNode()->isChecked(L"procedural:rectangular", true))
            getPanel()->getNode()->setChecked(L"procedural:rectangular", true, false, true);
        break;
    case KIND_RADIAL:
        if (!getPanel()->getNode()->isChecked(L"procedural:radial", true))
            getPanel()->getNode()->setChecked(L"procedural:radial", true, false, true);
        break;
    case KIND_STAR:
        if (!getPanel()->getNode()->isChecked(L"procedural:star", true))
            getPanel()->getNode()->setChecked(L"procedural:star", true, false, true);
        break;
    case KIND_SPIRAL:
        if (!getPanel()->getNode()->isChecked(L"procedural:spiral", true))
            getPanel()->getNode()->setChecked(L"procedural:spiral", true, false, true);
        break;
    }

    // Only a star has inner corners (the code leaves the "soft" button of the inner curvature enabled).
    getPanel()->getNode()->setEnabled(L"procedural:inner:angle", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:angle:drag", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:radius", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:radius:drag", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:roundness", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:roundness:drag", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:corners:round", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:corners:sharp", m_kind == KIND_STAR, true);
    getPanel()->getNode()->setEnabled(L"procedural:inner:corners:parametric", m_kind == KIND_STAR, true);
    m_workspace->m_toolModule->m_proceduralTool.invalidatePreview();
}

void ProceduralShapeModule::generateShape(plasma::SmoothMeshShape *shape)
{
    // The faces: one through every vertex, or for a star a face through the inner corners plus one per point.
    std::vector<std::vector<int>> faces;

    // The vertices of each corner: the outer corner plus its parametric neighbours or round midpoint, and a star's
    // inner corner likewise.
    int innerVertices = 0;
    int outerVertices = 1;
    if (m_kind == KIND_STAR) {
        innerVertices = 1;
        if (m_innerCornerStyle == CORNERS_PARAMETRIC)
            innerVertices = 3;
        if (m_innerCornerStyle == CORNERS_ROUND)
            innerVertices = 1;
    }
    if (m_cornerStyle == CORNERS_PARAMETRIC && m_kind != KIND_SPIRAL)
        outerVertices = 3;
    if (m_cornerStyle == CORNERS_ROUND && m_kind != KIND_SPIRAL)
        outerVertices = (m_kind != KIND_STAR) + 1;
    int perCorner = innerVertices + outerVertices;
    int count = m_corners * perCorner;

    shape->m_vertexPositions.resize(count);
    shape->m_vertexColors.resize(count);
    shape->m_strokeColors.resize(count);
    shape->m_strokeWidths.resize(count);
    shape->m_vertexParameters.resize(count, 0.0f);
    shape->m_vertexFlags.resize(count);

    if (m_kind == KIND_STAR) {
        faces.resize(m_corners + 1);
        faces[0].resize(m_corners);
        int first = perCorner / 2;
        for (int i = 0; i < m_corners; i++) {
            faces[0][i] = first % count;
            faces[i + 1].resize(perCorner + 1);
            int vertex = first;
            for (int j = 0; j <= perCorner; j++)
                faces[i + 1][j] = vertex++ % count;
            first += perCorner;
        }
    } else {
        faces.resize(1);
        faces[0].resize(count);
        for (int i = 0; i < count; i++)
            faces[0][i] = i;
    }

    for (int i = 0; i < count; i++) {
        shape->m_vertexFlags[i] = plasma::VERTEX_SMOOTH;
        shape->m_vertexParameters[i] = 0.0f;
    }

    // The outer corners on the unit circle, the first at the top; a spiral winds inwards instead.
    float start = kPi / m_corners + kPi / 2 + 0.0;
    for (int i = 0; i < m_corners; i++) {
        int vertex = i * perCorner;
        float angle = (float)i / (float)m_corners * kPi * 2.0 + start;
        float radius = 1.0f;
        if (m_kind == KIND_SPIRAL) {
            radius = 1.0f - (float)i / ((float)m_corners - 1.0f);
            if (radius > 1.0f)
                radius = 1.0f;
            if (i == 0)
                radius *= (double)0.95f;
            angle = (float)i / 10.0 * kPi * 2.0;
        }
        shape->m_vertexPositions.value()[vertex] = plasma::Vector<2, float>(std::cos(angle), std::sin(angle)) * radius;
        shape->m_vertexColors.value()[vertex] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
        shape->m_strokeColors.value()[vertex] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
        shape->m_strokeWidths.value()[vertex] = 0.0f;
        // Vertex types: 0 soft, 1 sharp, 3 arc, rounded corner (its radius in the vertex parameters).
        if (m_cornerStyle == CORNERS_SHARP) {
            shape->m_vertexFlags[vertex] = plasma::VERTEX_SHARP;
        } else if (m_cornerStyle == CORNERS_PARAMETRIC && m_kind != KIND_SPIRAL) {
            shape->m_vertexFlags[vertex] = plasma::VERTEX_ROUND | (1 << plasma::VERTEX_ROUNDED_CORNER);
            shape->m_vertexParameters[vertex] = m_roundness;
        } else if (m_cornerStyle == CORNERS_ROUND) {
            shape->m_vertexFlags[vertex] = plasma::VERTEX_ROUND;
        } else {
            shape->m_vertexFlags[vertex] = plasma::VERTEX_SMOOTH;
        }
    }

    // A star's inner corners, halfway between the outer ones.
    if (m_kind == KIND_STAR) {
        for (int i = 0; i < m_corners; i++) {
            int vertex = i * perCorner + perCorner / 2;
            float position = i + 0.5;
            float angle = position / m_corners * kPi * 2.0 + (kPi * m_innerAngle / 180.0 + start);
            float radius = m_innerRadius;
            shape->m_vertexPositions.value()[vertex] =
                plasma::Vector<2, float>(std::cos(angle), std::sin(angle)) * radius;
            shape->m_vertexColors.value()[vertex] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            shape->m_strokeColors.value()[vertex] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
            shape->m_strokeWidths.value()[vertex] = 0.0f;
            if (m_innerCornerStyle == CORNERS_SHARP) {
                shape->m_vertexFlags[vertex] = plasma::VERTEX_SHARP;
            } else if (m_innerCornerStyle == CORNERS_PARAMETRIC && m_kind != KIND_SPIRAL) {
                shape->m_vertexFlags[vertex] = plasma::VERTEX_ROUND | (1 << plasma::VERTEX_ROUNDED_CORNER);
                shape->m_vertexParameters[vertex] = m_innerRoundness;
            } else {
                shape->m_vertexFlags[vertex] = plasma::VERTEX_SMOOTH;
            }
        }
    }

    // Parametric outer corners: a sharp vertex on each side of the corner, the roundness away from it towards the
    // neighbouring corners.
    if (m_cornerStyle == CORNERS_PARAMETRIC && m_kind != KIND_SPIRAL) {
        for (int i = 0; i < m_corners; i++) {
            int vertex = i * perCorner;
            plasma::Vector<2, float> sum = shape->m_vertexPositions.value()[vertex] +
                                           shape->m_vertexPositions.value()[(vertex + perCorner) % count];
            float length = sum.m_elements[1] * sum.m_elements[1] + sum.m_elements[0] * sum.m_elements[0];
            length = std::sqrt(length);
            float t = m_roundness / length;
            shape->m_vertexPositions.value()[vertex + 1] =
                shape->m_vertexPositions.value()[vertex] * (1 - t) +
                shape->m_vertexPositions.value()[(vertex + perCorner) % count] * t;
            shape->m_vertexColors.value()[vertex + 1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            shape->m_strokeColors.value()[vertex + 1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
            shape->m_strokeWidths.value()[vertex + 1] = 0.0f;
            shape->m_vertexFlags[vertex + 1] = plasma::VERTEX_SHARP;
            int previous = (vertex + count - 1) % count;
            shape->m_vertexPositions.value()[previous] =
                shape->m_vertexPositions.value()[vertex] * (1 - t) +
                shape->m_vertexPositions.value()[(vertex + count - perCorner) % count] * t;
            shape->m_vertexColors.value()[previous] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            shape->m_strokeColors.value()[previous] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
            shape->m_strokeWidths.value()[previous] = 0.0f;
            shape->m_vertexFlags[previous] = plasma::VERTEX_SHARP;
        }
    }

    // Round outer corners (not a star's): a soft vertex halfway to the next corner.
    if (m_cornerStyle == CORNERS_ROUND && m_kind != KIND_SPIRAL && m_kind != KIND_STAR) {
        for (int i = 0; i < m_corners; i++) {
            int vertex = i * perCorner;
            shape->m_vertexPositions.value()[vertex + 1] =
                shape->m_vertexPositions.value()[vertex] * 0.5f +
                shape->m_vertexPositions.value()[(vertex + perCorner) % count] * 0.5f;
            shape->m_vertexColors.value()[vertex + 1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            shape->m_strokeColors.value()[vertex + 1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
            shape->m_strokeWidths.value()[vertex + 1] = 0.0f;
            shape->m_vertexFlags[vertex + 1] = plasma::VERTEX_SMOOTH;
        }
    }

    // Parametric inner corners of a star, like the outer ones.
    if (m_kind == KIND_STAR && m_innerCornerStyle == CORNERS_PARAMETRIC) {
        for (int i = 0; i < m_corners; i++) {
            int vertex = i * perCorner + perCorner / 2;
            float t = m_innerRoundness;
            shape->m_vertexPositions.value()[vertex + 1] =
                shape->m_vertexPositions.value()[vertex] * (1 - t) +
                shape->m_vertexPositions.value()[(vertex + perCorner / 2) % count] * t;
            shape->m_vertexColors.value()[vertex + 1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            shape->m_strokeColors.value()[vertex + 1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
            shape->m_strokeWidths.value()[vertex + 1] = 0.0f;
            shape->m_vertexFlags[vertex + 1] = plasma::VERTEX_SHARP;
            int previous = (vertex + count - 1) % count;
            shape->m_vertexPositions.value()[previous] =
                shape->m_vertexPositions.value()[vertex] * (1 - t) +
                shape->m_vertexPositions.value()[(vertex + count - perCorner / 2) % count] * t;
            shape->m_vertexColors.value()[previous] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
            shape->m_strokeColors.value()[previous] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f);
            shape->m_strokeWidths.value()[previous] = 0.0f;
            shape->m_vertexFlags[previous] = plasma::VERTEX_SHARP;
        }
    }

    // Fit the outline to the size: a rectangle stretches its corners' bounds to the width and height; the other kinds
    // scale the unit circle by the larger of the two. The texture spans the bounds.
    if ((int)shape->m_vertexPositions.m_values[0]->size() > 0) {
        plasma::Vector<2, float> minimum = shape->m_vertexPositions.value()[0];
        plasma::Vector<2, float> maximum = shape->m_vertexPositions.value()[0];
        if (m_cornerStyle == CORNERS_PARAMETRIC) {
            for (int i = 0; i < m_corners; i++) {
                minimum = plasma::Vector<2, float>(
                    min(shape->m_vertexPositions.value()[i * perCorner].m_elements[0], minimum.m_elements[0]),
                    min(shape->m_vertexPositions.value()[i * perCorner].m_elements[1], minimum.m_elements[1]));
                maximum = plasma::Vector<2, float>(
                    max(shape->m_vertexPositions.value()[i * perCorner].m_elements[0], maximum.m_elements[0]),
                    max(shape->m_vertexPositions.value()[i * perCorner].m_elements[1], maximum.m_elements[1]));
            }
        } else {
            for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
                minimum = plasma::Vector<2, float>(
                    min(shape->m_vertexPositions.value()[i].m_elements[0], minimum.m_elements[0]),
                    min(shape->m_vertexPositions.value()[i].m_elements[1], minimum.m_elements[1]));
                maximum = plasma::Vector<2, float>(
                    max(shape->m_vertexPositions.value()[i].m_elements[0], maximum.m_elements[0]),
                    max(shape->m_vertexPositions.value()[i].m_elements[1], maximum.m_elements[1]));
            }
        }
        plasma::Vector<2, float> offset(0.0f, 0.0f);
        plasma::Vector<2, float> scale;
        if (m_kind == KIND_STAR || m_kind == KIND_RADIAL || m_kind == KIND_SPIRAL) {
            float size = max(m_width, m_height);
            float half = 0.5 * size;
            scale = plasma::Vector<2, float>(half, half);
        } else {
            scale = plasma::Vector<2, float>(m_width, m_height) / (maximum - minimum);
            offset = (minimum + maximum) * 0.5f;
        }
        shape->m_vertexTexCoords.resize(shape->m_vertexPositions.m_values[0]->size());
        for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
            shape->m_vertexTexCoords.value()[i] = (shape->m_vertexPositions.value()[i] - minimum) / (maximum - minimum);
            shape->m_vertexPositions.value()[i] -= offset;
            plasma::Vector<2, float> &scaled = shape->m_vertexPositions.value()[i];
            scaled.m_elements[0] *= scale.m_elements[0];
            scaled.m_elements[1] *= scale.m_elements[1];
        }
    }

    // The current colours, stroke width and mesh settings.
    shape->m_extrusionFrontColors.resize(shape->m_vertexPositions.m_values[0]->size());
    shape->m_extrusionBackColors.resize(shape->m_vertexPositions.m_values[0]->size());
    float strokeWidth = m_workspace->m_strokeWidthModule->m_width;
    for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
        shape->m_vertexColors.value()[i] = m_workspace->getColorModule()->m_fillColor;
        shape->m_strokeColors.value()[i] = m_workspace->getColorModule()->m_strokeColor;
        shape->m_extrusionFrontColors.value()[i] = m_workspace->getColorModule()->m_extrusionColor;
        shape->m_extrusionBackColors.value()[i] = m_workspace->getColorModule()->m_extrusionColor;
        shape->m_strokeWidths.value()[i] = strokeWidth;
    }
    shape->setSubdivisions(m_workspace->m_meshModule->m_subdivisions, false);
    shape->setStrokeCapType(m_workspace->m_meshModule->m_strokeCap);
    shape->setStrokeJointType(m_workspace->m_meshModule->m_strokeJoint);
    if (m_workspace->m_meshModule->m_extruded)
        shape->m_flags |= (1 << plasma::MESH_EXTRUDED);
    else
        shape->m_flags &= ~(1 << plasma::MESH_EXTRUDED);
    shape->setStrokePattern(m_workspace->m_meshModule->m_strokePattern);
    shape->setStrokeDash(m_workspace->m_meshModule->m_strokeDash);
    shape->setStrokeGap(m_workspace->m_meshModule->m_strokeGap);
    shape->setFaces(faces, true);
    shape->applyVertexParameters();

    // The texture spans the vertex bounds.
    plasma::Matrix<float> identity;
    identity.identity();
    bool first = true;
    plasma::Vector<2, float> vertexMinimum;
    plasma::Vector<2, float> vertexMaximum;
    shape->getVertexBounds(vertexMinimum, vertexMaximum, identity, first);
    for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++)
        shape->m_vertexTexCoords.value()[i] =
            (shape->m_vertexPositions.value()[i] - vertexMinimum) / (vertexMaximum - vertexMinimum);

    if (m_kind == KIND_SPIRAL) {
        shape->m_flags |= (1 << plasma::MESH_STROKE) | (1 << plasma::MESH_NO_FILL) | (1 << plasma::MESH_OPEN);
        shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    } else {
        m_workspace->m_meshModule->setShapeModeFlags(shape);
    }

    // "procedural:boundary:shape": the drawn shape, not its control vertices, fills the size.
    if (m_shapeBoundary) {
        if (m_kind != KIND_RECTANGULAR) {
            float radius = 0.0f;
            if (plasma::SmoothMeshShape::hasFill(shape->m_flags) && shape->m_fillDrawing) {
                for (int i = 0; i < (int)shape->m_fillDrawing->m_pathVertices.size(); i++) {
                    const plasma::Vector<2, float> &vertex = shape->m_fillDrawing->m_pathVertices[i];
                    float distance = std::sqrt(vertex.m_elements[1] * vertex.m_elements[1] +
                                               vertex.m_elements[0] * vertex.m_elements[0]);
                    if (radius < distance)
                        radius = distance;
                }
            }
            if (plasma::SmoothMeshShape::hasStroke(shape->m_flags) && shape->m_strokeDrawing) {
                for (int i = 0; i < (int)shape->m_strokeDrawing->m_pathVertices.size(); i++) {
                    const plasma::Vector<2, float> &vertex = shape->m_strokeDrawing->m_pathVertices[i];
                    float distance = std::sqrt(vertex.m_elements[1] * vertex.m_elements[1] +
                                               vertex.m_elements[0] * vertex.m_elements[0]);
                    if (radius < distance)
                        radius = distance;
                }
            }

            // The control vertices' radius, computed but then replaced by the size.
            float size = 0.0f;
            for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
                const plasma::Vector<2, float> &position = shape->m_vertexPositions.value()[i];
                float distance = std::sqrt(position.m_elements[1] * position.m_elements[1] +
                                           position.m_elements[0] * position.m_elements[0]);
                if (size < distance)
                    size = distance;
            }
            size = max(m_width, m_height);
            float factor = size * 0.5 / radius;
            for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
                plasma::Vector<2, float> &position = shape->m_vertexPositions.value()[i];
                position.m_elements[0] = factor * position.m_elements[0];
                position.m_elements[1] = factor * position.m_elements[1];
            }
        } else {
            plasma::Vector<2, float> drawnMinimum;
            plasma::Vector<2, float> drawnMaximum;
            bool empty = true;
            shape->getTransformedBounds(drawnMinimum, drawnMaximum, identity, empty);
            plasma::Vector<2, float> controlMinimum;
            plasma::Vector<2, float> controlMaximum;
            empty = true;
            shape->getVertexBounds(controlMinimum, controlMaximum, identity, empty);

            // The stroke's outer half lies beyond the vertices.
            if (plasma::SmoothMeshShape::hasStroke(shape->m_flags)) {
                float half = strokeWidth * 0.5;
                drawnMinimum += plasma::Vector<2, float>(half, half);
                drawnMaximum -= plasma::Vector<2, float>(half, half);
                controlMinimum += plasma::Vector<2, float>(half, half);
                controlMaximum -= plasma::Vector<2, float>(half, half);
            }
            plasma::Vector<2, float> ratio = (controlMaximum - controlMinimum) / (drawnMaximum - drawnMinimum);
            for (int i = 0; i < (int)shape->m_vertexPositions.m_values[0]->size(); i++) {
                shape->m_vertexPositions.value()[i] -= drawnMinimum;
                plasma::Vector<2, float> &scaled = shape->m_vertexPositions.value()[i];
                scaled.m_elements[0] = ratio.m_elements[0] * scaled.m_elements[0];
                scaled.m_elements[1] = ratio.m_elements[1] * scaled.m_elements[1];
                plasma::Vector<2, float> &moved = shape->m_vertexPositions.value()[i];
                moved.m_elements[0] = controlMinimum.m_elements[0] + moved.m_elements[0];
                moved.m_elements[1] = controlMinimum.m_elements[1] + moved.m_elements[1];
            }
        }
    }
    shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
}

void ProceduralShapeModule::chooseShapeBoundary()
{
    m_shapeBoundary = true;
    updateWidgets();
}

void ProceduralShapeModule::chooseControlBoundary()
{
    m_shapeBoundary = false;
    updateWidgets();
}

void ProceduralShapeModule::chooseCornersSoft()
{
    m_cornerStyle = CORNERS_SOFT;
    updateWidgets();
}

void ProceduralShapeModule::chooseCornersSharp()
{
    m_cornerStyle = CORNERS_SHARP;
    updateWidgets();
}

void ProceduralShapeModule::chooseCornersParametric()
{
    m_cornerStyle = CORNERS_PARAMETRIC;
    updateWidgets();
}

void ProceduralShapeModule::chooseCornersRound()
{
    m_cornerStyle = CORNERS_ROUND;
    updateWidgets();
}

void ProceduralShapeModule::chooseInnerCornersSoft()
{
    m_innerCornerStyle = CORNERS_SOFT;
    updateWidgets();
}

void ProceduralShapeModule::chooseInnerCornersSharp()
{
    m_innerCornerStyle = CORNERS_SHARP;
    updateWidgets();
}

void ProceduralShapeModule::chooseInnerCornersParametric()
{
    m_innerCornerStyle = CORNERS_PARAMETRIC;
    updateWidgets();
}

void ProceduralShapeModule::chooseInnerCornersRound()
{
    m_innerCornerStyle = CORNERS_ROUND;
    updateWidgets();
}

void ProceduralShapeModule::chooseStar()
{
    m_kind = KIND_STAR;
    updateWidgets();
}

void ProceduralShapeModule::chooseSpiral()
{
    m_kind = KIND_SPIRAL;
    updateWidgets();
}

void ProceduralShapeModule::chooseRadial()
{
    m_kind = KIND_RADIAL;
    updateWidgets();
}

void ProceduralShapeModule::chooseRectangular()
{
    m_kind = KIND_RECTANGULAR;
    updateWidgets();
}

void ProceduralShapeModule::editCorners()
{
    m_corners = atoi(narrowString(getPanel()->getNode()->getText(L"procedural:corners", true)).c_str());
    updateWidgets();
}

void ProceduralShapeModule::dragCorners(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();

        // A corner per four pixels of movement.
        m_corners = (int)(m_dragPosition * 0.25 + m_dragStart);
        updateWidgets();
    }
}

void ProceduralShapeModule::stepCorners(plasma::Widget *sender)
{
    m_corners += pressDragButton(sender);
    updateWidgets();
    beginDrag((float)m_corners);
}

void ProceduralShapeModule::editAngle()
{
    m_angle = (float)atof(narrowString(getPanel()->getNode()->getText(L"procedural:angle", true)).c_str());
    updateWidgets();
}

void ProceduralShapeModule::dragAngle(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_angle -= movement;
        updateWidgets();
    }
}

void ProceduralShapeModule::stepAngle(plasma::Widget *sender)
{
    m_angle += pressDragButton(sender);
    updateWidgets();
}

void ProceduralShapeModule::editInnerAngle()
{
    m_innerAngle = (float)atof(narrowString(getPanel()->getNode()->getText(L"procedural:inner:angle", true)).c_str());
    updateWidgets();
}

void ProceduralShapeModule::dragInnerAngle(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_innerAngle -= movement;
        updateWidgets();
    }
}

void ProceduralShapeModule::stepInnerAngle(plasma::Widget *sender)
{
    m_innerAngle += pressDragButton(sender);
    updateWidgets();
}

void ProceduralShapeModule::editInnerRadius()
{
    // The panel shows the inner radius in percent.
    m_innerRadius =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"procedural:inner:radius", true)).c_str()) * 0.01f);
    updateWidgets();
}

void ProceduralShapeModule::dragInnerRadius(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_innerRadius -= movement * 0.01f;
        updateWidgets();
    }
}

void ProceduralShapeModule::stepInnerRadius(plasma::Widget *sender)
{
    m_innerRadius += pressDragButton(sender) * 0.01f;
    updateWidgets();
}

void ProceduralShapeModule::editRoundness()
{
    m_roundness = m_workspace->toDesignSheetInternalUnits(
        (float)atof(narrowString(getPanel()->getNode()->getText(L"procedural:roundness", true)).c_str()));
    updateWidgets();
}

void ProceduralShapeModule::dragRoundness(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_roundness -= m_workspace->toDesignSheetInternalUnits(movement);
        updateWidgets();
    }
}

void ProceduralShapeModule::stepRoundness(plasma::Widget *sender)
{
    m_roundness += m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender));
    updateWidgets();
}

void ProceduralShapeModule::editInnerRoundness()
{
    m_innerRoundness = m_workspace->toDesignSheetInternalUnits(
        (float)atof(narrowString(getPanel()->getNode()->getText(L"procedural:inner:roundness", true)).c_str()));
    updateWidgets();
}

void ProceduralShapeModule::dragInnerRoundness(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_innerRoundness -= m_workspace->toDesignSheetInternalUnits(movement);
        updateWidgets();
    }
}

void ProceduralShapeModule::stepInnerRoundness(plasma::Widget *sender)
{
    m_innerRoundness += m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender));
    updateWidgets();
}

void ProceduralShapeModule::editWidth()
{
    m_width = m_workspace->toDesignSheetInternalUnits(
        (float)atof(narrowString(getPanel()->getNode()->getText(L"procedural:width", true)).c_str()));
    if (m_panel->isChecked(L"procedural:link:widthheight", true))
        m_height = m_width;
    updateWidgets();
}

void ProceduralShapeModule::dragWidth(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_width -= m_workspace->toDesignSheetInternalUnits(movement);
        if (m_panel->isChecked(L"procedural:link:widthheight", true))
            m_height = m_width;
        updateWidgets();
    }
}

void ProceduralShapeModule::stepWidth(plasma::Widget *sender)
{
    m_width += m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender));
    if (m_panel->isChecked(L"procedural:link:widthheight", true))
        m_height = m_width;
    updateWidgets();
}

void ProceduralShapeModule::editHeight()
{
    m_height = m_workspace->toDesignSheetInternalUnits(
        (float)atof(narrowString(getPanel()->getNode()->getText(L"procedural:height", true)).c_str()));
    if (m_panel->isChecked(L"procedural:link:widthheight", true))
        m_width = m_height;
    updateWidgets();
}

void ProceduralShapeModule::dragHeight(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        float movement =
            m_workspace->m_engine.m_pointer.m_elements[1] - m_workspace->m_engine.m_previousPointer.m_elements[1];
        m_height -= m_workspace->toDesignSheetInternalUnits(movement);
        if (m_panel->isChecked(L"procedural:link:widthheight", true))
            m_width = m_height;
        updateWidgets();
    }
}

void ProceduralShapeModule::stepHeight(plasma::Widget *sender)
{
    m_height += m_workspace->toDesignSheetInternalUnits((float)pressDragButton(sender));
    if (m_panel->isChecked(L"procedural:link:widthheight", true))
        m_width = m_height;
    updateWidgets();
}

void ProceduralShapeModule::setSize(const plasma::Vector<2, float> &size)
{
    m_width = size.m_elements[0];
    m_height = size.m_elements[1];
    if (m_panel->isChecked(L"procedural:link:widthheight", true))
        m_height = m_width;
    updateWidgets();
}

void ProceduralShapeModule::linkWidthHeight()
{
    if (m_panel->isChecked(L"procedural:link:widthheight", true))
        m_height = m_width;
    updateWidgets();
}

void ProceduralShapeModule::setAngle(float angle)
{
    m_angle = angle;
    updateWidgets();
}

void ProceduralShapeModule::reset()
{
    m_roundness = 20.0f;
    m_innerRoundness = 20.0f;
    m_angle = 0.0f;
    m_innerAngle = 0.0f;
    m_innerRadius = 0.5f;
    updateWidgets();
}

void ProceduralShapeModule::presetStar()
{
    m_innerAngle = 0.0f;
    m_innerRadius = 0.5f;
    m_corners = StarPresetPointCount;
    m_kind = KIND_STAR;
    m_innerCornerStyle = CORNERS_SHARP;
    m_cornerStyle = CORNERS_SHARP;
    updateWidgets();
}

void ProceduralShapeModule::presetSpiral()
{
    m_innerAngle = 0.0f;
    m_innerRadius = 0.5f;
    m_corners = SpiralPresetSampleCount;
    m_kind = KIND_SPIRAL;
    m_innerCornerStyle = CORNERS_SOFT;
    m_cornerStyle = CORNERS_SOFT;
    updateWidgets();
}

void ProceduralShapeModule::presetCircle()
{
    m_innerAngle = 0.0f;
    m_innerRadius = 0.5f;
    m_corners = RectangularControlCornerCount;
    m_kind = KIND_RECTANGULAR;
    m_innerCornerStyle = CORNERS_SOFT;
    m_cornerStyle = CORNERS_SOFT;
    updateWidgets();
}

void ProceduralShapeModule::presetTriangle()
{
    m_innerAngle = 0.0f;
    m_innerRadius = 0.5f;
    m_corners = 3;
    m_kind = KIND_RECTANGULAR;
    m_innerCornerStyle = CORNERS_SHARP;
    m_cornerStyle = CORNERS_SHARP;
    updateWidgets();
}

void ProceduralShapeModule::presetRectangle()
{
    m_innerAngle = 0.0f;
    m_innerRadius = 0.5f;
    m_corners = RectangularControlCornerCount;
    m_kind = KIND_RECTANGULAR;
    m_innerCornerStyle = CORNERS_SHARP;
    m_cornerStyle = CORNERS_SHARP;
    updateWidgets();
}

void ProceduralShapeModule::unitChanged()
{
    updateWidgets();
}
