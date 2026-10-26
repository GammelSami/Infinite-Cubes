// Shared with the portable entry point and included by WindowsUI.cpp.
CheckMenuItem(g_sheetMenu, MENU_TEXTURE_SHEET, g_workspace.m_textureSheet ? MF_CHECKED : MF_UNCHECKED);
EnableMenuItem(g_fileMenu, MENU_SAVE,
               g_workspace.getOpenDesignSheet() && (!g_workspace.getOpenDesignSheet()->m_path.size() ||
                                                    g_workspace.getOpenDesignSheet()->m_modified)
                   ? MF_ENABLED
                   : MF_GRAYED | MF_DISABLED);
EnableMenuItem(g_editMenu, MENU_UNDO,
               g_workspace.getOpenDesignSheet() && !g_workspace.getOpenDesignSheet()->m_undoStack.empty()
                   ? MF_ENABLED
                   : MF_GRAYED | MF_DISABLED);
EnableMenuItem(g_editMenu, MENU_REDO,
               g_workspace.getOpenDesignSheet() && !g_workspace.getOpenDesignSheet()->m_redoStack.empty()
                   ? MF_ENABLED
                   : MF_GRAYED | MF_DISABLED);
