// File and sheet menu helpers shared by platform entry points.
DWORD hideUserInterface()
{
    g_workspace.m_guiNode->m_display->m_visibility.value() =
        g_workspace.m_guiNode->m_display->m_visibility.value() == 0;
    int visibility = g_workspace.m_guiNode->m_display->m_visibility.value();
    HMENU menu = g_settingsMenu;
    DWORD result = CheckMenuItem(menu, MENU_HIDE_USER_INTERFACE, visibility != 0 ? MF_UNCHECKED : MF_CHECKED);
    g_workspace.m_redrawRequested = true;
    return result;
}

// Whether the file name ends in the Plasma extension, ignoring case.
bool isPlasmaFile(std::wstring path)
{
    std::wstring extension;
    int dot = path.rfind(L'.');
    extension = path.substr(dot + 1, path.size() - dot - 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), tolower);
    return extension.compare(L"plg") == 0;
}

void buildSheetMenu()
{
    int count = GetMenuItemCount(g_sheetMenu);
    for (int position = count - 1; position >= 0; --position)
        DeleteMenu(g_sheetMenu, position, MF_BYPOSITION);
    AppendMenuW(g_sheetMenu, MF_STRING, MENU_NEW_SHEET, L"New Sheet");
    AppendMenuW(g_sheetMenu, MF_STRING, MENU_CLOSE_SHEET, L"Close Sheet");
    AppendMenuW(g_sheetMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_sheetMenu, g_workspace.m_textureSheet ? MF_CHECKED : MF_STRING, MENU_TEXTURE_SHEET, L"Texture sheet");
    AppendMenuW(g_sheetMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_sheetMenu, MF_STRING, MENU_TILE_HORIZONTALLY, L"Tile Horizontally");
    AppendMenuW(g_sheetMenu, MF_STRING, MENU_TILE_VERTICALLY, L"Tile Vertically");
    AppendMenuW(g_sheetMenu, MF_STRING, MENU_CASCADE, L"Cascade");
    AppendMenuW(g_sheetMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_sheetMenu, MF_STRING, MENU_SET_SHEET_COLOR, L"Set Sheet Color");
    AppendMenuW(g_sheetMenu, MF_STRING, MENU_GET_SHEET_COLOR, L"Get Sheet Color");
    if (g_workspace.m_sheets.begin() != g_workspace.m_sheets.end()) {
        AppendMenuW(g_sheetMenu, MF_SEPARATOR, 0, NULL);
        std::vector<Sheet *> sheets(g_workspace.m_sheets.begin(), g_workspace.m_sheets.end());
        std::sort(sheets.begin(), sheets.end(), SheetTitleLess());
        std::map<Sheet *, int> indices;
        int index = 0;
        for (std::list<Sheet *>::iterator it = g_workspace.m_sheets.begin(); it != g_workspace.m_sheets.end(); ++it)
            indices[*it] = index++;
        for (std::vector<Sheet *>::iterator it = sheets.begin(); it != sheets.end(); ++it)
            AppendMenuW(g_sheetMenu, g_workspace.getOpenSheet() == *it ? MF_CHECKED : MF_STRING,
                        indices[*it] + MENU_FIRST_SHEET, (*it)->getTitle().c_str());
    }
}

BOOL createFileMenu()
{
    for (int i = GetMenuItemCount(g_fileMenu) - 1; i >= 0; --i)
        DeleteMenu(g_fileMenu, i, MF_BYPOSITION);
    AppendMenuW(g_fileMenu, MF_STRING, MENU_NEW, L"&New");
    AppendMenuW(g_fileMenu, MF_STRING, MENU_OPEN, L"&Open...");
    AppendMenuW(g_fileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_fileMenu, MF_STRING, MENU_SAVE, L"&Save");
    AppendMenuW(g_fileMenu, MF_STRING, MENU_SAVE_AS, L"S&ave As...");
    AppendMenuW(g_fileMenu, MF_STRING, MENU_SAVE_COPY, L"S&ave Copy...");
    AppendMenuW(g_fileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_fileMenu, MF_STRING, MENU_OPEN_WORKSPACE, L"&Open Workspace...");
    AppendMenuW(g_fileMenu, MF_STRING, MENU_SAVE_WORKSPACE, L"&Save Workspace...");
    AppendMenuW(g_fileMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(g_fileMenu, MF_STRING, MENU_EXPORT_PDF, L"Export &PDF...");
    AppendMenuW(g_fileMenu, MF_STRING, MENU_IMPORT_PDF, L"Import &PDF/AI...");
    AppendMenuW(g_fileMenu, MF_STRING, MENU_IMPORT_SVG, L"Import S&VG...");
    AppendMenuW(g_fileMenu, MF_STRING, MENU_IMPORT_FONT, L"Import &TrueType Font...");
    if (g_recentFiles.size()) {
        AppendMenuW(g_fileMenu, MF_SEPARATOR, 0, NULL);
        int id = MENU_RECENT_FILE;
        for (std::list<std::wstring>::iterator it = g_recentFiles.begin(); it != g_recentFiles.end(); ++it) {
            // The menu shows the file name without its directory.
            int slash = it->rfind(L'/');
            int backslash = it->rfind(L'\\');
            if (slash > backslash)
                backslash = slash;
            std::wstring name = it->substr(backslash + 1, it->size() - backslash - 1);
            AppendMenuW(g_fileMenu, MF_STRING, id++, name.c_str());
        }
    }
    AppendMenuW(g_fileMenu, MF_SEPARATOR, 0, NULL);
    return AppendMenuW(g_fileMenu, MF_STRING, MENU_EXIT, L"E&xit");
}
