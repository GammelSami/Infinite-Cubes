// TextureModule.cpp: the panel identified by "module:texture" and "texture:*" widgets.
#pragma once
#include <set>
#include "TextModule.h"
#include "plasma/Texture.h"
#include "plasma/Button.h"
#include "TexturePreviewWidget.h"

// The nested container follows the thumbnail captions' per-sheet usage list.
typedef std::map<plasma::Texture *, std::set<Sheet *>> TextureUsers;

class TextureModule : public Module {
public:
    enum TextureTarget {
        TEXTURE_FILL = 0,
        TEXTURE_STROKE = 1
    };

    TextureModule();
    virtual void build();
    virtual void currentNodeChanged();
    void showSelection();
    // Hides the texture selection list.
    void hideSelection();

    // Deletes the textures only this sheet uses (not those of copied nodes), before the sheet is cleared or removed.
    void deleteSheetTextures(Sheet *sheet);
    void updateSelection();
    void chooseThumbnail(plasma::Widget *sender);
    void invalidateSheets();
    void filterNearest();
    void filterLinear();
    void filterMipmap();
    void wrapNone();
    void wrapRepeat();
    void wrapMirror();
    void applyAutomatic();
    void pick();
    void pickNode(plasma::Node *node);
    void chooseFill();
    void chooseStroke();
    void apply();
    void deleteTexture();
    void selectTexture(plasma::Texture *texture);
    void open();
    plasma::Texture *loadTexture(const std::wstring &path);
    void editOpacity();
    void dragOpacity(plasma::Widget *sender);
    void stepOpacity(plasma::Widget *sender);
    void applyOpacity(bool preview);
    void editBrightness();
    void dragBrightness(plasma::Widget *sender);
    void stepBrightness(plasma::Widget *sender);
    void applyBrightness(bool preview);
    void editContrast();
    void dragContrast(plasma::Widget *sender);
    void stepContrast(plasma::Widget *sender);
    void applyContrast(bool preview);
    void editSaturation();
    void dragSaturation(plasma::Widget *sender);
    void stepSaturation(plasma::Widget *sender);
    void applySaturation(bool preview);
    void editStretch();
    void dragStretch(plasma::Widget *sender);
    void stepStretch(plasma::Widget *sender);
    void applyStretch(bool preview);
    void applyTexture();
    void rename();
    void duplicate();

    plasma::Texture *getTexture() const { return m_texture; }

    plasma::Texture *m_texture;
    plasma::Widget *m_selectionPanel;
    TexturePreviewWidget *m_preview; // Combobutton.
    int m_filter;                    // plasma::TextureFilter.
    int m_mode;                      // TextureTarget.
    int m_wrap;                      // plasma::TextureWrap.
    float m_opacity;                 // "texture:opacity".
    float m_brightness;              // "texture:brightness".
    float m_contrast;                // "texture:contrast".
    float m_saturation;              // "texture:saturation".
    float m_stretch;                 // "texture:stretch".
    std::map<plasma::Widget *, plasma::Texture *> m_thumbnails;
};

void collectTextureUsers(plasma::Node *node, TextureUsers &users, Sheet *sheet);
void collectTextures(plasma::Node *node, std::set<plasma::Texture *> &textures);
bool usesTexture(plasma::Node *node, plasma::Texture *texture);
bool removeTexture(plasma::Node *node, plasma::Texture *texture);

// A texture thumbnail in the selection list, named "texture:thumbnail". The first entry has a null texture and is
// labelled "No texture".
class TextureThumbnailWidget : public plasma::Widget {
public:
    TextureThumbnailWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node, plasma::Texture *texture);

    // Draws the texture over the widget, scaled to keep its aspect ratio.
    virtual void draw();

    // Selects the texture, applies it when automatic and hides the list.
    virtual void pointerReleased();

    Workspace *m_workspace;
    plasma::Texture *m_texture;
};

namespace plasma {
template<>
void Widget::setConnection<TextureModule>(Connection *&connection, TextureModule *object,
                                          void (TextureModule::*method)(), bool enabledOnly);
}

