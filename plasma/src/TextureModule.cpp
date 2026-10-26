#include "TextureModule.h"
#include "ExportModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h" // The edit box filters.
#include "plasma/Button.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "Operations.h"
#include "ShapeInterface.h"
#include "main.h"
#include "ClipboardModule.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>

#include <FreeImage.h>

// Returns the shape as a smooth mesh when its type is SHAPE_SMOOTH_MESH.
static inline plasma::SmoothMeshShape *asSmoothMeshShape(plasma::Shape *shape)
{
    return shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH ? static_cast<plasma::SmoothMeshShape *>(shape) : 0;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
TextureModule::TextureModule()
    : m_texture(0),
      m_preview(0),
      m_filter(plasma::TEXTURE_FILTER_LINEAR),
      m_mode(TEXTURE_FILL),
      m_wrap(plasma::TEXTURE_WRAP_REPEAT),
      m_opacity(1.0f),
      m_brightness(1.0f),
      m_contrast(1.0f),
      m_saturation(1.0f),
      m_stretch(1.0f)
{
    {
        std::wstring name(L"module:texture");
        std::wstring kind(L"sidewidget");
        m_panel = g_workspace.m_engine.createGenericWidget(name, 0.0f, 0.0f, 300.0f, 140.0f, g_workspace.m_scalableNode,
                                                           kind);
    }
    m_panel->m_flags |= (1 << plasma::WIDGET_RENDER_SURFACE);
    m_panel->setCaption(L"Texture", true);
    {
        std::wstring name(L"texture:selection");
        std::wstring kind(L"listbox");
        m_selectionPanel =
            g_workspace.m_engine.createGenericWidget(name, 200.0f, 200.0f, 500.0f, 500.0f, g_workspace.m_guiNode, kind);
    }
    m_selectionPanel->m_flags |= (1 << plasma::WIDGET_POPUP);
    m_selectionPanel->m_node->m_display->m_visibility.value() = 0;
    m_orientation = ORIENTATION_HORIZONTAL;
}

void TextureModule::showSelection()
{
    updateSelection();
    m_workspace->m_engine.setPopUpWidget(m_selectionPanel, m_preview);
}

void TextureModule::hideSelection()
{
    m_selectionPanel->m_node->m_display->m_visibility.value() = 0;
}

void collectTextureUsers(plasma::Node *node, TextureUsers &users, Sheet *sheet)
{
    plasma::SmoothMeshShape *shape = asSmoothMeshShape(node->m_shape);
    if (shape) {
        if (shape->getFillTexture())
            users[shape->getFillTexture()].insert(sheet);
        if (shape->getOutlineTexture())
            users[shape->getOutlineTexture()].insert(sheet);
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        collectTextureUsers(*it, users, sheet);
}

void collectTextures(plasma::Node *node, std::set<plasma::Texture *> &textures)
{
    plasma::SmoothMeshShape *shape = asSmoothMeshShape(node->m_shape);
    if (shape) {
        if (shape->getFillTexture())
            textures.insert(shape->getFillTexture());
        if (shape->getOutlineTexture())
            textures.insert(shape->getOutlineTexture());
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        collectTextures(*it, textures);
}

void TextureModule::deleteSheetTextures(Sheet *sheet)
{
    TextureUsers users;
    for (std::list<Sheet *>::iterator it = m_workspace->m_sheets.begin(); it != m_workspace->m_sheets.end(); ++it) {
        DesignSheet *design = dynamic_cast<DesignSheet *>(*it);
        if (design) {
            collectTextureUsers(design->getRoot(), users, design);
            collectTextureUsers(design->getDeletedNodes(), users, design);
        }
    }

    // Textures of the copied nodes stay loaded for a later paste.
    std::set<plasma::Texture *> copied;
    for (std::list<plasma::Node *>::iterator it = m_workspace->m_clipboardModule->m_nodes.begin();
         it != m_workspace->m_clipboardModule->m_nodes.end(); ++it)
        collectTextures(*it, copied);
    for (TextureUsers::iterator it = users.begin(); it != users.end(); ++it) {
        if (it->second.size() == 1 && it->second.find(sheet) != it->second.end() &&
            copied.find(it->first) == copied.end()) {
            if (it->first == m_texture)
                selectTexture(0);
            m_workspace->m_engine.deleteTexture(it->first);
        }
    }
    updateSelection();
}

void TextureModule::updateSelection()
{
    TextureUsers users;
    for (std::list<Sheet *>::iterator it = m_workspace->m_sheets.begin(); it != m_workspace->m_sheets.end(); ++it)
        collectTextureUsers((*it)->getRoot(), users, *it);
    plasma::ListWidget *list =
        dynamic_cast<plasma::ListWidget *>(m_workspace->m_engine.findWidget(L"texture:selection:list"));
    m_thumbnails.clear();
    if (list) {
        list->clear();
        std::list<plasma::ListItem> items;
        items.push_back(plasma::ListItem(L"No texture", L"texture:thumbnail", L"thumbnail", true));
        for (std::list<plasma::Texture *>::iterator it = m_workspace->m_engine.m_textures.begin();
             it != m_workspace->m_engine.m_textures.end(); ++it) {
            if (!(*it)->m_transient) {
                std::wstring caption = (*it)->getName();
                std::set<Sheet *> &used = users[*it];
                if (!used.empty()) {
                    caption += L" (";
                    for (std::set<Sheet *>::iterator sheet = used.begin(); sheet != used.end(); ++sheet) {
                        if (sheet != used.begin())
                            caption += L", ";
                        caption += (*sheet)->getTitle();
                    }
                    caption += L")";
                }
                items.push_back(plasma::ListItem(caption, L"texture:thumbnail", L"thumbnail", true));
            }
        }
        list->setItems(items);
        std::list<plasma::Widget *> widgets = list->getChildWidgets();
        std::list<plasma::Texture *>::iterator texture = m_workspace->m_engine.m_textures.begin();
        bool first = true;
        for (std::list<plasma::Widget *>::iterator it = widgets.begin(); it != widgets.end(); ++it) {
            plasma::Texture *thumbnailTexture = 0;
            if (first) {
                first = false;
            } else {
                do
                    thumbnailTexture = *texture++;
                while (thumbnailTexture->m_transient);
            }
            plasma::Node *node = m_workspace->m_engine.createNode(0, 0, 0, 0);
            (*it)->getNode()->moveIntoWidget(
                L"background", new TextureThumbnailWidget(&m_workspace->m_engine, m_workspace, node, thumbnailTexture),
                true);
            if (thumbnailTexture == m_texture)
                (*it)->getNode()->setChecked(true, false);
            (*it)->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &TextureModule::chooseThumbnail, true, true);
            m_thumbnails[*it] = thumbnailTexture;
        }
    }

    // The code assumes the named list exists, even after a failed cast.
    list->getNode()->animate(0);
}

void TextureModule::chooseThumbnail(plasma::Widget *sender)
{
    selectTexture(m_thumbnails[sender]);
    applyAutomatic();
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void TextureModule::build()
{
    std::list<ModuleWidgetItem> items;
    items.push_back(ModuleWidgetItem(L" ", L"texture:open", L"button2", 0, 50, 33, 25));
    items.push_back(ModuleWidgetItem(L" ", L"texture:duplicate", L"button2", 33, 50, 33, 25));
    items.push_back(ModuleWidgetItem(L" ", L"texture:delete", L"button2", 66, 50, 34, 25));
    addPanelAt(items, L"frame", 10, 30);
    placeIcon(L"texture:open", L"icon:open");
    placeIcon(L"texture:duplicate", L"icon:texture:duplicate");
    placeIcon(L"texture:delete", L"icon:delete");
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:name", L"editbox", 0, 0, 100, 20));
    addPanelAt(items, L"frame", 10, 110);

    // The preview of the selected texture, above the buttons; it opens the texture selection.
    plasma::Node *frame = m_panel->getNode()->findNode(L"frame:background");
    if (frame) {
        plasma::Node *node = m_workspace->m_engine.createNode(0, 0, 0, frame, L"texturewidget");
        m_preview = new TexturePreviewWidget(&m_workspace->m_engine, m_workspace, node);
        m_preview->setViewRect(0.0f, 0.0f, 100.0f, 50.0f);
    }
    items.clear();
    items.push_back(ModuleWidgetItem(L"Nearest", L"texture:filter:nearest", L"radiobutton3", 0, 0, 50, 25));
    items.push_back(ModuleWidgetItem(L"Linear", L"texture:filter:linear", L"radiobutton3", 0, 25, 50, 25));
    items.push_back(ModuleWidgetItem(L"Mipmap", L"texture:filter:mipmap", L"radiobutton3", 0, 50, 50, 25));
    addPanelAt(items, L"", 115, 30);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:wrap:none", L"radiobutton3", 0, 0, 47, 25));
    items.push_back(ModuleWidgetItem(L" ", L"texture:wrap:repeat", L"radiobutton3", 0, 25, 47, 25));
    items.push_back(ModuleWidgetItem(L" ", L"texture:wrap:mirror", L"radiobutton3", 0, 50, 47, 25));
    addPanelAt(items, L"", 168, 30);
    placeIcon(L"texture:wrap:none", L"icon:wrap:none");
    placeIcon(L"texture:wrap:repeat", L"icon:wrap:repeat");
    placeIcon(L"texture:wrap:mirror", L"icon:wrap:mirror");
    m_cursor.m_elements[0] = 115;
    m_extent.m_elements[0] = 115;
    m_cursor.m_elements[1] = 110;
    addWidget(L"Stretch:", L"", L"static", 45, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:stretch", L"editbox", 0, 0, 35, 20));
    items.push_back(ModuleWidgetItem(L" ", L"texture:stretch:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 35, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    m_panel->getNode()->setEditFilter(L"texture:stretch", &plasma::g_unsignedFloatFilter, true);
    m_cursor.m_elements[0] = 220;
    m_margin.m_elements[0] = 220;
    m_cursor.m_elements[1] = 35;
    m_extent.m_elements[1] = 10;
    addWidget(L"Opacity:", L"", L"static", 55, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:opacity", L"editbox", 0, 0, 35, 20));
    items.push_back(ModuleWidgetItem(L" ", L"texture:opacity:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 35, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    m_panel->getNode()->setEditFilter(L"texture:opacity", &plasma::g_unsignedFloatFilter, true);
    newRow(5);
    addWidget(L"Brightness:", L"", L"static", 55, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:brightness", L"editbox", 0, 0, 35, 20));
    items.push_back(ModuleWidgetItem(L" ", L"texture:brightness:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 35, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    m_panel->getNode()->setEditFilter(L"texture:brightness", &plasma::g_unsignedFloatFilter, true);
    newRow(5);
    addWidget(L"Contrast:", L"", L"static", 55, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:contrast", L"editbox", 0, 0, 35, 20));
    items.push_back(ModuleWidgetItem(L" ", L"texture:contrast:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 35, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    m_panel->getNode()->setEditFilter(L"texture:contrast", &plasma::g_unsignedFloatFilter, true);
    newRow(5);
    addWidget(L"Saturation:", L"", L"static", 55, 20, 0);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:saturation", L"editbox", 0, 0, 35, 20));
    items.push_back(ModuleWidgetItem(L" ", L"texture:saturation:drag",
                                     m_orientation == ORIENTATION_HORIZONTAL ? L"hdragbutton" : L"vdragbutton", 35, 0,
                                     20, 20));
    addPanel(items, L"", 0);
    m_panel->getNode()->setEditFilter(L"texture:saturation", &plasma::g_unsignedFloatFilter, true);
    fitWidth(10);
    items.clear();
    items.push_back(ModuleWidgetItem(L" ", L"texture:mode:fill", L"radiobutton2", 0, 0, 30, 23));
    items.push_back(ModuleWidgetItem(L" ", L"texture:mode:stroke", L"radiobutton2", 30, 0, 30, 23));
    addUpperPanel(L"", items, (int)(m_panel->viewWidth() * 0.5 + 27.0));
    m_panel->setChecked(L"texture:mode:fill", true, false, true);
    placeIcon(L"texture:mode:fill", L"icon:colormode:fill");
    placeIcon(L"texture:mode:stroke", L"icon:colormode:stroke");
    addZoomBar();
    addApplyBar(L"texture", -30);
    m_panel->setChecked(L"texture:wrap:repeat", true, false, true);
    m_panel->setChecked(L"texture:filter:linear", true, false, true);
    m_panel->getNode()->connectWidget(L"texture:wrap:none", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::wrapNone, true);
    m_panel->getNode()->connectWidget(L"texture:wrap:repeat", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::wrapRepeat, true);
    m_panel->getNode()->connectWidget(L"texture:wrap:mirror", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::wrapMirror, true);
    m_panel->getNode()->connectWidget(L"texture:filter:nearest", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::filterNearest, true);
    m_panel->getNode()->connectWidget(L"texture:filter:linear", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::filterLinear, true);
    m_panel->getNode()->connectWidget(L"texture:filter:mipmap", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::filterMipmap, true);
    m_panel->getNode()->connectWidget(L"texture:open", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::open, true);
    m_panel->getNode()->connectWidget(L"texture:delete", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::deleteTexture, true);
    m_panel->getNode()->connectWidget(L"texture:duplicate", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::duplicate, true);
    m_panel->getNode()->connectWidget(L"texture:duplicate", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &TextureModule::duplicate, true);
    m_panel->getNode()->connectWidget(L"texture:name", plasma::WIDGET_EVENT_FOCUS_LOST, this, &TextureModule::rename,
                                      true);
    m_panel->getNode()->connectWidget(L"texture:mode:fill", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::chooseFill, true);
    m_panel->getNode()->connectWidget(L"texture:mode:stroke", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::chooseStroke, true);
    m_panel->getNode()->connectWidget(L"texture:opacity", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                      &TextureModule::editOpacity, true);
    m_panel->getNode()->connectWidget(L"texture:opacity:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::editOpacity, true);
    m_panel->getNode()->connectWidget(L"texture:opacity:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &TextureModule::dragOpacity, true);
    m_panel->getNode()->connectWidget(L"texture:opacity:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::stepOpacity, true);
    m_panel->getNode()->connectWidget(L"texture:opacity:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &TextureModule::stepOpacity, true);
    m_panel->getNode()->connectWidget(L"texture:brightness", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                      &TextureModule::editBrightness, true);
    m_panel->getNode()->connectWidget(L"texture:brightness:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::editBrightness, true);
    m_panel->getNode()->connectWidget(L"texture:brightness:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &TextureModule::dragBrightness, true);
    m_panel->getNode()->connectWidget(L"texture:brightness:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::stepBrightness, true);
    m_panel->getNode()->connectWidget(L"texture:brightness:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &TextureModule::stepBrightness, true);
    m_panel->getNode()->connectWidget(L"texture:contrast", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                      &TextureModule::editContrast, true);
    m_panel->getNode()->connectWidget(L"texture:contrast:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::editContrast, true);
    m_panel->getNode()->connectWidget(L"texture:contrast:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &TextureModule::dragContrast, true);
    m_panel->getNode()->connectWidget(L"texture:contrast:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::stepContrast, true);
    m_panel->getNode()->connectWidget(L"texture:contrast:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &TextureModule::stepContrast, true);
    m_panel->getNode()->connectWidget(L"texture:saturation", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                      &TextureModule::editSaturation, true);
    m_panel->getNode()->connectWidget(L"texture:saturation:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::editSaturation, true);
    m_panel->getNode()->connectWidget(L"texture:saturation:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &TextureModule::dragSaturation, true);
    m_panel->getNode()->connectWidget(L"texture:saturation:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::stepSaturation, true);
    m_panel->getNode()->connectWidget(L"texture:saturation:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &TextureModule::stepSaturation, true);
    m_panel->getNode()->connectWidget(L"texture:stretch", plasma::WIDGET_EVENT_FOCUS_LOST, this,
                                      &TextureModule::editStretch, true);
    m_panel->getNode()->connectWidget(L"texture:stretch:drag", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::editStretch, true);
    m_panel->getNode()->connectWidget(L"texture:stretch:drag", plasma::WIDGET_EVENT_POINTER_MOVED, this,
                                      &TextureModule::dragStretch, true);
    m_panel->getNode()->connectWidget(L"texture:stretch:drag", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                      &TextureModule::stepStretch, true);
    m_panel->getNode()->connectWidget(L"texture:stretch:drag", plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this,
                                      &TextureModule::stepStretch, true);
    m_panel->getNode()->connectWidget(L"texture:apply", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::apply, true);
    m_panel->getNode()->connectWidget(L"texture:pick", plasma::WIDGET_EVENT_POINTER_RELEASED, this,
                                      &TextureModule::pick, true);
    m_panel->setChecked(L"texture:apply:auto", true, false, true);
    m_panel->setChecked(L"texture:pick:auto", true, false, true);
    updateSelection();
    std::wstringstream stream;
    stream << m_opacity * 100.0f;
    m_panel->setText(L"texture:opacity", stream.str(), true);
    stream.str(L"");
    stream << m_brightness * 100.0f;
    m_panel->setText(L"texture:brightness", stream.str(), true);
    stream.str(L"");
    stream << m_contrast * 100.0f;
    m_panel->setText(L"texture:contrast", stream.str(), true);
    stream.str(L"");
    stream << m_saturation * 100.0f;
    m_panel->setText(L"texture:saturation", stream.str(), true);
    stream.str(L"");
    stream << m_stretch * 100.0f;
    m_panel->setText(L"texture:stretch", stream.str(), true);
    createZoomSequences();
    selectTexture(0);
}

bool usesTexture(plasma::Node *node, plasma::Texture *texture)
{
    if (!node->isShown() || node->isInternal())
        return false;
    plasma::SmoothMeshShape *shape = asSmoothMeshShape(node->m_shape);
    if (shape && (shape->m_fillTexture == texture || shape->m_outlineTexture == texture))
        return true;
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        if (usesTexture(*it, texture))
            return true;
    }
    return false;
}

void TextureModule::invalidateSheets()
{
    for (std::list<Sheet *>::iterator it = m_workspace->m_sheets.begin(); it != m_workspace->m_sheets.end(); ++it) {
        DesignSheet *sheet = dynamic_cast<DesignSheet *>(*it);
        if (sheet) {
            plasma::Node *root = sheet->m_root;
            if (usesTexture(root, m_texture)) {
                sheet->m_modified = true;
                root->m_transformation->m_changed = true;
            }
        }
    }
}

void TextureModule::filterNearest()
{
    if (m_texture) {
        plasma::TextureFormat format = m_texture->m_format;
        format.minFilter = plasma::TEXTURE_FILTER_NEAREST;
        format.maxFilter = plasma::TEXTURE_FILTER_NEAREST;
        m_texture->setFormat(format);
        if (!m_texture->m_pixels.empty())
            m_texture->upload(&m_texture->m_pixels[0]);
        m_filter = plasma::TEXTURE_FILTER_NEAREST;
        invalidateSheets();
    }
}

void TextureModule::filterLinear()
{
    if (m_texture) {
        plasma::TextureFormat format = m_texture->m_format;
        format.minFilter = plasma::TEXTURE_FILTER_LINEAR;
        format.maxFilter = plasma::TEXTURE_FILTER_LINEAR;
        m_texture->setFormat(format);
        if (!m_texture->m_pixels.empty())
            m_texture->upload(&m_texture->m_pixels[0]);

        // The code resets the cached choice to nearest after changing the format.
        m_filter = plasma::TEXTURE_FILTER_NEAREST;
        invalidateSheets();
    }
}

void TextureModule::filterMipmap()
{
    if (m_texture) {
        plasma::TextureFormat format = m_texture->m_format;
        format.minFilter = plasma::TEXTURE_FILTER_MIPMAP;
        format.maxFilter = plasma::TEXTURE_FILTER_MIPMAP;
        m_texture->setFormat(format);
        if (!m_texture->m_pixels.empty())
            m_texture->upload(&m_texture->m_pixels[0]);

        // The code resets the cached choice to nearest after changing the format.
        m_filter = plasma::TEXTURE_FILTER_NEAREST;
        invalidateSheets();
    }
}

void TextureModule::wrapNone()
{
    if (m_texture) {
        plasma::TextureFormat format = m_texture->m_format;
        format.horizontalWrap = plasma::TEXTURE_WRAP_NONE;
        format.verticalWrap = plasma::TEXTURE_WRAP_NONE;
        m_texture->setFormat(format);
        if (!m_texture->m_pixels.empty())
            m_texture->upload(&m_texture->m_pixels[0]);
        m_wrap = plasma::TEXTURE_WRAP_NONE;
        invalidateSheets();
    }
}

void TextureModule::wrapRepeat()
{
    if (m_texture) {
        plasma::TextureFormat format = m_texture->m_format;
        format.horizontalWrap = plasma::TEXTURE_WRAP_REPEAT;
        format.verticalWrap = plasma::TEXTURE_WRAP_REPEAT;
        m_texture->setFormat(format);
        if (!m_texture->m_pixels.empty())
            m_texture->upload(&m_texture->m_pixels[0]);
        m_wrap = plasma::TEXTURE_WRAP_REPEAT;
        invalidateSheets();
    }
}

void TextureModule::wrapMirror()
{
    if (m_texture) {
        plasma::TextureFormat format = m_texture->m_format;
        format.horizontalWrap = plasma::TEXTURE_WRAP_MIRROR;
        format.verticalWrap = plasma::TEXTURE_WRAP_MIRROR;
        m_texture->setFormat(format);
        if (!m_texture->m_pixels.empty())
            m_texture->upload(&m_texture->m_pixels[0]);
        m_wrap = plasma::TEXTURE_WRAP_MIRROR;
        invalidateSheets();
    }
}

void TextureModule::applyAutomatic()
{
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyTexture();
}

void TextureModule::pick()
{
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode) {
        pickNode(m_workspace->getOpenDesignSheet()->m_currentNode);
        updatePanel();
    }
}

void TextureModule::pickNode(plasma::Node *node)
{
    if (node) {
        plasma::SmoothMeshShape *shape = asSmoothMeshShape(node->m_shape);
        if (shape) {
            selectTexture(m_mode == TEXTURE_STROKE ? shape->m_outlineTexture : shape->m_fillTexture);
            if (!m_mode) {
                m_opacity = shape->m_textureOpacity.value();
                m_brightness = shape->m_textureBrightness.value();
                m_contrast = shape->m_textureContrast.value();
                m_saturation = shape->m_textureSaturation.value();
            } else {
                m_opacity = shape->m_strokeTextureOpacity.value();
                m_brightness = shape->m_strokeTextureBrightness.value();
                m_contrast = shape->m_strokeTextureContrast.value();
                m_saturation = shape->m_strokeTextureSaturation.value();
            }
            m_stretch = shape->m_strokeTextureStretch.value();
            std::wstringstream text;
            text << (float)(m_opacity * 100.0);
            getPanel()->setText(L"texture:opacity", text.str(), true);
            text.str(L"");
            text << (float)(m_brightness * 100.0);
            getPanel()->setText(L"texture:brightness", text.str(), true);
            text.str(L"");
            text << (float)(m_contrast * 100.0);
            getPanel()->setText(L"texture:contrast", text.str(), true);
            text.str(L"");
            text << (float)(m_saturation * 100.0);
            getPanel()->setText(L"texture:saturation", text.str(), true);
            text.str(L"");
            text << (float)(m_stretch * 100.0);
            getPanel()->setText(L"texture:stretch", text.str(), true);
        }
    }
}

void TextureModule::chooseFill()
{
    m_mode = TEXTURE_FILL;
    if (m_panel->isChecked(L"texture:pick:auto", true)) {
        if (m_workspace->getOpenDesignSheet()) {
            pick();
            if (m_workspace->getOpenDesignSheet()->m_currentNode)
                pickNode(m_workspace->getOpenDesignSheet()->m_currentNode);
        }
    }
}

void TextureModule::chooseStroke()
{
    m_mode = TEXTURE_STROKE;
    if (m_panel->isChecked(L"texture:pick:auto", true)) {
        if (m_workspace->getOpenDesignSheet()) {
            pick();
            if (m_workspace->getOpenDesignSheet()->m_currentNode)
                pickNode(m_workspace->getOpenDesignSheet()->m_currentNode);
        }
    }
}

void TextureModule::apply()
{
    if (m_workspace->getOpenDesignSheet()) {
        std::map<plasma::Node *, NodeInterface *>::iterator it = m_workspace->getOpenDesignSheet()->sheetNodesBegin();
        while (it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end()) {
            if (it->second->m_selected && it->first->isShown()) {
                plasma::SmoothMeshShape *shape = asSmoothMeshShape(it->first->m_shape);
                if (shape) {
                    float oldStretch = shape->m_strokeTextureStretch.value();
                    float oldOpacity;
                    float oldBrightness;
                    float oldContrast;
                    float oldSaturation;
                    plasma::Texture *oldTexture;
                    if (!m_mode) {
                        oldTexture = shape->m_fillTexture;
                        oldOpacity = shape->m_textureOpacity.value();
                        oldBrightness = shape->m_textureBrightness.value();
                        oldContrast = shape->m_textureContrast.value();
                        oldSaturation = shape->m_textureSaturation.value();
                    } else {
                        oldTexture = shape->m_outlineTexture;
                        oldOpacity = shape->m_strokeTextureOpacity.value();
                        oldBrightness = shape->m_strokeTextureBrightness.value();
                        oldContrast = shape->m_strokeTextureContrast.value();
                        oldSaturation = shape->m_strokeTextureSaturation.value();
                    }
                    m_workspace->getOpenDesignSheet()->addOperation(new ApplyTextureOperation(
                        m_workspace->getOpenDesignSheet(), "apply texture", shape, oldTexture, oldOpacity,
                        oldBrightness, oldContrast, oldSaturation, oldStretch, m_texture, m_opacity, m_brightness,
                        m_contrast, m_saturation, m_stretch, m_mode == TEXTURE_STROKE));
                    if (m_texture)
                        it->second->m_shapeInterface->setTexture(m_texture->m_id);
                    else
                        it->second->m_shapeInterface->setTexture(-1);
                    shape->m_strokeTextureStretch.value() = m_stretch;
                    if (!m_mode) {
                        shape->m_textureOpacity.value() = m_opacity;
                        shape->m_textureBrightness.value() = m_brightness;
                        shape->m_textureContrast.value() = m_contrast;
                        shape->m_textureSaturation.value() = m_saturation;
                    } else {
                        shape->m_strokeTextureOpacity.value() = m_opacity;
                        shape->m_strokeTextureBrightness.value() = m_brightness;
                        shape->m_strokeTextureContrast.value() = m_contrast;
                        shape->m_strokeTextureSaturation.value() = m_saturation;
                    }
                    shape->lock();
                    shape->updateDrawingTextures();
                    shape->unlock();
                }
            }
            ++it;
        }
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->redraw();
    }
}

bool removeTexture(plasma::Node *node, plasma::Texture *texture)
{
    plasma::SmoothMeshShape *shape = asSmoothMeshShape(node->m_shape);
    bool removed = false;
    if (shape) {
        if (shape->m_fillTexture == texture) {
            shape->m_texture.value() = -1;
            removed = true;
            shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        }
        if (shape->m_outlineTexture == texture) {
            shape->m_strokeTexture.value() = -1;
            removed = true;
            shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        }
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it) {
        if (removeTexture(*it, texture))
            removed = true;
    }
    return removed;
}

void TextureModule::deleteTexture()
{
    if (m_texture) {
        if (askQuestion(L"Picroma Plasma",
                        L"Do you really want to delete this texture? This operation will not be undoable.",
                        false) != ANSWER_YES)
            return;
        for (std::list<Sheet *>::iterator it = m_workspace->m_sheets.begin(); it != m_workspace->m_sheets.end(); ++it) {
            if ((*it)->m_busy) {
                std::wstring text(L"Please wait until all sheets are ready.");
                std::wstring caption(L"Picroma Plasma");
                showMessageBox(caption, text);
                return;
            }
        }
        for (std::list<Sheet *>::iterator it = m_workspace->m_sheets.begin(); it != m_workspace->m_sheets.end(); ++it) {
            DesignSheet *sheet = dynamic_cast<DesignSheet *>(*it);
            if (sheet) {
                if (removeTexture(sheet->getRoot(), m_texture)) {
                    sheet->m_modified = true;
                    sheet->getRoot()->m_transformation->m_changed = true;
                }
                if (sheet->m_texture == m_texture)
                    sheet->m_texture = 0;
            }
        }
        for (std::list<plasma::Shape *>::iterator it = m_workspace->m_engine.m_shapes.begin();
             it != m_workspace->m_engine.m_shapes.end(); ++it) {
            plasma::SmoothMeshShape *shape = asSmoothMeshShape(*it);
            if (shape) {
                if (shape->m_fillTexture == m_texture) {
                    shape->m_texture.value() = -1;
                    shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                }
                if (shape->m_outlineTexture == m_texture) {
                    shape->m_strokeTexture.value() = -1;
                    shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                }
            }
        }
        m_workspace->m_engine.deleteTexture(m_texture);
        selectTexture(0);
    }
}

void TextureModule::selectTexture(plasma::Texture *texture)
{
    m_texture = texture;
    if (m_workspace->getOpenDesignSheet())
        m_workspace->getOpenDesignSheet()->m_texture = texture;
    m_panel->m_changed = true;
    if (texture) {
        switch (texture->m_format.horizontalWrap) {
        case plasma::TEXTURE_WRAP_NONE:
            getPanel()->setChecked(L"texture:wrap:none", true, false, true);
            break;
        case plasma::TEXTURE_WRAP_REPEAT:
            getPanel()->setChecked(L"texture:wrap:repeat", true, false, true);
            break;
        case plasma::TEXTURE_WRAP_MIRROR:
            getPanel()->setChecked(L"texture:wrap:mirror", true, false, true);
            break;
        }
        m_wrap = texture->m_format.horizontalWrap;
        switch (texture->m_format.minFilter) {
        case plasma::TEXTURE_FILTER_NEAREST:
            getPanel()->setChecked(L"texture:filter:nearest", true, false, true);
            m_filter = plasma::TEXTURE_FILTER_NEAREST;
            break;
        case plasma::TEXTURE_FILTER_LINEAR:
            getPanel()->setChecked(L"texture:filter:linear", true, false, true);
            m_filter = plasma::TEXTURE_FILTER_LINEAR;
            break;
        case plasma::TEXTURE_FILTER_MIPMAP:
            getPanel()->setChecked(L"texture:filter:mipmap", true, false, true);
            m_filter = plasma::TEXTURE_FILTER_MIPMAP;
            break;
        }
        getPanel()->getNode()->setEnabled(L"texture:name", true, true);
        getPanel()->getNode()->setText(L"texture:name", m_texture->getName(), true);
    } else {
        getPanel()->getNode()->setText(L"texture:name", L"No texture", true);
        getPanel()->getNode()->setEnabled(L"texture:name", false, true);
    }
}

void TextureModule::currentNodeChanged()
{
    if (!m_panel->isChecked(L"texture:pick:auto", true))
        return;
    if (m_workspace->getOpenDesignSheet() && m_workspace->getOpenDesignSheet()->m_currentNode &&
        !m_workspace->getOpenDesignSheet()->m_currentNode->isFixed())
        pickNode(m_workspace->getOpenDesignSheet()->m_currentNode);
}

void TextureModule::open()
{
    std::wstring path =
        openFileBox(L"Open texture", L"All supported types\0*.JPG;*.JPEG;*.BMP;*.PNG;*.TIF;*.TGA\0"
                                     L"JPEG (*.jpg)\0*.JPG;*.JPEG\0BMP (*.bmp)\0*.bmp\0PNG (*.png)\0*.png\0"
                                     L"TIFF (*.tif)\0*.tif\0TARGA (*.tga)\0*.tga\0");
    if (!path.empty()) {
        plasma::Texture *texture = loadTexture(path);
        if (texture)
            selectTexture(texture);
        applyAutomatic();
        updateSelection();
    }
}

plasma::Texture *TextureModule::loadTexture(const std::wstring &path)
{
    unsigned dot = path.find_last_of(L'.');
    std::wstring extension = path.substr(dot + 1, path.size() - dot - 1);

    // The byte toupper has UB for non-byte wide characters.
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    int slash = (int)path.find_last_of(L'/');
    int backslash = (int)path.find_last_of(L'\\');
    int last = slash > backslash ? slash : backslash;
    std::wstring name = path.substr(last + 1, path.size() - last - 1);
    int format = kFormatUnknown;
    if (extension == L"JPG")
        format = kFormatJPEG;
    if (extension == L"BMP")
        format = kFormatBMP;
    if (extension == L"PNG")
        format = kFormatPNG;
    if (extension == L"PPM")
        format = kFormatPPM;
    if (extension == L"TIF")
        format = kFormatTIFF;
    if (extension == L"TGA")
        format = kFormatTarga;
    if (format == kFormatUnknown)
        return 0;

    // The code dereferences a failed load and divides by an unchecked width.
    FIBITMAP *bitmap = FreeImage_LoadU(static_cast<FREE_IMAGE_FORMAT>(format), path.c_str(), 0);
    int width = FreeImage_GetWidth(bitmap);
    int height = FreeImage_GetHeight(bitmap);
    std::vector<char> pixels(width * height * 4);
    int channels = FreeImage_GetLine(bitmap) / FreeImage_GetWidth(bitmap);
    int destination = 0;
    for (int y = 0; y < (int)FreeImage_GetHeight(bitmap); ++y) {
        unsigned char *scan = FreeImage_GetScanLine(bitmap, FreeImage_GetHeight(bitmap) - y - 1);
        for (int x = 0; x < (int)FreeImage_GetWidth(bitmap); ++x) {
            if (channels == 1) {
                pixels[destination] = scan[0];
                pixels[destination + 1] = scan[0];
                pixels[destination + 2] = scan[0];
                pixels[destination + 3] = 255;
            } else {
                pixels[destination] = scan[2];
                pixels[destination + 1] = scan[1];
                pixels[destination + 2] = scan[0];
                if (channels == 3)
                    pixels[destination + 3] = 255;
                else
                    pixels[destination + 3] = scan[3];
            }
            destination += 4;
            scan += channels;
        }
    }
    FreeImage_Unload(bitmap);
    plasma::TextureFormat textureFormat;

    // Taking pixels[0] for a zero-sized bitmap is undefined.
    selectTexture(m_workspace->m_engine.createTexture(
        width, height, reinterpret_cast<const unsigned char *>(&pixels[0]), textureFormat, name, false));
    return m_texture;
}

void TextureModule::editOpacity()
{
    m_opacity =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"texture:opacity", true)).c_str()) * (double)0.01f);
    if (m_opacity < 0.0f)
        m_opacity = 0.0f;
    if (m_opacity > 1.0f)
        m_opacity = 1.0f;
    std::wstringstream text;
    text << (float)(m_opacity * 100.0);
    m_panel->setText(L"texture:opacity", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyOpacity(false);
}

void TextureModule::dragOpacity(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_opacity = (float)(m_dragPosition * (double)0.01f + m_dragStart);
        if (m_opacity < 0.0f) {
            m_opacity = 0.0f;
            beginDrag(0.0f);
        }
        if (m_opacity > 1.0f) {
            m_opacity = 1.0f;
            beginDrag(1.0f);
        }
        std::wstringstream text;
        text << (float)(m_opacity * 100.0);
        m_panel->setText(L"texture:opacity", text.str(), true);
        if (m_panel->isChecked(L"texture:apply:auto", true))
            applyOpacity(true);
    }
}

void TextureModule::stepOpacity(plasma::Widget *sender)
{
    m_opacity = (float)(pressDragButton(sender) * (double)0.01f + m_opacity);
    if (m_opacity > 1.0f)
        m_opacity = 1.0f;
    if (m_opacity < 0.0f)
        m_opacity = 0.0f;
    beginDrag(m_opacity);
    std::wstringstream text;
    text << (float)(m_opacity * 100.0);
    m_panel->setText(L"texture:opacity", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyOpacity(true);
}

void TextureModule::editBrightness()
{
    m_brightness = (float)(atof(narrowString(getPanel()->getNode()->getText(L"texture:brightness", true)).c_str()) *
                           (double)0.01f);
    if (m_brightness < 0.0f)
        m_brightness = 0.0f;
    std::wstringstream text;
    text << (float)(m_brightness * 100.0);
    m_panel->setText(L"texture:brightness", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyBrightness(false);
}

void TextureModule::dragBrightness(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_brightness = (float)(m_dragPosition * (double)0.01f + m_dragStart);
        if (m_brightness < 0.0f) {
            m_brightness = 0.0f;
            beginDrag(0.0f);
        }
        std::wstringstream text;
        text << (float)(m_brightness * 100.0);
        m_panel->setText(L"texture:brightness", text.str(), true);
        if (m_panel->isChecked(L"texture:apply:auto", true))
            applyBrightness(true);
    }
}

void TextureModule::stepBrightness(plasma::Widget *sender)
{
    m_brightness = (float)(pressDragButton(sender) * (double)0.01f + m_brightness);
    if (m_brightness < 0.0f)
        m_brightness = 0.0f;
    beginDrag(m_brightness);
    std::wstringstream text;
    text << (float)(m_brightness * 100.0);
    m_panel->setText(L"texture:brightness", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyBrightness(true);
}

void TextureModule::editContrast()
{
    m_contrast =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"texture:contrast", true)).c_str()) * (double)0.01f);
    std::wstringstream text;
    text << (float)(m_contrast * 100.0);
    m_panel->setText(L"texture:contrast", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyContrast(false);
}

void TextureModule::dragContrast(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_contrast = (float)(m_dragPosition * (double)0.01f + m_dragStart);
        std::wstringstream text;
        text << (float)(m_contrast * 100.0);
        m_panel->setText(L"texture:contrast", text.str(), true);
        if (m_panel->isChecked(L"texture:apply:auto", true))
            applyContrast(true);
    }
}

void TextureModule::stepContrast(plasma::Widget *sender)
{
    m_contrast = (float)(pressDragButton(sender) * (double)0.01f + m_contrast);
    beginDrag(m_contrast);
    std::wstringstream text;
    text << (float)(m_contrast * 100.0);
    m_panel->setText(L"texture:contrast", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyContrast(true);
}

void TextureModule::editSaturation()
{
    m_saturation = (float)(atof(narrowString(getPanel()->getNode()->getText(L"texture:saturation", true)).c_str()) *
                           (double)0.01f);
    if (m_saturation < 0.0f)
        m_saturation = 0.0f;
    std::wstringstream text;
    text << (float)(m_saturation * 100.0);
    m_panel->setText(L"texture:saturation", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applySaturation(false);
}

void TextureModule::dragSaturation(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_saturation = (float)(m_dragPosition * (double)0.01f + m_dragStart);
        if (m_saturation < 0.0f) {
            m_saturation = 0.0f;
            beginDrag(0.0f);
        }
        std::wstringstream text;
        text << (float)(m_saturation * 100.0);
        m_panel->setText(L"texture:saturation", text.str(), true);
        if (m_panel->isChecked(L"texture:apply:auto", true))
            applySaturation(true);
    }
}

void TextureModule::stepSaturation(plasma::Widget *sender)
{
    m_saturation = (float)(pressDragButton(sender) * (double)0.01f + m_saturation);
    if (m_saturation < 0.0f)
        m_saturation = 0.0f;
    beginDrag(m_saturation);
    std::wstringstream text;
    text << (float)(m_saturation * 100.0);
    m_panel->setText(L"texture:saturation", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applySaturation(true);
}

void TextureModule::editStretch()
{
    m_stretch =
        (float)(atof(narrowString(getPanel()->getNode()->getText(L"texture:stretch", true)).c_str()) * (double)0.01f);
    if (m_stretch < 0.0f)
        m_stretch = 0.0f;
    std::wstringstream text;
    text << (float)(m_stretch * 100.0);
    m_panel->setText(L"texture:stretch", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyStretch(false);
}

void TextureModule::dragStretch(plasma::Widget *sender)
{
    if (isDragging(sender)) {
        updateDrag();
        m_stretch = (float)(m_dragPosition * (double)0.01f + m_dragStart);
        if (m_stretch < 0.0f) {
            m_stretch = 0.0f;
            beginDrag(0.0f);
        }
        std::wstringstream text;
        text << (float)(m_stretch * 100.0);
        m_panel->setText(L"texture:stretch", text.str(), true);
        if (m_panel->isChecked(L"texture:apply:auto", true))
            applyStretch(true);
    }
}

void TextureModule::stepStretch(plasma::Widget *sender)
{
    m_stretch = (float)(pressDragButton(sender) * (double)0.01f + m_stretch);
    if (m_stretch < 0.0f)
        m_stretch = 0.0f;
    beginDrag(m_stretch);
    std::wstringstream text;
    text << (float)(m_stretch * 100.0);
    m_panel->setText(L"texture:stretch", text.str(), true);
    if (m_panel->isChecked(L"texture:apply:auto", true))
        applyStretch(true);
}

void TextureModule::applyTexture()
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isFixed()) {
                plasma::SmoothMeshShape *shape = asSmoothMeshShape(it->first->m_shape);
                if (shape) {
                    float oldStretch = shape->m_strokeTextureStretch.value();
                    float oldOpacity;
                    float oldBrightness;
                    float oldContrast;
                    float oldSaturation;
                    plasma::Texture *oldTexture;
                    if (!m_mode) {
                        oldTexture = shape->m_fillTexture;
                        oldOpacity = shape->m_textureOpacity.value();
                        oldBrightness = shape->m_textureBrightness.value();
                        oldContrast = shape->m_textureContrast.value();
                        oldSaturation = shape->m_textureSaturation.value();
                    } else {
                        oldTexture = shape->m_outlineTexture;
                        oldOpacity = shape->m_strokeTextureOpacity.value();
                        oldBrightness = shape->m_strokeTextureBrightness.value();
                        oldContrast = shape->m_strokeTextureContrast.value();
                        oldSaturation = shape->m_strokeTextureSaturation.value();
                    }
                    m_workspace->getOpenDesignSheet()->addOperation(new ApplyTextureOperation(
                        m_workspace->getOpenDesignSheet(), "apply texture", shape, oldTexture, oldOpacity,
                        oldBrightness, oldContrast, oldSaturation, oldStretch, m_texture, oldOpacity, oldBrightness,
                        oldContrast, oldSaturation, oldStretch, m_mode == TEXTURE_STROKE));
                    if (m_texture)
                        it->second->m_shapeInterface->setTexture(m_texture->m_id);
                    else
                        it->second->m_shapeInterface->setTexture(-1);
                    shape->lock();
                    shape->updateDrawingTextures();
                    shape->unlock();
                }
            }
        }
        m_workspace->getOpenDesignSheet()->commit();
        m_workspace->redraw();
    }
}

void TextureModule::applyOpacity(bool preview)
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isFixed()) {
                plasma::SmoothMeshShape *shape = asSmoothMeshShape(it->first->m_shape);
                if (shape) {
                    float oldStretch = shape->m_strokeTextureStretch.value();
                    float oldOpacity;
                    float oldBrightness;
                    float oldContrast;
                    float oldSaturation;
                    plasma::Texture *oldTexture;
                    if (!m_mode) {
                        oldOpacity = shape->m_textureOpacity.value();
                        oldBrightness = shape->m_textureBrightness.value();
                        oldContrast = shape->m_textureContrast.value();
                        oldTexture = shape->m_fillTexture;
                        oldSaturation = shape->m_textureSaturation.value();
                    } else {
                        oldOpacity = shape->m_strokeTextureOpacity.value();
                        oldBrightness = shape->m_strokeTextureBrightness.value();
                        oldContrast = shape->m_strokeTextureContrast.value();
                        oldTexture = shape->m_outlineTexture;
                        oldSaturation = shape->m_strokeTextureSaturation.value();
                    }
                    float previous;
                    if (!m_mode) {
                        previous = shape->m_textureOpacity.value();
                        shape->m_textureOpacity.value() = m_opacity;
                    } else {
                        previous = shape->m_strokeTextureOpacity.value();
                        shape->m_strokeTextureOpacity.value() = m_opacity;
                    }
                    shape->lock();
                    shape->updateDrawingTextures();
                    shape->unlock();
                    if (preview) {
                        if (!m_mode)
                            shape->m_textureOpacity.value() = previous;
                        else
                            shape->m_strokeTextureOpacity.value() = previous;
                    } else {
                        m_workspace->getOpenDesignSheet()->addOperation(new ApplyTextureOperation(
                            m_workspace->getOpenDesignSheet(), "apply texture", shape, oldTexture, oldOpacity,
                            oldBrightness, oldContrast, oldSaturation, oldStretch, oldTexture, m_opacity, oldBrightness,
                            oldContrast, oldSaturation, oldStretch, m_mode == TEXTURE_STROKE));
                    }
                }
            }
        }
        if (!preview) {
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        }
        m_workspace->getOpenDesignSheet()->m_root->m_transformation->m_changed = true;
    }
}

void TextureModule::applyBrightness(bool preview)
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isFixed()) {
                plasma::SmoothMeshShape *shape = asSmoothMeshShape(it->first->m_shape);
                if (shape) {
                    float oldStretch = shape->m_strokeTextureStretch.value();
                    float oldOpacity;
                    float oldBrightness;
                    float oldContrast;
                    float oldSaturation;
                    plasma::Texture *oldTexture;
                    if (!m_mode) {
                        oldOpacity = shape->m_textureOpacity.value();
                        oldBrightness = shape->m_textureBrightness.value();
                        oldContrast = shape->m_textureContrast.value();
                        oldTexture = shape->m_fillTexture;
                        oldSaturation = shape->m_textureSaturation.value();
                    } else {
                        oldOpacity = shape->m_strokeTextureOpacity.value();
                        oldBrightness = shape->m_strokeTextureBrightness.value();
                        oldContrast = shape->m_strokeTextureContrast.value();
                        oldTexture = shape->m_outlineTexture;
                        oldSaturation = shape->m_strokeTextureSaturation.value();
                    }
                    float previous;
                    if (!m_mode) {
                        previous = shape->m_textureBrightness.value();
                        shape->m_textureBrightness.value() = m_brightness;
                    } else {
                        previous = shape->m_strokeTextureBrightness.value();
                        shape->m_strokeTextureBrightness.value() = m_brightness;
                    }
                    shape->lock();
                    shape->updateDrawingTextures();
                    shape->unlock();
                    if (preview) {
                        if (!m_mode)
                            shape->m_textureBrightness.value() = previous;
                        else
                            shape->m_strokeTextureBrightness.value() = previous;
                    } else {
                        m_workspace->getOpenDesignSheet()->addOperation(new ApplyTextureOperation(
                            m_workspace->getOpenDesignSheet(), "apply texture", shape, oldTexture, oldOpacity,
                            oldBrightness, oldContrast, oldSaturation, oldStretch, oldTexture, oldOpacity, m_brightness,
                            oldContrast, oldSaturation, oldStretch, m_mode == TEXTURE_STROKE));
                    }
                }
            }
        }
        if (!preview) {
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        }
        m_workspace->getOpenDesignSheet()->m_root->m_transformation->m_changed = true;
    }
}

void TextureModule::applyContrast(bool preview)
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isFixed()) {
                plasma::SmoothMeshShape *shape = asSmoothMeshShape(it->first->m_shape);
                if (shape) {
                    float oldStretch = shape->m_strokeTextureStretch.value();
                    float oldOpacity;
                    float oldBrightness;
                    float oldContrast;
                    float oldSaturation;
                    plasma::Texture *oldTexture;
                    if (!m_mode) {
                        oldOpacity = shape->m_textureOpacity.value();
                        oldBrightness = shape->m_textureBrightness.value();
                        oldContrast = shape->m_textureContrast.value();
                        oldTexture = shape->m_fillTexture;
                        oldSaturation = shape->m_textureSaturation.value();
                    } else {
                        oldOpacity = shape->m_strokeTextureOpacity.value();
                        oldBrightness = shape->m_strokeTextureBrightness.value();
                        oldContrast = shape->m_strokeTextureContrast.value();
                        oldTexture = shape->m_outlineTexture;
                        oldSaturation = shape->m_strokeTextureSaturation.value();
                    }
                    float previous;
                    if (!m_mode) {
                        previous = shape->m_textureContrast.value();
                        shape->m_textureContrast.value() = m_contrast;
                    } else {
                        previous = shape->m_strokeTextureContrast.value();
                        shape->m_strokeTextureContrast.value() = m_contrast;
                    }
                    shape->lock();
                    shape->updateDrawingTextures();
                    shape->unlock();
                    if (preview) {
                        if (!m_mode)
                            shape->m_textureContrast.value() = previous;
                        else
                            shape->m_strokeTextureContrast.value() = previous;
                    } else {
                        m_workspace->getOpenDesignSheet()->addOperation(new ApplyTextureOperation(
                            m_workspace->getOpenDesignSheet(), "apply texture", shape, oldTexture, oldOpacity,
                            oldBrightness, oldContrast, oldSaturation, oldStretch, oldTexture, oldOpacity,
                            oldBrightness, m_contrast, oldSaturation, oldStretch, m_mode == TEXTURE_STROKE));
                    }
                }
            }
        }
        if (!preview) {
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        }
        m_workspace->getOpenDesignSheet()->m_root->m_transformation->m_changed = true;
    }
}

void TextureModule::applySaturation(bool preview)
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isFixed()) {
                plasma::SmoothMeshShape *shape = asSmoothMeshShape(it->first->m_shape);
                if (shape) {
                    float oldStretch = shape->m_strokeTextureStretch.value();
                    float oldOpacity;
                    float oldBrightness;
                    float oldContrast;
                    float oldSaturation;
                    plasma::Texture *oldTexture;
                    if (!m_mode) {
                        oldOpacity = shape->m_textureOpacity.value();
                        oldBrightness = shape->m_textureBrightness.value();
                        oldContrast = shape->m_textureContrast.value();
                        oldTexture = shape->m_fillTexture;
                        oldSaturation = shape->m_textureSaturation.value();
                    } else {
                        oldOpacity = shape->m_strokeTextureOpacity.value();
                        oldBrightness = shape->m_strokeTextureBrightness.value();
                        oldContrast = shape->m_strokeTextureContrast.value();
                        oldTexture = shape->m_outlineTexture;
                        oldSaturation = shape->m_strokeTextureSaturation.value();
                    }
                    float previous;
                    if (!m_mode) {
                        previous = shape->m_textureSaturation.value();
                        shape->m_textureSaturation.value() = m_saturation;
                    } else {
                        previous = shape->m_strokeTextureSaturation.value();
                        shape->m_strokeTextureSaturation.value() = m_saturation;
                    }
                    shape->lock();
                    shape->updateDrawingTextures();
                    shape->unlock();
                    if (preview) {
                        if (!m_mode)
                            shape->m_textureSaturation.value() = previous;
                        else
                            shape->m_strokeTextureSaturation.value() = previous;
                    } else {
                        m_workspace->getOpenDesignSheet()->addOperation(new ApplyTextureOperation(
                            m_workspace->getOpenDesignSheet(), "apply texture", shape, oldTexture, oldOpacity,
                            oldBrightness, oldContrast, oldSaturation, oldStretch, oldTexture, oldOpacity,
                            oldBrightness, oldContrast, m_saturation, oldStretch, m_mode == TEXTURE_STROKE));
                    }
                }
            }
        }
        if (!preview) {
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        }
        m_workspace->getOpenDesignSheet()->m_root->m_transformation->m_changed = true;
    }
}

void TextureModule::applyStretch(bool preview)
{
    if (m_workspace->getOpenDesignSheet()) {
        for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                 m_workspace->getOpenDesignSheet()->sheetNodesBegin();
             it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isFixed()) {
                plasma::SmoothMeshShape *shape = asSmoothMeshShape(it->first->m_shape);
                if (shape) {
                    float oldStretch = shape->m_strokeTextureStretch.value();
                    float oldOpacity;
                    float oldBrightness;
                    float oldContrast;
                    float oldSaturation;
                    plasma::Texture *oldTexture;
                    if (!m_mode) {
                        oldOpacity = shape->m_textureOpacity.value();
                        oldBrightness = shape->m_textureBrightness.value();
                        oldContrast = shape->m_textureContrast.value();
                        oldTexture = shape->m_fillTexture;
                        oldSaturation = shape->m_textureSaturation.value();
                    } else {
                        oldOpacity = shape->m_strokeTextureOpacity.value();
                        oldBrightness = shape->m_strokeTextureBrightness.value();
                        oldContrast = shape->m_strokeTextureContrast.value();
                        oldTexture = shape->m_outlineTexture;
                        oldSaturation = shape->m_strokeTextureSaturation.value();
                    }

                    // Fill and stroke share the one stretch attribute.
                    float previous = shape->m_strokeTextureStretch.value();
                    shape->m_strokeTextureStretch.value() = m_stretch;
                    shape->lock();
                    shape->updateDrawingTextures();
                    shape->unlock();
                    if (preview) {
                        shape->m_strokeTextureStretch.value() = previous;
                    } else {
                        m_workspace->getOpenDesignSheet()->addOperation(new ApplyTextureOperation(
                            m_workspace->getOpenDesignSheet(), "apply texture", shape, oldTexture, oldOpacity,
                            oldBrightness, oldContrast, oldSaturation, oldStretch, oldTexture, oldOpacity,
                            oldBrightness, oldContrast, oldSaturation, m_stretch, m_mode == TEXTURE_STROKE));
                    }
                }
            }
        }
        if (!preview) {
            m_workspace->getOpenDesignSheet()->commit();
            m_workspace->redraw();
        }
        m_workspace->getOpenDesignSheet()->m_root->m_transformation->m_changed = true;
    }
}

void TextureModule::rename()
{
    if (m_texture)
        m_texture->setName(getPanel()->getNode()->getText(L"texture:name", true));
}

void TextureModule::duplicate()
{
    if (m_texture && !m_texture->getPixels().empty()) {
        selectTexture(m_workspace->m_engine.createTexture(m_texture->getWidth(), m_texture->getHeight(),
                                                          m_texture->getPixels().data(), m_texture->getFormat(),
                                                          m_texture->getName() + L"*", false));
    }
}

template void plasma::Node::connectWidget<TextureModule>(const std::wstring &, int, TextureModule *,
                                                         void (TextureModule::*)(), bool);

template void plasma::Node::connectWidget<TextureModule>(const std::wstring &, int, TextureModule *,
                                                         void (TextureModule::*)(plasma::Widget *), bool);

namespace plasma {
template<>
void Widget::setConnection<TextureModule>(Connection *&connection, TextureModule *object,
                                          void (TextureModule::*method)(), bool enabledOnly)
{
    if (connection)
        delete connection;
    connection = new MemberFunctionConnection<TextureModule>(object, method, enabledOnly);
}
}
