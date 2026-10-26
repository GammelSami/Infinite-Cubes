#include "TextModule.h"
#include "plasma/Transformation.h"
#include "plasma/Edit.h"
#include "FormatModule.h" // The edit box filters.
#include "FontWidget.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "plasma/ScalableFont.h"
#include "plasma/Button.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <sstream>
#include "TextInterface.h"
#include "Operations.h"

// Returns the shape as text when its type is SHAPE_TEXT.
static inline plasma::TextShape *asTextShape(plasma::Shape *shape)
{
    return shape && shape->getType() == plasma::SHAPE_TEXT ? static_cast<plasma::TextShape *>(shape) : 0;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
TextModule::TextModule()
    : m_fontName(L"arial.ttf")
{
    m_pixelSize = 16.0f;
    m_strokeWidth = 0.0f;
    m_spacing = 0.0f;
    m_lineSpacing = 0.0f;
    m_wrapWidth = 200.0f;
    m_verticalAlignment = plasma::ALIGN_TOP;
    m_horizontalAlignment = plasma::ALIGN_LEFT;
    m_trueType = true;
    m_wrap = false;
    {
        std::wstring name(L"module:text");
        std::wstring kind(L"sidewidget");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 275.0f, 140.0f, g_workspace.m_scalableNode,
                                                           kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Text", true);
    m_orientation = ORIENTATION_HORIZONTAL;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void TextModule::build()
{
    std::list<ModuleWidgetItem> items;
    addComboButton(L"Font", L"text:font", L"text:font:listbox", 200, 30, 300, 300, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:reload", L"button2", 0, 0, 30, 30));
    addPanel(items, L"", 3);
    placeIcon(L"text:reload", L"icon:reload");
    items.clear();
    items.push_back(ModuleWidgetItem(L"TTF", L"text:type:truetype", L"radiobutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L"PLG", L"text:type:plasma", L"radiobutton2", 30, 0, 30, 30));
    addPanel(items, L"", 3);
    m_panel->setChecked(L"text:type:truetype", true, false, true);
    newRow(3);
    addWidget(L"Size:", L"", L"static", 45, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:size", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"text:size:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"text:size", &plasma::g_unsignedFloatFilter, true);
    addWidget(L"Stroke:", L"", L"static", 45, 20, 5);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:stroke", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"text:stroke:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"text:stroke", &plasma::g_unsignedFloatFilter, true);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:horizontal:left", L"radiobutton3", 0, 0, 30, 22));
    items.push_back(ModuleWidgetItem(L" ", L"text:horizontal:center", L"radiobutton3", 0, 22, 30, 23));
    items.push_back(ModuleWidgetItem(L" ", L"text:horizontal:right", L"radiobutton3", 0, 45, 30, 22));
    addPanel(items, L"", 5);
    placeIcon(L"text:horizontal:left", L"icon:horizontal:left");
    placeIcon(L"text:horizontal:right", L"icon:horizontal:right");
    placeIcon(L"text:horizontal:center", L"icon:horizontal:center");
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:vertical:top", L"radiobutton3", 0, 0, 30, 22));
    items.push_back(ModuleWidgetItem(L" ", L"text:vertical:center", L"radiobutton3", 0, 22, 30, 23));
    items.push_back(ModuleWidgetItem(L" ", L"text:vertical:bottom", L"radiobutton3", 0, 45, 30, 22));
    addPanel(items, L"", 3);
    placeIcon(L"text:vertical:top", L"icon:vertical:top");
    placeIcon(L"text:vertical:center", L"icon:vertical:center");
    placeIcon(L"text:vertical:bottom", L"icon:vertical:bottom");
    m_extent.m_elements[1] -= 46;
    newRow(3);
    newRow(3);
    addWidget(L"Spacing:", L"", L"static", 45, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:spacing", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"text:spacing:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"text:spacing", &plasma::g_floatFilter, true);
    newRow(3);
    addWidget(L"Line:", L"", L"static", 45, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:line", L"editbox", 0, 0, 40, 20));
    items.push_back(ModuleWidgetItem(L" ", L"text:line:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 40, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    getPanel()->getNode()->setEditFilter(L"text:line", &plasma::g_unsignedFloatFilter, true);
    m_cursor.m_elements[1] -= 10;
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:wrap", L"checkbutton2", 0, 0, 30, 30));
    items.push_back(ModuleWidgetItem(L" ", L"text:wrap:width", L"editbox", 30, 0, 50, 30));
    items.push_back(ModuleWidgetItem(L" ", L"text:wrap:width:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 80, 0,
                                     20, 30));
    addPanel(items, L"", 9);
    getPanel()->getNode()->setEditFilter(L"text:wrap:width", &plasma::g_floatFilter, true);
    placeIcon(L"text:wrap", L"icon:linebreak");
    fitWidth(10);
    addZoomBar();
    addApplyBar(L"text", 0);
    createZoomSequences();
    m_fontWidget = new FontWidget(&m_workspace->m_engine, m_workspace, m_workspace->m_engine.createNode(0, 0, 0, 0));
    m_workspace->m_engine.m_root->moveIntoWidget(L"text:font:listbox:list", m_fontWidget, true);

    // The font search box goes above the font list.
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"text:font:search:filter", L"editbox", 0, 0, 150, 30));
    items.push_back(ModuleWidgetItem(L" ", L"text:font:search:clear", L"button2", 150, 0, 30, 30));
    addPanelAt(items, L"text:font:search", 0, -30);
    placeIcon(L"text:font:search:filter", L"icon:search");
    placeIcon(L"text:font:search:clear", L"icon:cancel");
    plasma::Node *icon = getPanel()->getNode()->findNode(L"icon:search");
    if (icon) {
        icon->m_transformation->m_translation.value().m_elements[0] -= 60.0f;
        icon->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    std::list<plasma::Edit *> edits;
    getPanel()->getNode()->findNode(L"text:font:search:filter")->collectWidgets(edits);
    if (edits.size()) {
        edits.front()->m_node->m_transformation->m_translation.value().m_elements[0] += 23.0f;
        edits.front()->m_node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }
    m_panel->setText(L"text:font:search:filter", L"", true);
    getPanel()->getNode()->connectWidget(L"text:font:search:filter", plasma::WIDGET_EVENT_TEXT_CHANGED, this,
                                         &TextModule::filterFonts, true);
    getPanel()->getNode()->connectWidget(L"text:font:search:clear", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                         &TextModule::clearFontSearch, true);
    getPanel()
        ->getNode()
        ->findNode(L"text:font:search")
        ->setParent(m_workspace->m_guiNode->findNode(L"text:font:listbox"));
    getPanel()->getNode()->findNode(L"text:font:search")->initializeWidgets();
    plasma::Widget *fontList = getPanel()->getNode()->findWidget(L"text:font:listbox");
    if (fontList)
        fontList->disconnect(plasma::WIDGET_EVENT_POINTER_WHEEL, true);
    findFonts();
    updatePanel();
    getPanel()->getNode()->connectWidget(L"text:size", plasma::WIDGET_EVENT_FOCUS_LOST, this, &TextModule::editSize,
                                         true);
    getPanel()->getNode()->connectWidget(L"text:size:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &TextModule::dragSize, true);
    getPanel()->getNode()->connectWidget(L"text:size:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, m_workspace,
                                         &Workspace::endModifySelected, true);
    getPanel()->getNode()->connectWidget(L"text:size:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::stepSize, true);
    getPanel()->getNode()->connectWidget(L"text:size:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::stepSize, true);
    getPanel()->getNode()->connectWidget(L"text:stroke", plasma::WIDGET_EVENT_FOCUS_LOST, this, &TextModule::editStroke,
                                         true);
    getPanel()->getNode()->connectWidget(L"text:stroke:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &TextModule::dragStroke, true);
    getPanel()->getNode()->connectWidget(L"text:stroke:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, m_workspace,
                                         &Workspace::endModifySelected, true);
    getPanel()->getNode()->connectWidget(L"text:stroke:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::stepStroke, true);
    getPanel()->getNode()->connectWidget(L"text:stroke:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::stepStroke, true);
    getPanel()->getNode()->connectWidget(L"text:spacing", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &TextModule::editSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:spacing:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &TextModule::dragSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:spacing:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, m_workspace,
                                         &Workspace::endModifySelected, true);
    getPanel()->getNode()->connectWidget(L"text:spacing:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::stepSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:spacing:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::stepSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:line", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &TextModule::editLineSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:line:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &TextModule::dragLineSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:line:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, m_workspace,
                                         &Workspace::endModifySelected, true);
    getPanel()->getNode()->connectWidget(L"text:line:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::stepLineSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:line:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::stepLineSpacing, true);
    getPanel()->getNode()->connectWidget(L"text:wrap:width", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                         &TextModule::editWrapWidth, true);
    getPanel()->getNode()->connectWidget(L"text:wrap:width:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                         &TextModule::dragWrapWidth, true);
    getPanel()->getNode()->connectWidget(L"text:wrap:width:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, m_workspace,
                                         &Workspace::endModifySelected, true);
    getPanel()->getNode()->connectWidget(L"text:wrap:width:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::stepWrapWidth, true);
    getPanel()->getNode()->connectWidget(L"text:wrap:width:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::stepWrapWidth, true);
    getPanel()->getNode()->connectWidget(L"text:horizontal:left", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseLeft, true);
    getPanel()->getNode()->connectWidget(L"text:horizontal:left", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::applyLeft, true);
    getPanel()->getNode()->connectWidget(L"text:horizontal:center", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseCenter, true);
    getPanel()->getNode()->connectWidget(L"text:horizontal:center", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::applyCenter, true);
    getPanel()->getNode()->connectWidget(L"text:horizontal:right", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseRight, true);
    getPanel()->getNode()->connectWidget(L"text:horizontal:right", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::applyRight, true);
    getPanel()->getNode()->connectWidget(L"text:vertical:top", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseTop, true);
    getPanel()->getNode()->connectWidget(L"text:vertical:top", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::applyTop, true);
    getPanel()->getNode()->connectWidget(L"text:vertical:center", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseMiddle, true);
    getPanel()->getNode()->connectWidget(L"text:vertical:center", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::applyMiddle, true);
    getPanel()->getNode()->connectWidget(L"text:vertical:bottom", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseBottom, true);
    getPanel()->getNode()->connectWidget(L"text:vertical:bottom", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::applyBottom, true);
    getPanel()->getNode()->connectWidget(L"text:wrap", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseWrap, true);
    getPanel()->getNode()->connectWidget(L"text:wrap", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                         &TextModule::chooseWrap, true);
    getPanel()->getNode()->connectWidget(L"text:apply", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &TextModule::apply,
                                         true);
    getPanel()->getNode()->connectWidget(L"text:pick", plasma::WIDGET_EVENT_POINTER_PRESSED, this, &TextModule::pick,
                                         true);
    getPanel()->getNode()->connectWidget(L"text:type:truetype", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseFontType, true);
    getPanel()->getNode()->connectWidget(L"text:type:plasma", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::chooseFontType, true);
    getPanel()->getNode()->connectWidget(L"text:font", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::findFonts, true);
    m_panel->setChecked(L"text:apply:auto", true, false, true);
    m_panel->setChecked(L"text:pick:auto", true, false, true);
    getPanel()->getNode()->connectWidget(L"text:reload", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                         &TextModule::reloadFont, true);
}

void TextModule::chooseFontType()
{
    m_trueType = m_panel->isChecked(L"text:type:truetype", true);
}

void TextModule::reloadFont()
{
    m_workspace->m_engine.getFontEngine()->reloadFont(m_fontName);
    m_workspace->markSheetsChanged();
}

void TextModule::updateSize()
{
    if (m_pixelSize < 0.0f)
        m_pixelSize = 0.0f;
    std::wstringstream text;
    text << m_pixelSize;
    m_panel->setText(L"text:size", text.str(), true);
}

void TextModule::updateStroke()
{
    if (m_strokeWidth < 0.0f)
        m_strokeWidth = 0.0f;
    std::wstringstream text;
    text << m_strokeWidth;
    m_panel->setText(L"text:stroke", text.str(), true);
}

void TextModule::updateSpacing()
{
    std::wstringstream text;
    text << m_spacing;
    m_panel->setText(L"text:spacing", text.str(), true);
}

void TextModule::updateLineSpacing()
{
    std::wstringstream text;
    text << m_lineSpacing;
    m_panel->setText(L"text:line", text.str(), true);
}

void TextModule::updateWrapWidth()
{
    std::wstringstream text;
    text << m_wrapWidth;
    m_panel->setText(L"text:wrap:width", text.str(), true);
}

void TextModule::applySize()
{
    if (m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                float inverseScale = 1.0f / getTextScale(it->second);
                if (text) {
                    double scaledPixelSize = m_pixelSize * inverseScale;
                    if (scaledPixelSize != text->m_shape->m_pixelSize) {
                        text->m_shape->m_pixelSize = (float)scaledPixelSize;
                        text->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        text->m_hasChanges = true;
                    }
                }
            }
            ++it;
        }
    }
}

void TextModule::applyStroke()
{
    if (m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                float inverseScale = 1.0f / getTextScale(it->second);
                if (text) {
                    double scaledStrokeRadius = m_strokeWidth * 0.5 * inverseScale;
                    if (scaledStrokeRadius != text->m_shape->m_strokeRadius) {
                        text->m_shape->m_strokeRadius = (float)scaledStrokeRadius;
                        text->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        text->m_hasChanges = true;
                    }
                }
            }
            ++it;
        }
    }
}

void TextModule::applySpacing()
{
    if (m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                float inverseScale = 1.0f / getTextScale(it->second);
                if (text) {
                    double scaledLetterSpacing = m_spacing * inverseScale;
                    if (scaledLetterSpacing != text->m_shape->m_spacing) {
                        text->m_shape->m_spacing = (float)scaledLetterSpacing;
                        text->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        text->m_hasChanges = true;
                    }
                }
            }
            ++it;
        }
    }
}

void TextModule::applyLineSpacing()
{
    if (m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                float inverseScale = 1.0f / getTextScale(it->second);
                if (text) {
                    double scaledLineSpacing = m_lineSpacing * inverseScale;
                    if (scaledLineSpacing != text->m_shape->m_lineSpacing) {
                        text->m_shape->m_lineSpacing = (float)scaledLineSpacing;
                        text->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        text->m_hasChanges = true;
                    }
                }
            }
            ++it;
        }
    }
}

void TextModule::applyWrapWidth()
{
    if (m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                float inverseScale = 1.0f / getTextScale(it->second);
                if (text) {
                    double scaledWrapWidth = m_wrapWidth * inverseScale;
                    if (scaledWrapWidth != text->m_shape->m_wrapWidth) {
                        text->m_shape->m_wrapWidth = (float)scaledWrapWidth;
                        text->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        text->m_hasChanges = true;
                    }
                }
            }
            ++it;
        }
    }
}

void TextModule::applyWrap()
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                if (text) {
                    if ((((unsigned)text->getShape()->m_flags >> plasma::TEXT_WRAP) & 1) != m_wrap) {
                        TextState before;
                        text->captureState(before);
                        plasma::TextShape *shape = text->getShape();
                        int flags = shape->m_flags;
                        shape->m_flags = m_wrap ? flags | (1 << plasma::TEXT_WRAP) : flags & ~(1 << plasma::TEXT_WRAP);
                        text->getShape()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        TextState after;
                        text->captureState(after);
                        m_workspace->getOpenDesignSheet()->addOperation(
                            new TextOperation(m_workspace->getOpenDesignSheet(), "change wrap", text, before, after));
                    }
                }
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}

TextState::~TextState() {}

void TextModule::applyVerticalAlignment()
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                if (text) {
                    int flags = text->getShape()->m_flags;
                    int alignment = (flags & (1 << plasma::TEXT_MIDDLE))
                                        ? plasma::ALIGN_MIDDLE
                                        : ((char)flags & (1u << plasma::TEXT_BOTTOM)) >> plasma::TEXT_BOTTOM;
                    if (alignment != m_verticalAlignment) {
                        // The text keeps its place: the node moves by the change of its minimum ("align text").
                        plasma::Vector<2, float> minimumBefore = it->first->getMinimum(0);
                        TextState textBefore;
                        text->captureState(textBefore);
                        text->getShape()->setVerticalAlignment(m_verticalAlignment);
                        text->getShape()->updateBounds(it->first->m_matrix);
                        text->getShape()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        plasma::Vector<2, float> minimumAfter = it->first->getMinimum(0);
                        TextState textAfter;
                        text->captureState(textAfter);
                        NodeState nodeBefore;
                        it->second->saveState(nodeBefore);
                        plasma::Vector<2, float> offset = minimumBefore - minimumAfter;
                        plasma::Vector<2, float> &translation = it->first->m_transformation->m_translation.value();
                        translation.m_elements[0] = offset.m_elements[0] + translation.m_elements[0];
                        translation.m_elements[1] = translation.m_elements[1] + offset.m_elements[1];
                        it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        NodeState nodeAfter;
                        it->second->saveState(nodeAfter);
                        m_workspace->getOpenDesignSheet()->addOperation(new NodeOperation(
                            m_workspace->getOpenDesignSheet(), "align text", it->second, nodeBefore, nodeAfter));
                        m_workspace->getOpenDesignSheet()->addOperation(
                            new TextOperation(m_workspace->getOpenDesignSheet(), "change vertical alignment", text,
                                              textBefore, textAfter));
                    }
                }
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void TextModule::applyHorizontalAlignment()
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected && !it->first->isFixed()) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                if (text) {
                    int flags = text->getShape()->m_flags;
                    int alignment = (flags & (1 << plasma::TEXT_CENTER))
                                        ? plasma::ALIGN_CENTER
                                        : ((char)flags & (1u << plasma::TEXT_RIGHT)) >> plasma::TEXT_RIGHT;
                    if (alignment != m_horizontalAlignment) {
                        // The text keeps its place: the node moves by the change of its minimum ("align text").
                        plasma::Vector<2, float> minimumBefore = it->first->getMinimum(0);
                        TextState textBefore;
                        text->captureState(textBefore);
                        text->getShape()->setHorizontalAlignment(m_horizontalAlignment);
                        text->getShape()->updateBounds(it->first->m_matrix);
                        text->getShape()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        plasma::Vector<2, float> minimumAfter = it->first->getMinimum(0);
                        TextState textAfter;
                        text->captureState(textAfter);
                        NodeState nodeBefore;
                        it->second->saveState(nodeBefore);
                        plasma::Vector<2, float> offset = minimumBefore - minimumAfter;
                        plasma::Vector<2, float> &translation = it->first->m_transformation->m_translation.value();
                        translation.m_elements[0] = offset.m_elements[0] + translation.m_elements[0];
                        translation.m_elements[1] = translation.m_elements[1] + offset.m_elements[1];
                        it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        NodeState nodeAfter;
                        it->second->saveState(nodeAfter);
                        m_workspace->getOpenDesignSheet()->addOperation(new NodeOperation(
                            m_workspace->getOpenDesignSheet(), "align text", it->second, nodeBefore, nodeAfter));
                        m_workspace->getOpenDesignSheet()->addOperation(
                            new TextOperation(m_workspace->getOpenDesignSheet(), "change horizontal alignment", text,
                                              textBefore, textAfter));
                    }
                }
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void TextModule::editSize()
{
    m_pixelSize = (float)atof(narrowString(getPanel()->getNode()->getText(L"text:size", true)).c_str());
    updateSize();
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applySize();
        m_workspace->endModifySelected();
    }
}

void TextModule::dragSize(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_pixelSize = (float)((int)(m_dragPosition * 0.25) + m_dragStart);
        updateSize();
        if (m_panel->isChecked(L"text:apply:auto", true))
            applySize();
    }
}

void TextModule::stepSize(plasma::Widget *sender)
{
    m_pixelSize = (float)((double)pressDragButton(sender) + m_pixelSize);
    updateSize();
    beginDrag(m_pixelSize);
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applySize();
    }
}

void TextModule::editStroke()
{
    m_strokeWidth = (float)atof(narrowString(getPanel()->getNode()->getText(L"text:stroke", true)).c_str());
    updateStroke();
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        // Unlike the other edit boxes, the stroke updates the selected sheet nodes directly instead of through
        // BeginModifySelected.
        m_workspace->getOpenDesignSheet()->saveSelectedStates();
        applyStroke();
        m_workspace->endModifySelected();
    }
}

void TextModule::dragStroke(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_strokeWidth = (float)((int)(m_dragPosition * 0.25) + m_dragStart);
        updateStroke();
        if (m_panel->isChecked(L"text:apply:auto", true))
            applyStroke();
    }
}

void TextModule::stepStroke(plasma::Widget *sender)
{
    m_strokeWidth = (float)((double)pressDragButton(sender) + m_strokeWidth);
    updateStroke();
    beginDrag(m_strokeWidth);
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applyStroke();
    }
}

void TextModule::editSpacing()
{
    m_spacing = (float)atof(narrowString(getPanel()->getNode()->getText(L"text:spacing", true)).c_str());
    updateSpacing();
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applySpacing();
        m_workspace->endModifySelected();
    }
}

void TextModule::dragSpacing(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_spacing = (float)((int)(m_dragPosition * 0.25) + m_dragStart);
        updateSpacing();
        if (m_panel->isChecked(L"text:apply:auto", true))
            applySpacing();
    }
}

void TextModule::stepSpacing(plasma::Widget *sender)
{
    m_spacing = (float)((double)pressDragButton(sender) + m_spacing);
    updateSpacing();
    beginDrag(m_spacing);
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applySpacing();
    }
}

void TextModule::editLineSpacing()
{
    m_lineSpacing = (float)atof(narrowString(getPanel()->getNode()->getText(L"text:line", true)).c_str());
    updateLineSpacing();
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applyLineSpacing();
        m_workspace->endModifySelected();
    }
}

void TextModule::dragLineSpacing(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_lineSpacing = (float)((int)(m_dragPosition * 0.25) + m_dragStart);
        updateLineSpacing();
        if (m_panel->isChecked(L"text:apply:auto", true))
            applyLineSpacing();
    }
}

void TextModule::stepLineSpacing(plasma::Widget *sender)
{
    m_lineSpacing = (float)((double)pressDragButton(sender) + m_lineSpacing);
    updateLineSpacing();
    beginDrag(m_lineSpacing);
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applyLineSpacing();
    }
}

void TextModule::editWrapWidth()
{
    m_wrapWidth = (float)atof(narrowString(getPanel()->getNode()->getText(L"text:wrap:width", true)).c_str());
    updateWrapWidth();
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applyWrapWidth();
        m_workspace->endModifySelected();
    }
}

void TextModule::dragWrapWidth(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_wrapWidth = (float)((int)(m_dragPosition * 0.25) + m_dragStart);
        updateWrapWidth();
        if (m_panel->isChecked(L"text:apply:auto", true))
            applyWrapWidth();
    }
}

void TextModule::stepWrapWidth(plasma::Widget *sender)
{
    m_wrapWidth = (float)((double)pressDragButton(sender) + m_wrapWidth);
    updateWrapWidth();
    beginDrag(m_wrapWidth);
    if (m_panel->isChecked(L"text:apply:auto", true) && m_workspace->getOpenDesignSheet()) {
        m_workspace->beginModifySelected();
        applyWrapWidth();
    }
}

void TextModule::chooseLeft()
{
    m_horizontalAlignment = plasma::ALIGN_LEFT;
    if (m_panel->isChecked(L"text:apply:auto", true))
        applyHorizontalAlignment();
}

void TextModule::chooseCenter()
{
    m_horizontalAlignment = plasma::ALIGN_CENTER;
    if (m_panel->isChecked(L"text:apply:auto", true))
        applyHorizontalAlignment();
}

void TextModule::chooseRight()
{
    m_horizontalAlignment = plasma::ALIGN_RIGHT;
    if (m_panel->isChecked(L"text:apply:auto", true))
        applyHorizontalAlignment();
}

void TextModule::chooseTop()
{
    m_verticalAlignment = plasma::ALIGN_TOP;
    if (m_panel->isChecked(L"text:apply:auto", true))
        applyVerticalAlignment();
}

void TextModule::chooseMiddle()
{
    m_verticalAlignment = plasma::ALIGN_MIDDLE;
    if (m_panel->isChecked(L"text:apply:auto", true))
        applyVerticalAlignment();
}

void TextModule::chooseBottom()
{
    m_verticalAlignment = plasma::ALIGN_BOTTOM;
    if (m_panel->isChecked(L"text:apply:auto", true))
        applyVerticalAlignment();
}

void TextModule::applyLeft()
{
    m_horizontalAlignment = plasma::ALIGN_LEFT;
    applyHorizontalAlignment();
}

void TextModule::applyCenter()
{
    m_horizontalAlignment = plasma::ALIGN_CENTER;
    applyHorizontalAlignment();
}

void TextModule::applyRight()
{
    m_horizontalAlignment = plasma::ALIGN_RIGHT;
    applyHorizontalAlignment();
}

void TextModule::applyTop()
{
    m_verticalAlignment = plasma::ALIGN_TOP;
    applyVerticalAlignment();
}

void TextModule::applyMiddle()
{
    m_verticalAlignment = plasma::ALIGN_MIDDLE;
    applyVerticalAlignment();
}

void TextModule::applyBottom()
{
    m_verticalAlignment = plasma::ALIGN_BOTTOM;
    applyVerticalAlignment();
}

void TextModule::pick()
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        pickNode(m_workspace->getOpenDesignSheet()->m_currentNode);
        updatePanel();
    }
}

void TextModule::apply()
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                ShapeInterface *shapeInterface = it->second->getShapeInterface();
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                if (text) {
                    int flags = text->getShape()->m_flags;
                    int alignment = (flags & (1 << plasma::TEXT_MIDDLE))
                                        ? plasma::ALIGN_MIDDLE
                                        : ((char)flags & (1u << plasma::TEXT_BOTTOM)) >> plasma::TEXT_BOTTOM;

                    // The Apply gates every property change on the vertical alignment.
                    if (alignment != m_verticalAlignment) {
                        plasma::Vector<2, float> minimumBefore = it->first->getMinimum(0);
                        TextState textBefore;
                        text->captureState(textBefore);
                        plasma::TextShape *shape = text->getShape();
                        int shapeFlags = shape->m_flags;
                        shape->m_flags =
                            m_wrap ? shapeFlags | (1 << plasma::TEXT_WRAP) : shapeFlags & ~(1 << plasma::TEXT_WRAP);
                        text->getShape()->m_wrapWidth = m_wrapWidth;
                        text->getShape()->m_pixelSize = m_pixelSize;
                        text->getShape()->m_strokeRadius = m_strokeWidth * 0.5;
                        text->getShape()->m_spacing = m_spacing;
                        text->getShape()->m_lineSpacing = m_lineSpacing;
                        text->getShape()->m_fontName = m_fontName;
                        text->getShape()->setVerticalAlignment(m_verticalAlignment);
                        // Both calls use the vertical alignment setter.
                        text->getShape()->setVerticalAlignment(m_horizontalAlignment);
                        text->getShape()->updateBounds(it->first->m_matrix);
                        text->getShape()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        plasma::Vector<2, float> minimumAfter = it->first->getMinimum(0);
                        TextState textAfter;
                        text->captureState(textAfter);
                        NodeState nodeBefore;
                        it->second->saveState(nodeBefore);
                        plasma::Vector<2, float> offset = minimumBefore - minimumAfter;
                        plasma::Vector<2, float> &translation = it->first->m_transformation->m_translation.value();
                        translation.m_elements[0] = offset.m_elements[0] + translation.m_elements[0];
                        translation.m_elements[1] = offset.m_elements[1] + translation.m_elements[1];
                        it->first->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                        NodeState nodeAfter;
                        it->second->saveState(nodeAfter);
                        m_workspace->getOpenDesignSheet()->addOperation(new NodeOperation(
                            m_workspace->getOpenDesignSheet(), "align text", it->second, nodeBefore, nodeAfter));
                        m_workspace->getOpenDesignSheet()->addOperation(
                            new TextOperation(m_workspace->getOpenDesignSheet(), "change vertical alignment", text,
                                              textBefore, textAfter));
                    }
                }
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void TextModule::currentNodeChanged()
{
    if (!m_panel->isChecked(L"text:pick:auto", true))
        return;
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode &&
        !m_workspace->getOpenDesignSheet()->m_currentNode->isFixed())
        pickNode(m_workspace->getOpenDesignSheet()->m_currentNode);
}

float TextModule::getTextScale(NodeInterface *node)
{
    // The node's matrix relative to the sheet's root.
    plasma::Matrix<float> matrix = node->m_sheet->m_root->m_inverseMatrix;
    matrix *= node->m_node->m_matrix;

    // The images of the unit axes (1, 0) and (0, 1).
    plasma::Vector<2, float> xAxis(matrix.m_elements[4] * 0.0f + matrix.m_elements[0],
                                   matrix.m_elements[5] * 0.0f + matrix.m_elements[1]);
    plasma::Vector<2, float> yAxis(matrix.m_elements[4] + matrix.m_elements[0] * 0.0f,
                                   matrix.m_elements[5] + matrix.m_elements[1] * 0.0f);
    float xScale = std::sqrt(xAxis.m_elements[0] * xAxis.m_elements[0] + xAxis.m_elements[1] * xAxis.m_elements[1]);
    float yScale = std::sqrt(yAxis.m_elements[0] * yAxis.m_elements[0] + yAxis.m_elements[1] * yAxis.m_elements[1]);
    return xScale < yScale ? xScale : yScale; // The smaller axis scale.
}

void TextModule::pickNode(plasma::Node *node)
{
    if (node && m_workspace->getOpenDesignSheet()) {
        plasma::TextShape *shape = asTextShape(node->m_shape);
        if (shape) {
            float scale = getTextScale(m_workspace->getOpenDesignSheet()->findSheetNode(node));
            m_pixelSize = shape->m_pixelSize * scale;
            m_strokeWidth = (shape->m_strokeRadius + shape->m_strokeRadius) * scale;
            int flags = shape->m_flags;
            m_verticalAlignment = (flags & (1 << plasma::TEXT_MIDDLE))
                                      ? plasma::ALIGN_MIDDLE
                                      : ((char)flags & (1u << plasma::TEXT_BOTTOM)) >> plasma::TEXT_BOTTOM;
            flags = shape->m_flags;
            m_horizontalAlignment = (flags & (1 << plasma::TEXT_CENTER))
                                        ? plasma::ALIGN_CENTER
                                        : ((char)flags & (1u << plasma::TEXT_RIGHT)) >> plasma::TEXT_RIGHT;
            m_spacing = (float)(shape->m_spacing * (double)scale);
            m_lineSpacing = (float)(shape->m_lineSpacing * (double)scale);
            m_wrapWidth = (float)((double)scale * shape->m_wrapWidth);
            m_wrap = (shape->m_flags & (1 << plasma::TEXT_WRAP)) != 0;
            selectFont(shape->m_fontName, false);
        }
    }
}

void TextModule::showSelectedValues()
{
    // Does nothing apart from constructing an unused string.
    std::wstring unused;
}

void TextModule::clearFontSearch()
{
    m_workspace->getGuiNode()->setText(L"text:font:search:filter", L"", true);
    filterFonts();
}

void TextModule::filterFonts()
{
    std::wstring filter = m_workspace->getGuiNode()->getText(L"text:font:search:filter", true);
    if (filter.empty()) {
        m_filteredFonts = m_fonts;
    } else {
        // The byte toupper on wchar_t retains UB for values outside unsigned char/EOF.
        std::transform(filter.begin(), filter.end(), filter.begin(), toupper);
        m_filteredFonts.erase(m_filteredFonts.begin(), m_filteredFonts.end());
        for (std::vector<std::wstring>::iterator it = m_fonts.begin(); it != m_fonts.end(); ++it) {
            std::wstring caption = m_fontCaptions[*it];
            std::transform(caption.begin(), caption.end(), caption.begin(), toupper);
            if (caption.substr(0, filter.size()) == filter)
                m_filteredFonts.push_back(*it);
        }
    }
    m_fontWidget->refresh();
}

void TextModule::findFonts()
{
    m_trueType = m_panel->isChecked(L"text:type:truetype", true);
    m_fonts.erase(m_fonts.begin(), m_fonts.end());
    m_fontCaptions.clear();

    // The code uses this fixed directory, rather than GetWindowsDirectory.
    addFontDirectory(L"c:\\windows\\fonts");
    const std::string &applicationPath = m_workspace->m_applicationPath;
    addFontDirectory(widenString(applicationPath + "\\fonts"));
    filterFonts();
}

void TextModule::addFontDirectory(const std::wstring &path)
{
    WIN32_FIND_DATAW fontFileData;
    std::wstring pattern = path + L"\\*";
    HANDLE find = FindFirstFileW(pattern.c_str(), &fontFileData);
    std::wstring wanted;
    if (m_trueType)
        wanted = L"TTF";
    else
        wanted = L"PLG";
    if (find != INVALID_HANDLE_VALUE) {
        do {
            if (!(fontFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                std::wstring name = fontFileData.cFileName;
                unsigned dot = name.find_last_of(L'.');
                std::wstring extension = name.substr(dot + 1, name.size() - dot - 1);

                // The byte toupper has UB for non-byte wide characters.
                std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
                if (extension == wanted) {
                    m_fonts.push_back(path + L"\\" + name);
                    plasma::ScalableFont *font = m_workspace->m_engine.getFontEngine()->getFont(m_fonts.back());
                    if (font)
                        m_fontCaptions[m_fonts.back()] = font->m_family + L" " + font->m_style;
                }
            }
        } while (FindNextFileW(find, &fontFileData));
        FindClose(find);
    }
}

void TextModule::selectFont(const std::wstring &path, bool apply)
{
    int slash = (int)path.find_last_of(L'/');
    int backslash = (int)path.find_last_of(L'\\');
    int last = slash > backslash ? slash : backslash;
    std::wstring name = path.substr(last + 1, path.size() - last - 1);
    m_fontName = name;
    updatePanel();
    bool automatic = apply && m_panel->isChecked(L"text:apply:auto", true);
    if (automatic && m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (!(it->first->m_flags & (1 << plasma::NODE_HIDDEN)) && it->second->m_selected &&
                !(it->first->m_flags & (1 << plasma::NODE_FIXED))) {
                ShapeInterface *shapeInterface = it->second->m_shapeInterface;
                TextInterface *text = shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0;
                if (text) {
                    TextState before;
                    text->captureState(before);
                    text->getShape()->m_fontName = m_fontName;
                    text->getShape()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                    TextState after;
                    text->captureState(after);
                    m_workspace->getOpenDesignSheet()->addOperation(
                        new TextOperation(m_workspace->getOpenDesignSheet(), "change font", text, before, after));
                }
            }
            ++it;
        }
        m_workspace->getOpenDesignSheet()->commit();
    }
}

const std::wstring &TextModule::getFontName()
{
    return m_fontName;
}

void TextModule::updatePanel()
{
    plasma::Node *button = m_workspace->m_engine.findNode(L"text:font:combobutton");
    if (button) {
        plasma::Node *caption = button->findNode(L"caption");
        if (caption) {
            plasma::Shape *captionShape = caption->m_shape;
            if (captionShape && captionShape->getType() == plasma::SHAPE_TEXT) {
                plasma::TextShape *shape = static_cast<plasma::TextShape *>(captionShape);
                plasma::ScalableFont *font = m_workspace->m_engine.getFontEngine()->getFont(m_fontName);
                if (font)
                    shape->m_string = font->m_family + L" " + font->m_style;
                else
                    shape->m_string = m_fontName;
                shape->m_fontName = m_fontName;
                shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            }
        }
    }
    switch (m_horizontalAlignment) {
    case plasma::ALIGN_LEFT:
        m_panel->setChecked(L"text:horizontal:left", true, false, true);
        break;
    case plasma::ALIGN_CENTER:
        m_panel->setChecked(L"text:horizontal:center", true, false, true);
        break;
    case plasma::ALIGN_RIGHT:
        m_panel->setChecked(L"text:horizontal:right", true, false, true);
        break;
    }
    switch (m_verticalAlignment) {
    case plasma::ALIGN_TOP:
        m_panel->setChecked(L"text:vertical:top", true, false, true);
        break;
    case plasma::ALIGN_MIDDLE:
        m_panel->setChecked(L"text:vertical:center", true, false, true);
        break;
    case plasma::ALIGN_BOTTOM:
        m_panel->setChecked(L"text:vertical:bottom", true, false, true);
        break;
    }
    std::wstringstream text;
    text << m_pixelSize;
    m_panel->setText(L"text:size", text.str(), true);
    text.str(L"");
    text << m_strokeWidth;
    m_panel->setText(L"text:stroke", text.str(), true);
    text.str(L"");
    text << m_spacing;
    m_panel->setText(L"text:spacing", text.str(), true);
    text.str(L"");
    text << m_lineSpacing;
    m_panel->setText(L"text:line", text.str(), true);
    text.str(L"");
    text << m_wrapWidth;
    m_panel->setText(L"text:wrap:width", text.str(), true);
    getPanel()->getNode()->setChecked(L"text:wrap", m_wrap, false, true);
}

void TextModule::setSize(float size)
{
    if (size < 0.0f)
        size = 0.0f;
    m_pixelSize = size;
    std::wstringstream text;
    text << m_pixelSize;
    m_panel->setText(L"text:size", text.str(), true);
}

void TextModule::setStrokeWidth(float width)
{
    if (width < 0.0f)
        width = 0.0f;
    m_strokeWidth = width;
    std::wstringstream text;
    text << m_strokeWidth;
    m_panel->setText(L"text:stroke", text.str(), true);
}

void TextModule::chooseWrap()
{
    m_wrap = m_panel->isChecked(L"text:wrap", true);
    if (m_panel->isChecked(L"text:apply:auto", true))
        applyWrap();
}

template void plasma::Node::connectWidget<TextModule>(const std::wstring &, int, TextModule *, void (TextModule::*)(),
                                                      bool);

template void plasma::Node::connectWidget<TextModule>(const std::wstring &, int, TextModule *,
                                                      void (TextModule::*)(plasma::Widget *), bool);

namespace plasma {
template<>
void Widget::setConnection<TextModule>(Connection *&connection, TextModule *object, void (TextModule::*method)(),
                                       bool enabledOnly)
{
    if (connection)
        delete connection;
    connection = new MemberFunctionConnection<TextModule>(object, method, enabledOnly);
}
}
