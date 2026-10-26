// FormatModule: the editor's page format toolbar ("module:format"): the page size and unit of the current sheet,
// the paper formats ("format:listbox:list"), the orientation ("format:landscape") and whether the page is shown
// ("format:visible").
#pragma once
#include "Module.h"
#include "DesignSheet.h"
#include "plasma/Edit.h"
#include "plasma/ListWidget.h"

namespace plasma {
// Each module translation unit has its own four static input filter objects.
static IntegerFilter g_integerFilter;
static FloatFilter g_floatFilter;
static UnsignedIntegerFilter g_unsignedIntegerFilter;
static UnsignedFloatFilter g_unsignedFloatFilter;
}

enum {
    PAPER_FORMAT_COUNT = 9
};

// Components of the page size used by the format and export panels.
enum PageSizeComponent {
    PAGE_WIDTH = 0,
    PAGE_HEIGHT = 1,
    PAGE_RESOLUTION = 2
};

// The paper formats of the format list (nine entries: "A4", "US Letter", "US Legal", "US Executive", "A0".. "A3",
// "A5").
struct PaperFormat {
    PaperFormat(const std::wstring &paperName, float paperWidth, float paperHeight, int paperUnit)
        : name(paperName),
          width(paperWidth),
          height(paperHeight),
          unit(paperUnit)
    {}

    std::wstring name;
    float width;
    float height;
    int unit; // Index of g_unitScale (ListWidget.cpp): 1 inch, 3 mm.
};

// The page units, in the order of the "unit:*" buttons and of the scale factors (plasma::ToInternalUnits).
#include "plasma/PageUnit.h"

class FormatModule : public Module {
public:
    FormatModule();

    virtual void build();
    virtual void sheetChanged(); // Update.

    // "format:visible": shows or hides the page.
    void toggleVisible();

    // Copies the open sheet's page settings.
    void savePage();

    // Records the sheet's changed page settings as a "change page format" operation (also "format:width:drag" and
    // "format:height:drag", event 3).
    void recordPageChange();

    // An item of "format:listbox:list": the paper format of that name.
    void choosePaperFormat(plasma::Widget *sender);

    // "unit:px".. "unit:pc".
    void setUnitPixels();
    void setUnitInches();
    void setUnitFeet();
    void setUnitMillimetres();
    void setUnitCentimetres();
    void setUnitMetres();
    void setUnitPoints();
    void setUnitPicas();
    void editWidth(); // "format:width".
    void dragWidth(plasma::Widget *sender);
    void stepWidth(plasma::Widget *sender);
    void editHeight(); // "format:height".
    void dragHeight(plasma::Widget *sender);
    void stepHeight(plasma::Widget *sender);

    // Shows the open sheet's page settings in the widgets.
    void update();

    // "format:landscape": swaps the width and height.
    void swapOrientation();

    plasma::Engine::Page m_page; // The sheet's page before a change.
    bool m_visible;              // "format:visible".
};
