// OptionsModule: the editor's options dialog ("module:options", captioned "Options"): the cursor, grid, TrueType
// import, export, backup and interface zoom settings, kept in the "options" file of the application data folder
// (one "name value" line each: "cursorStickiness", "cursorHardware", "gridSnapDistance", "gridOpacity",
// "ttfCharacters", "exportJPEGQuality", "exportTransparency", "backupEnabled", "backupInterval", "guiZoom").
#pragma once
#include <string>
#include "Module.h"
#include "FormatModule.h"

// The printable ASCII characters used as the default "ttfCharacters", filled by the constructor.
extern std::wstring g_defaultTtfCharacters;

class OptionsModule : public Module {
public:
    // Reads the options file.
    OptionsModule();

    virtual void build();

    // Takes the settings from the widgets and writes the options file (also Workspace::SetInterfaceZoom).
    void save();
    void accept();
    void close(); // Closes without saving.

    // Each setting has an edit box (typed text is clamped), a drag button and steps; they change only the text until
    // Save.
    void editCursorStickiness(); // "options:cursor:stickiness", 0..20.
    void dragCursorStickiness(plasma::Widget *sender);
    void stepCursorStickiness(plasma::Widget *sender);
    void editGridOpacity(); // "options:grid:opacity", 10..100.
    void dragGridOpacity(plasma::Widget *sender);
    void stepGridOpacity(plasma::Widget *sender);
    void editGridSnapDistance(); // "options:grid:snapdistance", 0..50.
    void dragGridSnapDistance(plasma::Widget *sender);
    void stepGridSnapDistance(plasma::Widget *sender);
    void editExportJpegQuality(); // "options:export:jpeg:quality", 0..100.
    void dragExportJpegQuality(plasma::Widget *sender);
    void stepExportJpegQuality(plasma::Widget *sender);
    void editBackupInterval(); // "options:backup:interval", >= 0.
    void dragBackupInterval(plasma::Widget *sender);
    void stepBackupInterval(plasma::Widget *sender);

    // The default settings of each group.
    void resetCursor();
    void resetGrid();
    void resetTtfCharacters();
    void resetExport();

    int m_cursorStickiness;       // "cursorStickiness", 5.
    bool m_cursorHardware;        // "cursorHardware".
    int m_gridSnapDistance;       // "gridSnapDistance", 10.
    int m_gridOpacity;            // "gridOpacity", 20.
    std::wstring m_ttfCharacters; // "ttfCharacters".
    int m_exportJpegQuality;      // "exportJPEGQuality", 90.
    bool m_exportTransparency;    // "exportTransparency".
    int m_backupInterval;         // "backupInterval", 50.
    bool m_backupEnabled;         // "backupEnabled".
    float m_guiZoom;              // "guiZoom": 0.5.. 1.0 in tenths.
};
