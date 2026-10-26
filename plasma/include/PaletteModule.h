// "module:palette": a palette of RGBA colours, persisted as "plp" version 1.
#pragma once
#include <vector>
#include "Module.h"
#include "PaletteWidget.h"

class PaletteModule : public Module {
public:
    PaletteModule();
    virtual void build();
    plasma::Vector<4, float> getColor();
    void setColor(const plasma::Vector<4, float> &color);
    void chooseExpand();
    bool updateExpansion();
    void savePalette();
    void savePaletteFile(const std::wstring &filename);
    void openPalette();
    void openPaletteFile(const std::wstring &filename);
    void loadDefaultPalette();
    void addColor();
    void removeColor();
    void replaceColor();
    void clearPalette();

    PaletteWidget *getPaletteWidget() const { return m_paletteWidget; }

    std::vector<plasma::Vector<4, float>> m_colors; // File's 16-byte entries.
    int m_selection;                                // Palette entry selected by PaletteWidget.
    PaletteWidget *m_paletteWidget;                 // The swatch grid.
    int m_expandDirection;                          // -1 opens, +1 closes.
    float m_expansion;                              // Expansion fraction, from 0 to 1.
};

#if !defined(PLASMA_PORTABLE) && !defined(_WIN64)

#endif
