#include "ColorTool.h"
#include <windows.h>
#include "ColorModule.h"
#include "DesignSheet.h"
#include "plasma/ListWidget.h"
#include "ColorToolModule.h"
#include <GL/glew.h>
#include "plasma/RenderSurface.h"

using plasma::Vector;

ColorTool::ColorTool()
    : Tool(TOOL_COLOR),
      m_picking(false)
{}

void ColorTool::pointerPressed()
{
    if (m_workspace->getOpenDesignSheet()) {
        if (m_workspace->m_engine.isKeyDown(VK_MENU)) {
            pickColor(false);
            m_savedColor = m_workspace->m_colorModule->getColor();
        } else {
            m_workspace->getOpenDesignSheet()->beginModifySelected();
            if (!m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->m_colorToolModule->m_colorize) {
                if (m_workspace->m_colorModule->m_link) {
                    colorize(COLOR_TARGET_FILL);
                    colorize(COLOR_TARGET_STROKE);
                    colorize(COLOR_TARGET_EXTRUSION);
                } else {
                    colorize(m_workspace->m_colorModule->getMode());
                }
            } else {
                applyColor();
            }
        }
    }
}

void ColorTool::pointerMoved(int, int)
{
    Workspace *workspace = m_workspace;
    plasma::Engine *engine = &workspace->m_engine;
    if (engine->isKeyDown(VK_MENU)) {
        if (!engine->isButtonDown(plasma::MOUSE_BUTTON_LEFT))
            pickColor(true);
    } else if (engine->isButtonDown(plasma::MOUSE_BUTTON_LEFT)) {
        if (!engine->isKeyDown(VK_MENU) && workspace->m_colorToolModule->m_colorize) {
            if (workspace->m_colorModule->m_link) {
                colorize(COLOR_TARGET_FILL);
                colorize(COLOR_TARGET_STROKE);
                colorize(COLOR_TARGET_EXTRUSION);
            } else {
                colorize(workspace->m_colorModule->getMode());
            }
        } else {
            applyColor();
        }
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Draws the tool's
// icon: the pipette at the picked vertex while Alt picks on an open sheet, otherwise beside the pointer the mode's
// icon (colorize, alpha with Ctrl or without the colour option, colour with Shift or without the alpha option).
void ColorTool::draw()
{
    if (m_workspace->m_engine.isKeyDown(VK_MENU) && m_workspace->getOpenSheet()) {
        drawIcon(L"icon:pipette", m_pickPosition);
        return;
    }
    if (m_workspace->m_colorToolModule->m_colorize) {
        std::wstring name(L"icon:color:colorize");
        Vector<2, float> position = m_workspace->m_sceneMousePosition;
        position.m_elements[0] += 25.0;
        position.m_elements[1] += 15.0;
        drawIcon(name, position);
    } else if (m_workspace->m_engine.isKeyDown(VK_CONTROL) || !m_workspace->m_colorToolModule->m_color) {
        std::wstring name(L"icon:color:alpha");
        Vector<2, float> position = m_workspace->m_sceneMousePosition;
        position.m_elements[0] += 25.0;
        position.m_elements[1] += 15.0;
        drawIcon(name, position);
    } else if (m_workspace->m_engine.isKeyDown(VK_SHIFT) || !m_workspace->m_colorToolModule->m_alpha) {
        std::wstring name(L"icon:color:color");
        Vector<2, float> position = m_workspace->m_sceneMousePosition;
        position.m_elements[0] += 25.0;
        position.m_elements[1] += 15.0;
        drawIcon(name, position);
    }
}

void ColorTool::pickColor(bool preview)
{
    if (!m_workspace->getOpenSheet())
        return;
    float nearestDistance = 0.0f;
    bool found = false;
    Vector<4, float> pickedColor;
    m_pickPosition = m_workspace->m_sceneMousePosition;
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                Vector<4, float> color;
                Vector<2, float> position;
                if (it->second->m_shapeInterface->pickColor(color, position)) {
                    Vector<2, float> offset = position - m_workspace->m_sceneMousePosition;
                    float distance = (float)(offset.m_elements[0] * offset.m_elements[0] +
                                             offset.m_elements[1] * offset.m_elements[1]);
                    if (!found || nearestDistance > distance) {
                        nearestDistance = distance;
                        m_pickPosition = position;
                        found = true;
                        pickedColor = color;
                    }
                }
            }
        }
    } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                Vector<4, float> color;
                Vector<2, float> position;
                if (it->second->m_shapeInterface->pickColor(color, position)) {
                    Vector<2, float> offset = position - m_workspace->m_sceneMousePosition;
                    float distance = (float)(offset.m_elements[0] * offset.m_elements[0] +
                                             offset.m_elements[1] * offset.m_elements[1]);
                    if (!found || nearestDistance > distance) {
                        nearestDistance = distance;
                        m_pickPosition = position;
                        found = true;
                        pickedColor = color;
                    }
                }
                it->second->m_shapeInterface->resetMatrix();
            }
        }
    }
    if (!found) {
        glPushAttrib(GL_COLOR_BUFFER_BIT);
        m_workspace->getOpenSheet()->m_viewWidget->m_surface->begin();
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        float pointerX = m_workspace->m_engine.m_pointer.m_elements[0];
        int x = (int)((pointerX - m_workspace->getOpenSheet()->m_viewWidget->getContentPosition()[0]) *
                      m_workspace->getOpenSheet()->m_viewWidget->m_surfaceScale);
        float pointerY = m_workspace->m_engine.m_pointer.m_elements[1];
        unsigned char pixels[4];
        glReadPixels(x,
                     (int)((pointerY - m_workspace->getOpenSheet()->m_viewWidget->getContentPosition()[1]) *
                           m_workspace->getOpenSheet()->m_viewWidget->m_surfaceScale),
                     1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        Vector<4, float> color;
        color.m_elements[0] = pixels[0] / 255.0;
        color.m_elements[1] = pixels[1] / 255.0;
        color.m_elements[2] = pixels[2] / 255.0;
        color.m_elements[3] = pixels[3] / 255.0;
        if (color.m_elements[3] > 0.0001) {
            color.m_elements[0] /= (double)color.m_elements[3];
            color.m_elements[1] /= (double)color.m_elements[3];
            color.m_elements[2] /= (double)color.m_elements[3];
        } else {
            color = m_workspace->getOpenSheet()->m_page.color;
        }
        pickedColor = color;
        m_workspace->getOpenSheet()->m_viewWidget->m_surface->end();
        if (m_workspace->m_engine.topSurface())
            m_workspace->m_engine.topSurface()->begin();
        glPopAttrib();
    }
    m_workspace->m_colorModule->setColor(pickedColor, true, !preview, false);
}

void ColorTool::applyColor()
{
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                Workspace *workspace = m_workspace;
                ColorModule *colorModule = workspace->m_colorModule;
                plasma::Engine *engine = &workspace->m_engine;
                if (colorModule->m_link && !engine->isKeyDown(VK_MENU)) {
                    it->second->m_shapeInterface->paintColor(COLOR_TARGET_FILL, false);
                    it->second->m_shapeInterface->paintColor(COLOR_TARGET_STROKE, false);
                    it->second->m_shapeInterface->paintColor(COLOR_TARGET_EXTRUSION, true);
                } else {
                    it->second->m_shapeInterface->paintColor(colorModule->getMode(), true);
                }
            }
        }
    } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                Workspace *workspace = m_workspace;
                ColorModule *colorModule = workspace->m_colorModule;
                plasma::Engine *engine = &workspace->m_engine;
                if (colorModule->m_link && !engine->isKeyDown(VK_MENU)) {
                    it->second->m_shapeInterface->paintColor(COLOR_TARGET_FILL, false);
                    it->second->m_shapeInterface->paintColor(COLOR_TARGET_STROKE, false);
                    it->second->m_shapeInterface->paintColor(COLOR_TARGET_EXTRUSION, true);
                } else {
                    it->second->m_shapeInterface->paintColor(colorModule->getMode(), true);
                }
                it->second->m_shapeInterface->resetMatrix();
            }
        }
    }
}

void ColorTool::colorize(int colorTarget)
{
    std::list<Vector<4, float>> colors;
    if (m_workspace->getDesignSheet()) {
        DesignSheet *sheet = m_workspace->getDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
                it->second->m_shapeInterface->getColorsUnderMouse(colors, colorTarget);
        }
    } else if (m_workspace->getTextureSheet() && m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
                it->second->getShapeInterface()->setMatrix(m_workspace->getTextureSheet()->getMatrix());
                it->second->m_shapeInterface->getColorsUnderMouse(colors, colorTarget);
                it->second->m_shapeInterface->resetMatrix();
            }
        }
    }
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<plasma::Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface) {
            it->second->m_shapeInterface->shiftColors(colors, m_workspace->m_colorModule->getColor(),
                                                      m_workspace->m_colorToolModule->m_tolerance, colorTarget, true);
        }
    }
}

void ColorTool::flattenSheetNode(NodeInterface *node)
{
    if (node->m_selected) {
        ShapeInterface *shapeInterface = node->m_shapeInterface;
        if (shapeInterface)
            shapeInterface->flattenTransparency();
    }
}

void ColorTool::flattenTransparency()
{
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->forEachUnlockedSheetNode(&ColorTool::flattenSheetNode);
        m_workspace->getOpenDesignSheet()->commit();
    }
}

void ColorTool::fillContextMenu()
{
    int count = 0;
    DesignSheet *sheet = m_workspace->getOpenDesignSheet();
    std::map<plasma::Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->second->m_selected && it->first->isShown() && it->second->m_shapeInterface)
            ++count;
    }
    {
        std::wstring caption(L"Resize Tool Radius...");
        Workspace *workspace = m_workspace;
        plasma::Widget *item = workspace->m_popupMenu->addItem(workspace->m_popupMenuItem, true);
        item->setPopUpItem(true, true);
        item->setCaption(caption, true);
        item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, workspace, &Workspace::resizeToolRadius, true, true);
    }
    m_workspace->addPopupMenuSeparator();
    std::wstring caption(L"Flatten Transparency");
    plasma::Widget *item = m_workspace->m_popupMenu->addItem(m_workspace->m_popupMenuItem, true);
    item->setPopUpItem(true, true);
    item->setCaption(caption, true);
    item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &ColorTool::flattenTransparency, true, true);
    if (count <= 0)
        item->setEnabled(false);
}

void ColorTool::keyPressed(unsigned short key)
{
    if (key == VK_MENU) {
        m_picking = true;
        m_savedColor = m_workspace->m_colorModule->getColor();
        pickColor(true);
        m_workspace->m_redrawRequested = true;
    }
}

void ColorTool::keyReleased(unsigned short key)
{
    if (key == VK_MENU && m_picking) {
        m_picking = false;
        m_workspace->m_colorModule->setColor(m_savedColor, true, false, false);
        m_workspace->m_redrawRequested = true;
    }
}

void ColorTool::pointerReleased()
{
    refreshChangedShapes();
    if (m_workspace->getOpenDesignSheet()) {
        m_workspace->getOpenDesignSheet()->endModifyChanged();
        m_workspace->getOpenDesignSheet()->commit();
    }
}
