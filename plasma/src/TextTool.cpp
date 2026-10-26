#include "TextTool.h"
#include "DesignSheet.h"
#include "plasma/ListWidget.h"
#include "TextInterface.h"
#include "ColorModule.h"
#include "CreateNodeOperation.h"
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include <GL/glew.h>
#include "plasma/Widget.h"
#include "TextModule.h"
#include "Operations.h"
#include <windows.h>

TextTool::TextTool()
    : Tool(TOOL_TEXT)
{}

// The pointer went down. Over a text: Ctrl deselects a selected one; else it is edited (Shift first adds it to the
// selection) with the cursor at the pointer. Elsewhere a new text shape in the text module's font, size, alignment
// and wrapping and the colour module's colours is placed at the snapped pointer ("create text") and edited.
void TextTool::pointerPressed()
{
    if (!m_workspace->getDesignSheet())
        return;
    Workspace *workspace = m_workspace;
    if (workspace->m_hoveredTextInterface) {
        if (workspace->m_engine.isKeyDown(VK_CONTROL) &&
            workspace->m_hoveredTextInterface->getSheetNode()->m_selected) {
            workspace->m_hoveredTextInterface->m_node->setSelected(false);
            if (m_workspace->m_hoveredTextInterface == m_workspace->getEditingTextInterface())
                m_workspace->setTextInterface(0);
            m_workspace->getDesignSheet()->addOperation(new NodeSelectionOperation(
                m_workspace->getDesignSheet(), "deselect node", m_workspace->m_hoveredTextInterface->m_node, false));
            m_workspace->getDesignSheet()->updateCurrentNode(0);
            m_workspace->updateModules();
        } else if (workspace->m_engine.isKeyDown(VK_SHIFT) && !workspace->m_hoveredTextInterface->m_node->m_selected) {
            workspace->getHoveredSheetNode()->setSelected(true);
            m_workspace->getDesignSheet()->addOperation(new NodeSelectionOperation(
                m_workspace->getDesignSheet(), "select node", m_workspace->m_hoveredTextInterface->m_node, true));
            m_workspace->getDesignSheet()->m_currentNode = m_workspace->m_hoveredTextInterface->m_node->getNode();
            m_workspace->setTextInterface(m_workspace->m_hoveredTextInterface);
            m_workspace->updateModules();
            m_workspace->m_hoveredTextInterface->setCursorToMouse();
            m_workspace->m_hoveredTextInterface->clearSelection();
        } else {
            workspace->setTextInterface(workspace->m_hoveredTextInterface);
            m_workspace->getDesignSheet()->m_currentNode = m_workspace->m_hoveredTextInterface->m_node->getNode();
            m_workspace->updateModules();
            m_workspace->m_hoveredTextInterface->setCursorToMouse();
            m_workspace->m_hoveredTextInterface->clearSelection();
        }
    } else {
        plasma::TextShape *text = m_workspace->m_engine.createTextShape(L"", L"Text");
        text->m_fontName = m_workspace->m_textModule->getFontName();
        text->m_pixelSize = m_workspace->m_textModule->getSize();
        text->m_strokeRadius = m_workspace->m_textModule->getStrokeWidth() * 0.5;
        text->m_color.value() = m_workspace->getColorModule()->m_fillColor;
        text->m_strokeColor.value() = m_workspace->getColorModule()->m_strokeColor;
        text->m_extrusionColor.value() = m_workspace->getColorModule()->m_extrusionColor;
        switch (m_workspace->m_textModule->m_horizontalAlignment) {
        case plasma::ALIGN_LEFT:
            text->m_flags &= ~plasma::TEXT_HORIZONTAL_MASK;
            break;
        case plasma::ALIGN_RIGHT:
            text->m_flags = (text->m_flags & ~plasma::TEXT_HORIZONTAL_MASK) | (1 << plasma::TEXT_RIGHT);
            break;
        case plasma::ALIGN_CENTER:
            text->m_flags = (text->m_flags & ~plasma::TEXT_HORIZONTAL_MASK) | (1 << plasma::TEXT_CENTER);
            break;
        }
        switch (m_workspace->m_textModule->m_verticalAlignment) {
        case plasma::ALIGN_TOP:
            text->m_flags &= ~plasma::TEXT_VERTICAL_MASK;
            break;
        case plasma::ALIGN_BOTTOM:
            text->m_flags = (text->m_flags & ~plasma::TEXT_VERTICAL_MASK) | (1 << plasma::TEXT_BOTTOM);
            break;
        case plasma::ALIGN_MIDDLE:
            text->m_flags = (text->m_flags & ~plasma::TEXT_VERTICAL_MASK) | (1 << plasma::TEXT_MIDDLE);
            break;
        }
        if (m_workspace->m_textModule->getWrap())
            text->m_flags |= (1 << plasma::TEXT_WRAP);
        text->m_wrapWidth = m_workspace->m_textModule->getWrapWidth();
        text->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        plasma::Node *node =
            m_workspace->m_engine.createNode(0, text, 0, m_workspace->getDesignSheet()->m_root, L"Text");
        plasma::Node *root = m_workspace->getDesignSheet()->m_root;
        plasma::Widget::getTranslation(node->m_transformation) =
            root->m_inverseMatrix.transform(m_workspace->getSnappedPointer(false));
        node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        NodeInterface *sheetNode = m_workspace->getDesignSheet()->createSheetNode(node);
        sheetNode->setSelected(true);
        m_workspace->getDesignSheet()->addOperation(
            new CreateNodeOperation(m_workspace->getDesignSheet(), "create text", node));
        m_workspace->moveSelectedNodes(node, m_workspace->getDesignSheet()->m_editRoot);
        ShapeInterface *shapeInterface = sheetNode->m_shapeInterface;
        m_workspace->setTextInterface(shapeInterface && shapeInterface->m_type == INTERFACE_TEXT
                                          ? static_cast<TextInterface *>(shapeInterface)
                                          : 0);
        m_workspace->getDesignSheet()->m_currentNode = node;
        m_workspace->updateModules();
        m_workspace->getDesignSheet()->commit();
    }
    m_workspace->redraw();
}

// Makes the shown text under the pointer (within the tool radius) the hovered one
// (Workspace::m_hoveredTextInterface); only selected texts count unless Shift is held.
void findHoveredTextInterface(DesignSheet *sheet, plasma::Node *node)
{
    if (!node->isShown())
        return;
    NodeInterface *sheetNode = sheet->findSheetNode(node);
    if (sheetNode->m_selected || sheet->m_workspace->m_engine.isKeyDown(VK_SHIFT)) {
        ShapeInterface *shapeInterface = sheetNode->m_shapeInterface;
        if (shapeInterface && shapeInterface->m_type == INTERFACE_TEXT) {
            TextInterface *text = static_cast<TextInterface *>(shapeInterface);
            if (text->m_shape->intersects(sheet->m_workspace->getSceneMousePosition(),
                                          sheet->m_workspace->getToolRadius(), node->m_matrix, node->m_inverseMatrix))
                sheet->m_workspace->m_hoveredTextInterface = text;
        }
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        findHoveredTextInterface(sheet, *it);
}

// The pointer moved: finds the text under it; dragging with the left button extends the edited text's selection.
void TextTool::pointerMoved(int x, int y)
{
    if (m_workspace->getOpenDesignSheet()) {
        plasma::Node *root = m_workspace->getOpenDesignSheet()->m_root;
        findHoveredTextInterface(m_workspace->getOpenDesignSheet(), root);
        Workspace *workspace = m_workspace;
        if (workspace->m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) && workspace->getEditingTextInterface())
            workspace->getEditingTextInterface()->moveCursorSelecting();
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Draws an "A" in the
// text module's font, size and colours at the snapped pointer (the view's middle in a modal panel) and a grey and
// white corner mark at its baseline.
void TextTool::draw()
{
    if (m_workspace->getDesignSheet() && !m_workspace->m_engine.isKeyDown(VK_SHIFT) &&
        (m_workspace->m_guiPointerNode || !m_workspace->m_hoveredTextInterface)) {
        glUseProgram(0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
        glPushMatrix();
        glLoadMatrixf(m_workspace->getDesignSheet()->getRoot()->getMatrix().m_elements);
        plasma::Node *root = m_workspace->getDesignSheet()->getRoot();
        plasma::Vector<2, float> position = root->getInverseMatrix().transform(m_workspace->getSnappedPointer(false));
        if (m_workspace->m_guiPointerNode) {
            plasma::Vector<2, float> halfSize = m_workspace->getDesignSheet()->m_panel->getViewSize();
            halfSize.m_elements[0] *= 0.5;
            halfSize.m_elements[1] *= 0.5;
            plasma::Vector<2, float> center = m_workspace->getDesignSheet()->m_panel->getContentPosition() + halfSize;
            position = m_workspace->getDesignSheet()->getRoot()->getInverseMatrix() * center;
        }
        glTranslatef(position.m_elements[0], position.m_elements[1], 0.0f);
        {
            std::wstring text(L"A");
            TextModule *module = m_workspace->m_textModule;
            m_workspace->m_engine.getFontEngine()->renderText(
                module->getFontName(), text, module->m_spacing, module->m_lineSpacing, 0.0f, 0.0f, module->m_pixelSize,
                (float)(module->m_strokeWidth * 0.5), m_workspace->m_colorModule->m_fillColor.m_elements,
                m_workspace->m_colorModule->m_strokeColor.m_elements,
                m_workspace->m_colorModule->m_extrusionColor.m_elements, 0, -1.0f, false);
        }
        glLoadIdentity();
        position = m_workspace->getDesignSheet()->getRoot()->getMatrix().transform(position);
        float pixelSize = m_workspace->m_textModule->m_pixelSize;
        plasma::Vector<2, float> size((float)(pixelSize * 0.5), pixelSize);
        size = m_workspace->getDesignSheet()->getRoot()->getMatrix().transformDirection(size);
        double lower = position.m_elements[1] - size.m_elements[1];
        double right = position.m_elements[0] + size.m_elements[0];
        glDisable(GL_LINE_SMOOTH);
        glLineWidth(3.0f);
        glColor3f(0.5f, 0.5f, 0.5f);
        glBegin(GL_LINES);
        glVertex2f(position.m_elements[0], (float)(lower - 1.0));
        glVertex2f(position.m_elements[0], position.m_elements[1] + 1.0f);
        glVertex2f(position.m_elements[0], position.m_elements[1]);
        glVertex2f((float)(right + 1.0), position.m_elements[1]);
        glEnd();
        glLineWidth(1.0f);
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_LINES);
        glVertex2f(position.m_elements[0], (float)lower);
        glVertex2f(position.m_elements[0], position.m_elements[1]);
        glVertex2f(position.m_elements[0], position.m_elements[1]);
        glVertex2f((float)right, position.m_elements[1]);
        glEnd();
        glPopMatrix();
    }
}

void TextTool::deactivate()
{
    m_workspace->setTextInterface(0);
}

void TextTool::fillContextMenu()
{
    std::list<plasma::Node *> selected;
    m_workspace->getOpenDesignSheet()->getSelectedNodes(selected);
    {
        std::wstring caption(L"Resize Tool Radius...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::resizeToolRadius, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    {
        std::wstring caption(L"Convert to Mesh");
        plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &TextTool::textToMesh, true, true);
        if ((int)selected.size() <= 0)
            item->setEnabled(false);
    }
}

void TextTool::textToMesh()
{
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            plasma::Node *node = it->first;
            if (it->second->m_selected && node->isShown() && node->m_shape &&
                node->m_shape->getType() == plasma::SHAPE_TEXT) {
                plasma::TextShape *text = static_cast<plasma::TextShape *>(node->m_shape);
                if (text->m_font) {
                    plasma::Transformation *transformation = node->m_transformation->clone();
                    plasma::Node *group = m_workspace->m_engine.createNode(transformation, 0, 0, 0, text->getName());
                    node->m_parent->insertChild(group, node->getNextSibling());
                    group->m_flags |= (1 << plasma::NODE_ADAPT) | (1 << plasma::NODE_FIXED);
                    std::wstring string = text->m_string.value();
                    m_workspace->m_engine.getFontEngine()->lock();
                    if (text->m_flags & (1 << plasma::TEXT_WRAP)) {
                        plasma::Matrix<float> matrix = m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix;
                        matrix *= node->m_matrix;
                        text->m_font->prepare(matrix, text->m_pixelSize, text->m_strokeRadius, text->m_spacing,
                                              text->m_lineSpacing, false);
                        text->m_font->wrapText(string, text->m_wrapWidth);
                    }
                    text->m_font->convertTextToNodes(
                        group, string, text->m_pixelSize, text->m_strokeRadius, text->m_spacing, text->m_lineSpacing,
                        text->m_color.value().m_elements, text->m_strokeColor.value().m_elements,
                        m_workspace->m_colorModule->m_extrusionColor.m_elements, text->m_flags);
                    m_workspace->m_engine.getFontEngine()->unlock();
                    createGroupBoundary(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 0.0f), group);
                    m_workspace->getOpenDesignSheet()->createSheetNodes(group);
                    m_workspace->getOpenDesignSheet()->addOperation(
                        new CreateNodeOperation(m_workspace->getOpenDesignSheet(), "text to mesh", group));
                    for (std::list<plasma::Node *>::iterator child = group->m_children.begin();
                         child != group->m_children.end(); ++child) {
                        m_workspace->getOpenDesignSheet()->findSheetNode(*child)->setSelected(true);
                        m_workspace->getOpenDesignSheet()->m_currentNode = *child;
                    }
                    std::list<plasma::Node *> children = node->getChildren();
                    for (std::list<plasma::Node *>::iterator child = children.begin(); child != children.end(); ++child)
                        (*child)->setParent(group);
                    m_workspace->getOpenDesignSheet()->deleteNode(node);
                }
            }
        }
        m_workspace->updateModules();
        m_workspace->setTextInterface(0);
        m_workspace->getOpenDesignSheet()->commit();
    }
}
