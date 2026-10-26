// Shared with the portable entry point and included by WindowsUI.cpp.
case WM_COMMAND:
switch (LOWORD(wparam)) {
case MENU_NEW:
    g_workspace.newFile();
    break;
case MENU_RECENT_FILE:
    if (g_recentFiles.size() > 0) {
        std::list<std::wstring>::iterator file = g_recentFiles.begin();
        g_workspace.openPlasmaFile(std::wstring(*file));
    }
    break;
case MENU_RECENT_FILE + 1:
    if (g_recentFiles.size() > 1) {
        std::list<std::wstring>::iterator file = g_recentFiles.begin();
        ++file;
        g_workspace.openPlasmaFile(std::wstring(*file));
    }
    break;
case MENU_RECENT_FILE + 2:
    if (g_recentFiles.size() > 2) {
        std::list<std::wstring>::iterator file = g_recentFiles.begin();
        ++file;
        ++file;
        g_workspace.openPlasmaFile(std::wstring(*file));
    }
    break;
case MENU_RECENT_FILE + 3:
    if (g_recentFiles.size() > 3) {
        std::list<std::wstring>::iterator file = g_recentFiles.begin();
        ++file;
        ++file;
        ++file;
        g_workspace.openPlasmaFile(std::wstring(*file));
    }
    break;
case MENU_RECENT_FILE + 4:
    if (g_recentFiles.size() > 4) {
        std::list<std::wstring>::iterator file = g_recentFiles.begin();
        ++file;
        ++file;
        ++file;
        ++file;
        g_workspace.openPlasmaFile(std::wstring(*file));
    }
    break;
case MENU_NEW_SHEET:
    g_workspace.newSheet();
    break;
case MENU_CLOSE_SHEET:
    g_workspace.closeSheet();
    break;
case MENU_TEXTURE_SHEET:
    if (g_workspace.m_textureSheet)
        g_workspace.closeTextureSheet();
    else
        g_workspace.openTextureSheet();
    buildSheetMenu();
    break;
case MENU_HIDE_USER_INTERFACE:
    hideUserInterface();
    break;
case MENU_TILE_HORIZONTALLY:
    g_workspace.tileHorizontally();
    break;
case MENU_TILE_VERTICALLY:
    g_workspace.tileVertically();
    break;
case MENU_CASCADE:
    g_workspace.cascade();
    break;
case MENU_SET_SHEET_COLOR:
    g_workspace.setSheetColor();
    break;
case MENU_GET_SHEET_COLOR:
    g_workspace.getSheetColor();
    break;
case MENU_VIEW_PAGE:
    g_workspace.viewPage();
    break;
case MENU_VIEW_DRAWING:
    g_workspace.viewDrawing();
    break;
case MENU_VIEW_SELECTION:
    g_workspace.viewSelection();
    break;
case MENU_OPEN:
    g_workspace.openPlasmaFileDialog();
    break;
case MENU_IMPORT_SVG:
    g_workspace.importSVG();
    break;
case MENU_IMPORT_PDF:
    g_workspace.importPDF();
    break;
case MENU_IMPORT_PGD:
    g_workspace.importPGD();
    break;
case MENU_OPEN_WORKSPACE:
    g_workspace.openPlasmaWorkspace();
    break;
case MENU_SAVE_WORKSPACE:
    g_workspace.savePlasmaWorkspace(false);
    break;
case MENU_PRINT:
    print();
    break;
case MENU_SAVE:
    g_workspace.savePlasmaFile();
    break;
case MENU_SAVE_AS:
    g_workspace.savePlasmaFileAs();
    break;
case MENU_SAVE_COPY:
    g_workspace.savePlasmaFileCopy();
    break;
case MENU_EXPORT_POSTSCRIPT:
    g_workspace.exportPostscript();
    break;
case MENU_EXPORT_PDF:
    g_workspace.showExportPDF();
    break;
case MENU_EXPORT_WEB:
    g_workspace.exportWeb();
    break;
case MENU_IMPORT_FONT:
    g_workspace.importTTF();
    break;
case MENU_EXIT:
    if (g_workspace.canClose()) {
        g_workspace.closeAllSheets();
        PostMessageW(window, WM_QUIT, 0, 0);
    }
    break;
case MENU_OPTIONS:
    g_workspace.showOptions();
    break;
case MENU_PRODUCT_ACTIVATION:
#ifndef PLASMA_NO_LICENSING
    g_workspace.showProductActivation();
#endif
    break;
case MENU_ABOUT:
    plasma::Widget::setDisplayVisibility(g_workspace.m_aboutModule->getPanel()->getNode()->m_display, 1);
    break;
case MENU_ONLINE_MANUAL:
#ifndef PLASMA_NO_INTERNET
    ShellExecuteA(NULL, "open", "http://www.picroma.com/Plasma/Manual", NULL, NULL, SW_SHOWNORMAL);
#endif
    break;
case MENU_INTERFACE_ZOOM_100:
    g_workspace.setInterfaceZoom(1.0f, true);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_100, MF_CHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_90, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_80, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_70, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_60, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_50, MF_UNCHECKED);
    break;
case MENU_INTERFACE_ZOOM_90:
    g_workspace.setInterfaceZoom(0.9f, true);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_100, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_90, MF_CHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_80, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_70, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_60, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_50, MF_UNCHECKED);
    break;
case MENU_INTERFACE_ZOOM_80:
    g_workspace.setInterfaceZoom(0.8f, true);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_100, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_90, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_80, MF_CHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_70, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_60, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_50, MF_UNCHECKED);
    break;
case MENU_INTERFACE_ZOOM_70:
    g_workspace.setInterfaceZoom(0.7f, true);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_100, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_90, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_80, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_70, MF_CHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_60, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_50, MF_UNCHECKED);
    break;
case MENU_INTERFACE_ZOOM_60:
    g_workspace.setInterfaceZoom(0.6f, true);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_100, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_90, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_80, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_70, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_60, MF_CHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_50, MF_UNCHECKED);
    break;
case MENU_INTERFACE_ZOOM_50:
    g_workspace.setInterfaceZoom(0.5f, true);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_100, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_90, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_80, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_70, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_60, MF_UNCHECKED);
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_50, MF_CHECKED);
    break;
case MENU_SELECT_SAME_COLOR:
    g_workspace.selectSameColor();
    break;
case MENU_SELECT_PARENT:
    g_workspace.m_toolModule->m_nodeTool.selectParent();
    break;
case MENU_SELECT_CHILDREN:
    g_workspace.m_toolModule->m_nodeTool.selectChildren();
    break;
case MENU_SELECT_HIERARCHY:
    g_workspace.m_toolModule->m_nodeTool.selectHierarchy();
    break;
case MENU_SELECT_ALL:
    g_workspace.m_toolModule->m_nodeTool.selectAll();
    break;
case MENU_DESELECT_ALL:
    g_workspace.m_toolModule->m_nodeTool.deselectAll();
    break;
case MENU_MERGE_MESHES:
    g_workspace.mergeMeshes();
    break;
case MENU_TRACE_MESHES:
    g_workspace.traceMeshes();
    break;
case MENU_TEXT_TO_MESH:
    g_workspace.m_toolModule->m_textTool.textToMesh();
    break;
case MENU_STROKE_TO_MESH:
    g_workspace.strokeToMesh();
    break;
case MENU_CREATE_INNER_EDGES:
    g_workspace.createInnerEdges();
    break;
case MENU_PROJECT_TEXTURE_COORDINATES:
    g_workspace.m_toolModule->m_nodeTool.projectTextureCoordinates();
    break;
case MENU_RESET_TEXTURE_TRANSFORMATION:
    g_workspace.m_toolModule->m_nodeTool.resetTextureTransformation();
    break;
case MENU_CREATE_TEXTURED_RECTANGLE:
    g_workspace.createTexturedRectangle();
    break;
case MENU_OPEN_TEXTURED_RECTANGLE:
    g_workspace.openTexturedRectangle();
    break;
case MENU_ROTATE_45_LEFT:
    g_workspace.m_toolModule->m_nodeTool.rotate45Left();
    break;
case MENU_ROTATE_45_RIGHT:
    g_workspace.m_toolModule->m_nodeTool.rotate45Right();
    break;
case MENU_ROTATE_90_LEFT:
    g_workspace.m_toolModule->m_nodeTool.rotate90Left();
    break;
case MENU_ROTATE_90_RIGHT:
    g_workspace.m_toolModule->m_nodeTool.rotate90Right();
    break;
case MENU_ROTATE_180:
    g_workspace.m_toolModule->m_nodeTool.rotate180();
    break;
case MENU_FLIP_HORIZONTALLY:
    g_workspace.m_toolModule->m_nodeTool.flipHorizontally();
    break;
case MENU_FLIP_VERTICALLY:
    g_workspace.m_toolModule->m_nodeTool.flipVertically();
    break;
case MENU_APPLY_DEFORMATION:
    g_workspace.m_toolModule->m_vertexTool.applyDeformation();
    break;
case MENU_APPLY_ROTATION_AND_DEFORMATION:
    g_workspace.m_toolModule->m_vertexTool.applyRotationAndDeformation();
    break;
case MENU_COPY_INSTANCES:
    g_workspace.m_toolModule->m_nodeTool.uninstantiate();
    break;
case MENU_LOCK_OBJECTS:
    g_workspace.m_toolModule->m_nodeTool.lockObjects();
    break;
case MENU_LOCK_CHILDREN:
    g_workspace.m_toolModule->m_nodeTool.lockChildren();
    break;
case MENU_LOCK_CHILD_HIERARCHY:
    g_workspace.m_toolModule->m_nodeTool.lockChildrenRecursively();
    break;
case MENU_LOCK_HIERARCHY:
    g_workspace.m_toolModule->m_nodeTool.lockSelected();
    break;
case MENU_UNLOCK_CHILDREN:
    g_workspace.m_toolModule->m_nodeTool.unlockChildren();
    break;
case MENU_UNLOCK_HIERARCHY:
    g_workspace.m_toolModule->m_nodeTool.unlockSelected();
    break;
case MENU_UNLOCK_ALL:
    g_workspace.m_toolModule->m_nodeTool.unlockAll();
    break;
case MENU_VERTEX_FREE:
    g_workspace.m_toolModule->m_vertexTool.setFree();
    break;
case MENU_VERTEX_FIXED_ON_EDGE:
    g_workspace.m_toolModule->m_vertexTool.setFixedOnEdge();
    break;
case MENU_VERTEX_ROUNDED_CORNER:
    g_workspace.m_toolModule->m_vertexTool.setRoundedCorner();
    break;
case MENU_OPEN_PALETTE:
    g_workspace.m_paletteModule->openPalette();
    break;
case MENU_SAVE_PALETTE:
    g_workspace.m_paletteModule->savePalette();
    break;
case MENU_LOAD_DEFAULT_PALETTE:
    g_workspace.m_paletteModule->loadDefaultPalette();
    break;
case MENU_CLEAR_PALETTE:
    g_workspace.m_paletteModule->clearPalette();
    break;
case MENU_UNDO:
    g_workspace.m_historyModule->undo();
    break;
case MENU_REDO:
    g_workspace.m_historyModule->redo();
    break;
case MENU_CLEAR_HISTORY:
    if (g_workspace.getOpenDesignSheet())
        g_workspace.getOpenDesignSheet()->clearHistory();
    break;
case MENU_CUT:
    g_workspace.m_clipboardModule->cut();
    break;
case MENU_COPY:
    g_workspace.m_clipboardModule->copy();
    break;
case MENU_PASTE:
    g_workspace.m_clipboardModule->paste();
    break;
case MENU_PASTE_INSTANCE:
    g_workspace.m_clipboardModule->pasteInstance();
    break;
default:
    if ((int)(unsigned short)wparam - MENU_FIRST_SHEET >= 0) {
        int index = 0;
        for (std::list<Sheet *>::iterator it = g_workspace.m_sheets.begin(); it != g_workspace.m_sheets.end();
             ++it, ++index) {
            if (index == (unsigned short)wparam - MENU_FIRST_SHEET) {
                bool currentOnly = g_workspace.m_currentSheetOnly;
                if (currentOnly)
                    g_workspace.showAllSheets();
                g_workspace.setSheet(*it);
                g_workspace.m_toolModule->m_sheetTool.bringToFront();
                if (currentOnly)
                    g_workspace.showCurrentSheetOnly();
                break;
            }
        }
    }
    break;
}
break;
