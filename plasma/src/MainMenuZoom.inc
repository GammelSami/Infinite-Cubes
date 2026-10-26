// Shared with the portable entry point and included by WindowsUI.cpp.
if (g_workspace.m_optionsModule->m_guiZoom == 1.0)
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_100, MF_CHECKED);
else if (g_workspace.m_optionsModule->m_guiZoom == (double)0.9f)
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_90, MF_CHECKED);
else if (g_workspace.m_optionsModule->m_guiZoom == (double)0.8f)
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_80, MF_CHECKED);
else if (g_workspace.m_optionsModule->m_guiZoom == 0.7f)
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_70, MF_CHECKED);
else if (g_workspace.m_optionsModule->m_guiZoom == 0.6f)
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_60, MF_CHECKED);
else if (g_workspace.m_optionsModule->m_guiZoom == 0.5f)
    CheckMenuItem(g_interfaceZoomMenu, MENU_INTERFACE_ZOOM_50, MF_CHECKED);
