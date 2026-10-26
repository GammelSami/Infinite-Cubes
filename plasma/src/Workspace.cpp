// Workspace.cpp: the editor's global workspace (g_workspace, WindowsUI.cpp): the engine and script machine it
// embeds, the modules, the sheets, the keyboard shortcuts and the interface's node tree.
#include "plasma/FilePath.h"
#include "plasma/PointerTypes.h"
#include "HttpClient.h"
#include <windows.h>
#include <GL/glew.h>
#include <algorithm>
#include <math.h>
#include <cmath>
#include <sstream>
#include <list>
#include <set>
#include <map>
#include <string>

#include "Workspace.h"
#include "ToolModule.h"
#include "SheetTool.h"
#include "DrawFreehandTool.h"
#include "DrawEdgeTool.h"
#include "ProceduralTool.h"
#include "PaletteModule.h"
#include "ShadowModule.h"
#include "OnlineModule.h"
#include "DrawFreehandModule.h"
#include "SequenceModule.h"
#include "Module.h"
#include "DesignSheet.h"
#include "Operations.h"
#include "CreateNodeOperation.h"
#include "ExportModule.h"
#include "NodeModule.h"
#include "TextInterface.h"
#include "Tool.h"
#include "ShapeInterface.h"
#include "plasma/ListWidget.h"
#include "plasma/Widget.h"
#include "plasma/Edit.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "OptionsModule.h"
#include "plasma/Display.h"
#include "plasma/RenderSurface.h"
#include "plasma/Shape.h"
#include "main.h"
#ifndef PLASMA_NO_LICENSING
#include "WindowsSystemAdapter.h"
#endif
#include "ColorModule.h"
#include "ColorManagementModule.h"
#include "SpecialColorModule.h"
#include "ProceduralShapeModule.h"
#include "ScaleBoxModule.h"
#include "ClipboardModule.h"
#include "GridModule.h"
#include "ZoomModule.h"
#include "StrokeWidthModule.h"
#include "LayerModule.h"
#include "ToolInfoModule.h"
#include "PerformanceModule.h"
#include "ColorToolModule.h"
#include "NodeToolModule.h"
#include "VertexModule.h"
#include "BoundingBoxTool.h"
#include "ExportPDFModule.h"
#include "AboutModule.h"
#include "StatusModule.h"
#include "ProgressModule.h"
#include "Thread.h"
#include "StampModule.h"
#include "TextureModule.h"
#include "FormatModule.h"
#include "WidgetModule.h"
#include "MeshModule.h"
#include "TextModule.h"
#include "MeshInterface.h"
#include "abstr/Method.h"
#include "plasma/PLXStreamReader.h"
#include <direct.h>
#include <fstream>
#include <iomanip>
#include "plasma/SmoothMeshShape.h"
#include "plasma/Drawing.h"
#include "svg/SVGReader.h"
#include "pdf/PDFReader.h"
#include <stdio.h>
#include "Image.h"
#include <GL/glu.h>
#include <GL/glew.h>
#include "plasma/FontEngine.h"
#include "plasma/ScalableFont.h"
#include "PaletteModule.h"

// FreeType (freetype/freetype.h), for ImportFont.
extern "C" int FT_Init_FreeType(FT_Library *library);
extern "C" int FT_Done_FreeType(FT_Library library);
extern "C" int FT_New_Face(FT_Library library, const char *path, long index, FT_Face *face);
extern "C" int FT_Done_Face(FT_Face face);
extern "C" int FT_Set_Pixel_Sizes(FT_Face face, unsigned width, unsigned height);
#ifndef PLASMA_PORTABLE
extern "C" int FT_Load_Char(FT_Face face, unsigned long character, long flags);
#endif

// The newest release's folder on the download server: release.xml's "directory" (the frame update;
// DownloadUpdate).
static std::string s_releaseDirectory;

// The licence server: "<host><path>/Activation/<request>/?id=<id>".
static std::string s_licenseHost = "picroma.de";
static std::string s_licensePath = "/LS";
static std::string s_downloadHost = "picroma.de";       // (Workspace::Init).
static std::string s_downloadPath = "/Download/Plasma"; // (Workspace::Init).

// The script natives' system adapter ("node:fixed"... Workspace::Init).
#ifndef PLASMA_NO_LICENSING
WindowsSystemAdapter g_systemAdapter;
#endif
static std::stringstream s_response; // The server's answer.

// The update check's results (release.xml, read by the frame update): the newest release's version and its files'
// paths.
static int s_releaseVersion;
static std::set<std::string> s_releaseFiles;
static std::list<std::string> s_downloadQueue; // The files still to download.
static DWORD s_rightButtonTime;                // When the right button went down.

static const int PhraseHashMultiplier = 31;
static const int ApplicationDirectoryBufferCapacity = 1024;
static const int DefaultScreenshotWidth = 600;
static const int DefaultScreenshotHeight = 600;
static const int LicenseStatusResponseLengthLimit = 5;
static const int StrokeWidthDragScale = 2;
static const int ContextMenuClickTimeLimitMilliseconds = 200;
static const int MaximumSealLength = 256;
static const int ImportedPGDMeshSubdivisions = 3;
static const int StrokeConversionCapSegments = 6;
static const int SolidStrokeConversionSubdivisionLimit = 2;
static const int ColorWorkerPollIntervalMilliseconds = 100;

// The update download's progress in percent.
static float getDownloadProgress()
{
    HttpClient *client = g_httpClient;
    unsigned int length = client->getContentLength();
    return length ? (float)client->getReceived() * 100.0 / (float)length : 0.0;
}

// This release's version number: the script's (GetPlasmaVersion) and the one release.xml's releases are compared
// with.
enum {
    PLASMA_VERSION = 101
};

// The script's version (native "layers:forward").
static int getPlasmaVersion()
{
    return PLASMA_VERSION;
}

// The obfuscation key of a phrase (the PLX reader's and Workspace::Init's "PlasmaXGraphics"): a polynomial hash
// from 2^50 - 27, modulo 2^64.
unsigned __int64 plasma::keyFromPhrase(const std::string &phrase)
{
    unsigned __int64 result = 0x0003FFFFFFFFFFE5ULL;
    for (int i = 0; i < static_cast<int>(phrase.size()); ++i)
        result = result * PhraseHashMultiplier + static_cast<unsigned char>(phrase[i]);
    return result;
}

// The dot product, summed in a float.
static inline float dot(const plasma::Vector<2, float> &a, const plasma::Vector<2, float> &b)
{
    float result = a[0] * b[0];
    for (int i = 1; i < 2; ++i)
        result += a[i] * b[i];
    return result;
}

// The squared distance from the point to the segment from start to end (as SquaredDistanceToSegment in
// HalfEdgeMesh.cpp, with a larger degenerate length).
static float segmentDistanceSquared(const plasma::Vector<2, float> &point, const plasma::Vector<2, float> &start,
                                    const plasma::Vector<2, float> &end)
{
    plasma::Vector<2, float> relative = point - start;
    plasma::Vector<2, float> direction = end - start;
    float length = direction[0] * direction[0] + direction[1] * direction[1];
    if (length < 1e-14f)
        return relative[0] * relative[0] + relative[1] * relative[1];
    float projection = relative[0] * direction[0];
    projection += relative[1] * direction[1];
    projection /= length;
    if (projection <= 0.0f)
        return relative[0] * relative[0] + relative[1] * relative[1];
    if (projection >= 1.0f) {
        relative = point - end;
        return relative[0] * relative[0] + relative[1] * relative[1];
    }
    direction = direction * projection;
    relative = relative - direction;
    return relative[0] * relative[0] + relative[1] * relative[1];
}

// The point of the segment from start to end nearest to the point.
static plasma::Vector<2, float> closestPointOnSegment(const plasma::Vector<2, float> &start,
                                                      const plasma::Vector<2, float> &end,
                                                      const plasma::Vector<2, float> &point)
{
    plasma::Vector<2, float> toPoint = point - start;
    plasma::Vector<2, float> direction = end - start;
    float t = dot(toPoint, direction) / (float)(direction.m_elements[0] * direction.m_elements[0] +
                                                direction.m_elements[1] * direction.m_elements[1]);
    if (t <= 0.0f)
        return start;
    if (t >= 1.0f)
        return end;
    return start + direction * t;
}

Workspace::Workspace()
    : m_restartAfterUpdate(false),
      m_unused350(0),
      m_sheet(0),
      m_designSheet(0),
      m_textureSheet(0),
      m_toolRadius(20.0f),
      m_textInterface(0),
      m_hoveredTextInterface(0),
      m_redrawRequested(false),
      m_popupMenu(0),
      m_currentSheetOnly(false),
      m_buttonDown(false),
      m_toolDragStarted(false),
      m_guiPointerNode(0),
      m_tabletPressure(1.0f),
      m_resizeToolRadius(false),
      m_resizeStrokeWidth(false),
      m_chooseParentRequested(false),
      m_sendBehindRequested(false),
      m_sendInFrontOfRequested(false),
      m_setPivotRequested(false),
      m_unused4CE(false),
      m_textureSheetPosition(0.1f, 0.1f),
      m_textureSheetSize(0.5f, 0.5f),
      m_activated(false),
      m_demo(true)
{
    m_keyActions['S'] = ACTION_SHEET_TOOL;
    m_keyActions['O'] = ACTION_NODE_TOOL;
    m_keyActions['V'] = ACTION_VERTEX_TOOL;
    m_keyActions['E'] = ACTION_EDGE_TOOL;
    m_keyActions['F'] = ACTION_FACE_TOOL;
    m_keyActions['C'] = ACTION_COLOR_TOOL;
    m_keyActions['X'] = ACTION_EXTRUSION_TOOL;
    m_keyActions['M'] = ACTION_DRAW_TOOL;
    m_keyActions['D'] = ACTION_SPLIT_TOOL;
    m_keyActions['T'] = ACTION_TEXT_TOOL;
    m_keyActions['H'] = ACTION_DRAW_FREEHAND_TOOL;
    m_keyActions['W'] = ACTION_WIDTH_TOOL;
    m_keyActions['G'] = ACTION_PROCEDURAL_TOOL;
    m_keyActions['P'] = ACTION_STAMP_TOOL;
    m_keyActions['Y'] = ACTION_WIDGET_TOOL;
    m_keyActions[VK_ESCAPE] = ACTION_RESET_TOOLS;
    m_keyActions[VK_TAB] = ACTION_TOGGLE_SELECT_ALL;
    m_keyActions[VK_NEXT] = ACTION_SEND_BACKWARD;
    m_keyActions[VK_PRIOR] = ACTION_BRING_FORWARD;
    m_keyActions[VK_END] = ACTION_SEND_TO_BACK;
    m_keyActions[VK_HOME] = ACTION_BRING_TO_FRONT;
    m_keyActions[VK_DELETE] = ACTION_DELETE;
    m_controlKeyActions['O'] = ACTION_OPEN;
    m_controlKeyActions['S'] = ACTION_SAVE;
    m_controlKeyActions['C'] = ACTION_COPY;
    m_controlKeyActions['X'] = ACTION_CUT;
    m_controlKeyActions['V'] = ACTION_PASTE;
    m_controlKeyActions['D'] = ACTION_DUPLICATE;
    m_controlKeyActions['Z'] = ACTION_UNDO;
    m_controlKeyActions['Y'] = ACTION_UNDO;
    m_controlShiftKeyActions['V'] = ACTION_PASTE_INSTANCE;
    m_controlShiftKeyActions['D'] = ACTION_DUPLICATE_INSTANCE;
    m_controlShiftKeyActions['Z'] = ACTION_REDO;
    m_controlShiftKeyActions['Y'] = ACTION_REDO;

    m_sheetsNode = m_engine.createNode(0, 0, 0, m_engine.m_root, L"sheet");
    m_sheetsNode->m_flags |= (1 << plasma::NODE_IGNORES_POINTER);
    m_guiNode = m_engine.createNode(0, 0, 0, m_engine.m_root, L"gui");
    m_scalableNode = m_engine.createNode(0, 0, 0, m_guiNode, L"scalable");
    m_guiLeftNode = m_engine.createNode(0, 0, 0, m_scalableNode, L"gui:left");
    m_guiRightNode = m_engine.createNode(0, 0, 0, m_scalableNode, L"gui:right");
    m_guiBottomNode = m_engine.createNode(0, 0, 0, m_scalableNode, L"gui:bottom");
    m_guiBottom2Node = m_engine.createNode(0, 0, 0, m_scalableNode, L"gui:bottom2");
    m_guiTopNode = m_engine.createNode(0, 0, 0, m_scalableNode, L"gui:top");
    m_engine.m_overlayRoot = m_guiNode;
    InitializeCriticalSectionAndSpinCount(&m_lock, 0x80000400);
}

Workspace::~Workspace()
{
    for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it)
        delete *it;
    for (std::list<Sheet *>::iterator it = m_sheets.begin(); it != m_sheets.end(); ++it)
        delete *it;
    DeleteCriticalSection(&m_lock);
#ifndef PLASMA_NO_INTERNET
    WSACleanup();
#endif
}

// Stops the download client, logs out synchronously if needed, and saves the palette before exit. The client is
// deleted without clearing g_httpClient.
void Workspace::cleanupForExit()
{
#ifndef PLASMA_NO_INTERNET
    if (g_httpClient) {
        HttpClient::cancelDownload();
        delete g_httpClient;
    }
#endif
#if !defined(PLASMA_NO_INTERNET) && !defined(PLASMA_NO_LICENSING)
    if (m_onlineModule->m_licenseMode == OnlineModule::LICENSE_LOGIN && m_activated && m_licenseKey.size()) {
        g_httpClient = new HttpClient(s_licenseHost, std::string("http"));
        s_response.str("");
        g_httpClient->download(s_licensePath + std::string("/Activation/Logout/?id=") + m_licenseKey, s_response);
        delete g_httpClient;
    }
#endif
    m_paletteModule->savePaletteFile(getAppDataPath() + L"\\palette.plp");
}

void Workspace::markSheetsChanged()
{
    for (std::list<Sheet *>::iterator it = m_sheets.begin(); it != m_sheets.end(); ++it)
        (*it)->m_root->m_transformation->m_changed = true;
    m_redrawRequested = true;
}

void Workspace::setDimensions(int width, int height)
{
    if (!width || !height)
        return;
    int oldWidth = m_engine.m_viewportWidth;
    int oldHeight = m_engine.m_viewportHeight;
    if (!oldWidth)
        oldWidth = 1;
    if (!oldHeight)
        oldHeight = 1;
    bool currentSheetOnly = m_currentSheetOnly;
    plasma::Vector<2, float> scale((float)width / oldWidth, (float)height / oldHeight);
    if (currentSheetOnly)
        showAllSheets();

    // The sheets' frames keep their margins (20 for the size, 10 for the position) while the rest scales.
    std::for_each(m_sheets.begin(), m_sheets.end(), [&scale](Sheet *sheet) {
        plasma::Vector<2, float> size =
            scale * (sheet->m_frame->getFrameSize() - plasma::Vector<2, float>(20.0f, 20.0f)) +
            plasma::Vector<2, float>(20.0f, 20.0f);
        plasma::Vector<2, float> position =
            scale * (sheet->m_frame->getViewPosition() + plasma::Vector<2, float>(10.0f, 10.0f)) -
            plasma::Vector<2, float>(10.0f, 10.0f);
        sheet->m_frame->setViewRect(position, size);
    });
    glViewport(0, 0, width, height);
    m_engine.setViewport(0, 0, width, height, true);
    if (currentSheetOnly)
        showCurrentSheetOnly();
    updateLayout();
}

// The script natives "gui:root", "gui:frame" and "gui:node" do nothing.
static void guiRoot(int) {}

static void guiFrame(float) {}

static void guiNode(std::string) {}

void Workspace::activate(std::string id)
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (g_httpClient)
        return;
    if (id.empty())
        return;
    g_httpClient = new HttpClient(s_licenseHost, "http");
    s_response.str("");
    HttpClient::startDownload(s_licensePath + std::string("/Activation/Activate/?id=") + id, s_response);
    m_serverRequest = REQUEST_ACTIVATE;
#endif
}

void Workspace::login(std::string id)
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (g_httpClient)
        return;
    if (id.empty())
        return;
    m_licenseKey = id;
    g_httpClient = new HttpClient(s_licenseHost, "http");
    s_response.str("");
    HttpClient::startDownload(s_licensePath + std::string("/Activation/Login/?id=") + id, s_response);
    m_serverRequest = REQUEST_LOGIN;
#endif
}

void Workspace::validate(std::string id)
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (g_httpClient)
        return;
    if (id.empty())
        return;
    m_licenseKey = id;
    g_httpClient = new HttpClient(s_licenseHost, "http");
    s_response.str("");
    HttpClient::startDownload(s_licensePath + std::string("/Activation/Validate/?id=") + id, s_response);
    m_serverRequest = REQUEST_VALIDATE;
#endif
}

void Workspace::logout(std::string id)
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (g_httpClient)
        return;
    if (id.empty())
        return;
    g_httpClient = new HttpClient(s_licenseHost, "http");
    s_response.str("");
    HttpClient::startDownload(s_licensePath + std::string("/Activation/Logout/?id=") + id, s_response);
    m_serverRequest = REQUEST_LOGOUT;
#endif
}

void Workspace::deactivate(std::string id)
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (g_httpClient)
        return;
    if (id.empty())
        return;
    g_httpClient = new HttpClient(s_licenseHost, "http");
    s_response.str("");
    HttpClient::startDownload(s_licensePath + std::string("/Activation/Deactivate/?id=") + id, s_response);
    m_serverRequest = REQUEST_DEACTIVATE;
#endif
}

bool Workspace::init()
{
    char directory[ApplicationDirectoryBufferCapacity];
    _getcwd(directory, sizeof directory);
    m_applicationPath = directory;
#ifndef PLASMA_NO_INTERNET
    WSADATA winsock;
    WSAStartup(MAKEWORD(2, 0), &winsock);
#endif
    m_guiNode->m_transformation->m_deformation.value().identity();
    m_guiNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_engine.addFontPath(widenString(getWindowsFontsPath()));
    m_engine.addFontPath(widenString(m_applicationPath + "/fonts"));
    plasma::Engine &engine = m_engine;
    engine.setupShadersAndFrameBuffer();

    unsigned __int64 key = plasma::keyFromPhrase(std::string("PlasmaXGraphics"));
    std::vector<unsigned __int64> keys(1, key);
    plasma::Node *icons = m_engine.createNode(0, 0, 0, 0, L"");
    if (!m_engine.readPlasmaFileIntoNode(L"icons.plx", icons,
                                         (1 << plasma::LOAD_SHARED) | (1 << plasma::LOAD_NO_WIDGET_INIT) |
                                             (1 << plasma::LOAD_IGNORE_PAGE) | (1 << plasma::LOAD_STATIC_MESHES),
                                         0, keys))
        return false;
    for (std::list<plasma::Node *>::iterator it = icons->m_children.begin(); it != icons->m_children.end(); ++it) {
        plasma::Node *icon = *it;
        m_icons[icon->getName()] = icon;
    }
    m_cursorArrow = icons->findNode(L"cursor:arrow");
    if (m_cursorArrow) {
        m_cursorArrow->m_flags |= (1 << plasma::NODE_INACTIVE);
        for (std::list<plasma::Node *>::iterator it = m_cursorArrow->m_children.begin();
             it != m_cursorArrow->m_children.end(); ++it)
            (*it)->setHitTestable(false);
        m_cursorArrow->m_display->m_visibility.value() = 0;
    }
    m_softwareCursor = m_cursorArrow;
    m_sheetTemplate = m_engine.createNode(0, 0, 0, 0, L"");
    m_progressDisplay = m_engine.createNode(0, 0, 0, 0, L"");
    try {
        m_engine.readPlasmaFileIntoNode(L"progress-display.plx", m_progressDisplay,
                                        (1 << plasma::LOAD_SHARED) | (1 << plasma::LOAD_NO_WIDGET_INIT) |
                                            (1 << plasma::LOAD_IGNORE_PAGE) | (1 << plasma::LOAD_STATIC_MESHES),
                                        0, keys);
        m_engine.readPlasmaFileIntoNode(L"tool-info.plx", m_guiNode,
                                        (1 << plasma::LOAD_NO_WIDGET_INIT) | (1 << plasma::LOAD_HINTED_TEXT), 0, keys);
        m_engine.readPlasmaFileIntoNode(
            L"tool-display.plx", m_scalableNode,
            (1 << plasma::LOAD_SHARED) | (1 << plasma::LOAD_NO_WIDGET_INIT) | (1 << plasma::LOAD_IGNORE_PAGE), 0, keys);
    } catch (...) {
        return false;
    }
    m_progressWidgetNode = m_progressDisplay->clone(0);
    m_engine.createWidget(m_progressWidgetNode, L"");
    m_progressWidgetNode->m_widget->m_flags |= (1 << plasma::WIDGET_ALWAYS_HIT);

    m_specialColorModule = new SpecialColorModule;
    m_modules.push_back(m_specialColorModule);
    m_colorManagementModule = new ColorManagementModule;
    m_modules.push_back(m_colorManagementModule);
    m_paletteModule = new PaletteModule;
    m_modules.push_back(m_paletteModule);
    m_colorModule = new ColorModule;
    m_modules.push_back(m_colorModule);
    m_toolInfoModule = new ToolInfoModule;
    m_modules.push_back(m_toolInfoModule);
    m_performanceModule = new PerformanceModule;
    m_modules.push_back(m_performanceModule);
    m_proceduralShapeModule = new ProceduralShapeModule;
    m_modules.push_back(m_proceduralShapeModule);
    m_proceduralShapeModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_colorToolModule = new ColorToolModule;
    m_modules.push_back(m_colorToolModule);
    m_colorToolModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_scaleBoxModule = new ScaleBoxModule;
    m_modules.push_back(m_scaleBoxModule);
    m_scaleBoxModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_nodeToolModule = new NodeToolModule;
    m_modules.push_back(m_nodeToolModule);
    m_nodeToolModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_shadowModule = new ShadowModule;
    m_modules.push_back(m_shadowModule);
    m_onlineModule = new OnlineModule;
    m_modules.push_back(m_onlineModule);
    m_aboutModule = new AboutModule;
    m_modules.push_back(m_aboutModule);
    m_exportPDFModule = new ExportPDFModule;
    m_modules.push_back(m_exportPDFModule);
    m_vertexModule = new VertexModule;
    m_modules.push_back(m_vertexModule);
    m_vertexModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_stampModule = new StampModule;
    m_modules.push_back(m_stampModule);
    m_stampModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_strokeWidthModule = new StrokeWidthModule;
    m_modules.push_back(m_strokeWidthModule);
    m_strokeWidthModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_drawFreehandModule = new DrawFreehandModule;
    m_modules.push_back(m_drawFreehandModule);
    m_drawFreehandModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_textureModule = new TextureModule;
    m_modules.push_back(m_textureModule);
    m_formatModule = new FormatModule;
    m_modules.push_back(m_formatModule);
    m_exportModule = new ExportModule;
    m_modules.push_back(m_exportModule);
    m_layerModule = new LayerModule;
    m_modules.push_back(m_layerModule);
    m_widgetModule = new WidgetModule;
    m_modules.push_back(m_widgetModule);
    m_toolModule = new ToolModule;
    m_modules.push_back(m_toolModule);
    m_nodeModule = new NodeModule;
    m_modules.push_back(m_nodeModule);
    m_meshModule = new MeshModule;
    m_modules.push_back(m_meshModule);
    m_historyModule = new HistoryModule;
    m_modules.push_back(m_historyModule);
    m_clipboardModule = new ClipboardModule;
    m_modules.push_back(m_clipboardModule);
    m_gridModule = new GridModule;
    m_modules.push_back(m_gridModule);
    m_zoomModule = new ZoomModule;
    m_modules.push_back(m_zoomModule);
    m_textModule = new TextModule;
    m_modules.push_back(m_textModule);
    m_animationModule = 0;
    m_sequenceModule = 0;
    m_optionsModule = new OptionsModule;
    m_modules.push_back(m_optionsModule);
    m_optionsModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_statusModule = new StatusModule;
    m_modules.push_back(m_statusModule);
    m_statusModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    m_progressModule = new ProgressModule;
    m_modules.push_back(m_progressModule);

    addRightPanel(m_colorModule->m_panel);
    addRightPanel(m_layerModule->m_panel);
    addBottomPanel(m_meshModule->m_panel);
    addBottomPanel(m_textModule->m_panel);
    addBottomPanel(m_textureModule->m_panel);
    addTopPanel(m_historyModule->m_panel);
    addTopPanel(m_clipboardModule->m_panel);
    addTopPanel(m_gridModule->m_panel);
    addTopPanel(m_zoomModule->m_panel);
    addTopPanel(m_formatModule->m_panel);
    addTopPanel(m_performanceModule->m_panel);
    addTopPanel(m_exportModule->m_panel);
    addBottom2Panel(m_nodeModule->m_panel);
    m_toolModule->m_panel->getNode()->setParent(m_guiLeftNode);
    if (m_animationModule && m_animationModule->m_panel)
        m_animationModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    if (m_sequenceModule && m_sequenceModule->m_panel)
        m_sequenceModule->m_panel->getNode()->m_display->m_visibility.value() = 0;

    m_engine.readSources(L"gui-elements.plx", key);
#ifndef PLASMA_NO_LICENSING
    m_machine.loadProgramFromFile("gui.plx");

    // The script's natives; their names are decoys (the names say nothing about the targets).
    WindowsSystemAdapter *adapter = &g_systemAdapter;
    m_machine.addFunction("gui:root", guiRoot);
    m_machine.addFunction("gui:frame", guiFrame);
    m_machine.addFunction("gui:node", guiNode);
    m_machine.addMethod("node:fixed", adapter, &WindowsSystemAdapter::coInitializeEx);
    m_machine.addMethod("node:visible", adapter, &WindowsSystemAdapter::coUninitialize);
    m_machine.addMethod("node:snapable", adapter, &WindowsSystemAdapter::coInitializeSecurity);
    m_machine.addMethod("node:nodeformer", adapter, &WindowsSystemAdapter::failed);
    m_machine.addMethod("node:deformer", adapter, &WindowsSystemAdapter::createLocator);
    m_machine.addMethod("node:align:outline", adapter, &WindowsSystemAdapter::connectServer);
    m_machine.addMethod("node:align:outline:rotation", adapter, &WindowsSystemAdapter::release);
    m_machine.addMethod("node:adapt", adapter, &WindowsSystemAdapter::coSetProxyBlanket);
    m_machine.addMethod("mesh:mesh", adapter, &WindowsSystemAdapter::execQuery);
    m_machine.addMethod("mesh:mesh:stroke", adapter, &WindowsSystemAdapter::next);
    m_machine.addMethod("mesh:contour", adapter, &WindowsSystemAdapter::getProperty);
    m_machine.addMethod("mesh:contour:stroke", adapter, &WindowsSystemAdapter::newBuffer);
    m_machine.addMethod("mesh:stroke", adapter, &WindowsSystemAdapter::freeBuffer);
    m_machine.addMethod("mesh:stroke:open", adapter, &WindowsSystemAdapter::setByte);
    m_machine.addMethod("mesh:extruded", adapter, &WindowsSystemAdapter::getByte);
    m_machine.addMethod("mesh:stroke:dashed", adapter, &WindowsSystemAdapter::getSize);
    m_machine.addMethod("text:reload", adapter, &WindowsSystemAdapter::resize);
    m_machine.addMethod("text:type:truetype", adapter, &WindowsSystemAdapter::setText);
    m_machine.addMethod("text:type:plasma", adapter, &WindowsSystemAdapter::getText);
    m_machine.addMethod("text:size", adapter, &WindowsSystemAdapter::loadFile);
    m_machine.addMethod("text:stroke", m_onlineModule, &OnlineModule::getLicenseSerial);
    m_machine.addMethod("text:horizontal:left", adapter, &WindowsSystemAdapter::getCreationTimeHigh);
    m_machine.addMethod("text:horizontal:right", adapter, &WindowsSystemAdapter::getCreationTimeLow);
    m_machine.addMethod("text:horizontal:center", adapter, &WindowsSystemAdapter::getAccessTimeHigh);
    m_machine.addMethod("text:vertical:top", adapter, &WindowsSystemAdapter::getAccessTimeLow);
    m_machine.addMethod("text:vertical:center", adapter, &WindowsSystemAdapter::getWriteTimeHigh);
    m_machine.addMethod("text:vertical:bottom", adapter, &WindowsSystemAdapter::getWriteTimeLow);
    m_machine.addMethod("texture:open", adapter, &WindowsSystemAdapter::openFile);
    m_machine.addMethod("texture:duplicate", adapter, &WindowsSystemAdapter::closeHandle);
    m_machine.addMethod("texture:delete", this, &Workspace::activate);
    m_machine.addMethod("texture:filter:nearest", adapter, &WindowsSystemAdapter::getModulePath);
    m_machine.addMethod("texture:filter:linear", this, &Workspace::readResponse);
    m_machine.addMethod("texture:filter:mipmap", this, &Workspace::readDecryptedSheet);
    m_machine.addMethod("texture:wrap:none", adapter, &WindowsSystemAdapter::saveFile);
    m_machine.addMethod("texture:wrap:repeat", this, &Workspace::deactivate);
    m_machine.addMethod("texture:wrap:mirror", this, &Workspace::login);
    m_machine.addMethod("layers:delete", this, &Workspace::logout);
    m_machine.addMethod("layers:new", this, &Workspace::validate);
    m_machine.addMethod("layers:front", adapter, &WindowsSystemAdapter::hash);
    m_machine.addFunction("layers:forward", getPlasmaVersion);
    m_machine.addMethod("layers:backward", adapter, &WindowsSystemAdapter::getModuleDrive);
    m_machine.addMethod("layers:back", adapter, &WindowsSystemAdapter::compare);
    m_machine.addMethod("layers:find", adapter, &WindowsSystemAdapter::getVolumeSerialNumber);

#else

    // Without licensing, the sheet the server would send is a local asset.
    if (!m_engine.readPlasmaFileIntoNode(L"sheet.plx", m_sheetTemplate,
                                         (1 << plasma::LOAD_SHARED) | (1 << plasma::LOAD_NO_WIDGET_INIT) |
                                             (1 << plasma::LOAD_IGNORE_PAGE) | (1 << plasma::LOAD_STATIC_MESHES) |
                                             (1 << plasma::LOAD_CHECK_SIGNATURE),
                                         0, std::vector<unsigned __int64>(1, 0)))
        return false;
    m_activated = true;
    m_demo = false;
    m_sheetTemplate->animate(0);
#endif
    m_engine.resolveSources();
    m_popupMenuTemplate = dynamic_cast<plasma::ListWidget *>(m_engine.m_sources->findWidget(L"popup"));
    m_popupMenuItem = m_engine.m_sources->findWidget(L"menuitem");
    m_popupMenuSeparator = m_engine.m_sources->findWidget(L"separator");
    m_engine.initializeWidgets();
    for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it)
        (*it)->build();
    m_toolModule->setTool(Tool::TOOL_NODE);
    setInterfaceZoom(m_optionsModule->m_guiZoom, false);
    updateLayout();
    if (m_cursorArrow)
        m_cursorArrow->setParent(m_engine.m_root);
    plasma::Vector<2, int> viewport(m_engine.m_viewportWidth, m_engine.m_viewportHeight);
    plasma::Vector<2, int> square(viewport.m_elements[1], viewport.m_elements[1]);
    m_textureSheetSize = plasma::Vector<2, float>(square) * 0.5f / plasma::Vector<2, float>(viewport);
    m_engine.m_root->animate(0);
    m_updateButton = m_engine.m_sources->findNode(L"updatebutton");
    m_guiNode->addChild(m_updateButton);
    m_updateButton->m_display->m_visibility.value() = 0;
    m_updateButton->connectWidget(L"updatebutton", plasma::WIDGET_EVENT_POINTER_PRESSED, this,
                                  &Workspace::downloadUpdate, true);
    m_onlineModule->update();
#ifndef PLASMA_NO_INTERNET
    m_serverRequest = REQUEST_RELEASE_INFO;
    g_httpClient = new HttpClient(s_downloadHost, "http");
    HttpClient::startDownload(s_downloadPath + "/release.xml", s_response);
#endif
    return true;
}

void Workspace::runScriptFunction(const std::string &name)
{
#if !defined(PLASMA_NO_LICENSING)
    EnterCriticalSection(&m_lock);
    abstr::Machine &machine = m_machine;
    int position = machine.m_symbols[name];
    if (position >= 0)
        machine.runProgram(position);
    machine.m_stack.pop_back(); // The entry point's result; popping an empty stack has no effect here.
    LeaveCriticalSection(&m_lock);
#endif
}

// WinMain's start-up script run: from the common application data folder, the script's "initialize", "update" and
// "button:press" entry points.
void Workspace::runStartupScript()
{
#if !defined(PLASMA_NO_LICENSING)
    _wchdir(getCommonAppDataPath().c_str());
    runScriptFunction("initialize");
    runScriptFunction("update");
    runScriptFunction("button:press");
#endif
}

// The "updatebutton" handler: once the sheets may close, makes the Update folder and starts downloading the newer
// release's first file (the frame update downloads the rest: REQUEST_UPDATE_FILE).
void Workspace::downloadUpdate()
{
#if !defined(PLASMA_NO_INTERNET)
    if (g_httpClient || !canClose())
        return;
    closeAllSheets();
    _mkdir((m_applicationPath + "\\Update").c_str());
    if (s_releaseVersion > PLASMA_VERSION && s_releaseFiles.size()) {
        s_downloadQueue.assign(s_releaseFiles.begin(), s_releaseFiles.end());
        g_httpClient = new HttpClient(s_downloadHost, "http");
        std::string file = s_downloadQueue.front();
        HttpClient::startDownload(s_downloadPath + std::string("/") + s_releaseDirectory + "/" + file, s_response);
        m_serverRequest = REQUEST_UPDATE_FILE;
    }
#endif
}

// The calls of RefreshFittedShapeInterfaces (counted, never read).
static int s_refreshCount;

// The shown subtree's nodes that fit their shape to their children and changed refresh their editing interfaces.
void Workspace::refreshFittedShapeInterfaces(plasma::Node *node)
{
    ++s_refreshCount;
    if (node->isShown()) {
        if (node->fitsShapeToChildren() && node->m_shape && node->m_changed)
            getOpenDesignSheet()->findSheetNode(node)->m_shapeInterface->refresh();
        for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
            refreshFittedShapeInterfaces(*it);
    }
}

// Not written anywhere in the program: the frame-rate overlay stays off, and the sum the overlay keeps beside the
// frame time is of a value nothing sets.
static bool s_showFrameRate;
static int s_unsetFrameValue;
static int s_renderTargetSwitches; // Engine::DrawWindow's count, never read.
int g_screenshotWidth = DefaultScreenshotWidth;
int g_screenshotHeight = DefaultScreenshotHeight;
int g_mouseX;
int g_mouseY;

// Layout positions, extents and spacing use display units; colour tuples style the overlays.
void Workspace::draw()
{
    glClearColor(0.9f, 0.9f, 0.9f, 1.0f);
    glClearDepth(0.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Blur: realtime sets RENDER_BLUR; disabled, or "afterwards" while a text is edited, clears it.
    if (m_performanceModule->m_blurMode == PerformanceModule::BLUR_REALTIME)
        m_engine.enableBlur();
    if (m_performanceModule->m_blurMode == PerformanceModule::BLUR_DISABLED ||
        (m_textInterface && m_performanceModule->m_blurMode != PerformanceModule::BLUR_REALTIME))
        m_engine.m_renderFlags &= ~(1 << plasma::RENDER_BLUR);

    // The export's pixel preview: the view shows the sheet at the export resolution (points at 72 per inch, the root's
    // zoom, the samples per pixel), aligned to the export's pixel grid.
    if (getDesignSheet()) {
        plasma::Transformation *transformation = getDesignSheet()->m_root->transformation();
        if (m_exportModule->m_pixelPreview) {
            int samples = m_exportModule->m_antialiasing;
            float scale = getDesignSheet()->m_page.size.m_elements[2] / 72.0 /
                          transformation->m_deformation.value().m_elements[0] * samples;
            if (scale > 1.0f) {
                getDesignSheet()->m_viewWidget->m_surfaceScale = 1.0f;
                plasma::Vector<2, float> offset(0.0f, 0.0f);
                getDesignSheet()->m_viewWidget->m_surfaceOffset = offset;
                getDesignSheet()->m_viewWidget->m_surfaceSamples = 1;
            } else {
                getDesignSheet()->m_viewWidget->m_surfaceScale = scale;
                plasma::Vector<2, float> min, max;
                m_exportModule->getExportBounds(min, max);
                int gridSamples = m_exportModule->m_antialiasing;
                plasma::Vector<2, float> corner =
                    getDesignSheet()->m_root->transformation()->m_matrix.transformPoint(min);
                plasma::Vector<2, float> offset = corner * scale / (float)gridSamples;
                plasma::Vector<2, int> pixel(offset);
                offset = offset - plasma::Vector<2, float>(pixel);
                plasma::Vector<2, int> margin(-m_exportModule->m_antialiasing, -m_exportModule->m_antialiasing);
                offset = plasma::Vector<2, float>(margin) + offset;
                offset = offset / (scale / m_exportModule->m_antialiasing);
                offset = plasma::Vector<2, float>(offset.m_elements[0] * -1.0, offset.m_elements[1] * -1.0);
                getDesignSheet()->m_viewWidget->m_surfaceOffset = offset;
                getDesignSheet()->m_viewWidget->m_surfaceSamples = m_exportModule->m_antialiasing;
            }
        } else {
            getDesignSheet()->m_viewWidget->m_surfaceScale = 1.0f;
            plasma::Vector<2, float> offset(0.0f, 0.0f);
            getDesignSheet()->m_viewWidget->m_surfaceOffset = offset;
            getDesignSheet()->m_viewWidget->m_surfaceSamples = 1;
        }
    }

    GLenum error = glGetError();
    if (error)
        fprintf(stderr, "OpenGL Error: %s\n", gluErrorString(error));
    s_renderTargetSwitches = m_engine.drawWindow();

    // Keep redrawing while something moves: a download or licence request, the special colour preview, a fading panel,
    // the palette or the scale box opening or closing.
    m_redrawRequested =
        (g_httpClient && g_httpClient->isRunning()) || m_serverRequest || m_specialColorModule->m_previewRunning;
    for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it) {
        if ((*it)->m_fade)
            m_redrawRequested = true;
    }
    if (m_paletteModule->m_expandDirection || m_scaleBoxModule->m_expandDirection)
        m_redrawRequested = true;

    if (s_showFrameRate) {
        static int frames;
        static int time;
        static int unsetValueSum;
        markDesignSheetChanged();
        m_redrawRequested = true;
        if (frames > 500) {
            frames = 0;
            time = 0;
            unsetValueSum = 0;
        }
        time += m_engine.m_frameTime;
        unsetValueSum += s_unsetFrameValue;
        frames++;
        if (time > 0) {
            std::wstringstream text;
            text << 1000 * frames / time << " fps" << std::endl;
            glEnable(GL_TEXTURE_2D);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // White on a black outline.
            m_engine.getFontEngine()->renderText(L"tahomabd.ttf", text.str(), 0.0f, 0.0f, 10.0f, 40.0f, 10.0f, 1.0f,
                                                 plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f,
                                                 true);
            m_engine.getFontEngine()->renderText(L"tahomabd.ttf", text.str(), 0.0f, 0.0f, 10.0f, 40.0f, 10.0f, 0.0f,
                                                 plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f).m_elements,
                                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                                 plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f,
                                                 true);
        }
    }

    // Screenshot mode (F1): a red frame of the screenshot's size around the pointer, and the size as text.
    if (g_screenshotMode) {
        glDisable(GL_DEPTH_TEST);
        glUseProgram(0);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_INDEX_ARRAY);
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_LINE_SMOOTH);
        glBegin(GL_LINE_LOOP);
        glColor3f(1.0f, 0.0f, 0.0f);
        glVertex2f(g_mouseX - g_screenshotWidth * 0.5 - 2.0, g_mouseY - g_screenshotHeight * 0.5 - 2.0);
        glVertex2f(g_mouseX + g_screenshotWidth * 0.5 + 2.0, g_mouseY - g_screenshotHeight * 0.5 - 2.0);
        glVertex2f(g_mouseX + g_screenshotWidth * 0.5 + 2.0, g_mouseY + g_screenshotHeight * 0.5 + 2.0);
        glVertex2f(g_mouseX - g_screenshotWidth * 0.5 - 2.0, g_mouseY + g_screenshotHeight * 0.5 + 2.0);
        glEnd();
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        std::wstringstream text;
        text << g_screenshotWidth << L" x " << g_screenshotHeight << std::endl;
        m_engine.getFontEngine()->renderText(L"tahomabd.ttf", text.str(), 0.0f, 0.0f, 10.0f, 40.0f, 10.0f, 0.0f,
                                             plasma::Vector<4, float>(1.0f, 0.0f, 0.0f, 1.0f).m_elements,
                                             plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements,
                                             plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f).m_elements, 0, -1.0f,
                                             true);
    }
}

// The online panel's "Login" (OnlineModule::Login): when no licence request is under way and the sheets may close,
// empties the licensed node, shows the progress display and runs the script's "update" and "frame" on a thread
// (the script sends the Login request).
void Workspace::startLogin()
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (!g_httpClient && canClose()) {
        m_sheetTemplate->destroyChildren();
        m_activated = false;
        m_engine.setPopUpWidget(m_progressWidgetNode->m_widget, 0);
        m_progressWidgetNode->play(L"progress", m_progressWidgetNode->getSequenceEnd(L"progress"));
        startThread([this]() {
            _wchdir(getCommonAppDataPath().c_str());
            runScriptFunction("update");
            runScriptFunction("frame");
        });
    }
#endif
}

// The online panel's "Logout": closes every sheet and the sheet template's children, shows the progress pop-up and
// sends the Logout request with the kept id (unlike StartLogin it keeps m_activated).
void Workspace::startLogout()
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (!g_httpClient && canClose()) {
        closeAllSheets();
        m_sheetTemplate->destroyChildren();
        m_engine.setPopUpWidget(m_progressWidgetNode->m_widget, 0);
        m_progressWidgetNode->play(L"progress", m_progressWidgetNode->getSequenceEnd(L"progress"));
        logout(m_licenseKey);
    }
#endif
}

// The online panel's "Activate": shows the progress display and runs the script's "update" and "click" on a
// thread.
void Workspace::startActivation()
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (!g_httpClient) {
        m_engine.setPopUpWidget(m_progressWidgetNode->m_widget, 0);
        m_progressWidgetNode->play(L"progress", m_progressWidgetNode->getSequenceEnd(L"progress"));
        startThread([this]() {
            _wchdir(getCommonAppDataPath().c_str());
            runScriptFunction("update");
            runScriptFunction("click");
        });
    }
#endif
}

// The online panel's "Deactivate": when the sheets may close, shows the progress display and runs the script's
// "popup" on a thread, after which the product counts as not activated.
void Workspace::startDeactivation()
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (canClose() && !g_httpClient) {
        closeAllSheets();
        m_engine.setPopUpWidget(m_progressWidgetNode->m_widget, 0);
        m_progressWidgetNode->play(L"progress", m_progressWidgetNode->getSequenceEnd(L"progress"));
        startThread([this]() {
            _wchdir(getCommonAppDataPath().c_str());
            runScriptFunction("popup");
            m_activated = false;
        });
    }
#endif
}

void Workspace::readResponse(int buffer)
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    std::vector<char> *bytes = (std::vector<char> *)buffer; // A system adapter buffer.
    s_response.seekg(0, std::ios::end);
    int size = s_response.tellg();
    if (size > 0) {
        bytes->resize(size);
        s_response.seekg(0, std::ios::beg);
        s_response.read(&(*bytes)[0], size);
    }
#endif
}

static void __stdcall showLicenseError(Workspace *workspace);

// The script that reads the decrypted licence sheet from a system adapter buffer into m_sheetTemplate: its "id"
// text decides the demo flag. A failure reports the licence request's error.
void Workspace::readDecryptedSheet(int buffer)
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    std::vector<char> *bytes = (std::vector<char> *)buffer; // A system adapter buffer.
    std::stringstream stream;
    stream.write(&(*bytes)[0], bytes->size());
    try {
        m_engine.readPLXFile(stream, m_sheetTemplate, 0x5b, 0, std::vector<unsigned __int64>(1, 0));
        _chdir(m_applicationPath.c_str());
        if (m_serverRequest != REQUEST_LOGIN)
            m_activated = true;
        m_demo = m_sheetTemplate->getText(L"id", true).size() == 0;
        m_onlineModule->update();
        m_sheetTemplate->animate(0);
    } catch (...) {
        m_activated = false;
        showLicenseError(this);
    }
#endif
}

// Tells why the activation (REQUEST_ACTIVATE) or login (REQUEST_LOGIN) failed: no connection, or the server's
// short answer ("5" activated or logged in elsewhere, "6" demo, "7" preview).
static void __stdcall showLicenseError(Workspace *workspace)
{
    if (!g_httpClient)
        return;
    if (g_httpClient->hasFailed()) {
        if (workspace->m_serverRequest == Workspace::REQUEST_ACTIVATE)
            showMessageBox(L"Plasma", L"Could not connect to activation server. Your product could not be activated.");
        if (workspace->m_serverRequest == Workspace::REQUEST_LOGIN)
            showMessageBox(L"Plasma", L"Could not connect to login server. Login failed. Please retry again later.");
        return;
    }
    std::string answer;
    if (g_httpClient->getReceived() < LicenseStatusResponseLengthLimit)
        answer = s_response.str();
    if (workspace->m_serverRequest == Workspace::REQUEST_ACTIVATE) {
        if (answer == "5")
            showMessageBox(
                L"Plasma",
                L"Your product is currently activated on a different computer. Please deactivate it there first.");
        else if (answer == "6")
            showMessageBox(L"Plasma", L"The demo version can be used online only. Please use the login button.");
        else if (answer == "7")
            showMessageBox(L"Plasma", L"The preview version can be used online only. Please use the login button.");
        else
            showMessageBox(L"Plasma", L"Your product could not be activated. Please enter a valid serial number.");
    } else if (workspace->m_serverRequest == Workspace::REQUEST_LOGIN) {
        if (answer == "5")
            showMessageBox(L"Plasma",
                           L"Login failed. Another computer is currently logged in with this serial number.");
        else
            showMessageBox(L"Plasma", L"Login failed. The trial license for this serial number might have expired or "
                                      L"an invalid serial number was used.");
    }
}

// The frame's update: the software cursor and the progress display follow the window, the licence server's or
// download server's answer is handled once its request ends, the sheets whose loading ended are shown (or closed
// after a failure), the changed shapes' editing interfaces refresh, the engine advances and the panels fade.
// Returns the engine's result.
bool Workspace::update(int milliseconds)
{
    if (m_softwareCursor) {
        if (g_screenshotMode) {
            m_softwareCursor->m_display->m_visibility.value() = 1;
            m_softwareCursor->m_transformation->m_translation.value() =
                m_engine.getPointer() - m_softwareCursor->m_transformation->m_pivot.value();
            m_softwareCursor->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        } else if (m_optionsModule->m_cursorHardware) {
            m_softwareCursor->m_display->m_visibility.value() = 0;
        }
    }
    if (m_animationModule)
        m_animationModule->advanceFrame(milliseconds);
    if (m_progressWidgetNode->m_display->m_visibility.value()) {
        plasma::Vector<2, int> viewport(m_engine.m_viewportWidth, m_engine.m_viewportHeight);
        plasma::Vector<2, float> center(viewport);
        center[0] *= (double)0.5f;
        center[1] *= (double)0.5f;
        m_progressWidgetNode->m_transformation->m_translation.value() = center;
        m_progressWidgetNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    }

#ifndef PLASMA_NO_INTERNET
    if (g_httpClient) {
        if (g_httpClient->isRunning()) {
            if (m_serverRequest == REQUEST_UPDATE_FILE) {
                m_progressModule->setText(std::wstring(L"Downloading update: ") + widenString(s_downloadQueue.front()));
                m_progressModule->setProgress(getDownloadProgress() * (double)0.01f);
            }
            m_redrawRequested = true;
        } else if (m_serverRequest) {
            if (m_serverRequest == REQUEST_RELEASE_INFO) {
                // Release.xml: the newest release, its notes for the update button's tooltip and its files.
                if (!g_httpClient->hasFailed()) {
                    s_response.seekg(0, std::ios::end);
                    int size = s_response.tellg();
                    if (size > 0) {
                        std::vector<char> bytes;
                        bytes.resize(size);
                        s_response.seekg(0, std::ios::beg);
                        s_response.read(&bytes[0], size);
                        pugi::xml_document document;
                        s_releaseFiles.clear();
                        s_releaseDirectory = "";
                        s_releaseVersion = 0;
                        if (document.load_buffer(&bytes[0], size)) {
                            pugi::xml_node root = document.child("root");
                            if (root) {
                                StatusInfo *tooltip = m_statusModule->m_tooltips["updatebutton"];
                                for (pugi::xml_node_iterator release = root.begin(); release != root.end(); release++) {
                                    if (strcmp(release->name(), "release") != 0)
                                        continue;
                                    s_releaseVersion = release->attribute("version").as_int();
                                    if (s_releaseVersion <= PLASMA_VERSION)
                                        continue;
                                    s_releaseDirectory = release->attribute("directory").value();
                                    if (tooltip)
                                        tooltip->m_text =
                                            widenString(std::string(release->child("notes").first_child().value()));
                                    for (pugi::xml_node_iterator file = release->begin(); file != release->end();
                                         file++) {
                                        if (strcmp(file->name(), "file") == 0) {
                                            std::string path = file->attribute("path").value();
                                            if (path.size())
                                                s_releaseFiles.insert(path);
                                        }
                                    }
                                }
                            }
                            if (s_releaseVersion > PLASMA_VERSION)
                                m_updateButton->m_display->m_visibility.value() = 1;
                        }
                    }
                }
                delete g_httpClient;
                g_httpClient = 0;
                s_response.str("");
                m_serverRequest = REQUEST_NONE;

                // Then the start-up licence request the options ask for.
                if (m_onlineModule->m_licenseMode == OnlineModule::LICENSE_ACTIVATE) {
                    if (m_sheetTemplate->m_children.size() == 0)
                        startActivation();
                    else
                        runScriptFunction("clientrect");
                } else if (m_onlineModule->m_licenseMode == OnlineModule::LICENSE_LOGIN) {
                    if (m_sheetTemplate->m_children.size() == 0)
                        startLogin();
                    else
                        runScriptFunction("clientrect");
                }
            } else if (m_serverRequest == REQUEST_UPDATE_FILE) {
                // An update file arrived: it goes to the Update folder; the next one is requested, or after the last the release
                // is installed over the program's files.
                m_progressModule->setProgress(1.0f);
                if (g_httpClient->hasFailed()) {
                    showMessageBox(L"Error", L"Could not download update. Please try again or visit www.picroma.com to "
                                             L"download the latest version manually.");
                    delete g_httpClient;
                    g_httpClient = 0;
                    s_response.str("");
                    m_serverRequest = REQUEST_NONE;
                } else {
                    std::string file = s_downloadQueue.front();
                    bool failed = false;
                    s_response.seekg(0, std::ios::end);
                    int size = s_response.tellg();
                    if (size > 0) {
                        std::vector<char> bytes;
                        bytes.resize(size);
                        s_response.seekg(0, std::ios::beg);
                        s_response.read(&bytes[0], size);
                        for (int i = 0; i < (int)file.size(); i++) {
                            if (file[i] == '/')
                                _mkdir((m_applicationPath + "/Update/" + file.substr(0, i)).c_str());
                        }
                        std::ofstream out(PLASMA_FILE_PATH(m_applicationPath + "/Update/" + file), std::ios::binary);
                        if (out.good())
                            out.write(&bytes[0], size);
                        else
                            failed = true;
                    }
                    s_downloadQueue.pop_front();
                    s_response.str("");
                    if (failed) {
                        delete g_httpClient;
                        g_httpClient = 0;
                        m_serverRequest = REQUEST_NONE;
                        showMessageBox(L"Plasma", L"Update failed. Please make sure that you have access rights to the "
                                                  L"Plasma program directory");
                    } else if (s_downloadQueue.size() == 0) {
                        delete g_httpClient;
                        m_serverRequest = REQUEST_NONE;
                        g_httpClient = 0;
                        for (std::set<std::string>::const_iterator it = s_releaseFiles.begin();
                             it != s_releaseFiles.end(); ++it) {
                            std::string file = *it;
                            for (int i = 0; i < (int)file.size(); i++) {
                                if (file[i] == '/')
                                    _mkdir((m_applicationPath + "/" + file.substr(0, i)).c_str());
                            }
                            std::string path = m_applicationPath + "/" + file;

                            // The running program can be renamed, not replaced.
                            if (file == "plasma.exe") {
                                remove("plasma_old.exe");
                                rename(path.c_str(), "plasma_old.exe");
                            } else {
                                remove(path.c_str());
                            }
                            rename((m_applicationPath + "/Update/" + file).c_str(), path.c_str());
                            for (int i = 0; i < (int)file.size(); i++) {
                                if (file[i] == '/')
                                    _rmdir((m_applicationPath + "/Update/" + file.substr(0, i)).c_str());
                            }
                        }
                        m_restartAfterUpdate = true;
                    } else {
                        std::string next = s_downloadQueue.front();
                        HttpClient::startDownload(s_downloadPath + std::string("/") + s_releaseDirectory + "/" + next,
                                                  s_response);
                    }
                }
            } else {
                // A licence request ended (see m_serverRequest).
                m_engine.setPopUpWidget(0, 0);
                if (m_serverRequest == REQUEST_ACTIVATE) {
                    _wchdir(getCommonAppDataPath().c_str());
                    runScriptFunction("release");
                    if (m_sheetTemplate->m_children.size() && !m_sheetsNode->m_children.size())
                        newSheet();
                } else {
                    if (m_serverRequest == REQUEST_DEACTIVATE) {
                        m_sheetTemplate->destroyChildren();
                        m_onlineModule->update();
                        if (g_httpClient->hasFailed()) {
                            showMessageBox(L"Plasma",
                                           L"Could not connect to activation server. Please try again later.");
                        } else {
                            std::string answer = s_response.str();
                            if (answer != "0")
                                showMessageBox(L"Plasma", L"Your product could not be deactivated because it seems to "
                                                          L"be installed on a different computer.");
                        }
                    }
                    if (m_serverRequest == REQUEST_LOGOUT)
                        m_onlineModule->update();
                    if (m_serverRequest == REQUEST_LOGIN && !m_sheetTemplate->m_children.size()) {
                        runScriptFunction("doubleclick");
                        if (m_sheetTemplate->m_children.size())
                            newSheet();
                    }
                    if (m_serverRequest == REQUEST_RELOGIN) {
                        std::string answer("");
                        if (!g_httpClient->hasFailed())
                            answer = s_response.str();
                        if (answer != "0") {
                            m_sheetTemplate->destroyChildren();
                            m_onlineModule->update();
                            showMessageBox(L"Plasma", L"Lost connection to server. Logging out.");
                        }
                    }
                    if (m_serverRequest == REQUEST_VALIDATE) {
                        std::string answer("0");
                        if (!g_httpClient->hasFailed())
                            answer = s_response.str();
                        if (answer != "0") {
                            m_sheetTemplate->destroyChildren();
                            closeAllSheets();
                            _wchdir(getCommonAppDataPath().c_str());
                            runScriptFunction("popup");
                            m_activated = false;
                            m_onlineModule->update();
                        }
                    }
                }
                delete g_httpClient;
                g_httpClient = 0;
                s_response.str("");
                m_serverRequest = REQUEST_NONE;
            }
        }
    }
#endif

    if (!m_engine.getFocusWidget() && m_scaleBoxModule->getExpansion() < 1.0f)
        m_scaleBoxModule->update();

    // The sheets whose loading ended.
    std::list<Sheet *> sheets = m_sheets;
    for (std::list<Sheet *>::iterator it = sheets.begin(); it != sheets.end(); ++it) {
        if (!(*it)->m_loadFinished)
            continue;
        (*it)->setBusy(false, false);
        (*it)->waitForThread();
        (*it)->applyBackgroundColor();
        if ((*it)->m_loadResult == Sheet::LOAD_SUCCEEDED) {
            if (*it == m_designSheet) {
                for (std::list<Module *>::iterator module = m_modules.begin(); module != m_modules.end(); ++module)
                    (*module)->sheetChanged();
                selectionChanged();
            }
            viewPage(*it);
            if (getOpenDesignSheet())
                updateWindowTitle(getOpenDesignSheet()->getTitle());
            if ((*it)->m_path.size())
                addRecentFile((*it)->m_path);
            (*it)->m_loadFinished = false;
            (*it)->m_loadResult = Sheet::LOAD_SUCCEEDED;
            (*it)->m_closeIfLoadFails = false;
        } else {
            (*it)->m_loadFinished = false;
            (*it)->m_loadResult = Sheet::LOAD_SUCCEEDED;
            (*it)->setPath(std::wstring(L""));
            if ((*it)->m_closeIfLoadFails) {
                // A sheet made for this file closes again.
                if (m_currentSheetOnly)
                    showAllSheets();
                m_engine.destroyNode((*it)->m_frame->getNode());
                m_sheets.remove(*it);
                delete *it;
                m_sheet = 0;
                m_designSheet = 0;
                for (std::list<plasma::Node *>::iterator child = m_sheetsNode->m_children.begin();
                     child != m_sheetsNode->m_children.end(); ++child) {
                    if ((*child)->m_display->m_visibility.value()) {
                        setSheet(static_cast<Sheet *>((*child)->m_userData));
                        break;
                    }
                }
            } else {
                (*it)->m_closeIfLoadFails = false;
            }
        }
    }

    // Realtime update: the shapes whose interfaces changed are told.
    if (m_performanceModule->m_realtimeUpdate && getOpenDesignSheet()) {
        DesignSheet *sheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it = sheet->m_sheetNodes.end();
        if (!sheet->isBusy())
            it = sheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_shapeInterface &&
                it->second->m_shapeInterface->m_refreshPending) {
                it->first->m_shape->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
                it->second->m_shapeInterface->updateChildren();
            }
        }
    }
    if (getOpenDesignSheet()) {
        refreshFittedShapeInterfaces(getOpenDesignSheet()->m_root);
        DesignSheet *sheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it = sheet->m_sheetNodes.end();
        if (!sheet->isBusy())
            it = sheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_shapeInterface) {
                // A changed shape refreshes its interface and those of the other selected nodes that show it.
                if (it->second->m_shapeInterface->m_refreshPending || it->first->m_shape->m_changed) {
                    it->second->m_shapeInterface->refresh();
                    for (std::set<plasma::Node *>::iterator node = it->first->m_shape->m_nodes.begin();
                         node != it->first->m_shape->m_nodes.end(); ++node) {
                        if (*node != it->first) {
                            NodeInterface *other = m_designSheet->findSheetNode(*node);
                            if (other && other->m_selected)
                                other->m_shapeInterface->refresh();
                        }
                    }
                }
                it->second->m_shapeInterface->m_refreshPending = false;
            }
        }
    }

    bool result = m_engine.advance(milliseconds);
    bool layoutChanged = false;
    for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it) {
        if ((*it)->m_fade) {
            (*it)->updateFade();
            layoutChanged = true;
        }
    }
    if (m_paletteModule->updateExpansion())
        layoutChanged = true;
    if (m_scaleBoxModule->updateExpansion())
        layoutChanged = true;
    if (m_guiNode->m_changed || layoutChanged)
        updateLayout();
    return result;
}

void Workspace::relogin()
{
#if !defined(PLASMA_NO_LICENSING) && !defined(PLASMA_NO_INTERNET)
    if (!m_serverRequest && m_licenseKey.size() && !g_httpClient && m_sheetTemplate->m_children.size() &&
        !m_activated) {
        m_serverRequest = REQUEST_RELOGIN;
        g_httpClient = new HttpClient(s_licenseHost, std::string("http"));
        s_response.str("");
        HttpClient::startDownload(s_licensePath + std::string("/Activation/Relogin/?id=") + m_licenseKey, s_response);
        m_redrawRequested = true;
    }
#endif
}

void Workspace::releaseButtons()
{
    if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT))
        pointerReleased();
    if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT))
        rightButtonReleased();
    if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE))
        middleButtonReleased();
    m_engine.resetInput();
    m_buttonDown = false;
}

void Workspace::characterEntered(int character)
{
    m_engine.characterEntered(character);
    m_redrawRequested = true;
    if (m_textInterface)
        m_textInterface->typeCharacter(character);
}

// Runs a keyboard shortcut's action (the action numbers of m_keyActions and its siblings). The arrangement actions
// go to the sheet tool when it is current (type 0), otherwise to the node tool.
void Workspace::dispatchAction(int action)
{
    switch (action) {
    case ACTION_UNDO:
        m_historyModule->undo();
        break;
    case ACTION_REDO:
        m_historyModule->redo();
        break;
    case ACTION_OPEN:
        openPlasmaFileDialog();
        break;
    case ACTION_CLOSE_SHEET:
        closeSheet();
        break;
    case ACTION_SAVE:
        savePlasmaFile();
        break;
    case ACTION_SAVE_AS:
        savePlasmaFileAs();
        break;
    case ACTION_DELETE:
        m_toolModule->m_currentTool->deleteSelected();
        break;
    case ACTION_COPY:
        m_clipboardModule->copy();
        break;
    case ACTION_CUT:
        m_clipboardModule->cut();
        break;
    case ACTION_PASTE:
        m_clipboardModule->paste();
        break;
    case ACTION_PASTE_INSTANCE:
        m_clipboardModule->pasteInstance();
        break;
    case ACTION_DUPLICATE:
        m_clipboardModule->duplicate();
        break;
    case ACTION_DUPLICATE_INSTANCE:
        m_clipboardModule->duplicateInstance();
        break;
    case ACTION_SHEET_TOOL:
        m_toolModule->setTool(Tool::TOOL_SHEET);
        break;
    case ACTION_NODE_TOOL:
        m_toolModule->setTool(Tool::TOOL_NODE);
        break;
    case ACTION_VERTEX_TOOL:
        m_toolModule->setTool(Tool::TOOL_VERTEX);
        break;
    case ACTION_EDGE_TOOL:
        m_toolModule->setTool(Tool::TOOL_EDGE);
        break;
    case ACTION_FACE_TOOL:
        m_toolModule->setTool(Tool::TOOL_FACE);
        break;
    case ACTION_COLOR_TOOL:
        m_toolModule->setTool(Tool::TOOL_COLOR);
        break;
    case ACTION_WIDTH_TOOL:
        m_toolModule->setTool(Tool::TOOL_WIDTH);
        break;
    case ACTION_EXTRUSION_TOOL:
        m_toolModule->setTool(Tool::TOOL_EXTRUSION);
        break;
    case ACTION_DRAW_TOOL:
        m_toolModule->setTool(Tool::TOOL_DRAW_FACE);
        break;
    case ACTION_SPLIT_TOOL:
        m_toolModule->setTool(Tool::TOOL_DRAW_EDGE);
        break;
    case ACTION_TEXT_TOOL:
        m_toolModule->setTool(Tool::TOOL_TEXT);
        break;
    case ACTION_WIDGET_TOOL:
        m_toolModule->setTool(Tool::TOOL_WIDGET);
        break;
    case ACTION_GENERIC_TOOL:
        m_toolModule->setTool(Tool::TOOL_GENERIC);
        break;
    case ACTION_DRAW_FREEHAND_TOOL:
        m_toolModule->setTool(Tool::TOOL_DRAW_FREEHAND);
        break;
    case ACTION_PROCEDURAL_TOOL:
        m_toolModule->setTool(Tool::TOOL_PROCEDURAL);
        break;
    case ACTION_STAMP_TOOL:
        m_toolModule->setTool(Tool::TOOL_STAMP);
        break;
    case ACTION_RESET_TOOLS:
        resetTools();
        break;
    case ACTION_TOGGLE_SELECT_ALL:
        m_toolModule->m_currentTool->toggleSelectAll();
        break;
    case ACTION_SEND_BACKWARD:
        if (m_toolModule->m_currentTool->getType() == Tool::TOOL_SHEET)
            m_toolModule->m_sheetTool.sendBackward();
        else
            m_toolModule->m_nodeTool.sendBackward();
        break;
    case ACTION_BRING_FORWARD:
        if (m_toolModule->m_currentTool->getType() == Tool::TOOL_SHEET)
            m_toolModule->m_sheetTool.bringForward();
        else
            m_toolModule->m_nodeTool.bringForward();
        break;
    case ACTION_SEND_TO_BACK:
        if (m_toolModule->m_currentTool->getType() == Tool::TOOL_SHEET)
            m_toolModule->m_sheetTool.sendToBack();
        else
            m_toolModule->m_nodeTool.sendToBack();
        break;
    case ACTION_BRING_TO_FRONT:
        if (m_toolModule->m_currentTool->getType() == Tool::TOOL_SHEET)
            m_toolModule->m_sheetTool.bringToFront();
        else
            m_toolModule->m_nodeTool.bringToFront();
        break;
    default:
        return;
    }
    m_redrawRequested = true;
}

// The outermost widget above the widget (the widget itself at the top).
static plasma::Widget *getRootWidget(plasma::Widget *widget)
{
    for (plasma::Widget *parent = widget->getParentWidget(); parent; parent = parent->getParentWidget())
        widget = parent;
    return widget;
}

// Shift+Tab: the edit field before the edit in its window (the last after the first); none when the window has
// fewer than two.
static plasma::Edit *getPreviousEdit(plasma::Edit *edit)
{
    plasma::Widget *root = getRootWidget(edit);
    if (!root)
        return 0;
    std::list<plasma::Edit *> edits;
    root->m_node->collectWidgets(edits);
    if (edits.size() < 2)
        return 0;
    bool found = false;
    for (std::list<plasma::Edit *>::reverse_iterator it = edits.rbegin(); it != edits.rend(); ++it) {
        if (*it == edit)
            found = true;
        else if (found)
            return *it;
    }
    return edits.back();
}

// Tab: the edit field after the edit in its window (the first after the last).
static plasma::Edit *getNextEdit(plasma::Edit *edit)
{
    plasma::Widget *root = getRootWidget(edit);
    if (!root)
        return 0;
    std::list<plasma::Edit *> edits;
    root->m_node->collectWidgets(edits);
    if (edits.size() < 2)
        return 0;
    bool found = false;
    for (std::list<plasma::Edit *>::iterator it = edits.begin(); it != edits.end(); ++it) {
        if (*it == edit)
            found = true;
        else if (found)
            return *it;
    }
    return edits.front();
}

// Raster conversion and export through FreeImage.
#include <FreeImage.h>

// Asks for a file and writes the image there as PNG (FreeImage's format 13): its rows bottom up as BGR(A),
// premultiplied colours divided by the alpha.
void Image::exportScreenshot()
{
    std::wstring path = saveFileBox(L"Export screenshot", 0, L"Portable Network Graphics (*.png)\0*.png\0", 1);
    if (path.empty())
        return;
    int bytesPerPixel = getBytesPerPixel();
    FIBITMAP *bitmap = FreeImage_Allocate(getWidth(), getHeight(), bytesPerPixel * 8);
    for (int y = 0; y < m_height; y++) {
        int height = FreeImage_GetHeight(bitmap);
        unsigned char *line = FreeImage_GetScanLine(bitmap, height - y - 1);
        for (int x = 0, offset = 0; x < m_width; x++, offset += bytesPerPixel) {
            if (bytesPerPixel == 4) {
                int alpha = m_rows[y][4 * x + 3];
                line[4 * x + 3] = (unsigned char)alpha;
                if (alpha) {
                    line[offset] = (unsigned char)(m_rows[y][offset + 2] * 255 / alpha);
                    line[offset + 1] = (unsigned char)(m_rows[y][offset + 1] * 255 / alpha);
                    line[offset + 2] = (unsigned char)(m_rows[y][offset] * 255 / alpha);
                    continue;
                }
            }
            line[offset] = m_rows[y][offset + 2];
            line[offset + 1] = m_rows[y][offset + 1];
            line[offset + 2] = m_rows[y][offset];
        }
    }
    FreeImage_SaveU(static_cast<FREE_IMAGE_FORMAT>(kFormatPNG), bitmap, path.c_str());
    FreeImage_Unload(bitmap);
}

// A key went down (WndProc's WM_KEYDOWN). F1 toggles the screenshot mode. The text being edited takes the key
// (Ctrl+C and Ctrl+V copy and paste its text); otherwise the tool, then a focused edit field (Tab moves the focus,
// Ctrl+C and Ctrl+V), else the shortcut of the key under the modifiers held.
void Workspace::keyPressed(unsigned short key)
{
    if (key == VK_F1) {
        g_screenshotMode = !g_screenshotMode;
        m_redrawRequested = true;
        return;
    }
    if (!m_engine.isKeyDown(key)) {
        m_redrawRequested = true;
        closePopupMenu();
    }
    m_engine.keyPressed(key);
    if (TextInterface *textInterface = m_textInterface) {
        if (m_engine.isKeyDown(VK_CONTROL)) {
            std::wstring text;
            switch (key) {
            case 'C':
                text = textInterface->getSelectedText();
                setClipboardText(text);
                return;
            case 'V':
                if (getClipboardText(text))
                    m_textInterface->insertText(text);
                return;
            }
        }
        m_textInterface->keyPressed(key);
        return;
    }
    getToolModule()->getCurrentTool()->keyPressed(key);
    if (m_engine.getFocusWidget()) {
        if (plasma::Edit *edit = dynamic_cast<plasma::Edit *>(m_engine.getFocusWidget())) {
            switch (key) {
            case VK_TAB: {
                plasma::Edit *next = m_engine.isKeyDown(VK_SHIFT) ? getPreviousEdit(edit) : getNextEdit(edit);
                m_engine.setFocusWidget(next);
                if (next)
                    next->selectAll();
                break;
            }
            case 'C':
                if (m_engine.isKeyDown(VK_CONTROL)) {
                    std::wstring text = edit->getSelectedText();
                    if (text.size())
                        setClipboardText(text);
                }
                break;
            case 'V':
                if (m_engine.isKeyDown(VK_CONTROL)) {
                    std::wstring text;
                    if (getClipboardText(text))
                        edit->insertText(text);
                }
                break;
            }
            return;
        }
    }
    if (m_engine.isKeyDown(VK_CONTROL) && m_engine.isKeyDown(VK_SHIFT))
        dispatchAction(m_controlShiftKeyActions[key]);
    else if (m_engine.isKeyDown(VK_CONTROL))
        dispatchAction(m_controlKeyActions[key]);
    else if (m_engine.isKeyDown(VK_SHIFT))
        dispatchAction(m_shiftKeyActions[key]);
    else
        dispatchAction(m_keyActions[key]);
}

void Workspace::keyReleased(unsigned short key)
{
    if (m_engine.isKeyDown(key))
        m_redrawRequested = true;
    m_engine.keyReleased(key);
    m_toolModule->m_currentTool->keyReleased(key);
    if (key == VK_SPACE)
        m_redrawRequested = true;
}

void Workspace::markDesignSheetChanged()
{
    if (getOpenDesignSheet())
        getOpenDesignSheet()->m_root->m_transformation->m_changed = true;
    m_redrawRequested = true;
}

// The current tool's IsRotating after the last pointer movement (written only).
static bool s_toolRotating;

// The pointer moved to (x, y) in window pixels. With the screenshot frame shown, the middle and right buttons
// resize it. Otherwise the engine and the software cursor follow, the modal node is the pointer's node under
// "gui", and the movement (m_pointerDeltaX, m_pointerDeltaY) uses up the cursor stickiness. A button drag then
// pans (middle button, or space bar held) or zooms (right button), sets the text tool's size (right) or stroke
// (Ctrl+right), the tool radius ("Resize Tool Radius...", right) or the stroke width (Ctrl+right while its panel
// shows, or tool 9), and the grid module can lock an axis. Without a button the hovered node follows the pointer;
// with the sheet tool, the left button moves or resizes the current sheet's frame (m_frameMove, m_frameResize);
// otherwise the tool's PointerMoved receives the pointer.
void Workspace::mouseMove(int x, int y)
{
    m_redrawRequested = true;
    static plasma::Vector<2, float> s_pointer((float)x, (float)y);
    g_mouseX = x;
    g_mouseY = y;
    if (g_screenshotMode) {
        if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE)) {
            int previous[2];
            for (int i = 0; i < 2; i++)
                previous[i] = (int)s_pointer.m_elements[i];
            plasma::Vector<2, int> growth =
                (plasma::Vector<2, int>(x, y) - plasma::Vector<2, int>(previous[0], previous[1])) * 2;
            g_screenshotWidth += growth.m_elements[0];
            g_screenshotHeight += growth.m_elements[1];
        }
        if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT))
            g_screenshotHeight += 2 * (y - (int)s_pointer.m_elements[1]);
        s_pointer = plasma::Vector<2, float>((float)x, (float)y);
        return;
    }
    m_hoveredNode = 0;
    m_hoveredTextInterface = 0;
    if (!m_buttonDown)
        m_engine.movePointer((float)x, (float)y);
    if (m_softwareCursor) {
        if (m_optionsModule->m_cursorHardware && !g_screenshotMode) {
            m_softwareCursor->m_display->m_visibility.value() = 0;
        } else {
            m_softwareCursor->m_display->m_visibility.value() = 1;
            m_softwareCursor->m_transformation->m_translation.value() =
                plasma::Vector<2, float>((float)x, (float)y) - m_softwareCursor->m_transformation->m_pivot.value();
            m_softwareCursor->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        }
    }
    if (!m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) || m_guiPointerNode) {
        plasma::Node *node = m_engine.getPointerNode();
        if (node && m_guiNode->isAncestorOf(node))
            m_guiPointerNode = node;
        else
            m_guiPointerNode = 0;
    }
    m_statusModule->updatePanel();
    if (m_toolInfoModule) {
        if (m_guiPointerNode && m_guiPointerNode->getName() == L"tool:display:background")
            m_toolInfoModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
        else
            m_toolInfoModule->m_panel->getNode()->m_display->m_visibility.value() = 0;
    }
    if (!m_buttonDown && m_guiPointerNode)
        return;

    plasma::Vector<2, float> delta = plasma::Vector<2, float>((float)x, (float)y) - s_pointer;
    m_pointerDeltaX = delta.m_elements[0];
    m_pointerDeltaY = delta.m_elements[1];
    s_pointer = plasma::Vector<2, float>((float)x, (float)y);
    m_cursorStickiness -= std::sqrt(m_pointerDeltaY * m_pointerDeltaY + m_pointerDeltaX * m_pointerDeltaX);
    bool panning;
    bool leftButton;
    if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE) ||
        ((leftButton = m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT)) ||
         m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT)) &&
            m_engine.m_pressedKeys.find(VK_SPACE) != m_engine.m_pressedKeys.end()) {
        panning = true;
    } else {
        Tool *tool = m_toolModule->m_currentTool;
        int type = tool->getType();
        panning = false;
        bool rightButton;
        if (type == Tool::TOOL_TEXT) {
            // The text tool: the right button sets the text size, with Ctrl the stroke.
            rightButton = m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT);
            if (rightButton && !m_engine.isKeyDown(VK_CONTROL)) {
                m_textModule->setSize(m_textModule->getSize() +
                                      m_pointerDeltaX / getOpenSheet()->getRoot()->getMatrix().m_elements[0]);
                return;
            }
            if (rightButton && m_engine.isKeyDown(VK_CONTROL)) {
                m_textModule->setStrokeWidth(m_textModule->getStrokeWidth() +
                                             m_pointerDeltaX / getOpenSheet()->getRoot()->getMatrix().m_elements[0]);
                return;
            }
        } else if (type != Tool::TOOL_PROCEDURAL && type != Tool::TOOL_STAMP) {
            if (m_resizeToolRadius || (rightButton = m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT)) &&
                                          !leftButton && !m_engine.isKeyDown(VK_CONTROL) &&
                                          type != Tool::TOOL_DRAW_FREEHAND && type != Tool::TOOL_EXTRUSION) {
                float radius = m_pointerDeltaX + m_toolRadius;
                m_toolRadius = radius;
                if (radius < 1.0f)
                    m_toolRadius = 1.0f;
                return;
            }
            if (m_resizeStrokeWidth ||
                rightButton && (m_engine.isKeyDown(VK_CONTROL) &&
                                    m_strokeWidthModule->m_panel->getNode()->m_display->m_visibility.value() ||
                                type == Tool::TOOL_DRAW_FREEHAND)) {
                float width =
                    m_strokeWidthModule->getWidth() +
                    StrokeWidthDragScale * m_pointerDeltaX / getOpenSheet()->getRoot()->getMatrix().m_elements[0];
                float minimum = 2.0 / getOpenSheet()->getRoot()->getMatrix().m_elements[0];
                if (width < minimum)
                    width = minimum;
                m_strokeWidthModule->setWidth(width);
                return;
            }
        }
        if (leftButton && tool->isRotating())
            collectSnapOffsets();
        if (!m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) && !m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT) ||
            !m_toolModule->m_currentTool->isRotating()) {
            bool selecting = false;
            BoundingBoxTool *boxTool = dynamic_cast<BoundingBoxTool *>(m_toolModule->m_currentTool);
            if (boxTool && boxTool->m_selectionMode)
                selecting = true;
            if ((m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) ||
                 m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT)) &&
                !selecting && m_toolModule->m_currentTool->getType()) {
                // An axis the grid module does not move keeps its position.
                if (!m_gridModule->m_moveY) {
                    y = (int)((float)y - m_pointerDeltaY);
                    s_pointer.m_elements[1] = (float)y;
                }
                if (!m_gridModule->m_moveX) {
                    x = (int)((float)x - m_pointerDeltaX);
                    s_pointer.m_elements[0] = (float)x;
                }
            }
            m_sceneMousePosition = plasma::Vector<2, float>((float)x, (float)y);
            if (m_cursorStickiness <= 0.0f && getOpenSheet()) {
                m_sheetPointer = getOpenSheet()->getRoot()->getInverseMatrix() * m_sceneMousePosition;
                m_previousDragPosition = m_dragPosition;
                m_dragPosition = m_sheetPointer;
                Tool *currentTool = m_toolModule->m_currentTool;
                plasma::Node *root = getOpenSheet()->getRoot();
                plasma::Vector<2, float> snapped =
                    root->getInverseMatrix() * getSnappedPointer(!currentTool->isDraggingHandle());
                m_dragPosition = snapped;
            }
        }
        s_toolRotating = m_toolModule->m_currentTool->isRotating();
    }
    if (!m_toolModule->m_currentTool->isOverHandle() && !panning)
        m_hoveredNode = m_sheetsNode->hitTest(m_sceneMousePosition, m_toolRadius,
                                              (1 << plasma::HIT_SKIP_LOCKED) | (1 << plasma::HIT_INCLUDE_INACTIVE) |
                                                  (1 << plasma::HIT_SKIP_INTERNAL));
    bool leftDown = m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT);
    if ((leftDown || m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT)) && !m_buttonDown)
        return;
    if (m_toolModule->m_currentTool->getType() == Tool::TOOL_SHEET && leftDown && m_sheet && !m_currentSheetOnly) {
        // The sheet tool drags the current sheet's frame.
        plasma::Vector<2, float> position = m_sheet->m_panel->getContentPosition();
        plasma::Vector<2, float> corner = position + m_sheet->m_panel->screenSize();
        bool outside = position.m_elements[0] < 0.0f || position.m_elements[1] < 0.0f ||
                       corner.m_elements[0] > m_engine.m_viewportWidth ||
                       corner.m_elements[1] > m_engine.m_viewportHeight;
        plasma::Vector<2, float> resize(m_pointerDeltaX, m_pointerDeltaY);
        resize.m_elements[0] = m_frameResize.m_elements[0] * resize.m_elements[0];
        resize.m_elements[1] = m_frameResize.m_elements[1] * resize.m_elements[1];
        plasma::Vector<2, float> size =
            m_sheet->m_frame->getFrameSize() + m_sheetsNode->m_inverseMatrix.transformDirection(resize);
        plasma::Vector<2, float> move(m_pointerDeltaX, m_pointerDeltaY);
        move.m_elements[0] = m_frameMove.m_elements[0] * move.m_elements[0];
        move.m_elements[1] = m_frameMove.m_elements[1] * move.m_elements[1];
        plasma::Vector<2, float> framePosition =
            m_sheet->m_frame->getViewPosition() + m_sheetsNode->m_inverseMatrix.transformDirection(move);
        m_sheet->m_frame->setViewRect(framePosition, size);
        m_engine.m_renderFlags &= ~(1 << plasma::RENDER_BLUR);
        if (outside)
            markDesignSheetChanged();
        return;
    }
    if (!getOpenSheet())
        return;
    if (panning) {
        if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT)) {
            if (m_performanceModule->m_blurMode != PerformanceModule::BLUR_REALTIME)
                m_engine.m_renderFlags &= ~(1 << plasma::RENDER_BLUR);
            zoomView(m_pointerDeltaY * (double)-0.1f, false);
            return;
        }

        // The root moves with the pointer, in its parent's coordinates.
        const plasma::Matrix<float> &inverse = getOpenSheet()->getRoot()->m_parent->getInverseMatrix();
        plasma::Vector<2, float> shift(
            inverse.m_elements[4] * m_pointerDeltaY + m_pointerDeltaX * inverse.m_elements[0],
            inverse.m_elements[5] * m_pointerDeltaY + inverse.m_elements[1] * m_pointerDeltaX);
        plasma::Vector<2, float> &translation = getOpenSheet()->getRoot()->m_transformation->m_translation.value();
        translation.m_elements[0] = shift.m_elements[0] + translation.m_elements[0];
        translation.m_elements[1] = shift.m_elements[1] + translation.m_elements[1];
        getOpenSheet()->getRoot()->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        getOpenSheet()->getRoot()->updateMatricesRecursively();
    } else {
        m_toolModule->m_currentTool->pointerMoved(x, y);
    }
    if (m_performanceModule->m_blurMode != PerformanceModule::BLUR_REALTIME &&
        (m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) || m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT) ||
         m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE)))
        m_engine.m_renderFlags &= ~(1 << plasma::RENDER_BLUR);
}

void Workspace::setSheet(Sheet *sheet)
{
    if (sheet == m_sheet)
        return;
    waitForColorWorker();
    resetTools();
    if (m_sheet)
        m_sheet->getFrame()->getNode()->getShape()->play(L"deactivate", 0);
    m_sheet = sheet;
    if (sheet)
        m_sheet->getFrame()->getNode()->getShape()->play(L"activate", 0);
    DesignSheet *designSheet = dynamic_cast<DesignSheet *>(sheet);
    if (designSheet) {
        m_designSheet = designSheet;
        updateWindowTitle(sheet->getTitle());
    }
    for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it)
        (*it)->sheetChanged();
    selectionChanged();
}

void Workspace::selectionChanged()
{
    if (m_sequenceModule)
        m_sequenceModule->update();
    refreshTimeline();
    redraw();
}

void Workspace::refreshPointer()
{
    m_engine.m_root->updateMatricesRecursively();
    mouseMove((int)m_engine.getPointer().m_elements[0], (int)m_engine.getPointer().m_elements[1]);
}

// Zooms the current sheet's view about its centre: with absolute, to delta times the sheet's zoom scale (the zoom
// module); otherwise by 1.5^delta (the mouse wheel).
void Workspace::zoomView(float delta, bool absolute)
{
    if (!getOpenSheet())
        return;
    // The view centre in sheet coordinates before zooming, halved with a double-precision 0.5.
    plasma::Vector<2, float> center;
    center = getOpenSheet()->m_panel->getViewSize();
    center.m_elements[0] *= 0.5;
    center.m_elements[1] *= 0.5;
    plasma::Vector<2, float> before = getOpenSheet()->m_root->m_transformation->m_inverseMatrix.transform(center);
    if (absolute) {
        if (delta < 0.0f)
            delta = 0.0f;
        getOpenSheet()->m_root->m_transformation->m_deformation.value().identity();
        getOpenSheet()->m_root->m_transformation->m_deformation.value().scale(
            getOpenSheet()->getZoomScale() * delta, getOpenSheet()->getZoomScale() * delta, 1.0f);
    } else {
        float scale = pow(1.5f, delta);
        getOpenSheet()->m_root->m_transformation->m_deformation.value().scale(scale, scale, 1.0f);
    }
    getOpenSheet()->m_root->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    getOpenSheet()->m_root->updateMatrices();

    // Moves the view so that the same point stays at the centre.
    plasma::Vector<2, float> newCenter;
    newCenter = getOpenSheet()->m_panel->getViewSize();
    newCenter.m_elements[0] *= 0.5;
    newCenter.m_elements[1] *= 0.5;
    plasma::Vector<2, float> after = getOpenSheet()->m_root->m_transformation->m_inverseMatrix.transform(newCenter);
    plasma::Vector<2, float> offset = after - before;
    plasma::Vector<2, float> shift = getOpenSheet()->m_root->m_transformation->m_matrix.transformDirection(offset);
    plasma::Vector<2, float> &translation = getOpenSheet()->m_root->m_transformation->m_translation.value();
    translation.m_elements[0] += shift.m_elements[0];
    translation.m_elements[1] += shift.m_elements[1];
    getOpenSheet()->m_root->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    getOpenSheet()->m_root->updateMatricesRecursively();
    m_zoomModule->update();
    getOpenSheet()->zoomChanged();
}

void Workspace::pointerWheel(int delta)
{
    m_redrawRequested = true;
    m_engine.pointerWheel(delta);
    if (m_guiPointerNode)
        return;
    zoomView((float)delta, false);
    if (!m_engine.isButtonDown(plasma::MOUSE_BUTTON_RIGHT))
        refreshPointer();
    m_toolModule->m_currentTool->pointerWheel(delta);
}

// A press that answers a pending "Choose Parent...", "Send Behind..." or "Send in Front of..." request: the
// selected nodes move relative to the node (the sheet's root when it has no sheet node), committed. False when no
// request is pending.
bool Workspace::pickRequestedNode(plasma::Node *node)
{
    if (m_chooseParentRequested && getDesignSheet()) {
        m_chooseParentRequested = false;
        if (node && getDesignSheet()->findSheetNode(node))
            moveSelectedNodes(0, node);
        else
            moveSelectedNodes(0, getDesignSheet()->m_root);
        getDesignSheet()->commit();
        return true;
    }
    if (m_sendBehindRequested && getDesignSheet()) {
        m_sendBehindRequested = false;
        if (node && getDesignSheet()->findSheetNode(node))
            moveSelectedNodesBehind(0, node);
        else
            moveSelectedNodesBehind(0, getDesignSheet()->m_root);
        getDesignSheet()->commit();
        return true;
    }
    if (m_sendInFrontOfRequested && getDesignSheet()) {
        m_sendInFrontOfRequested = false;
        std::list<plasma::Node *> nodes;
        getDesignSheet()->getSelectedNodes(nodes);
        if (node && getDesignSheet()->findSheetNode(node))
            moveSelectedNodesInFrontOf(0, node);
        else
            moveSelectedNodesInFrontOf(0, getDesignSheet()->m_root);
        getDesignSheet()->commit();
        return true;
    }
    return false;
}

// The left button (WM_LBUTTONDOWN). In screenshot mode (F1) it saves the frame around the pointer instead.
// Otherwise a press on the scene answers the pending requests ("Set Pivot..." takes the snapped pointer), makes
// the sheet under the pointer current and, unless the view pans, starts the tool's drag (PointerPressed). With the
// sheet tool, a press on the current sheet's view near an edge resizes the frame from that edge (m_frameResize,
// with m_frameMove for the left and top edges) and inside it moves the frame.
void Workspace::pointerPressed()
{
    m_redrawRequested = true;
    if (g_screenshotMode) {
        g_screenshotMode = false;
        glReadBuffer(GL_BACK);
        Image image(g_screenshotWidth, g_screenshotHeight, 3);
        for (int row = 0; row < g_screenshotHeight; row++)
            glReadPixels(g_mouseX - g_screenshotWidth / 2,
                         g_screenshotHeight / 2 - g_mouseY - row + m_engine.m_viewportHeight - 1, g_screenshotWidth, 1,
                         GL_RGB, GL_UNSIGNED_BYTE, image.m_rows[row]);
        image.exportScreenshot();
        return;
    }
    m_engine.pointerPressed();
    m_cursorStickiness = (float)m_optionsModule->m_cursorStickiness;
    if (m_guiPointerNode)
        return;
    m_buttonDown = true;
    if (m_resizeToolRadius) {
        m_resizeToolRadius = false;
        return;
    }
    if (m_resizeStrokeWidth) {
        m_resizeStrokeWidth = false;
        return;
    }
    if (m_setPivotRequested) {
        m_setPivotRequested = false;
        setPivotAt(getSnappedPointer(false));
        return;
    }
    if (pickRequestedNode(m_hoveredNode))
        return;
    activateHoveredSheet();
    m_frameMove[0] = 0.0f;
    m_frameMove[1] = 0.0f;
    m_frameResize[0] = 0.0f;
    m_frameResize[1] = 0.0f;
    if (getToolModule()->getCurrentTool()->getType() == Tool::TOOL_SHEET && m_sheet) {
        plasma::Vector<2, float> pointer = m_engine.m_pointer;
        plasma::Vector<2, float> point = m_sheet->m_panel->undeform(
            m_sheet->m_panel->getNode()->getInverseMatrix() * pointer, m_sheet->m_panel->getNode());
        plasma::Vector<2, float> minimum = m_sheet->m_panel->getNode()->m_shape->getMinimum();
        plasma::Vector<2, float> maximum = m_sheet->m_panel->getNode()->m_shape->getMaximum();
        if (m_sheet->m_panel->getNode()->m_shape && m_sheet->m_panel->getNode()->m_shape->contains(point)) {
            int inside = 0;
            if (point[0] < minimum[0] + 20.0) {
                m_frameMove[0] = 1.0f;
                m_frameResize[0] = -1.0f;
            } else if (point[0] >= maximum[0] - 20.0) {
                m_frameResize[0] = 1.0f;
            } else {
                inside++;
            }
            if (point[1] < minimum[1] + 20.0) {
                m_frameMove[1] = 1.0f;
                m_frameResize[1] = -1.0f;
            } else if (point[1] >= maximum[1] - 20.0) {
                m_frameResize[1] = 1.0f;
            } else {
                inside++;
            }
            if (inside == 2) {
                m_frameMove[0] = 1.0f;
                m_frameMove[1] = 1.0f;
            }
        }
    }
    if (getOpenSheet()) {
        m_sheetPointer = getOpenSheet()->m_root->getInverseMatrix() * m_sceneMousePosition;
        if (!isPanning()) {
            getToolModule()->getCurrentTool()->pointerPressed();
            m_toolDragStarted = true;
            m_previousDragPosition = m_sheetPointer;
            m_dragPosition = m_sheetPointer;
            collectSnapOffsets();
        }
    }
}

// Collects the snap offsets of the drag (m_sheetPointer): the current tool's own, else the selected shapes' (in
// the texture sheet's space on a texture sheet); for a tool of type 1 on a design sheet without the node tool
// panel's scale box, the selected nodes' pivots in the root's coordinates.
void Workspace::collectSnapOffsets()
{
    m_snapOffsets.clear();
    if (!getOpenDesignSheet())
        return;
    if (m_toolModule->getCurrentTool()->getType() == Tool::TOOL_NODE && !getTextureSheet() &&
        !m_nodeToolModule->m_scaleBox) {
        DesignSheet *designSheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (designSheet->isBusy())
            it = designSheet->m_sheetNodes.end();
        else
            it = designSheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                m_snapOffsets.push_back((getOpenDesignSheet()->getRoot()->getInverseMatrix() *
                                         (it->first->getMatrix() * it->first->m_transformation->m_pivot.value())) -
                                        m_sheetPointer);
            }
        }
    } else if (!m_toolModule->getCurrentTool()->getSnapOffsets(m_sheetPointer, m_snapOffsets)) {
        DesignSheet *designSheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (designSheet->isBusy())
            it = designSheet->m_sheetNodes.end();
        else
            it = designSheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface) {
                if (getTextureSheet()) {
                    ShapeInterface *shape = it->second->m_shapeInterface;
                    shape->setMatrix(getTextureSheet()->getMatrix());
                }
                it->second->m_shapeInterface->getSnapOffsets(m_sheetPointer, m_snapOffsets);
                it->second->m_shapeInterface->resetMatrix();
            }
        }
    }
}

void Workspace::activateHoveredSheet()
{
    // A sheet root under "sheet" keeps a pointer to its sheet owner.
    for (plasma::Node *node = m_hoveredNode; node; node = node->m_parent) {
        if (node->m_parent == m_sheetsNode && node->m_userData) {
            setSheet(static_cast<Sheet *>(node->m_userData));
            return;
        }
    }
}

void Workspace::pointerReleased()
{
    m_redrawRequested = true;
    m_buttonDown = false;
    m_engine.pointerReleased();
    if (m_engine.getFocusWidget())
        setTextInterface(0);
    if (m_guiPointerNode)
        return;
    if (m_toolDragStarted) {
        m_toolModule->m_currentTool->pointerReleased();
        m_toolDragStarted = false;
    }
    markDesignSheetChanged();
    m_engine.enableBlur();
    m_exportModule->updateWidgets();
    m_layerModule->updatePanel();
    m_nodeModule->updatePanel();
}

void Workspace::middleButtonPressed()
{
    m_redrawRequested = true;
    m_engine.middleButtonPressed();
    if (g_screenshotMode)
        return;
    closePopupMenu();
    if (m_guiPointerNode)
        return;
    m_buttonDown = true;
    activateHoveredSheet();
    m_toolModule->m_currentTool->middleButtonPressed();
}

void Workspace::middleButtonReleased()
{
    m_redrawRequested = true;
    m_buttonDown = false;
    m_engine.middleButtonReleased();
    if (m_guiPointerNode)
        return;
    m_toolModule->m_currentTool->middleButtonReleased();
    markDesignSheetChanged();
    m_engine.enableBlur();
}

// The right button on the scene: a press on the current sheet starts a drag (the tool's RightButtonPressed) unless
// the view pans; the context menu waits for the release.
void Workspace::rightButtonPressed()
{
    m_redrawRequested = true;
    m_engine.rightButtonPressed();
    if (g_screenshotMode)
        return;
    closePopupMenu();
    s_rightButtonTime = timeGetTime();
    m_cursorStickiness = (float)m_optionsModule->m_cursorStickiness;
    m_resizeToolRadius = false;
    m_resizeStrokeWidth = false;
    m_chooseParentRequested = false;
    m_sendBehindRequested = false;
    m_sendInFrontOfRequested = false;
    m_setPivotRequested = false;
    if (m_guiPointerNode)
        return;
    m_buttonDown = true;
    activateHoveredSheet();
    if (m_sheet && m_sheet->m_busy)
        return;
    if (!m_sheet)
        return;

    // The pointer in sheet-root coordinates, transformed explicitly.
    const plasma::Matrix<float> &inverse = getOpenSheet()->m_root->m_inverseMatrix;
    const float *m = inverse.m_elements;
    const plasma::Vector<2, float> &pointer = m_sceneMousePosition;
    float w = m[3] * pointer.m_elements[0] + m[7] * pointer.m_elements[1] + m[15];
    m_sheetPointer = plasma::Vector<2, float>(m[4] * pointer.m_elements[1] + pointer.m_elements[0] * m[0] + m[12],
                                              m[1] * pointer.m_elements[0] + m[5] * pointer.m_elements[1] + m[13]) *
                     (1 / w);
    if (isPanning())
        return;
    m_toolModule->getCurrentTool()->rightButtonPressed();
    m_previousDragPosition = m_sheetPointer;
    m_dragPosition = m_sheetPointer;
    collectSnapOffsets();
}

// The right button's release: a click (quick, within the cursor stickiness) or a shift click opens the context
// menu, unless control is held.
void Workspace::rightButtonReleased()
{
    m_redrawRequested = true;
    m_buttonDown = false;
    m_engine.rightButtonReleased();
    float moved = (float)m_optionsModule->m_cursorStickiness - m_cursorStickiness;
    if (((timeGetTime() - s_rightButtonTime < ContextMenuClickTimeLimitMilliseconds && moved < 20.0f) ||
         m_engine.isKeyDown(VK_SHIFT)) &&
        !m_engine.isKeyDown(VK_CONTROL))
        showPopupMenu();
    if (!m_guiPointerNode) {
        markDesignSheetChanged();
        m_engine.enableBlur();
        m_toolModule->getCurrentTool()->rightButtonReleased();
    }
}

void Workspace::pointerDoubleClicked()
{
    m_redrawRequested = true;
    m_engine.pointerDoubleClicked();
    if (!m_guiPointerNode)
        m_toolModule->m_currentTool->pointerDoubleClicked();
}

void Workspace::updateModules()
{
    for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it)
        (*it)->currentNodeChanged();
    redraw();
}

void Workspace::refreshTimeline()
{
    if (m_animationModule)
        m_animationModule->refresh(false);
}

// File menu "New": without a licence, asks for activation; without an open design sheet, opens one; otherwise
// empties the shown design sheet (asking to save it if modified).
void Workspace::newFile()
{
    setTextInterface(0);
    if (m_sheetTemplate->m_children.size() == 0) {
        showMessageBox(std::wstring(L"Plasma"), L"Please activate your product or use the online version.");
        m_onlineModule->update();
        m_onlineModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
        return;
    }
    if (!getOpenDesignSheet()) {
        newSheet();
        return;
    }
    if (getOpenDesignSheet()->getRoot()->m_display->m_visibility.value()) {
        if (getOpenDesignSheet()->m_modified) {
            if (!askSaveChanges())
                return;
            m_designSheet->waitForThread();
        }
        waitForColorWorker();
        getTextureModule()->deleteSheetTextures(getOpenDesignSheet());
        getOpenDesignSheet()->clear();

        // The code copies the empty name without discarding its allocation.
        {
            const std::wstring emptyName(L"");
            getOpenDesignSheet()->m_backupFileName = emptyName;
        }
        updateWindowTitle(getOpenDesignSheet()->getTitle());
        refreshTimeline();
        for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it)
            (*it)->sheetChanged();
        selectionChanged();
    }
}

// Sheet menu "New Sheet": without a licence, asks for activation and shows the online panel; otherwise opens a new
// design sheet and makes it current.
void Workspace::newSheet()
{
    if (m_sheetTemplate->m_children.size() == 0) {
        showMessageBox(std::wstring(L"Plasma"), L"Please activate your product or use the online version.");
        m_onlineModule->update();
        m_onlineModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
        return;
    }
    setTextInterface(0);
    bool currentSheetOnly = m_currentSheetOnly;
    if (currentSheetOnly)
        showAllSheets();
    DesignSheet *sheet = new DesignSheet(this);
    setSheet(sheet);
    m_sheets.push_back(sheet);
    sheet->m_viewWidget->updateSurface();
    updateWindowTitle(sheet->getTitle());
    if (currentSheetOnly)
        showCurrentSheetOnly();
    refreshTimeline();
}

// Asks "Save changes in <file name>?" for the open design sheet and saves it on "yes". False if there is none, on
// "cancel" or if the save fails.
bool Workspace::askSaveChanges()
{
    if (!getOpenDesignSheet())
        return false;
    int slash = getOpenDesignSheet()->m_path.rfind(L'/');
    int backslash = getOpenDesignSheet()->m_path.rfind(L'\\');
    int separator = (std::max)(slash, backslash);
    std::wstring name =
        getOpenDesignSheet()->m_path.substr(separator + 1, getOpenDesignSheet()->m_path.size() - separator - 1);
    if (name.empty())
        name = L" new design";
    std::wstringstream message;
    message << "Save changes in " << name << "?";
    int answer = askQuestion(L"Picroma Plasma", message.str().c_str(), true);
    if (answer == ANSWER_YES) {
        if (!savePlasmaFile())
            return false;
    } else if (answer == ANSWER_CANCEL) {
        return false;
    }
    return true;
}

// Before the program exits (and before DownloadUpdate, StartLogin, StartLogout and StartDeactivation): brings each
// modified design sheet to the front and asks to save it. False if the user cancels.
bool Workspace::canClose()
{
    std::list<Sheet *> sheets = m_sheets;
    for (std::list<Sheet *>::iterator it = sheets.begin(); it != sheets.end(); ++it) {
        DesignSheet *sheet = dynamic_cast<DesignSheet *>(*it);
        if (sheet && sheet->m_modified) {
            setSheet(sheet);
            m_toolModule->m_sheetTool.bringToFront();
            if (!askSaveChanges())
                return false;
        }
    }
    return true;
}

#ifdef PLASMA_NO_LICENSING

// The sealed signature of a file saved with a license key (a demo version's Seal chunk, which follows the
// PlasmaGraphics chunk), or "" if it has none.
static std::string readFileSeal(const std::wstring &path)
{
    std::ifstream stream(PLASMA_FILE_PATH(path.c_str()), std::ios::binary);
    if (!stream.is_open())
        return "";
    try {
        plasma::ChunkStreamReader reader(stream);
        for (int i = 0; i < 2 && stream; ++i) {
            std::string chunk = reader.parseChunkHeader();
            if (!stream)
                break;
            if (chunk == "PlasmaGraphics") {
                reader.skipChunk();
                continue;
            }
            if (chunk != "Seal")
                break;
            reader.enterChunk();
            int length = reader.readInt();
            if (!stream || length <= 0 || length > MaximumSealLength)
                break;
            std::string sealed(length, '\0');
            stream.read(&sealed[0], length);
            if (stream.gcount() == length)
                return sealed;
            break;
        }
    } catch (...) {
    }
    return "";
}

// Whether key unseals the signature (as PLXStreamReader::Read checks it).
static bool keyOpensSeal(const std::string &sealed, unsigned __int64 key)
{
    std::vector<char> clear(sealed.begin(), sealed.end());
    plasma::deobfuscate(&clear[0], static_cast<int>(clear.size()), key);
    return std::string(clear.begin(), clear.end()) == "PlasmaGraphics";
}

// Without licensing there is no license serial to open sealed files with, so a file sealed with a key the reader
// does not try by itself asks for its license key until one opens it. The key is any text; a serial typed with
// dashes, spaces or lower case letters also works (the online panel stores it as 16 upper case characters). False
// if the user cancels.
static bool chooseFileKey(const std::wstring &path, unsigned __int64 &key)
{
    key = 0;
    std::string sealed = readFileSeal(path);
    if (sealed.empty() || keyOpensSeal(sealed, 0) ||
        keyOpensSeal(sealed, plasma::keyFromPhrase(std::string("PlasmaGraphics"))))
        return true;
    std::wstring prompt = L"This file was saved with a license key. Enter the key to open it:";
    for (;;) {
        std::wstring answer;
        if (!askText(L"Picroma Plasma", prompt, answer))
            return false;
        std::string typed = narrowString(answer);
        std::string serial;
        for (size_t i = 0; i < typed.size(); ++i) {
            if (typed[i] != '-' && typed[i] != ' ')
                serial += static_cast<char>(toupper(static_cast<unsigned char>(typed[i])));
        }
        const std::string phrases[] = {typed, serial};
        for (int i = 0; i < 2; ++i) {
            if (keyOpensSeal(sealed, plasma::keyFromPhrase(phrases[i]))) {
                key = plasma::keyFromPhrase(phrases[i]);
                return true;
            }
        }
        prompt = L"That key does not open this file. Enter the license key it was saved with:";
    }
}
#endif

void Workspace::openPlasmaFileDialog()
{
    std::wstring path = openFileBox(L"Open Plasma File", L"Plasma graphics (*.plg, *.pld)\0*.plg;*.pld\0");
    openPlasmaFile(path);
}

// The documents opened since start-up (written only).
int g_openedFileCount;

// Opens the Plasma file in the design sheet (a new one without an open design sheet), asking to save the shown
// one's changes first. The file is read on a GL thread with the licence's key; the sheet is busy meanwhile.
// Without licensing, a sealed file asks for its key first (ChooseFileKey).
void Workspace::openPlasmaFile(const std::wstring &path)
{
    setTextInterface(0);
    waitForColorWorker();
    resetTools();
    if (path.empty())
        return;
    if (m_sheetTemplate->m_children.size() == 0) {
        showMessageBox(std::wstring(L"Plasma"), L"Please activate your product or use the online version.");
        m_onlineModule->update();
        m_onlineModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
        return;
    }
    if (m_designSheet)
        m_designSheet->waitForThread();
#ifdef PLASMA_NO_LICENSING
    unsigned __int64 key;
    if (!chooseFileKey(path, key))
        return;
#endif
    if (getOpenDesignSheet() && getOpenDesignSheet()->m_modified && !askSaveChanges())
        return;
    if (!getOpenDesignSheet())
        newSheet();
    if (!getOpenDesignSheet())
        return;
    _chdir(m_applicationPath.c_str());
    DesignSheet *sheet = getOpenDesignSheet();
    getTextureModule()->deleteSheetTextures(sheet);
    sheet->clear();
    ++g_openedFileCount;
    sheet->setPath(path);
    sheet->setBusy(true, false);
    sheet->m_root->setShown(false);
    sheet->m_loadResult = Sheet::LOAD_SUCCEEDED;
    closePopupMenu();
#ifndef PLASMA_NO_LICENSING
    std::string serial = m_onlineModule->getLicenseSerial();
    unsigned __int64 key = plasma::keyFromPhrase(serial);
#endif
    m_designSheet->m_thread = startGLThread([this, sheet, key]() {
        try {
            std::vector<unsigned __int64> keys(1, key);
            if (!sheet->m_workspace->m_engine.readPlasmaFileIntoNode(sheet->m_path, sheet->m_root,
                                                                     (1 << plasma::LOAD_SHARED) |
                                                                         (1 << plasma::LOAD_NO_WIDGET_INIT) |
                                                                         (1 << plasma::LOAD_CHECK_SIGNATURE),
                                                                     &sheet->m_page, keys)) {
                showMessageBox(std::wstring(L"Picroma Plasma"), L"Could not open file.");
                sheet->setPath(L"");
                sheet->m_loadResult = Sheet::LOAD_FAILED;
            }
            for (std::list<plasma::Node *>::iterator it = sheet->m_root->m_children.begin();
                 it != sheet->m_root->m_children.end(); ++it)
                (*it)->animate(0);
            sheet->rebuildSheetNodes();
        } catch (plasma::InvalidVersionException &) {
            showMessageBox(L"Picroma Plasma", L"The file you tried to open was created in a newer version of Plasma. "
                                              L"Please update your version to be able to open this file.");
            sheet->setPath(L"");
            sheet->m_loadResult = Sheet::LOAD_NEWER_VERSION;
        } catch (plasma::InvalidFileFormatException &) {
            showMessageBox(L"Picroma Plasma", L"The file you tried to open is no valid Plasma file.");
            sheet->setPath(L"");
            sheet->m_loadResult = Sheet::LOAD_INVALID_FILE;
        } catch (plasma::InvalidDemoLicenseException &) {
            showMessageBox(L"Picroma Plasma",
                           L"The file you tried to open has been created with a different demo license.");
            sheet->setPath(L"");
            sheet->m_loadResult = Sheet::LOAD_INVALID_FILE;
        }
        sheet->m_root->setShown(true);
        if (sheet->getBackupFileName().size() != 0) {
            sheet->setPath(sheet->getBackupFileName());
            {
                const std::wstring emptyName(L"");
                sheet->m_backupFileName = emptyName;
            }
        }
        sheet->m_loadFinished = true;
        sheet->m_thread = 0;
    });
}

// File menu "Import SVG...": reads the SVG file into the emptied design sheet (a new one without an open design
// sheet) on a GL thread.
void Workspace::importSVG()
{
    setTextInterface(0);
    resetTools();
    std::wstring path = openFileBox(L"Import SVG", L"Scalable Vector Graphics (*.svg)\0*.svg\0");
    if (path.size() == 0)
        return;
    if (getOpenDesignSheet() && getOpenDesignSheet()->m_modified && !askSaveChanges())
        return;
    if (!getOpenDesignSheet())
        newSheet();
    _chdir(m_applicationPath.c_str());
    DesignSheet *sheet = getOpenDesignSheet();
    sheet->clear();
    sheet->setPath(path);
    sheet->setBusy(true, false);
    sheet->m_root->setShown(false);
    sheet->m_loadResult = Sheet::LOAD_SUCCEEDED;
    closePopupMenu();
    sheet->m_thread = startGLThread([this, path, sheet]() {
        svg::SVGReader(&m_engine, path, &sheet->m_page, sheet->getRoot()).read();
        for (std::list<plasma::Node *>::iterator it = sheet->getRoot()->m_children.begin();
             it != sheet->getRoot()->m_children.end(); ++it)
            (*it)->animate(0);
        sheet->rebuildSheetNodes();
        sheet->setPath(L"");
        sheet->getRoot()->setShown(true);
        sheet->m_loadFinished = true;
        sheet->m_thread = 0;
    });
}

// File menu "Import PDF/AI...": as ImportSVG, for PDF and Illustrator files.
void Workspace::importPDF()
{
    setTextInterface(0);
    resetTools();
    std::wstring path = openFileBox(L"Import PDF", L"Portable Document Format (*.pdf; *.ai)\0*.pdf;*.ai\0");
    if (path.size() == 0)
        return;
    if (getOpenDesignSheet() && getOpenDesignSheet()->m_modified && !askSaveChanges())
        return;
    if (!getOpenDesignSheet())
        newSheet();
    _chdir(m_applicationPath.c_str());
    DesignSheet *sheet = getOpenDesignSheet();
    sheet->clear();
    sheet->setPath(path);
    sheet->setBusy(true, false);
    sheet->m_root->setShown(false);
    sheet->m_loadResult = Sheet::LOAD_SUCCEEDED;
    closePopupMenu();
    sheet->m_thread = startGLThread([this, path, sheet]() {
        pdf::PDFReader(sheet->getPage(), sheet->getRoot(), &m_engine, path).read();
        for (std::list<plasma::Node *>::iterator it = sheet->getRoot()->m_children.begin();
             it != sheet->getRoot()->m_children.end(); ++it)
            (*it)->animate(0);
        sheet->rebuildSheetNodes();
        sheet->setPath(L"");
        sheet->getRoot()->setShown(true);
        sheet->m_loadFinished = true;
        sheet->m_thread = 0;
    });
}

// File menu "Open Plasma Workspace": the open sheets close and those the workspace file lists open again, each
// frame where it was ("PlasmaWorkspace", "Version", then per sheet "DesignSheet" with the document's path on the
// next line, or "TextureSheet" and whether it is open; then the frame's rectangle as fractions of the viewport, as
// SavePlasmaWorkspace writes it).
void Workspace::openPlasmaWorkspace()
{
    setTextInterface(0);
    resetTools();
    std::wstring path;
    path = openFileBox(L"Open Plasma Workspace", L"Plasma Workspaces (*.plw)\0*.plw\0");
    if (path.size()) {
        std::wifstream file(PLASMA_FILE_PATH(path));
        if (!file.is_open())
            return;
        std::wstring word;
        file >> word;
        if (word != L"PlasmaWorkspace")
            return;
        file >> word;
        if (word != L"Version")
            return;
        int version;
        file >> version;
        closeAllSheets();
        m_redrawRequested = true;
        while (!file.eof()) {
            file >> word;
            if (file.eof())
                break;
            if (word == L"DesignSheet") {
                std::wstring documentPath;
                wchar_t name[MAX_PATH];
                file.getline(name, 1);
                file.getline(name, MAX_PATH);
                newSheet();
                float x, y, width, height;
                file >> x;
                file >> y;
                file >> width;
                file >> height;
                getOpenSheet()->m_frame->setViewRect(
                    m_engine.viewportWidth() * x - 10.0, m_engine.viewportHeight() * y - 10.0,
                    m_engine.viewportWidth() * width + 20.0, m_engine.viewportHeight() * height + 20.0);
                openPlasmaFile(name);
            } else if (word == L"TextureSheet") {
                int open;
                file >> open;
                if (open) {
                    if (!m_textureSheet)
                        openTextureSheet();
                } else if (m_textureSheet) {
                    closeTextureSheet();
                }
                setSheet(m_textureSheet);
                float x, y, width, height;
                file >> x;
                file >> y;
                file >> width;
                file >> height;
                getOpenSheet()->m_frame->setViewRect(
                    m_engine.viewportWidth() * x - 10.0, m_engine.viewportHeight() * y - 10.0,
                    m_engine.viewportWidth() * width + 20.0, m_engine.viewportHeight() * height + 20.0);
            } else {
                break;
            }
        }
    }
}

// "Save Plasma Workspace" (*.plw): the open sheets and their frames' rectangles, as OpenPlasmaWorkspace reads
// them. The backup (DesignSheet's autosave) goes to "\Backup\workspace.plw" under the application's directory
// without asking and names each design sheet's file too.
void Workspace::savePlasmaWorkspace(bool backup)
{
    std::wstring path;
    if (backup) {
        path = widenString(m_applicationPath) + L"\\Backup\\workspace.plw";
    } else {
        path = saveFileBox(L"Save Plasma Workspace", 0, L"Plasma Workspaces (*.plw)\0*.plw\0", 1);
        if (path.size() == 0)
            return;
    }
    std::wofstream file(PLASMA_FILE_PATH(path));
    if (!file.is_open())
        return;
    file << L"PlasmaWorkspace" << std::endl;
    file << L"Version " << 1 << std::endl;
    for (std::list<Sheet *>::iterator it = m_sheets.begin(); it != m_sheets.end(); ++it) {
        DesignSheet *designSheet = dynamic_cast<DesignSheet *>(*it);
        if (designSheet) {
            if (backup)
                file << L"DesignSheet " << designSheet->getBackupFileName() << std::endl;
            else
                file << L"DesignSheet" << std::endl;
            file << designSheet->m_path << std::endl;
        } else {
            TextureSheet *textureSheet = dynamic_cast<TextureSheet *>(*it);
            file << L"TextureSheet " << (int)(textureSheet->m_frame->getNode()->m_display->m_visibility.value() != 0)
                 << std::endl;
        }
        file << (float)(((*it)->m_frame->viewX() + 10.0) / m_engine.viewportWidth()) << " ";
        file << (float)(((*it)->m_frame->viewY() + 10.0) / m_engine.viewportHeight()) << " ";
        file << (float)(((*it)->m_frame->viewWidth() - 20.0) / m_engine.viewportWidth()) << " ";
        file << (float)(((*it)->m_frame->getViewHeight() - 20.0) / m_engine.viewportHeight()) << std::endl;
    }
}

// Where the line through start along direction crosses the line through otherStart along otherDirection, as the
// parameter along direction.
static float intersectionParameter(const plasma::Vector<2, float> &start, const plasma::Vector<2, float> &direction,
                                   const plasma::Vector<2, float> &otherStart,
                                   const plasma::Vector<2, float> &otherDirection)
{
    float t =
        (start.m_elements[0] * otherDirection.m_elements[1] - otherStart.m_elements[0] * otherDirection.m_elements[1] -
         start.m_elements[1] * otherDirection.m_elements[0] + otherStart.m_elements[1] * otherDirection.m_elements[0]) /
        (direction.m_elements[0] * otherDirection.m_elements[1] -
         direction.m_elements[1] * otherDirection.m_elements[0]);
    return -t;
}

// File menu "Import PGD...": reads a Plasmapaint design ("former" text format) into the emptied design sheet (a
// new one without an open design sheet): its shapes become smooth meshes under the sheet's root, scaled by the
// design's resolution with y up. The page, texture and group sections are read past.
void Workspace::importPGD()
{
    setTextInterface(0);
    resetTools();
    std::wstring path = openFileBox(L"Import PGD", L"Plasmapaint Designs (*.pgd)\0*.pgd\0");
    if (path.size() == 0)
        return;
    if (getOpenDesignSheet() && getOpenDesignSheet()->m_modified && !askSaveChanges())
        return;
    if (!getOpenDesignSheet())
        newSheet();
    _chdir(m_applicationPath.c_str());
    DesignSheet *sheet = getOpenDesignSheet();
    sheet->clear();
    setSheet(sheet);
    std::ifstream file(PLASMA_FILE_PATH(path.c_str()));
    std::string format;
    file >> format;
    if (format != "former") {
        file.close();
        return;
    }
    float resolution = 100.0f;
    float gridX = 0.0f;
    float gridY = 0.0f;
    while (!file.eof()) {
        std::string section;
        file >> section;
        if (section == "resolution")
            file >> resolution;
        if (section == "grid") {
            file >> gridX;
            file >> gridY;
        }
        if (section == "horizontalGuides") {
            int count;
            file >> count;
            for (int i = 0; i < count; i++) {
                float guide;
                file >> guide;
            }
        }
        if (section == "verticalGuides") {
            int count;
            file >> count;
            for (int i = 0; i < count; i++) {
                float guide;
                file >> guide;
            }
        }
        if (section == "textures") {
            int count;
            file >> count;
            for (int i = 0; i < count; i++) {
                std::string header;
                file >> header;
                std::string name = "default";
                int width = 1;
                int height = 1;
                std::string key;
                std::vector<plasma::Vector<4, unsigned char>> pixels(1);
                float textureGridX = 0.1f;
                float textureGridY = 0.1f;
                do {
                    file >> key;
                    if (key == "name")
                        file >> name;
                    if (key == "width")
                        file >> width;
                    if (key == "height")
                        file >> height;
                    if (key == "grid") {
                        file >> textureGridX;
                        file >> textureGridY;
                    }
                    if (key == "horizontalGuides") {
                        int guides;
                        file >> guides;
                        for (int j = 0; j < guides; j++) {
                            float guide;
                            file >> guide;
                        }
                    }
                    if (key == "verticalGuides") {
                        int guides;
                        file >> guides;
                        for (int j = 0; j < guides; j++) {
                            float guide;
                            file >> guide;
                        }
                    }
                    if (key == "pixels") {
                        pixels.resize(width * height);
                        for (int j = 0; j < width * height; j++) {
                            plasma::Vector<4, int> pixel;
                            file >> pixel[0];
                            file >> pixel[1];
                            file >> pixel[2];
                            file >> pixel[3];
                            pixels[j] = pixel;
                        }
                    }
                } while (key != "}");
            }
        }
        if (section == "shapes") {
            int count;
            file >> count;
            for (int i = 0; i < count; i++) {
                std::string header;
                file >> header;
                std::string key;
                plasma::SmoothMeshShape *shape = m_engine.createSmoothMeshShape(L"");
                plasma::Node *node = m_engine.createNode(0, shape, 0, sheet->getRoot(), L"");
                do {
                    file >> key;
                    if (key == "name") {
                        std::string name;
                        file >> name;
                        shape->setName(widenString(name));
                        node->setName(widenString(name));
                    }
                    if (key == "locked") {
                        bool locked;
                        file >> locked;
                        if (locked)
                            node->m_flags |= (1 << plasma::NODE_LOCKED);
                        else
                            node->m_flags &= ~(1 << plasma::NODE_LOCKED);
                    }
                    if (key == "visible") {
                        bool visible;
                        file >> visible;
                        node->m_display->m_visibility.value() = visible ? 1 : 0;
                    }
                    if (key == "texture") {
                        int texture;
                        file >> texture;
                    }
                    if (key == "vertexPositions") {
                        int vertices;
                        file >> vertices;
                        shape->m_vertexPositions.resize(vertices);
                        for (int j = 0; j < vertices; j++) {
                            plasma::Vector<2, float> position;
                            file >> position[0];
                            file >> position[1];
                            position[0] *= resolution;
                            position[1] *= resolution;
                            position[1] *= -1.0;
                            shape->m_vertexPositions.value()[j] = position;
                        }
                    }
                    if (key == "vertexColors") {
                        int vertices;
                        file >> vertices;
                        shape->m_vertexColors.resize(vertices);
                        for (int j = 0; j < vertices; j++) {
                            plasma::Vector<4, float> color;
                            file >> color[0];
                            file >> color[1];
                            file >> color[2];
                            file >> color[3];
                            shape->m_vertexColors.value()[j] = color;
                        }
                    }
                    if (key == "vertexFlags") {
                        int vertices;
                        file >> vertices;
                        shape->m_vertexFlags.resize(vertices);
                        for (int j = 0; j < vertices; j++)
                            file >> shape->m_vertexFlags[j];
                    }
                    if (key == "faceVertices") {
                        int faceCount;
                        file >> faceCount;
                        std::vector<std::vector<int>> faces;
                        faces.resize(faceCount);
                        for (int j = 0; j < faceCount; j++) {
                            int corners;
                            file >> corners;
                            faces[j].resize(corners);
                            for (int k = 0; k < corners; k++)
                                file >> faces[j][k];
                        }
                        shape->setFaces(faces, false);
                    }
                } while (key != "}");
                shape->setSubdivisions(ImportedPGDMeshSubdivisions, true);
            }
        }
        if (section == "groups") {
            int count;
            file >> count;
            for (int i = 0; i < count; i++) {
                std::string header;
                file >> header;
                std::string key;
                do {
                    file >> key;
                    if (key == "name") {
                        std::string name;
                        file >> name;
                    }
                    if (key == "shapes") {
                        int shapes;
                        file >> shapes;
                        for (int j = 0; j < shapes; j++) {
                            int index;
                            file >> index;
                        }
                    }
                } while (key != "}");
            }
        }
    }
    file.close();
    sheet->rebuildSheetNodes();
    m_layerModule->updatePanel();
    m_exportModule->updatePanel();
    m_textureModule->updateSelection();
    refreshTimeline();
    selectionChanged();
}

// File menu "Save": a modified document with a path is saved there; a new one, or one whose extension does not
// suit the version (".PLG" in the demo, ".PLD" otherwise: the licence sheet's last child is "id" unless in the
// demo), asks for a file first. False without a shown open design sheet, if there is nothing to save or the user
// cancels.
bool Workspace::savePlasmaFile()
{
    if (!getOpenDesignSheet())
        return false;
    if (!getOpenDesignSheet()->getRoot()->display()->m_visibility.value())
        return false;
    if (getOpenDesignSheet()->m_busy)
        return false;
    int dot = getOpenDesignSheet()->m_path.rfind(L'.');
    std::wstring extension =
        getOpenDesignSheet()->m_path.substr(dot + 1, getOpenDesignSheet()->m_path.size() - dot - 1);
    std::transform(extension.begin(), extension.end(), extension.begin(), toupper);
    bool demo = getOpenDesignSheet()->getFrame()->getNode()->getChildren().back()->getName().compare(L"id") != 0;
    if (demo && extension == L"PLG")
        showMessageBox(L"Plasma", L"This file needs to be saved in Plasma Demo format.");
    else if (!demo && extension == L"PLD")
        showMessageBox(L"Plasma", L"This file needs to be saved in Plasma format.");
    else if (getOpenDesignSheet()->m_path.size()) {
        if (!getOpenDesignSheet()->m_modified)
            return false;
        saveDocument(getOpenDesignSheet()->m_path, false);
        return true;
    }
    std::wstring path;
    if (demo)
        path = saveFileBox(L"Save Plasma Demo File", 0, L"Plasma demo files (*.pld)\0*.pld\0", 1);
    else
        path = saveFileBox(L"Save Plasma File", 0,
                           L"Plasma graphics (*.plg)\0*.plg\0Plasma development graphics (*.plx)\0*.plx\0", 1);
    if (!path.size())
        return false;
    saveDocument(path, false);
    return true;
}

// File menu "Save As...".
void Workspace::savePlasmaFileAs()
{
    if (getOpenDesignSheet() && !getOpenDesignSheet()->isBusy()) {
        std::wstring path;
        if (m_demo)
            path = saveFileBox(L"Save Plasma Demo File", 0, L"Plasma Demo graphics (*.pld)\0*.pld\0", 1);
        else
            path = saveFileBox(L"Save Plasma File", 0, L"Plasma graphics (*.plg)\0*.plg\0", 1);
        if (path.size())
            saveDocument(path, false);
    }
}

// File menu "Save Copy...": as Save As, suggesting the document's file name with "-Copy" before the extension; the
// sheet keeps its own path.
void Workspace::savePlasmaFileCopy()
{
    if (getOpenDesignSheet() && !getOpenDesignSheet()->isBusy()) {
        std::wstring path;
        std::wstring name;
        int separator = getOpenDesignSheet()->m_path.rfind(L'/');
        int backslash = getOpenDesignSheet()->m_path.rfind(L'\\');
        if (separator <= backslash)
            separator = backslash;
        if (!getOpenDesignSheet()->m_path.empty()) {
            name =
                getOpenDesignSheet()->m_path.substr(separator + 1, getOpenDesignSheet()->m_path.size() - separator - 1);
            name = name.substr(0, name.rfind(L"."));
            name += L"-Copy";
        }
        if (m_demo)
            path = saveFileBox(L"Save Plasma Demo File", name.c_str(), L"Plasma Demo graphics (*.pld)\0*.pld\0", 1);
        else
            path = saveFileBox(L"Save Plasma File", name.c_str(), L"Plasma graphics (*.plg)\0*.plg\0", 1);
        if (path.size())
            saveDocument(path, true);
    }
}

void Workspace::saveDocument(const std::wstring &path, bool copy)
{
    if (!getOpenDesignSheet())
        return;
    waitForColorWorker();
    DesignSheet *sheet = getOpenDesignSheet();
    std::wstring previousPath = sheet->m_path;
    sheet->setPath(path);
    if (!copy)
        updateWindowTitle(sheet->getTitle());
    addRecentFile(path);
    sheet->setBusy(true, true);

    // A demo version's files are keyed to its licence.
    unsigned __int64 key = 0;
    if (m_demo)
        key = plasma::keyFromPhrase(m_onlineModule->getLicenseSerial());
    m_designSheet->m_thread = startGLThread([this, sheet, key, copy, previousPath]() {
        if (!copy)
            sheet->m_modified = false;
        m_engine.writePlasmaFile(sheet->m_path, sheet->m_root, &sheet->m_page, key);
        if (copy)
            sheet->setPath(previousPath);
        sheet->setBusy(false, false);
        sheet->m_thread = 0;
    });
}

// Sheet menu "Close Sheet": closes the current sheet (asking to save a modified design) and makes the first shown
// sheet current.
void Workspace::closeSheet()
{
    if (getOpenSheet() && getOpenSheet()->getRoot()->m_display->m_visibility.value()) {
        getOpenSheet()->waitForThread();
        if (m_currentSheetOnly)
            showAllSheets();
        waitForColorWorker();
        if (getOpenSheet() == m_textureSheet) {
            closeTextureSheet();
        } else {
            if (getDesignSheet() && getDesignSheet()->m_modified) {
                if (!askSaveChanges())
                    return;
                m_sheet->waitForThread();
            }
            if (getDesignSheet())
                getTextureModule()->deleteSheetTextures(getDesignSheet());
            m_sheets.remove(getOpenSheet());
            m_engine.destroyNode(getOpenSheet()->m_frame->getNode());
            delete getOpenSheet();
        }
        m_sheet = 0;
        m_designSheet = 0;
        for (std::list<plasma::Node *>::iterator it = m_sheetsNode->m_children.begin();
             it != m_sheetsNode->m_children.end(); ++it) {
            if ((*it)->m_display->m_visibility.value()) {
                setSheet(static_cast<Sheet *>((*it)->m_userData));
                break;
            }
        }
        if (!m_designSheet) {
            updateWindowTitle(L"");
            for (std::list<Module *>::iterator it = m_modules.begin(); it != m_modules.end(); ++it)
                (*it)->sheetChanged();
            selectionChanged();
        }
    }
}

// Closes every sheet (WndProc before exit, and before a workspace file is opened).
void Workspace::closeAllSheets()
{
    resetTools();
    waitForColorWorker();
    while (m_sheet) {
        if (m_sheet == m_textureSheet) {
            closeTextureSheet();
        } else {
            m_sheet->waitForThread();
            if (getDesignSheet())
                getTextureModule()->deleteSheetTextures(getDesignSheet());
            m_sheets.remove(m_sheet);
            m_engine.destroyNode(m_sheet->m_frame->getNode());
            delete m_sheet;
        }
        m_sheet = 0;
        m_designSheet = 0;
        for (std::list<plasma::Node *>::iterator it = m_sheetsNode->m_children.begin();
             it != m_sheetsNode->m_children.end(); ++it) {
            if ((*it)->m_display->m_visibility.value()) {
                setSheet(static_cast<Sheet *>((*it)->m_userData));
                break;
            }
        }
    }
}

// Writes the value's low bytes as two hexadecimal digits each, then a line end. The shift count starts at 4
// regardless of byte count, so only four-byte calls write the bytes most significant first. Counts above four
// cause out-of-range shifts.
// --- File menu "Export Postscript..." ---------------------------------------- The shown meshes as PostScript:
// each drawing's contours become the clipping path of a free-form triangle shading (ShadingType 4) whose vertices
// and colours are written as hexadecimal data.
static void writePostscriptHex(std::ostream &file, unsigned value, unsigned bytes)
{
    for (unsigned i = 1; i <= bytes; i++)
        file << std::uppercase << std::setw(2) << std::setfill('0') << std::hex
             << (int)((value >> (8 * (4 - i))) & 0xff);
    file << std::endl;
}

// One drawing: the contours (in the matrix's coordinates) clip a shading of the drawing's triangles over the
// contours' bounds. With clip the clipping state stays open for the node's children.
void Workspace::writePostscriptDrawing(const plasma::Drawing *drawing, std::ostream &file,
                                       const plasma::Matrix<float> &matrix, bool clip)
{
    file << "gsave" << std::endl;
    file << "newpath" << std::endl;
    plasma::Vector<2, float> minimum;
    plasma::Vector<2, float> maximum;
    bool first = true;
    for (int i = 0; i < (int)drawing->m_contours.size(); i++) {
        const std::vector<int> &contour = drawing->m_contours[i];
        if (contour.begin() == contour.end())
            continue;
        plasma::Vector<2, float> point = matrix.transform(drawing->m_pathVertices[contour[0]]);
        file << point[0] << " " << point[1] << " moveto" << std::endl;
        if (first) {
            minimum = point;
            maximum = point;
            first = false;
        } else {
            if (!(minimum[0] < point[0]))
                minimum[0] = point[0];
            if (!(minimum[1] < point[1]))
                minimum[1] = point[1];
            if (!(maximum[0] > point[0]))
                maximum[0] = point[0];
            if (!(maximum[1] > point[1]))
                maximum[1] = point[1];
        }
        for (int j = 1; j < (int)contour.size(); j++) {
            point = matrix.transform(drawing->m_pathVertices[contour[j]]);
            file << point[0] << " " << point[1] << " lineto" << std::endl;
            if (!(minimum[0] < point[0]))
                minimum[0] = point[0];
            if (!(minimum[1] < point[1]))
                minimum[1] = point[1];
            if (!(maximum[0] > point[0]))
                maximum[0] = point[0];
            if (!(maximum[1] > point[1]))
                maximum[1] = point[1];
        }
    }
    file << "closepath" << std::endl;
    file << "clip" << std::endl;
    file << "gsave" << std::endl;
    file << "currentfile /ASCIIHexDecode filter /ReusableStreamDecode filter" << std::endl;

    // Per triangle vertex: a flag byte, 32-bit coordinates over the bounds, 8-bit colour components. The extents stay
    // double (fstp qword before the divisions).
    double width = maximum[0] - minimum[0];
    double height = maximum[1] - minimum[1];
    for (int i = 0; i < (int)drawing->m_pathIndices.size(); i++) {
        int index = drawing->m_pathIndices[i];
        plasma::Vector<2, float> point = matrix.transform(drawing->m_pathVertices[index]);
        const plasma::Vector<4, float> &color = drawing->m_pathColors[index];
        unsigned x = (unsigned)(__int64)((point[0] - minimum[0]) / width * 4294967294.0);
        unsigned y = (unsigned)(__int64)((point[1] - minimum[1]) / height * 4294967294.0);
        unsigned red = (unsigned)(__int64)(color[0] * 4294967294.0);
        unsigned green = (unsigned)(__int64)(color[1] * 4294967294.0);
        unsigned blue = (unsigned)(__int64)(4294967294.0 * color[2]);
        file << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << 0;
        file << std::endl;
        writePostscriptHex(file, x, 4);
        writePostscriptHex(file, y, 4);
        file << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (int)(red >> 24);
        file << std::endl;
        file << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (int)(green >> 24);
        file << std::endl;
        file << std::uppercase << std::setw(2) << std::setfill('0') << std::hex << (int)(blue >> 24);
        file << std::endl;
    }
    file << ">" << std::endl;
    file << "/datastream exch def" << std::endl;
    file << "<<" << std::endl
         << "/ShadingType 4" << std::endl
         << "/AntiAlias true" << std::endl
         << "/ColorSpace [/DeviceRGB]" << std::endl
         << "/BitsPerCoordinate 32" << std::endl
         << "/BitsPerComponent 8" << std::endl
         << "/BitsPerFlag 8 " << std::endl
         << "/Decode [" << minimum[0] << " " << maximum[0] << " " << minimum[1] << " " << maximum[1] << " 0 1 0 1 0 1]"
         << std::endl
         << "/DataSource datastream" << std::endl;
    file << ">>" << std::endl;
    file << "shfill" << std::endl;
    file << "grestore" << std::endl;
    if (!clip)
        file << "grestore" << std::endl;
}

// Opens the file and writes the document header: the file name as the title, the page size as the media and
// bounding box.
static void writePostscriptHeader(std::ofstream &file, std::wstring path, float width, float height)
{
    int slash = (int)path.rfind(L'/');
    int separator = (int)path.rfind(L'\\');
    if (slash > separator)
        separator = slash;
    std::wstring name = path.substr(separator + 1, path.size() - separator - 1);
    file.open(PLASMA_FILE_PATH(path.c_str()), std::ios::binary);
    file << "%!PS-Adobe-3.0" << std::endl
         << "%%Title: " << narrowString(name) << std::endl
         << "%%Creator: Picroma plasma, (C) 2010 Picroma" << std::endl
         << "%%LanguageLevel: 3" << std::endl
         << "%%Orientation: Portrait" << std::endl
         << "%%DocumentMedia: Default " << width << " " << height << " 0 () ()" << std::endl
         << "%%BoundingBox: 0 0 " << (int)width << " " << (int)height << std::endl
         << std::endl;
}

static void writePostscriptTrailer(std::ofstream &file)
{
    file << "showpage" << std::endl;
    file.close();
}

// The children through one function object: its std::_For_each instance is, shared by both callers (it returns the
// object, the stream and the matrix).
struct PostscriptNodeWriter {
    PostscriptNodeWriter(std::ostream &file, const plasma::Matrix<float> &matrix)
        : m_file(file),
          m_matrix(matrix)
    {}
    void operator()(plasma::Node *node) const { Workspace::writePostscriptNode(m_file, node, m_matrix); }
    std::ostream &m_file;
    const plasma::Matrix<float> &m_matrix;
};

// A shown node's meshes (extrusion, fill, stroke) and its children; a clipping node's children are clipped by its
// fill.
void Workspace::writePostscriptNode(std::ostream &file, plasma::Node *node, const plasma::Matrix<float> &parentMatrix)
{
    if (!node->m_display->m_visibility.value() || !node->isShown())
        return;
    const plasma::Matrix<float> matrix = parentMatrix * node->transformation()->m_matrix;
    plasma::SmoothMeshShape *shape = static_cast<plasma::SmoothMeshShape *>(node->getShape());
    if (shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH) {
        if (shape->drawsExtrusion() && shape->m_extrusionDrawing && shape->m_extrusionDrawing->m_pathIndices.size() > 2)
            writePostscriptDrawing(shape->m_extrusionDrawing, file, matrix, false);
        if (shape->drawsFill() && shape->m_fillDrawing && shape->m_fillDrawing->m_pathIndices.size() > 2) {
            writePostscriptDrawing(shape->m_fillDrawing, file, matrix, node->m_display->m_clipping.value() != 0);
            if (node->m_display->m_clipping.value()) {
                std::for_each(node->m_children.begin(), node->m_children.end(), PostscriptNodeWriter(file, matrix));
                file << "grestore" << std::endl;
            }
        }
        if (shape->drawsStroke() && shape->m_strokeDrawing && shape->m_strokeDrawing->m_pathIndices.size() > 2)
            writePostscriptDrawing(shape->m_strokeDrawing, file, matrix, false);
    }
    if (!node->m_display->m_clipping.value())
        std::for_each(node->m_children.begin(), node->m_children.end(), PostscriptNodeWriter(file, matrix));
}

void Workspace::showExportPDF()
{
    m_exportPDFModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
}

// File menu "Export Postscript...": the open design sheet's page, y up.
void Workspace::exportPostscript()
{
    if (getOpenDesignSheet()) {
        std::wstring path = saveFileBox(L"Export Postscript", 0, L"Postscript (*.ps)\0*.ps\0", 1);
        if (path.size()) {
            std::ofstream file;
            writePostscriptHeader(file, path, getOpenDesignSheet()->m_page.size.m_elements[0],
                                  getOpenDesignSheet()->m_page.size.m_elements[1]);
            plasma::Matrix<float> matrix;
            matrix.identity();
            matrix.translate(0.0f, getOpenDesignSheet()->m_page.size.m_elements[1]);
            matrix.scale(1.0f, -1.0f);
            std::for_each(getOpenDesignSheet()->m_root->m_children.begin(),
                          getOpenDesignSheet()->m_root->m_children.end(), PostscriptNodeWriter(file, matrix));
            writePostscriptTrailer(file);
        }
    }
}

bool Workspace::isPanning()
{
    return !m_engine.m_pointerCapture && (m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE) ||
                                          m_engine.m_pressedKeys.find(VK_SPACE) != m_engine.m_pressedKeys.end());
}

plasma::Node *Workspace::getHoveredNode()
{
    return m_hoveredNode;
}

DesignSheet *Workspace::getDesignSheet()
{
    DesignSheet *sheet = dynamic_cast<DesignSheet *>(m_sheet);
    if (sheet && sheet->isBusy())
        return 0;
    return sheet;
}

TextureSheet *Workspace::getTextureSheet()
{
    return dynamic_cast<TextureSheet *>(m_sheet);
}

// The "Texture sheet" menu item, checked (with a licence): opens a texture sheet and makes it current. Its frame
// takes the placement the last one had (CloseTextureSheet); the first one is a square of half the viewport's
// height.
void Workspace::openTextureSheet()
{
    static bool first = true;
    if ((int)m_sheetTemplate->m_children.size() > 0) {
        bool currentSheetOnly = m_currentSheetOnly;
        if (currentSheetOnly)
            showAllSheets();
        m_textureSheet = new TextureSheet(this);
        m_sheets.push_back(m_textureSheet);
        setSheet(m_textureSheet);
        plasma::Widget *frame = m_textureSheet->m_frame;
        if (first) {
            float height = (float)m_engine.m_viewportHeight;
            plasma::Vector<2, float> size = plasma::Vector<2, float>(height, height) * 0.5f;
            plasma::Vector<2, float> viewport((float)m_engine.m_viewportWidth, height);
            frame->setViewRect(m_textureSheetPosition * viewport, size);
            first = false;
        } else {
            plasma::Vector<2, float> viewport((float)m_engine.m_viewportWidth, (float)m_engine.m_viewportHeight);
            plasma::Vector<2, float> size = m_textureSheetSize * viewport;
            frame->setViewRect(m_textureSheetPosition * viewport, size);
        }
        if (currentSheetOnly && getOpenSheet())
            showCurrentSheetOnly();
    }
}

// The "Texture sheet" menu item, unchecked: closes the texture sheet (m_textureSheet). Its frame's placement is
// kept (as fractions of the viewport) for the next one, the design sheet becomes current again, and the sheet, its
// frame and its node are destroyed.
void Workspace::closeTextureSheet()
{
    if (m_textureSheet) {
        bool currentSheetOnly = m_currentSheetOnly;
        if (currentSheetOnly)
            showAllSheets();
        m_textureSheetPosition =
            m_textureSheet->m_frame->getViewPosition() /
            plasma::Vector<2, float>((float)m_engine.m_viewportWidth, (float)m_engine.m_viewportHeight);
        m_textureSheetSize =
            m_textureSheet->m_frame->getViewSize() /
            plasma::Vector<2, float>((float)m_engine.m_viewportWidth, (float)m_engine.m_viewportHeight);
        if (getOpenDesignSheet())
            setSheet(getOpenDesignSheet());
        else
            m_sheet = 0;
        m_sheets.remove(m_textureSheet);
        m_engine.destroyNode(m_textureSheet->m_frame->m_node);
        delete m_textureSheet;
        m_textureSheet = 0;
        if (currentSheetOnly && getOpenSheet())
            showCurrentSheetOnly();
    }
}

plasma::Vector<2, float> Workspace::snapPoint(const std::vector<plasma::Vector<2, float>> &segments,
                                              const plasma::Vector<2, float> &point)
{
    return snapToGrid(snapToSegments(segments, point));
}

// The nearest intersection of two segments within a distance of 10 (100 squared), else the nearest point on a
// segment within it, else the point.
plasma::Vector<2, float> Workspace::snapToSegments(const std::vector<plasma::Vector<2, float>> &segments,
                                                   const plasma::Vector<2, float> &point)
{
    if (getOpenDesignSheet()) {
        float bestDistance = 100.0f;
        plasma::Vector<2, float> best;
        bool found = false;
        int count = (int)segments.size();
        for (int i = 0; i < count; i += 2) {
            for (int j = 0; j < count; j += 2) {
                plasma::Vector<2, float> start = segments[i];
                plasma::Vector<2, float> direction = segments[i + 1] - start;
                plasma::Vector<2, float> otherStart = segments[j];
                plasma::Vector<2, float> otherDirection = segments[j + 1] - otherStart;
                float cross = otherDirection.m_elements[1] * direction.m_elements[0] -
                              otherDirection.m_elements[0] * direction.m_elements[1];
                if (std::fabs(cross) > 0.0f) {
                    float t = intersectionParameter(otherStart, otherDirection, start, direction);
                    if (t >= 0.0f && t <= 1.0f) {
                        float s = intersectionParameter(start, direction, otherStart, otherDirection);
                        if (s >= 0.0f && s <= 1.0f) {
                            plasma::Vector<2, float> intersection = start + direction * s;
                            plasma::Vector<2, float> delta = point - intersection;
                            float distance =
                                delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
                            if (distance <= bestDistance) {
                                bestDistance = distance;
                                best = intersection;
                                found = true;
                            }
                        }
                    }
                }
            }
        }
        if (!found) {
            for (int i = 0; i < (int)segments.size(); i += 2) {
                plasma::Vector<2, float> closest = closestPointOnSegment(segments[i], segments[i + 1], point);
                plasma::Vector<2, float> delta = point - closest;
                float distance = delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
                if (distance <= bestDistance) {
                    bestDistance = distance;
                    best = closest;
                    found = true;
                }
            }
        }
        if (found)
            return best;
    }
    return point;
}

// Adds the edges of the snapable meshes in the node's shown subtree, as point pairs on the sheet. Under the
// vertex, edge and face tools (types 2 to 4) a selected mesh adds only its unselected edges; otherwise selected
// meshes are skipped.
void Workspace::collectSnapSegments(plasma::Node *node, std::vector<plasma::Vector<2, float>> &segments)
{
    if (!node->isShown())
        return;
    if (node->isSnapable()) {
        ShapeInterface *shape = getOpenDesignSheet()->findSheetNode(node)->m_shapeInterface;
        if (shape && shape->m_type == INTERFACE_MESH) {
            int type = m_toolModule->m_currentTool->getType();
            bool meshTool = type == Tool::TOOL_VERTEX || type == Tool::TOOL_EDGE || type == Tool::TOOL_FACE;
            if (meshTool || !shape->m_node->m_selected) {
                if (getTextureSheet())
                    shape->setMatrix(getTextureSheet()->getMatrix());
                static_cast<MeshInterface *>(shape)->getEdgeSegments(segments, meshTool && shape->m_node->m_selected);
                shape->resetMatrix();
            }
        }
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        collectSnapSegments(*it, segments);
}

// Grid rounding adds a positive billion-unit bias before integer truncation, then removes it as an integer; retain
// the overflow behavior. With the grid on, the point moved to the nearest grid point of the current sheet along
// the grid module's axes, unless that is more than 10 away.
plasma::Vector<2, float> Workspace::snapToGrid(const plasma::Vector<2, float> &point)
{
    if (!m_gridModule->m_enabled || !getOpenSheet())
        return point;
    float size = getOpenSheet()->m_gridSize;

    // Reads the point through a reference before transforming it.
    const plasma::Vector<2, float> &scenePoint = point;
    plasma::Vector<2, float> snapped = getOpenSheet()->m_root->m_inverseMatrix.transform(scenePoint);
    snapped.m_elements[0] = (float)((int)(snapped.m_elements[0] / size + 0.5 + 1000000000.0) - 1000000000) * size;
    snapped.m_elements[1] = size * (float)((int)(snapped.m_elements[1] / size + 0.5 + 1000000000.0) - 1000000000);
    snapped.transform(getOpenSheet()->m_root->m_matrix);
    if (std::fabs(snapped.m_elements[0] - point.m_elements[0]) > 10.0f || !m_gridModule->m_moveX)
        snapped.m_elements[0] = point.m_elements[0];
    if (std::fabs(snapped.m_elements[1] - point.m_elements[1]) > 10.0f || !m_gridModule->m_moveY)
        snapped.m_elements[1] = point.m_elements[1];
    return snapped;
}

// The pointer (scene coordinates) snapped to the snapable meshes' edges and the grid: with offsets, by the
// smallest correction (per axis) that snaps one of the drag's snap points; corrections over 10 are dropped.
plasma::Vector<2, float> Workspace::getSnappedPointer(bool offsets)
{
    plasma::Vector<2, float> correction(10000.0f, 10000.0f);
    std::vector<plasma::Vector<2, float>> segments;
    if (getOpenDesignSheet())
        collectSnapSegments(getOpenDesignSheet()->m_root, segments);
    if (offsets) {
        for (std::list<plasma::Vector<2, float>>::iterator it = m_snapOffsets.begin(); it != m_snapOffsets.end();
             ++it) {
            plasma::Vector<2, float> offset = getOpenSheet()->m_root->m_matrix.transformDirection(*it);
            plasma::Vector<2, float> point = m_sceneMousePosition + offset;
            plasma::Vector<2, float> delta = snapPoint(segments, point) - point;
            float lengthSquared = delta[0] * delta[0] + delta[1] * delta[1];
            if (lengthSquared < 1e-15f)
                continue;
            for (int i = 0; i < 2; i++) {
                float distance = std::fabs(delta[i]);
                if (distance > 0.0f && distance < std::fabs(correction[i]))
                    correction[i] = delta[i];
            }
        }
    } else {
        correction = snapPoint(segments, m_sceneMousePosition) - m_sceneMousePosition;
    }
    if (std::fabs(correction[0]) > 10.0f)
        correction[0] = 0.0f;
    if (std::fabs(correction[1]) > 10.0f)
        correction[1] = 0.0f;
    return m_sceneMousePosition + correction;
}

plasma::Vector<2, float> Workspace::getDragMovement()
{
    if (m_cursorStickiness > 0.0f)
        return plasma::Vector<2, float>(0.0f, 0.0f);
    plasma::Vector<2, float> offset = m_dragPosition - m_previousDragPosition;
    return getOpenSheet()->m_root->m_matrix.transformDirection(offset);
}

void Workspace::redraw()
{
    m_exportModule->updateWidgets();
    m_layerModule->updatePanel();
    m_nodeModule->updatePanel();
    if (getDesignSheet() && m_exportModule->m_pixelPreview)
        getDesignSheet()->m_root->m_transformation->m_changed = true;
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. The current tool's
// Render, then the constraint icons by the pointer while the grid does not move along that axis.
void Workspace::drawConstraintIcons()
{
    getToolModule()->getCurrentTool()->draw();
    bool moveX = getGridModule()->m_moveX;
    if (!moveX) {
        std::wstring icon(L"icon:constrain:y");
        plasma::Vector<2, float> position = m_sceneMousePosition;
        position.m_elements[0] -= 15.0;
        position.m_elements[1] += 15.0;
        getToolModule()->getCurrentTool()->drawIcon(icon, position);
    }
    if (!getGridModule()->m_moveY) {
        std::wstring icon(L"icon:constrain:x");
        plasma::Vector<2, float> position = m_sceneMousePosition;
        position.m_elements[0] -= 15.0;
        position.m_elements[1] += 15.0;
        getToolModule()->getCurrentTool()->drawIcon(icon, position);
    }
    Sheet *sheet = getOpenSheet(); // Unused; m_busy is read as a volatile.
}

// Stops the drawing tools and clears the pending node requests.
void Workspace::resetTools()
{
    m_toolModule->m_drawFaceTool.stopDrawing();
    m_toolModule->m_drawEdgeTool.stopDrawing();
    m_chooseParentRequested = false;
    m_sendBehindRequested = false;
    m_sendInFrontOfRequested = false;
}

void Workspace::addBottomPanel(plasma::Widget *panel)
{
    if (panel) {
        m_bottomPanels.push_back(panel);
        panel->getNode()->setParent(m_guiBottomNode);
    }
}

void Workspace::addTopPanel(plasma::Widget *panel)
{
    if (panel) {
        m_topPanels.push_back(panel);
        panel->getNode()->setParent(m_guiTopNode);
    }
}

void Workspace::addBottom2Panel(plasma::Widget *panel)
{
    if (panel) {
        m_bottom2Panels.push_back(panel);
        panel->getNode()->setParent(m_guiBottom2Node);
    }
}

void Workspace::addRightPanel(plasma::Widget *panel)
{
    if (panel) {
        m_rightPanels.push_back(panel);
        panel->getNode()->setParent(m_guiRightNode);
    }
}

// Layout positions, extents and spacing use display units; colour tuples style the overlays. Lays out the
// interface's panels over the viewport: the top panels in a row (the last at the right edge), the layer panel
// below them down to the colour panel (at least 200 high), the right panels stacked at the right edge, the bottom2
// and bottom panels centred along the bottom between the tool panel and the right panels (the bottom ones grow in
// as their surfaces fade in, scaled down when the right panels do not fit), the status, tool info and update
// displays centred near the top, the palette, scale box, special colour and colour management panels beside the
// panels they follow (sharing their deformation), and the dialogs centred.
void Workspace::updateLayout()
{
    if (m_modules.empty())
        return;
    for (std::list<plasma::Widget *>::iterator it = m_topPanels.begin(); it != m_topPanels.end(); ++it)
        (*it)->viewWidth();
    float x = 0.0f;
    for (std::list<plasma::Widget *>::iterator it = m_topPanels.begin(); it != m_topPanels.end(); ++it) {
        if (*it == m_topPanels.back()) {
            (*it)->moveTo((float)m_engine.viewportWidth() - (*it)->viewWidth(), 0.0f, false);
        } else {
            (*it)->moveTo(x, 0.0f, false);
            x += (*it)->viewWidth();
        }
    }

    // The layer panel fills the height between the top panels and the colour panel.
    float layerHeight = (float)m_engine.viewportHeight() - m_topPanels.back()->getViewHeight() -
                                    m_colorModule->getPanel()->getViewHeight() <
                                200.0
                            ? 200.0
                            : (float)m_engine.viewportHeight() - m_topPanels.back()->getViewHeight() -
                                  m_colorModule->getPanel()->getViewHeight();
    plasma::Widget *layerPanel = m_layerModule->getPanel();
    layerPanel->resizeTo(layerPanel->viewWidth(), layerHeight);

    float rightHeight = 0.0f;
    for (std::list<plasma::Widget *>::iterator it = m_rightPanels.begin(); it != m_rightPanels.end(); ++it)
        rightHeight += (*it)->getViewHeight();
    float scale = ((float)m_engine.viewportHeight() - m_topPanels.back()->getViewHeight()) / rightHeight;
    if (scale > 1.0f)
        scale = 1.0f;
    float y = m_topPanels.back()->getViewHeight();
    for (std::list<plasma::Widget *>::iterator it = m_rightPanels.begin(); it != m_rightPanels.end(); ++it) {
        (*it)->moveTo((float)m_engine.viewportWidth() - (*it)->viewWidth(), y, false);
        y += (*it)->getViewHeight();
    }

    float bottom2Width = 0.0f;
    for (std::list<plasma::Widget *>::iterator it = m_bottom2Panels.begin(); it != m_bottom2Panels.end(); ++it)
        bottom2Width += (*it)->viewWidth();
    m_guiBottom2Node->m_transformation->m_deformation.value().identity();
    x = m_toolModule->getPanel()->viewWidth() + m_toolModule->getPanel()->viewX();
    for (std::list<plasma::Widget *>::iterator it = m_bottom2Panels.begin(); it != m_bottom2Panels.end(); ++it) {
        (*it)->moveTo(((float)m_engine.viewportWidth() - m_toolModule->getPanel()->viewX() -
                       m_rightPanels.back()->viewWidth() - m_toolModule->getPanel()->viewWidth() -
                       bottom2Width * scale) *
                              0.5 +
                          x,
                      (float)m_engine.viewportHeight() - (*it)->getViewHeight(), false);
        x += (*it)->viewWidth();
    }

    float bottomWidth = 0.0f;
    for (std::list<plasma::Widget *>::iterator it = m_bottomPanels.begin(); it != m_bottomPanels.end(); ++it) {
        float width = (*it)->viewWidth();
        if ((*it)->m_surface) {
            float hidden =
                1.0f - (*it)->m_surface->m_drawColor.m_elements[3] * (*it)->m_surface->m_drawColor.m_elements[3];
            float shown = 1.0f - hidden * hidden;
            width = shown * width;
        }
        bottomWidth += width;
    }
    x = m_toolModule->getPanel()->viewX() + m_toolModule->getPanel()->viewWidth();
    for (std::list<plasma::Widget *>::iterator it = m_bottomPanels.begin(); it != m_bottomPanels.end(); ++it) {
        float width = (*it)->viewWidth();
        float height = (*it)->getViewHeight();
        if ((*it)->m_surface) {
            float hidden =
                1.0f - (*it)->m_surface->m_drawColor.m_elements[3] * (*it)->m_surface->m_drawColor.m_elements[3];
            float shown = 1.0f - hidden * hidden;
            width = shown * width;
            height = height - (1.0f - shown) * 20.0;
        }
        (*it)->moveTo((width - (*it)->viewWidth() + m_engine.viewportWidth() - m_toolModule->getPanel()->viewX() -
                       m_rightPanels.back()->viewWidth() - m_toolModule->getPanel()->viewWidth() -
                       bottomWidth * scale) *
                              0.5 +
                          x,
                      m_engine.viewportHeight() - height - m_nodeModule->getPanel()->getViewHeight() + 5.0, false);
        x += width;
    }

    // The status, tool info and update displays, centred near the top.
    if (m_statusModule) {
        plasma::Widget *panel = m_statusModule->getPanel();
        panel->moveTo((float)(m_engine.viewportWidth() / 2 - (int)panel->viewWidth() / 2), 120.0f, false);
    }
    if (m_toolInfoModule && m_toolInfoModule->getPanel()) {
        plasma::Widget *panel = m_toolInfoModule->getPanel();
        panel->moveTo((float)(m_engine.viewportWidth() / 2 - (int)panel->viewWidth() / 2), 60.0f, false);
    }
    if (m_updateButton && m_updateButton->m_widget) {
        plasma::Widget *widget = m_updateButton->m_widget;
        widget->moveTo((float)(m_engine.viewportWidth() / 2 - (int)widget->viewWidth() / 2), 40.0f, false);
    }

    // The palette slides out to the left of the colour panel.
    if (m_paletteModule) {
        m_paletteModule->getPanel()->getNode()->m_transformation->m_deformation =
            m_colorModule->getPanel()->getNode()->m_transformation->m_deformation;
        m_paletteModule->getPanel()->getNode()->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        m_paletteModule->getPanel()->getNode()->m_transformation->m_changed = false;
        float expansion = m_paletteModule->m_expansion;
        float hidden = 1.0f - expansion * expansion;
        plasma::Matrix<float> matrix = m_paletteModule->getPanel()->getContentMatrix();
        float offset = (float)(1.0f - hidden * hidden) * 85.0;
        plasma::Widget *colorPanel = m_colorModule->getPanel();
        plasma::Widget *palettePanel = m_paletteModule->getPanel();
        palettePanel->moveTo(colorPanel->viewX() - (offset + 20.0) * matrix.m_elements[0],
                             colorPanel->viewY() + matrix.m_elements[0] * 10.0, false);
    }

    // The scale box follows the node tool's panel, or the vertex panel for the other tools.
    if (m_scaleBoxModule) {
        Module *anchor = m_nodeToolModule;
        if (m_toolModule->m_currentTool->getType() != Tool::TOOL_NODE)
            anchor = m_vertexModule;
        m_scaleBoxModule->getPanel()->getNode()->m_transformation->m_deformation =
            anchor->m_panel->getNode()->m_transformation->m_deformation;
        m_scaleBoxModule->getPanel()->getNode()->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        m_scaleBoxModule->getPanel()->getNode()->m_transformation->m_changed = false;
        plasma::Matrix<float> matrix = anchor->m_panel->getContentMatrix();
        m_scaleBoxModule->getPanel()->resizeTo((470.0 - m_scaleBoxModule->m_expansion * 200.0) * matrix.m_elements[0],
                                               m_scaleBoxModule->getPanel()->getViewHeight());
        float hidden = 1.0f - m_scaleBoxModule->m_expansion * m_scaleBoxModule->m_expansion;
        float offset = (m_scaleBoxModule->getPanel()->getContentWidth() - 40.0) * (float)(1.0f - hidden * hidden);
        m_scaleBoxModule->getPanel()->moveTo(anchor->m_panel->viewX() + anchor->m_panel->viewWidth() -
                                                 (offset + 20.0) * matrix.m_elements[0],
                                             anchor->m_panel->viewY() + matrix.m_elements[0] * 5.0, false);
    }

    // The special colour panel follows the palette.
    if (m_specialColorModule && m_paletteModule) {
        m_specialColorModule->getPanel()->getNode()->m_transformation->m_deformation =
            m_paletteModule->getPanel()->getNode()->m_transformation->m_deformation;
        m_specialColorModule->getPanel()->getNode()->m_transformation->attributesChanged(
            plasma::ATTRIBUTES_CHANGED_ALL);
        float offset = 0.0f;
        m_specialColorModule->getPanel()->getNode()->m_transformation->m_changed = false;
        if (m_specialColorModule->getPanel()->m_surface) {
            float hidden = 1.0f - m_specialColorModule->getPanel()->m_surface->m_drawColor.m_elements[3] *
                                      m_specialColorModule->getPanel()->m_surface->m_drawColor.m_elements[3];
            float squared = hidden * hidden;
            offset = squared * 50.0;
        }
        plasma::Matrix<float> matrix = m_specialColorModule->getPanel()->getContentMatrix();
        m_specialColorModule->getPanel()->moveTo(
            m_paletteModule->getPanel()->viewX() - m_specialColorModule->getPanel()->viewWidth() +
                (offset + 40.0) * matrix.m_elements[0],
            m_paletteModule->getPanel()->viewY() + (matrix.m_elements[0] + matrix.m_elements[0]), false);
    }

    // The colour management panel follows the colour panel.
    if (m_colorManagementModule && m_colorModule) {
        m_colorManagementModule->getPanel()->getNode()->m_transformation->m_deformation =
            m_colorModule->getPanel()->getNode()->m_transformation->m_deformation;
        m_colorManagementModule->getPanel()->getNode()->m_transformation->attributesChanged(
            plasma::ATTRIBUTES_CHANGED_ALL);
        float offset = 0.0f;
        m_colorManagementModule->getPanel()->getNode()->m_transformation->m_changed = false;
        if (m_colorManagementModule->getPanel()->m_surface) {
            float hidden = 1.0f - m_colorManagementModule->getPanel()->m_surface->m_drawColor.m_elements[3] *
                                      m_colorManagementModule->getPanel()->m_surface->m_drawColor.m_elements[3];
            float squared = hidden * hidden;
            offset = squared * 50.0;
        }
        plasma::Matrix<float> matrix = m_colorModule->getPanel()->getContentMatrix();
        m_colorManagementModule->getPanel()->moveTo(
            m_colorModule->getPanel()->viewX() - m_specialColorModule->getPanel()->viewWidth() +
                (offset + 20.0) * matrix.m_elements[0],
            m_colorModule->getPanel()->viewY() + matrix.m_elements[0] * 318.0, false);
    }

    // The dialogs, centred in the viewport.
    {
        plasma::Vector<2, int> viewport(m_engine.viewportWidth(), m_engine.viewportHeight());
        plasma::Widget *panel = m_optionsModule->getPanel();
        float width = (float)viewport[0] - panel->viewWidth();
        float height = (float)viewport[1] - panel->getViewHeight();
        plasma::Vector<2, float> center(width, height);
        center = center * 0.5f;
        m_optionsModule->getPanel()->setViewPosition(center, false);
    }
    {
        plasma::Vector<2, int> viewport(m_engine.viewportWidth(), m_engine.viewportHeight());
        plasma::Widget *panel = m_shadowModule->getPanel();
        float width = (float)viewport[0] - panel->viewWidth();
        float height = (float)viewport[1] - panel->getViewHeight();
        plasma::Vector<2, float> center(width, height);
        center = center * 0.5f;
        m_shadowModule->getPanel()->setViewPosition(center, false);
    }
    {
        plasma::Vector<2, int> viewport(m_engine.viewportWidth(), m_engine.viewportHeight());
        plasma::Widget *panel = m_onlineModule->getPanel();
        float width = (float)viewport[0] - panel->viewWidth();
        float height = (float)viewport[1] - panel->getViewHeight();
        plasma::Vector<2, float> center(width, height);
        center = center * 0.5f;
        m_onlineModule->getPanel()->setViewPosition(center, false);
    }
    {
        plasma::Vector<2, int> viewport(m_engine.viewportWidth(), m_engine.viewportHeight());
        plasma::Widget *panel = m_aboutModule->getPanel();
        float width = (float)viewport[0] - panel->viewWidth();
        float height = (float)viewport[1] - panel->getViewHeight();
        plasma::Vector<2, float> center(width, height);
        center = center * 0.5f;
        m_aboutModule->getPanel()->setViewPosition(center, false);
    }
    if (m_exportPDFModule->getPanel()->getNode()->m_display->m_visibility.value()) {
        {
            plasma::Vector<2, int> viewport(m_engine.viewportWidth(), m_engine.viewportHeight());
            plasma::Widget *panel = m_exportPDFModule->getPanel();
            float width = (float)viewport[0] - panel->viewWidth();
            float height = (float)viewport[1] - panel->getViewHeight();
            plasma::Vector<2, float> center(width, height);
            center = center * 0.5f;
            m_exportPDFModule->getPanel()->setViewPosition(center, false);
        }
    }
    m_guiNode->updateMatricesRecursively();
}

void Workspace::getSelectedNodes(std::list<plasma::Node *> &nodes)
{
    nodes.clear();
    if (getDesignSheet())
        getDesignSheet()->getSelectedNodes(nodes);
}

void Workspace::forEachUnlockedSheetNode(void (*function)(NodeInterface *))
{
    if (!getOpenDesignSheet())
        return;
    if (!getOpenDesignSheet()->m_root->m_display->m_visibility.value())
        return;
    plasma::Node *root = getOpenDesignSheet()->m_root;
    forEachShownSheetNode(getOpenDesignSheet()->findSheetNode(root), function);
}

void Workspace::forEachShownSheetNode(NodeInterface *sheetNode, void (*function)(NodeInterface *))
{
    if (!sheetNode || !sheetNode->m_node->isShown())
        return;
    if (!sheetNode->m_node->isLocked())
        function(sheetNode);
    for (std::list<plasma::Node *>::iterator it = sheetNode->m_node->m_children.begin();
         it != sheetNode->m_node->m_children.end(); ++it)
        forEachShownSheetNode(getOpenDesignSheet()->findSheetNode(*it), function);
}

NodeInterface *Workspace::getHoveredSheetNode()
{
    if (!getDesignSheet() || !getHoveredNode())
        return 0;
    return getDesignSheet()->findSheetNode(getHoveredNode());
}

void Workspace::findNearestVertex(EditableMesh **mesh, plasma::Node **node, const plasma::Vector<2, float> &point,
                                  MeshVertex **vertex, float *distance)
{
    *vertex = 0;
    *mesh = 0;
    *distance = -1.0f;
    *node = 0;
    DesignSheet *designSheet = getOpenDesignSheet();
    if (!designSheet)
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    std::map<plasma::Node *, NodeInterface *>::iterator it;
    if (designSheet->isBusy())
        it = designSheet->m_sheetNodes.end();
    else
        it = designSheet->m_sheetNodes.begin();
    for (; it != designSheet->m_sheetNodes.end(); ++it) {
        if (!it->second->m_selected || it->first->isLocked() || !it->first->isShown())
            continue;
        ShapeInterface *shapeInterface = it->second->m_shapeInterface;
        if (!shapeInterface || shapeInterface->m_type != INTERFACE_MESH)
            continue;
        MeshInterface *meshInterface = static_cast<MeshInterface *>(shapeInterface);
        if (getTextureSheet())
            meshInterface->setMatrix(getTextureSheet()->getMatrix());
        plasma::Matrix<float> matrix;
        meshInterface->getMatrix(matrix);
        for (std::list<MeshVertex *>::iterator v = meshInterface->m_mesh.beginVertices();
             v != meshInterface->m_mesh.endVertices(); ++v) {
            plasma::Vector<2, float> offset = (matrix * meshInterface->getVertexPosition(*v)) - point;
            float squared = offset[0] * offset[0] + offset[1] * offset[1];
            if (*distance < 0.0f || *distance > squared) {
                // The vertices of the face being drawn do not count.
                std::list<MeshVertex *> &drawn = m_toolModule->m_drawFaceTool.m_vertices;
                std::list<MeshVertex *>::iterator d = drawn.begin();
                for (; d != drawn.end(); ++d) {
                    if (*v == *d)
                        break;
                }
                if (d == drawn.end()) {
                    *distance = squared;
                    *vertex = *v;
                    *mesh = &meshInterface->m_mesh;
                    *node = it->first;
                }
            }
        }
        meshInterface->resetMatrix();
    }
    *distance = sqrtf(*distance);
}

// As DrawFaceTool.cpp's: the open test is the negated closed bool (shr 3; not; test 1) and the face count a signed
// int.
static inline bool isClosed(unsigned flags)
{
    return !(flags & (1 << plasma::MESH_OPEN));
}

void Workspace::findNearestEdge(EditableMesh **mesh, plasma::Node **node, const plasma::Vector<2, float> &point,
                                MeshHalfEdge **edge, float *distance)
{
    *edge = 0;
    *mesh = 0;
    *distance = -1.0f;
    *node = 0;
    DesignSheet *designSheet = getOpenDesignSheet();
    if (!designSheet)
        return;

    // Starts with the first sheet node, or the end while the sheet is busy.
    std::map<plasma::Node *, NodeInterface *>::iterator it;
    if (designSheet->isBusy())
        it = designSheet->m_sheetNodes.end();
    else
        it = designSheet->m_sheetNodes.begin();
    for (; it != designSheet->m_sheetNodes.end(); ++it) {
        if (!it->second->m_selected || it->first->isLocked() || !it->first->isShown())
            continue;
        ShapeInterface *shapeInterface = it->second->m_shapeInterface;
        if (!shapeInterface || shapeInterface->m_type != INTERFACE_MESH)
            continue;
        MeshInterface *meshInterface = static_cast<MeshInterface *>(shapeInterface);
        if (getTextureSheet())
            meshInterface->setMatrix(getTextureSheet()->getMatrix());
        plasma::Matrix<float> matrix;
        meshInterface->getMatrix(matrix);

        // An open path's closing edge (MESH_OPEN) does not count.
        MeshHalfEdge *closing = 0;
        if (!isClosed(meshInterface->m_mesh.m_shape->m_flags) && (int)meshInterface->m_mesh.getFaceCount() > 0)
            closing = (*meshInterface->m_mesh.beginFaces())->m_edge->m_previous;
        for (std::list<MeshHalfEdge *>::iterator e = meshInterface->m_mesh.beginEdges();
             e != meshInterface->m_mesh.endEdges(); ++e) {
            plasma::Vector<2, float> start = matrix * meshInterface->getVertexPosition((*e)->m_vertex);
            plasma::Vector<2, float> end = matrix * meshInterface->getVertexPosition((*e)->m_next->m_vertex);
            float squared = segmentDistanceSquared(point, start, end);
            if (*e != closing && (*distance < 0.0f || *distance > squared)) {
                *distance = squared;
                *edge = *e;
                *mesh = &meshInterface->m_mesh;
                *node = it->first;
            }
        }
        meshInterface->resetMatrix();
    }
    *distance = *distance < 0.0f ? 0.0f : sqrtf(*distance);
}

// The scene's context menu (a right click): a copy of the popup template under "gui" with the current tool's
// items, then "Refine", "Refine Contour" and "Add Border Edges", shown at the pointer and kept inside the window.
void Workspace::showPopupMenu()
{
    if (m_popupMenuTemplate && m_popupMenuItem && m_popupMenuSeparator) {
        closePopupMenu();
        plasma::Node *menu = m_popupMenuTemplate->getNode()->instance(getGuiNode());
        m_popupMenu = dynamic_cast<plasma::ListWidget *>(menu->widget());
        m_engine.setPopUpWidget(m_popupMenu, 0);
        getToolModule()->getCurrentTool()->fillContextMenu();
        addPopupMenuSeparator();
        {
            std::wstring caption(L"Refine");
            plasma::Widget *item = m_popupMenu->addItem(m_popupMenuItem, true);
            item->setPopUpItem(true, true);
            item->setCaption(caption, true);
            item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Workspace::refine, true, true);
        }
        {
            std::wstring caption(L"Refine Contour");
            plasma::Widget *item = m_popupMenu->addItem(m_popupMenuItem, true);
            item->setPopUpItem(true, true);
            item->setCaption(caption, true);
            item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Workspace::refineContour, true, true);
        }
        {
            std::wstring caption(L"Add Border Edges");
            plasma::Widget *item = m_popupMenu->addItem(m_popupMenuItem, true);
            item->setPopUpItem(true, true);
            item->setCaption(caption, true);
            item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &Workspace::addBorderEdges, true, true);
        }
        m_popupMenu->layout();
        m_popupMenu->getNode()->play(L"menu:show", 0);
        plasma::Vector<2, float> position = m_engine.m_pointer;
        if (m_popupMenu->viewWidth() + position.m_elements[0] > m_engine.viewportWidth())
            position.m_elements[0] = (float)m_engine.viewportWidth() - m_popupMenu->viewWidth();
        if (m_popupMenu->getViewHeight() + position.m_elements[1] > m_engine.viewportHeight())
            position.m_elements[1] = (float)m_engine.viewportHeight() - m_popupMenu->getViewHeight();
        if (position.m_elements[0] < 0)
            position.m_elements[0] = 0;
        if (position.m_elements[1] < 0)
            position.m_elements[1] = 0;
        m_popupMenu->setViewPosition(position, true);
    }
}

plasma::Widget *Workspace::addPopupMenuSeparator()
{
    return m_popupMenu->addItem(m_popupMenuSeparator, true);
}

void Workspace::closePopupMenu()
{
    if (m_popupMenu) {
        if (m_guiPointerNode && m_popupMenu->getNode()->isAncestorOf(m_guiPointerNode))
            m_guiPointerNode = 0;
        m_engine.destroyNode(m_popupMenu->getNode());
        m_popupMenu = 0;
    }
}

void Workspace::showCurrentSheetOnly()
{
    if (!m_sheet)
        return;
    for (std::list<plasma::Node *>::iterator it = m_sheetsNode->m_children.begin();
         it != m_sheetsNode->m_children.end(); ++it)
        if (static_cast<Sheet *>((*it)->m_userData) != m_sheet)
            (*it)->m_display->m_visibility.value() = 0;
    m_sheet->expandFrame();
    m_currentSheetOnly = true;
}

void Workspace::showAllSheets()
{
    for (std::list<plasma::Node *>::iterator it = m_sheetsNode->m_children.begin();
         it != m_sheetsNode->m_children.end(); ++it)
        (*it)->m_display->m_visibility.value() = 1;
    if (m_sheet)
        m_sheet->restoreFrame();
    m_currentSheetOnly = false;
}

void Workspace::resizeToolRadius()
{
    m_resizeToolRadius = true;
}

void Workspace::chooseParent()
{
    m_chooseParentRequested = true;
}

void Workspace::sendBehind()
{
    m_sendBehindRequested = true;
}

void Workspace::sendInFrontOf()
{
    m_sendInFrontOfRequested = true;
}

void Workspace::setPivot()
{
    m_setPivotRequested = true;
}

// "Set Pivot...": the selected shown nodes' pivots move to the point (scene coordinates) without moving the nodes
// ("set pivot").
void Workspace::setPivotAt(const plasma::Vector<2, float> &point)
{
    if (getDesignSheet()) {
        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *designSheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (designSheet->isBusy())
            it = designSheet->m_sheetNodes.end();
        else
            it = designSheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown()) {
                NodeState before;
                it->second->saveState(before);
                it->first->m_transformation->setPivot(it->first->m_inverseMatrix.transform(point));
                NodeState after;
                it->second->saveState(after);
                getDesignSheet()->addOperation(
                    new NodeOperation(getDesignSheet(), "set pivot", it->second, before, after));
                realignChildren(it->first->m_parent);
            }
        }
        getDesignSheet()->commit();
    }
}

// Moves the node under parent, before the sibling (at the end if null), keeping where it is drawn: every key of
// its translation (about the pivot) and of its deformation is carried from the old parent's space into the new
// one's. A transformation shared with other nodes is first replaced by a copy ("uninstantiate transformation").
// Records the hierarchy change ("choose parent") and the state change (std::string("adapt parent
// transformation")).
void Workspace::moveNode(plasma::Node *node, plasma::Node *parent, plasma::Node *before)
{
    NodeState oldState;
    getOpenDesignSheet()->findSheetNode(node)->saveState(oldState);
    plasma::Matrix<float> matrix = parent->m_inverseMatrix * node->m_parent->m_matrix;
    if (node->m_transformation->countNodes() > 1) {
        plasma::Transformation *shared = node->m_transformation;
        node->setTransformation(shared->clone());
        getOpenDesignSheet()->addOperation(new InstanceOperation(
            getOpenDesignSheet(), "uninstantiate transformation", getOpenDesignSheet()->findSheetNode(node),
            node->m_shape, shared, node->m_display, node->m_shape, node->m_transformation, node->m_display));
    }
    for (int key = 0; key < node->m_transformation->m_translation.size(); key++) {
        plasma::Vector<2, float> &pivot = node->m_transformation->m_pivot.m_values[key];
        node->m_transformation->m_translation.m_values[key] =
            matrix.transform(node->m_transformation->m_translation.m_values[key] + pivot) - pivot;
    }
    for (int key = 0; key < node->m_transformation->m_deformation.size(); key++) {
        plasma::Transformation *transformation = node->m_transformation;
        plasma::Matrix<float> rotation;
        rotation.identity();
        rotation.rotateX(transformation->m_rotation.value().m_elements[0]);
        rotation.rotateY(transformation->m_rotation.value().m_elements[1]);
        rotation.rotateZ(transformation->m_rotation.value().m_elements[2]);
        plasma::Matrix<float> inverse = rotation;
        inverse.invert();
        plasma::Matrix<float> linear = inverse * matrix * rotation;
        linear.m_elements[12] = 0;
        linear.m_elements[13] = 0;
        linear.m_elements[14] = 0;
        node->m_transformation->m_deformation.m_values[key] =
            linear * node->m_transformation->m_deformation.m_values[key];
    }
    node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    plasma::Node *oldParent = node->m_parent;
    plasma::Node *oldNextSibling = node->getNextSibling();
    parent->insertChild(node, before);
    getOpenDesignSheet()->addOperation(new NodeHierarchyOperation(getOpenDesignSheet(), std::string("choose parent"),
                                                                  node, oldParent, oldNextSibling));
    NodeState newState;
    getOpenDesignSheet()->findSheetNode(node)->saveState(newState);
    getOpenDesignSheet()->addOperation(
        new NodeOperation(getOpenDesignSheet(), std::string("adapt parent transformation"),
                          getOpenDesignSheet()->findSheetNode(node), oldState, newState));
    realignChildren(oldParent);
    realignChildren(parent);
}

// Moves the selected nodes at or below node (the open design sheet's root if null) into parent; hidden nodes and
// nodes with flag bit 3 are skipped with their subtrees. A selected node that is an ancestor of parent first gives
// parent its place.
void Workspace::moveSelectedNodes(plasma::Node *node, plasma::Node *parent)
{
    if (!node)
        node = getOpenDesignSheet()->m_root;
    if (node->isShown() && !node->isInternal()) {
        if (getOpenDesignSheet()->findSheetNode(node)->m_selected && parent != node) {
            if (node->isAncestorOf(parent))
                moveNode(parent, node->m_parent, node->getNextSibling());
            moveNode(node, parent, 0);
        } else {
            std::list<plasma::Node *> children = node->getChildren();
            for (std::list<plasma::Node *>::iterator it = children.begin(); it != children.end(); ++it)
                moveSelectedNodes(*it, parent);
        }
    }
}

// "Send Behind...": moves the selected nodes at or below node (the root if null) before the target, under the
// target's parent; a selected parent of the target lets the target out after it instead. Before the root means
// before its first shown child.
void Workspace::moveSelectedNodesBehind(plasma::Node *node, plasma::Node *target)
{
    if (getOpenDesignSheet()->getRoot() == target) {
        for (std::list<plasma::Node *>::iterator it = getOpenDesignSheet()->getRoot()->m_children.begin();
             it != getOpenDesignSheet()->getRoot()->m_children.end(); ++it) {
            if ((*it)->isShown()) {
                moveSelectedNodesBehind(node, *it);
                return;
            }
        }
        moveSelectedNodes(node, target);
        return;
    }
    if (!node)
        node = getOpenDesignSheet()->getRoot();
    if (node->isShown() && !node->isInternal()) {
        plasma::Node *parent = target->getParent();
        if (getOpenDesignSheet()->findSheetNode(node)->m_selected && parent == node) {
            moveNode(target, parent->getParent(), parent->getNextSibling());
        } else if (getOpenDesignSheet()->findSheetNode(node)->m_selected && parent != node && target != node) {
            if (node->isAncestorOf(parent))
                moveNode(parent, node->getParent(), node->getNextSibling());
            moveNode(node, parent, target);
        } else {
            std::list<plasma::Node *> children = node->getChildren();
            for (std::list<plasma::Node *>::iterator it = children.begin(); it != children.end(); ++it)
                moveSelectedNodesBehind(*it, target);
        }
    }
}

// "Send in Front of...": the same, after the target (the children in reverse order, keeping their order); in front
// of the root means into it.
void Workspace::moveSelectedNodesInFrontOf(plasma::Node *node, plasma::Node *target)
{
    if (target == getOpenDesignSheet()->getRoot()) {
        moveSelectedNodes(node, target);
        return;
    }
    if (!node)
        node = getOpenDesignSheet()->getRoot();
    if (node->isShown() && !node->isInternal()) {
        plasma::Node *parent = target->getParent();
        if (getOpenDesignSheet()->findSheetNode(node)->m_selected && parent == node) {
            moveNode(target, parent->getParent(), parent);
        } else if (getOpenDesignSheet()->findSheetNode(node)->m_selected && parent != node && target != node &&
                   target->getNextSibling() != node) {
            if (node->isAncestorOf(parent))
                moveNode(parent, node->getParent(), node->getNextSibling());
            moveNode(node, parent, target->getNextSibling());
        } else {
            std::list<plasma::Node *> children = node->getChildren();
            for (std::list<plasma::Node *>::reverse_iterator it = children.rbegin(); it != children.rend(); ++it)
                moveSelectedNodesInFrontOf(*it, target);
        }
    }
}

void Workspace::realignChildren(plasma::Node *node)
{
    if (!getOpenDesignSheet() || !node->alignsToOutline() || !getOpenDesignSheet()->findSheetNode(node))
        return;
    if (getOpenDesignSheet()->findSheetNode(node)->m_shapeInterface)
        getOpenDesignSheet()->findSheetNode(node)->m_shapeInterface->alignChildrenToOutline();
}

// The node tool's context menu "Unparent": moves the selected nodes to the sheet's root.
void Workspace::unparent()
{
    if (getDesignSheet()) {
        moveSelectedNodes(0, getDesignSheet()->m_root);
        getDesignSheet()->commit();
        redraw();
    }
}

// The context menu's "Refine": SubdivideMesh for each selected shown shape, once per shape shared by several
// nodes.
void Workspace::refine()
{
    if (getOpenDesignSheet()) {
        resetTools();
        std::set<void *> shapes; // The shapes done.

        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *designSheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (designSheet->isBusy())
            it = designSheet->m_sheetNodes.end();
        else
            it = designSheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown()) {
                if (ShapeInterface *shape = it->second->m_shapeInterface) {
                    if (shapes.find(it->first->m_shape) == shapes.end()) {
                        shape->subdivideMesh();
                        shapes.insert(static_cast<void *>(it->first->m_shape));
                    }
                }
            }
        }
        getOpenDesignSheet()->commit();
        getOpenDesignSheet()->rebuildSharedInterfaces();
        updateModules();
    }
}

// The context menu's "Refine Contour": SubdivideContour for each selected shown shape, once per shape shared by
// several nodes.
void Workspace::refineContour()
{
    if (getOpenDesignSheet()) {
        resetTools();
        std::set<void *> shapes; // The shapes done.

        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *designSheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (designSheet->isBusy())
            it = designSheet->m_sheetNodes.end();
        else
            it = designSheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown()) {
                if (ShapeInterface *shape = it->second->m_shapeInterface) {
                    if (shapes.find(it->first->m_shape) == shapes.end()) {
                        shape->subdivideContour();
                        shapes.insert(static_cast<void *>(it->first->m_shape));
                    }
                }
            }
        }
        getOpenDesignSheet()->commit();
        getOpenDesignSheet()->rebuildSharedInterfaces();
        updateModules();
    }
}

// The context menu's "Add Border Edges": AddBorder for each selected shown shape, once per shape shared by several
// nodes.
void Workspace::addBorderEdges()
{
    if (getOpenDesignSheet()) {
        resetTools();
        std::set<void *> shapes; // The shapes done.

        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *designSheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (designSheet->isBusy())
            it = designSheet->m_sheetNodes.end();
        else
            it = designSheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown()) {
                if (ShapeInterface *shape = it->second->m_shapeInterface) {
                    if (shapes.find(it->first->m_shape) == shapes.end() && shape->m_type == INTERFACE_MESH) {
                        static_cast<MeshInterface *>(shape)->addBorder();
                        shapes.insert(static_cast<void *>(it->first->m_shape));
                    }
                }
            }
        }
        getOpenDesignSheet()->commit();
        getOpenDesignSheet()->rebuildSharedInterfaces();
    }
}

void Workspace::showOptions()
{
    m_optionsModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
}

void Workspace::showProductActivation()
{
#ifndef PLASMA_NO_LICENSING
    m_onlineModule->m_panel->getNode()->m_display->m_visibility.value() = 1;
#endif
}

// The resize cursor for a bounding box handle: the corners (0-3) and the edge middles (4-7), each pair on a
// diagonal or an axis.
static int getHandleCursor(int handle)
{
    switch (handle) {
    case 0:
    case 2:
        return CURSOR_SIZE_NWSE;
    case 1:
    case 3:
        return CURSOR_SIZE_NESW;
    case 4:
    case 6:
        return CURSOR_SIZE_NS;
    case 5:
    case 7:
        return CURSOR_SIZE_WE;
    default:
        return CURSOR_ARROW;
    }
}

// WM_SETCURSOR: the cursor for what is under the pointer (none while the options draw their own cursor or in
// screenshot mode).
void Workspace::updateCursor()
{
    if (g_screenshotMode || !m_optionsModule->m_cursorHardware) {
        setCursorShape(CURSOR_NONE);
    } else if (m_guiPointerNode) {
        if (m_hoveredNode) {
            setCursorShape(CURSOR_HAND);
        } else if (m_guiPointerNode->m_enclosingWidget &&
                   m_guiPointerNode->m_enclosingWidget->m_kind == plasma::WIDGET_EDIT) {
            setCursorShape(CURSOR_IBEAM);
        } else if (m_guiPointerNode->getName() == L"texturewidget" ||
                   m_guiPointerNode->m_enclosingWidget &&
                       m_guiPointerNode->m_enclosingWidget->getName() == L"palettewidget") {
            setCursorShape(CURSOR_HAND);
        } else {
            if (m_guiPointerNode->getName().size() > 11) {
                // The drag buttons resize along their axis in their middle half.
                std::wstring suffix = m_guiPointerNode->getName().substr(m_guiPointerNode->getName().size() - 11, 11);
                if (suffix == L"hdragbutton") {
                    if (m_guiPointerNode->m_enclosingWidget->getViewSize()[0] * 0.25 <
                            m_guiPointerNode->m_enclosingWidget->getPointerOffset()[0] &&
                        m_guiPointerNode->m_enclosingWidget->getViewSize()[0] * 3.0 * 0.25 - 1.0 >
                            m_guiPointerNode->m_enclosingWidget->getPointerOffset()[0])
                        setCursorShape(CURSOR_SIZE_WE);
                    else
                        setCursorShape(CURSOR_HAND);
                    return;
                }
                if (suffix == L"vdragbutton") {
                    if (m_guiPointerNode->m_enclosingWidget->getViewSize()[1] * 0.25 <
                            m_guiPointerNode->m_enclosingWidget->getPointerOffset()[1] &&
                        m_guiPointerNode->m_enclosingWidget->getViewSize()[1] * 3.0 * 0.25 - 1.0 >
                            m_guiPointerNode->m_enclosingWidget->getPointerOffset()[1])
                        setCursorShape(CURSOR_SIZE_NS);
                    else
                        setCursorShape(CURSOR_HAND);
                    return;
                }
            }
            setCursorShape(CURSOR_ARROW);
        }
    } else if (m_engine.isButtonDown(plasma::MOUSE_BUTTON_MIDDLE) ||
               m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) && m_engine.isKeyDown(VK_SPACE)) {
        // Panning: the middle button, or the left one with the space bar.
        setCursorShape(CURSOR_SIZE_ALL);
    } else {
        int handle;
        switch (getToolModule()->getCurrentTool()->getType()) {
        case Tool::TOOL_DRAW_FACE:
        case Tool::TOOL_DRAW_EDGE:
            setCursorShape(CURSOR_TOOL_ARROW);
            return;
        case Tool::TOOL_NODE:

            // The node tool: its scale box's handles, or a hand over a node to pick for a pending "Choose Parent...", "Send in
            // Front of..." or "Send Behind...".
            if (m_toolModule->m_nodeTool.handlesShown() && m_nodeToolModule->m_scaleBox) {
                if (!m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) &&
                        (handle = m_toolModule->m_nodeTool.findHandle()) != BoundingBoxTool::NoHandle ||
                    m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) &&
                        (handle = m_toolModule->m_nodeTool.getDragHandle()) != BoundingBoxTool::NoHandle) {
                    setCursorShape(getHandleCursor(handle));
                    return;
                }
                if (m_hoveredNode && getOpenDesignSheet() && getOpenDesignSheet()->findSheetNode(m_hoveredNode)) {
                    setCursorShape(CURSOR_HAND);
                    return;
                }
            } else if ((m_chooseParentRequested || m_sendInFrontOfRequested || m_sendBehindRequested) &&
                       getHoveredNode() && getDesignSheet() && getDesignSheet()->findSheetNode(m_hoveredNode)) {
                setCursorShape(CURSOR_HAND);
                return;
            }
            break;
        case Tool::TOOL_VERTEX:
        case Tool::TOOL_EDGE:
        case Tool::TOOL_FACE:
        case Tool::TOOL_EXTRUSION:

            // The mesh tools' selection box (always for type 13).
            if (dynamic_cast<BoundingBoxTool *>(getToolModule()->getCurrentTool())->handlesShown() &&
                (m_vertexModule->m_scaleBoxEnabled ||
                 getToolModule()->getCurrentTool()->getType() == Tool::TOOL_EXTRUSION)) {
                if (!m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) &&
                        (handle = dynamic_cast<BoundingBoxTool *>(getToolModule()->getCurrentTool())->findHandle()) !=
                            BoundingBoxTool::NoHandle ||
                    m_engine.isButtonDown(plasma::MOUSE_BUTTON_LEFT) &&
                        (handle =
                             dynamic_cast<BoundingBoxTool *>(getToolModule()->getCurrentTool())->getDragHandle()) !=
                            BoundingBoxTool::NoHandle) {
                    setCursorShape(getHandleCursor(handle));
                    return;
                }
            }
            break;
        case Tool::TOOL_TEXT:
            if (m_hoveredTextInterface) {
                setCursorShape(CURSOR_IBEAM);
                return;
            }
            break;
        }
        setCursorShape(CURSOR_ARROW);
    }
}

// The text being edited, while the current tool is the text tool (type 6).
TextInterface *Workspace::getEditingTextInterface()
{
    if (m_toolModule->getCurrentTool()->getType() != Tool::TOOL_TEXT)
        return 0;
    return m_textInterface;
}

// Whether the shape is a smooth mesh (type 2).
static inline bool isSmoothMesh(plasma::Shape *shape)
{
    return shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH;
}

// The last shown, selected smooth mesh node in the subtree (children searched from the top of the drawing order
// down).
static plasma::Node *findSelectedMesh(plasma::Node *node, DesignSheet *sheet)
{
    if (!node->isShown())
        return 0;
    for (std::list<plasma::Node *>::reverse_iterator it = node->m_children.rbegin(); it != node->m_children.rend();
         ++it) {
        plasma::Node *found = findSelectedMesh(*it, sheet);
        if (found)
            return found;
    }
    if (node->m_shape && node->m_shape->getType() == plasma::SHAPE_SMOOTH_MESH && sheet->findSheetNode(node) &&
        sheet->findSheetNode(node)->m_selected)
        return node;
    return 0;
}

// Mesh menu "Merge Meshes": the other selected shown meshes are merged into the current node's mesh (else the
// topmost selected mesh), in its coordinates and with their orientation matched, then deleted and deselected
// ("deselect node").
void Workspace::mergeMeshes()
{
    if (!getOpenDesignSheet())
        return;
    plasma::Node *target;
    if (getOpenDesignSheet()->m_currentNode && isSmoothMesh(getOpenDesignSheet()->m_currentNode->m_shape))
        target = getOpenDesignSheet()->m_currentNode;
    else
        target = findSelectedMesh(getOpenDesignSheet()->getRoot(), getOpenDesignSheet());
    if (!target)
        return;
    ShapeInterface *shapeInterface = getOpenDesignSheet()->findSheetNode(target)->m_shapeInterface;
    MeshInterface *targetMesh =
        shapeInterface && shapeInterface->m_type == INTERFACE_MESH ? static_cast<MeshInterface *>(shapeInterface) : 0;
    targetMesh->beginModify();
    float area = targetMesh->m_mesh.getSignedArea();

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *designSheet = getOpenDesignSheet();
    std::map<plasma::Node *, NodeInterface *>::iterator it;
    it = designSheet->sheetNodesBegin();
    for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && it->first != target) {
            ShapeInterface *otherInterface = it->second->m_shapeInterface;
            if (otherInterface && otherInterface->m_type == INTERFACE_MESH) {
                MeshInterface *other = static_cast<MeshInterface *>(otherInterface);
                other->beginModify();
                plasma::Matrix<float> matrix = target->m_inverseMatrix;
                matrix *= it->first->m_matrix;
                other->m_mesh.transform(matrix);
                other->endModify();
                MeshVertex *vertex = 0;
                float otherArea = other->m_mesh.getSignedArea();
                targetMesh->mergeMesh(&other->m_mesh, &vertex, otherArea * area < 0, false);
                getOpenDesignSheet()->deleteNode(it->first);
                NodeState before;
                it->second->saveState(before);
                it->second->setSelected(false);
                NodeState after;
                it->second->saveState(after);
                getOpenDesignSheet()->addOperation(
                    new NodeOperation(getOpenDesignSheet(), "deselect node", it->second, before, after));
            }
        }
    }
    targetMesh->m_mesh.updateShape();
    targetMesh->refresh();
    targetMesh->endModify();
    getOpenDesignSheet()->commit();
    getOpenDesignSheet()->m_currentNode = target;
    updateModules();
    redraw();
}

// Ends editing the current text: an emptied text's node is deleted (with its selection and the current node
// cleared) and the change committed. Then the given text interface is current.
void Workspace::setTextInterface(TextInterface *textInterface)
{
    if (TextInterface *current = m_textInterface) {
        if (current->m_shape->m_string.value().size() == 0) {
            current->m_sheet->m_currentNode = 0;
            plasma::Node *node = m_textInterface->m_node->m_node;
            m_textInterface->m_sheet->updateCurrentNode(0);
            updateModules();
            m_textInterface->m_node->setSelected(false);
            m_textInterface->m_sheet->deleteNode(node);
            m_textInterface->m_sheet->commit();
            refreshTimeline();
        }
    }
    m_textInterface = textInterface;
}

// Selection menu "Select Same Color": selects every node that uses a colour of the selected nodes.
void Workspace::selectSameColor()
{
    if (getOpenDesignSheet()) {
        std::list<plasma::Vector<4, float>> colors;
        for (std::map<plasma::Node *, NodeInterface *>::iterator it = getOpenDesignSheet()->sheetNodesBegin();
             it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface)
                it->second->m_shapeInterface->getColors(colors);
        }
        selectNodesWithColors(getOpenDesignSheet()->m_root, colors);
        getOpenDesignSheet()->commit();
        redraw();
        markDesignSheetChanged();
    }
}

// Selects ("select node") each shown, unlocked node under node whose shape uses one of the colours (red, green and
// blue within 0.1).
void Workspace::selectNodesWithColors(plasma::Node *node, std::list<plasma::Vector<4, float>> &colors)
{
    if (!node->isShown())
        return;
    if (!node->isLocked() && getOpenDesignSheet()->findSheetNode(node)->m_shapeInterface &&
        !getOpenDesignSheet()->findSheetNode(node)->m_selected) {
        std::list<plasma::Vector<4, float>> nodeColors;
        getOpenDesignSheet()->findSheetNode(node)->getShapeInterface()->getColors(nodeColors);
        for (std::list<plasma::Vector<4, float>>::iterator color = colors.begin(); color != colors.end(); ++color) {
            for (std::list<plasma::Vector<4, float>>::iterator nodeColor = nodeColors.begin();
                 nodeColor != nodeColors.end(); ++nodeColor) {
                if (std::fabs(color->m_elements[0] - nodeColor->m_elements[0]) < 0.1f &&
                    std::fabs(color->m_elements[1] - nodeColor->m_elements[1]) < 0.1f &&
                    std::fabs(color->m_elements[2] - nodeColor->m_elements[2]) < 0.1f) {
                    NodeState before;
                    getOpenDesignSheet()->findSheetNode(node)->saveState(before);
                    getOpenDesignSheet()->findSheetNode(node)->setSelected(true);
                    NodeState after;
                    getOpenDesignSheet()->findSheetNode(node)->saveState(after);
                    getOpenDesignSheet()->addOperation(new NodeOperation(
                        getOpenDesignSheet(), "select node", getOpenDesignSheet()->findSheetNode(node), before, after));
                    goto selected;
                }
            }
        }
    selected:;
    }
    for (std::list<plasma::Node *>::iterator child = node->m_children.begin(); child != node->m_children.end(); ++child)
        selectNodesWithColors(*child, colors);
}

// Scales the interface ("scalable") by the GUI zoom of the options ("guiZoom") and lays it out again; saves the
// options when asked.
void Workspace::setInterfaceZoom(float zoom, bool save)
{
    plasma::Transformation *transformation = m_scalableNode->m_transformation;
    transformation->m_deformation.value().identity();
    transformation->m_deformation.value().scale(zoom, zoom);
    m_scalableNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_scalableNode->animate(0);
    updateLayout();
    OptionsModule *options = m_optionsModule; // Read once.
    options->m_guiZoom = zoom;
    if (save)
        options->save();
}

// Sheet menu "Tile Vertically": the shown sheets' frames in two rows over the viewport (a single sheet fills it).
void Workspace::tileVertically()
{
    int count = 0;
    std::for_each(m_sheets.begin(), m_sheets.end(), [&count](Sheet *sheet) {
        if (sheet->m_frame->getNode()->m_display->m_visibility.value())
            ++count;
    });
    if (count == 1) {
        std::for_each(m_sheets.begin(), m_sheets.end(), [this](Sheet *sheet) {
            if (sheet->m_frame->getNode()->m_display->m_visibility.value())
                sheet->m_frame->setViewRect(-10.0f, -10.0f, (float)(m_engine.m_viewportWidth + 20),
                                            (float)(m_engine.m_viewportHeight + 20));
        });
        return;
    }
    int columns = count / 2;
    int column = 0;
    int row = 0;
    std::for_each(m_sheets.begin(), m_sheets.end(), [&column, this, &columns, &row, &count](Sheet *sheet) {
        if (sheet->m_frame->getNode()->m_display->m_visibility.value()) {
            sheet->m_frame->setViewRect((float)(m_engine.m_viewportWidth * column / columns - 10),
                                        (float)(m_engine.m_viewportHeight * row / 2 - 10),
                                        (float)(m_engine.m_viewportWidth / columns + 20),
                                        (float)(m_engine.m_viewportHeight / 2 + 20));
            ++column;
            if (column >= columns) {
                column = 0;
                columns = count - columns;
                ++row;
            }
        }
    });
}

// Sheet menu "Tile Horizontally": the shown sheets' frames in two columns.
void Workspace::tileHorizontally()
{
    int count = 0;
    std::for_each(m_sheets.begin(), m_sheets.end(), [&count](Sheet *sheet) {
        if (sheet->m_frame->getNode()->m_display->m_visibility.value())
            ++count;
    });
    if (count == 1) {
        std::for_each(m_sheets.begin(), m_sheets.end(), [this](Sheet *sheet) {
            if (sheet->m_frame->getNode()->m_display->m_visibility.value())
                sheet->m_frame->setViewRect(-10.0f, -10.0f, (float)(m_engine.m_viewportWidth + 20),
                                            (float)(m_engine.m_viewportHeight + 20));
        });
        return;
    }
    int rows = count / 2;
    int column = 0;
    int row = 0;
    std::for_each(m_sheets.begin(), m_sheets.end(), [&column, this, &row, &rows, &count](Sheet *sheet) {
        if (sheet->m_frame->getNode()->m_display->m_visibility.value()) {
            sheet->m_frame->setViewRect((float)(m_engine.m_viewportWidth * column / 2 - 10),
                                        (float)(m_engine.m_viewportHeight * row / rows - 10),
                                        (float)(m_engine.m_viewportWidth / 2 + 20),
                                        (float)(m_engine.m_viewportHeight / rows + 20));
            ++row;
            if (row >= rows) {
                row = 0;
                rows = count - rows;
                ++column;
            }
        }
    });
}

// Sheet menu "Cascade": the shown sheets' frames cascaded, each two thirds of the viewport.
void Workspace::cascade()
{
    int count = 0;
    std::for_each(m_sheets.begin(), m_sheets.end(), [&count](Sheet *sheet) {
        if (sheet->m_frame->getNode()->m_display->m_visibility.value())
            ++count;
    });
    if (count == 1) {
        std::for_each(m_sheets.begin(), m_sheets.end(), [this](Sheet *sheet) {
            if (sheet->m_frame->getNode()->m_display->m_visibility.value())
                sheet->m_frame->setViewRect(-10.0f, -10.0f, (float)(m_engine.m_viewportWidth + 20),
                                            (float)(m_engine.m_viewportHeight + 20));
        });
        return;
    }
    int width = m_engine.m_viewportWidth * 2 / 3;
    int height = m_engine.m_viewportHeight * 2 / 3;
    --count;
    int index = 0;
    std::for_each(m_sheets.begin(), m_sheets.end(), [&index, this, &width, &count, &height](Sheet *sheet) {
        if (sheet->m_frame->getNode()->m_display->m_visibility.value()) {
            sheet->m_frame->setViewRect((float)(index * (m_engine.m_viewportWidth - width) / count - 10),
                                        (float)(index * (m_engine.m_viewportHeight - height) / count - 10),
                                        (float)(width + 20), (float)(height + 20));
            ++index;
        }
    });
}

// After the sheet tool restacks the frames: orders m_sheets as their frames are stacked and rebuilds the sheet
// menu.
void Workspace::updateSheetOrder()
{
    m_sheets.clear();
    for (std::list<plasma::Node *>::iterator it = m_sheetsNode->m_children.begin();
         it != m_sheetsNode->m_children.end(); ++it)
        m_sheets.push_back(static_cast<Sheet *>((*it)->m_userData));
    buildSheetMenu();
}

// File menu "Import TrueType Font...": into the open design sheet (asking to save a modified one), or a new one.
void Workspace::importTTF()
{
    setTextInterface(0);
    resetTools();
    std::wstring path = openFileBox(L"Import TrueType Font", L"TrueType fonts\0*.ttf\0");
    if (path.size()) {
        if (getOpenDesignSheet() && getOpenDesignSheet()->m_modified && !askSaveChanges())
            return;
        if (!getOpenDesignSheet())
            newSheet();
        _chdir(m_applicationPath.c_str());
        getOpenDesignSheet()->clear();
        importFont(path);
    }
}

// Makes a glyph node for each character of the options' "ttfCharacters" from the TrueType font: a transparent
// background quad from the glyph's left edge to its advance, and under it the outline as a stroked smooth mesh
// (off-curve points make curve vertices; collinear reversals are dropped). The glyphs are laid out left to right
// on a page as wide as the row and 100 high.
void Workspace::importFont(const std::wstring &path)
{
    if (!getOpenDesignSheet())
        return;
    FT_Library library;
    FT_Init_FreeType(&library);
    plasma::Vector<2, float> offset(0.0f, 100.0f);
    FT_Face face;
    FT_New_Face(library, narrowString(path).c_str(), 0, &face);
    FT_Set_Pixel_Sizes(face, plasma::ImportedGlyphPixelHeight, plasma::ImportedGlyphPixelHeight);
    int count = m_optionsModule->m_ttfCharacters.size();
    for (int character = 0; character < count; ++character) {
        wchar_t code = m_optionsModule->m_ttfCharacters[character];
        wchar_t name[2] = {code, 0};
        FT_Load_Char(face, code, FT_LOAD_NO_BITMAP | FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING);
        plasma::SmoothMeshShape *background = m_engine.createSmoothMeshShape(name);
        plasma::Node *glyphNode = m_engine.createNode(0, background, 0, getOpenDesignSheet()->getRoot(), name);
        background->setSubdivisions(0, true);
        background->m_vertexPositions.resize(4);
        background->m_vertexColors.resize(4);
        background->m_vertexPositions.value()[0] =
            plasma::Vector<2, float>(face->glyph->bitmap_left * (double)0.015625f, -100.0f);
        background->m_vertexPositions.value()[1] =
            plasma::Vector<2, float>(face->glyph->advance.x * (double)0.015625f, -100.0f);
        background->m_vertexPositions.value()[2] =
            plasma::Vector<2, float>(face->glyph->advance.x * (double)0.015625f, 0.0f);
        background->m_vertexPositions.value()[3] =
            plasma::Vector<2, float>(face->glyph->bitmap_left * (double)0.015625f, 0.0f);
        background->m_vertexColors.value()[0] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        background->m_vertexColors.value()[1] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        background->m_vertexColors.value()[2] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        background->m_vertexColors.value()[3] = plasma::Vector<4, float>(0.0f, 0.0f, 0.0f, 0.0f);
        std::vector<std::vector<int>> backgroundFaces(1);
        backgroundFaces.back().resize(4);
        backgroundFaces.back()[0] = 0;
        backgroundFaces.back()[1] = 1;
        backgroundFaces.back()[2] = 2;
        backgroundFaces.back()[3] = 3;
        background->setFaces(backgroundFaces, true);
        plasma::SmoothMeshShape *shape = m_engine.createSmoothMeshShape(name);
        m_engine.createNode(0, shape, 0, glyphNode, name);
        std::vector<std::vector<int>> faces;
        int start = 0;
        for (int contour = 0; contour < face->glyph->outline.n_contours; ++contour) {
            int last = face->glyph->outline.contours[contour];
            plasma::Vector<2, float> previous(static_cast<float>(face->glyph->outline.points[last].x),
                                              static_cast<float>(-face->glyph->outline.points[last].y));
            previous = previous * 0.015625f;
            plasma::Vector<2, float> previousDelta(0.0f, 0.0f);
            bool previousOffCurve = !(face->glyph->outline.tags[last] & 1);
            std::vector<int> vertices;
            for (int point = start; point <= face->glyph->outline.contours[contour]; ++point) {
                plasma::Vector<2, float> position(static_cast<float>(face->glyph->outline.points[point].x),
                                                  static_cast<float>(-face->glyph->outline.points[point].y));
                position = position * 0.015625f;
                plasma::Vector<2, float> delta = position - previous;
                float distanceSquared = delta[0] * delta[0] + delta[1] * delta[1];
                if (!(distanceSquared < 0.001f)) {
                    if (delta[0] * previousDelta[1] - previousDelta[0] * delta[1] == 0.0f) {
                        float dot = delta[0] * previousDelta[0];
                        dot += delta[1] * previousDelta[1];
                        if (dot < 0.0f) {
                            shape->m_vertexPositions.erase(shape->m_vertexPositions.m_values[0]->size() - 1);
                            shape->m_vertexTexCoords.erase(shape->m_vertexPositions.m_values[0]->size());
                            shape->m_vertexColors.erase(shape->m_vertexPositions.m_values[0]->size());
                            shape->m_strokeColors.erase(shape->m_vertexPositions.m_values[0]->size());
                            shape->m_strokeWidths.erase(shape->m_vertexPositions.m_values[0]->size());
                            if (!shape->m_vertexFlags.empty())
                                shape->m_vertexFlags.pop_back();
                            if (!vertices.empty())
                                vertices.pop_back();
                        }
                    }
                    previousDelta = delta;
                    if (previousOffCurve && !(face->glyph->outline.tags[point] & FT_CURVE_TAG_ON)) {
                        vertices.push_back(shape->m_vertexPositions.m_values[0]->size());
                        plasma::Vector<2, float> midpoint = (previous + position) * 0.5f;
                        shape->m_vertexPositions.append(midpoint);
                        shape->m_vertexColors.append(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                        shape->m_vertexTexCoords.append(plasma::Vector<2, float>(0.0f, 0.0f));
                        shape->m_strokeColors.append(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                        float width = m_strokeWidthModule->getWidth();
                        shape->m_strokeWidths.append(width);
                        shape->m_vertexFlags.push_back(plasma::VERTEX_SHARP);
                    }
                    vertices.push_back(shape->m_vertexPositions.m_values[0]->size());
                    shape->m_vertexPositions.append(position);
                    shape->m_vertexColors.append(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                    shape->m_vertexTexCoords.append(plasma::Vector<2, float>(0.0f, 0.0f));
                    shape->m_strokeColors.append(plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f));
                    float width = m_strokeWidthModule->getWidth();
                    shape->m_strokeWidths.append(width + width);
                    shape->m_vertexFlags.push_back(face->glyph->outline.tags[point] & FT_CURVE_TAG_ON);
                    previous = position;
                    previousOffCurve = !(face->glyph->outline.tags[point] & FT_CURVE_TAG_ON);
                }
            }
            faces.push_back(vertices);
            start = face->glyph->outline.contours[contour] + 1;
        }
        shape->m_smoothWeight = 1.0f;
        shape->m_flags =
            (shape->m_flags & ~((1 << plasma::MESH_STROKE) | (1 << plasma::MESH_NO_FILL) | (1 << plasma::MESH_OPEN))) |
            (1 << plasma::MESH_CONTOUR);
        shape->setFaces(faces, true);
        glyphNode->m_transformation->m_translation.value() = offset;
        glyphNode->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
        offset[0] = glyphNode->getMaximum(0)[0];
        getOpenDesignSheet()->createSheetNodes(glyphNode);
    }
    FT_Done_Face(face);
    FT_Done_FreeType(library);
    getOpenDesignSheet()->m_page = plasma::Engine::Page(1000.0f, 1000.0f, 72.0f);
    getOpenDesignSheet()->m_page.size[0] = (float)(int)offset[0];
    getOpenDesignSheet()->m_page.size[1] = 100.0f;
    getOpenDesignSheet()->m_page.color = plasma::Vector<4, float>(0.5f, 0.5f, 0.5f, 1.0f);
    getOpenDesignSheet()->applyBackgroundColor();
    m_formatModule->update();
    m_zoomModule->update();
    selectionChanged();
}

float Workspace::toDesignSheetInternalUnits(float value)
{
    if (!getOpenDesignSheet())
        return value;
    return plasma::toInternalUnits(getOpenDesignSheet()->m_page.unit, value);
}

float Workspace::fromDesignSheetInternalUnits(float value)
{
    if (!getOpenDesignSheet())
        return value;
    return plasma::fromInternalUnits(getOpenDesignSheet()->m_page.unit, value);
}

float Workspace::fromDisplayUnits(float value)
{
    if (!getOpenSheet())
        return value;
    return plasma::toInternalUnits(getOpenSheet()->m_page.unit, value);
}

float Workspace::toDisplayUnits(float value)
{
    if (!getOpenSheet())
        return value;
    return plasma::fromInternalUnits(getOpenSheet()->m_page.unit, value);
}

void Workspace::unitChanged()
{
    for (std::list<Module *>::iterator it = m_modules.begin(), end = m_modules.end(); it != end; ++it)
        (*it)->unitChanged();
}

void Workspace::fitSheetView(Sheet *sheet, const plasma::Vector<2, float> &minimum,
                             const plasma::Vector<2, float> &maximum)
{
    if (!sheet)
        return;
    plasma::Widget *panel = sheet->m_panel;
    float scale;
    if (panel->viewWidth() / (maximum.m_elements[0] - minimum.m_elements[0]) <
        panel->getViewHeight() / (maximum.m_elements[1] - minimum.m_elements[1]))
        scale = sheet->m_panel->viewWidth() / (maximum.m_elements[0] - minimum.m_elements[0]);
    else
        scale = sheet->m_panel->getViewHeight() / (maximum.m_elements[1] - minimum.m_elements[1]);
    sheet->m_root->transformation()->m_translation.value() = plasma::Vector<2, float>(0.0f, 0.0f);
    sheet->m_root->transformation()->m_deformation.value().identity();
    sheet->m_root->transformation()->m_deformation.value().scale(scale, scale);
    sheet->m_root->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    plasma::Vector<2, float> center;
    center = sheet->m_root->transformation()->m_matrix.transform((minimum + maximum) * 0.5f);
    panel = sheet->m_panel;
    plasma::Vector<2, float> viewCenter(panel->viewWidth(), panel->getViewHeight());
    viewCenter[0] *= (double)0.5f;
    viewCenter[1] *= (double)0.5f;
    sheet->m_root->transformation()->m_translation.value() = viewCenter - center;
    sheet->m_root->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
    m_zoomModule->update();
}

void Workspace::viewPage(Sheet *sheet)
{
    if (sheet || (sheet = getOpenSheet()))
        fitSheetView(sheet, plasma::Vector<2, float>(0.0f, 0.0f),
                     plasma::Vector<2, float>(sheet->m_page.size.m_elements[0], sheet->m_page.size.m_elements[1]));
}

// The design's bounds; on a texture sheet, as "Selection".
void Workspace::viewDrawing()
{
    if (getOpenSheet()) {
        if (DesignSheet *sheet = dynamic_cast<DesignSheet *>(m_sheet)) {
            plasma::Vector<2, float> minimum(0.0f, 0.0f);
            plasma::Vector<2, float> maximum(0.0f, 0.0f);
            plasma::Matrix<float> identity;
            identity.identity();
            bool empty = true;
            sheet->m_root->extendShapeBounds(minimum, maximum, identity, empty);
            fitSheetView(sheet, minimum, maximum);
        } else {
            viewSelection();
        }
    }
}

// "Selection": fits the view to the selected nodes' bounds. On a texture sheet, to the bounds of the selected
// shapes' texture mapping (each shape interface measured through the texture sheet's matrix).
void Workspace::viewSelection()
{
    if (!getOpenSheet())
        return;
    if (DesignSheet *sheet = dynamic_cast<DesignSheet *>(m_sheet)) {
        plasma::Vector<2, float> minimum(0.0f, 0.0f);
        bool empty = true;
        plasma::Vector<2, float> maximum(0.0f, 0.0f);

        // Starts with the first sheet node, or the end while the sheet is busy.
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != sheet->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isLocked()) {
                plasma::Matrix<float> matrix = sheet->getRoot()->getInverseMatrix();
                matrix *= it->first->getMatrix();
                it->first->extendShapeBounds(minimum, maximum, matrix, empty);
            }
        }
        if (!empty)
            fitSheetView(sheet, minimum, maximum);
    } else if (getOpenDesignSheet()) {
        plasma::Vector<2, float> minimum(0.0f, 0.0f);
        bool empty = true;
        plasma::Vector<2, float> maximum(0.0f, 0.0f);

        // Starts with the first sheet node, or the end while the sheet is busy.
        DesignSheet *designSheet = getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (designSheet->isBusy())
            it = designSheet->m_sheetNodes.end();
        else
            it = designSheet->m_sheetNodes.begin();
        for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown() && !it->first->isLocked() &&
                it->second->getShapeInterface()) {
                it->second->getShapeInterface()->setMatrix(getTextureSheet()->getMatrix());
                plasma::Vector<2, float> shapeMinimum;
                plasma::Vector<2, float> shapeMaximum;
                it->second->getShapeInterface()->getBounds(&shapeMinimum, &shapeMaximum);
                shapeMinimum.transform(getTextureSheet()->getRoot()->getInverseMatrix());
                shapeMaximum.transform(getTextureSheet()->getRoot()->getInverseMatrix());
                if (empty) {
                    minimum = shapeMinimum;
                    maximum = shapeMaximum;
                    empty = false;
                } else {
                    if (shapeMinimum.m_elements[0] < minimum.m_elements[0])
                        minimum.m_elements[0] = shapeMinimum.m_elements[0];
                    if (shapeMinimum.m_elements[1] < minimum.m_elements[1])
                        minimum.m_elements[1] = shapeMinimum.m_elements[1];
                    if (shapeMaximum.m_elements[0] > maximum.m_elements[0])
                        maximum.m_elements[0] = shapeMaximum.m_elements[0];
                    if (shapeMaximum.m_elements[1] > maximum.m_elements[1])
                        maximum.m_elements[1] = shapeMaximum.m_elements[1];
                }
                it->second->getShapeInterface()->resetMatrix();
            }
        }
        if (!empty)
            fitSheetView(getTextureSheet(), minimum, maximum);
    }
}

// Mesh menu "Open Textured Rectangle...": loads the texture and makes a rectangle with it.
void Workspace::openTexturedRectangle()
{
    if (getOpenDesignSheet()) {
        std::wstring path =
            openFileBox(L"Open texture", L"All supported types\0*.JPG;*.JPEG;*.BMP;*.PNG;*.TIG;*.TGA\0"
                                         L"JPEG (*.jpg)\0*.JPG;*.JPEG\0BMP (*.bmp)\0*.bmp\0PNG (*.png)\0*.png\0"
                                         L"TIFF (*.tif)\0*.tif\0TARGA (*.tga)\0*.tga\0");
        if (path.empty())
            return;
        m_textureModule->loadTexture(path);
        createTexturedRectangle();
    }
}

// "Create Textured Rectangle": deselects everything and adds a rectangle the size of the texture panel's texture,
// mapped with it, to the edit root.
void Workspace::createTexturedRectangle()
{
    if (!getOpenDesignSheet() || !getTextureModule()->getTexture())
        return;
    resetTools();

    // Starts with the first sheet node, or the end while the sheet is busy.
    DesignSheet *sheet = getOpenDesignSheet();
    std::map<plasma::Node *, NodeInterface *>::iterator it;
    if (sheet->isBusy())
        it = sheet->m_sheetNodes.end();
    else
        it = sheet->m_sheetNodes.begin();
    for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (!it->first->isLocked() && it->first->isShown() && it->second->m_selected) {
            it->second->saveState();
            it->second->setSelected(false);
            it->second->recordModification();
        }
    }
    plasma::SmoothMeshShape *shape = m_engine.createSmoothMeshShape(L"Image");
    shape->m_vertexPositions.resize(4);
    shape->m_vertexTexCoords.resize(4);
    shape->m_vertexFlags.resize(4, plasma::VERTEX_SHARP);
    shape->m_vertexColors.resize(4);
    shape->m_strokeColors.resize(4);
    shape->m_extrusionFrontColors.resize(4);
    shape->m_extrusionBackColors.resize(4);
    shape->m_vertexPositions.value()[0] = plasma::Vector<2, float>(0.0f, 0.0f);
    shape->m_vertexPositions.value()[1] =
        plasma::Vector<2, float>((float)getTextureModule()->getTexture()->getWidth(), 0.0f);
    shape->m_vertexPositions.value()[2] = plasma::Vector<2, float>(
        (float)getTextureModule()->getTexture()->getWidth(), (float)getTextureModule()->getTexture()->getHeight());
    shape->m_vertexPositions.value()[3] =
        plasma::Vector<2, float>(0.0f, (float)getTextureModule()->getTexture()->getHeight());
    shape->m_vertexTexCoords.value()[0] = plasma::Vector<2, float>(0.0f, 0.0f);
    shape->m_vertexTexCoords.value()[1] = plasma::Vector<2, float>(1.0f, 0.0f);
    shape->m_vertexTexCoords.value()[2] = plasma::Vector<2, float>(1.0f, 1.0f);
    shape->m_vertexTexCoords.value()[3] = plasma::Vector<2, float>(0.0f, 1.0f);
    shape->m_vertexColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_vertexColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_vertexColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_vertexColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_strokeColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_strokeColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_strokeColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_strokeColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionFrontColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[0] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[1] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[2] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->m_extrusionBackColors.value()[3] = plasma::Vector<4, float>(1.0f, 1.0f, 1.0f, 1.0f);
    shape->setSubdivisions(m_meshModule->m_subdivisions, false);
    shape->m_texture.value() = getTextureModule()->getTexture()->m_id;
    std::vector<std::vector<int>> faces(1);
    faces[0].resize(4);
    faces[0][0] = 0;
    faces[0][1] = 1;
    faces[0][2] = 2;
    faces[0][3] = 3;
    shape->setFaces(faces, true);
    plasma::Node *node = m_engine.createNode(0, shape, 0, getOpenDesignSheet()->getRoot(), shape->getName());
    node->updateMatrices();
    NodeInterface *sheetNode = getOpenDesignSheet()->createSheetNode(node);
    sheetNode->createShapeInterface();
    sheetNode->setSelected(true);
    getOpenDesignSheet()->addOperation(
        new CreateNodeOperation(getOpenDesignSheet(), "create textured rectangle", node));
    moveSelectedNodes(node, getOpenDesignSheet()->m_editRoot);
    getOpenDesignSheet()->commit();
    getOpenDesignSheet()->m_currentNode = node;
    updateModules();
}

// "set sheet color": the current sheet takes the colour panel's colour as its background, recorded as a page
// operation on a design sheet.
void Workspace::setSheetColor()
{
    if (getOpenSheet()) {
        const plasma::Engine::Page before = m_sheet->m_page;
        m_sheet->m_page.color = m_colorModule->getColor();
        m_sheet->getRoot()->transformation()->m_changed = true;
        m_sheet->applyBackgroundColor();
        if (getDesignSheet()) {
            getDesignSheet()->addOperation(
                new PageOperation(getDesignSheet(), "set sheet color", before, m_sheet->m_page));
            getDesignSheet()->commit();
        }
    }
}

// The colour panel takes the current sheet's background colour (WndProc, SheetTool).
void Workspace::getSheetColor()
{
    if (m_sheet)
        m_colorModule->setColor(m_sheet->m_page.color, true, true, false);
}

void Workspace::recordModifiedNodes()
{
    if (!getOpenDesignSheet())
        return;
    getOpenDesignSheet()->recordModifiedNodes();
    getOpenDesignSheet()->commit();
}

void Workspace::endModifySelected()
{
    if (!getOpenDesignSheet())
        return;
    getOpenDesignSheet()->endModifyChanged();
    getOpenDesignSheet()->commit();
    redraw();
}

void Workspace::saveSelectedStates()
{
    if (getOpenDesignSheet())
        getOpenDesignSheet()->saveSelectedStates();
}

void Workspace::beginModifySelected()
{
    if (getOpenDesignSheet())
        getOpenDesignSheet()->beginModifySelected();
}

// The node of the interface icon of that name ("icon:sheet"...), if any.
plasma::Node *Workspace::findIcon(const std::wstring &name)
{
    std::map<std::wstring, plasma::Node *>::iterator it = m_icons.find(name);
    if (it == m_icons.end())
        return 0;
    return it->second;
}

// Converts the stroke of each selected shown smooth mesh into fill faces: a dashed stroke (pattern set) becomes
// one face per dash outline of the stroke drawing, a solid one quads between the stroke's sample pairs, with two
// cap quads 0.7 of the end segment long for a round cap on an open path. The stroke colours take the colour
// module's stroke and extrusion colours; the mesh is rebuilt one subdivision lower, without its stroke.
void Workspace::strokeToMesh()
{
    if (!getOpenDesignSheet())
        return;
    resetTools();
    beginModifySelected();
    for (std::map<plasma::Node *, NodeInterface *>::iterator it = getOpenDesignSheet()->sheetNodesBegin();
         it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (!it->first->isShown() || !it->second->m_selected || !it->second->m_shapeInterface ||
            it->second->m_shapeInterface->m_type != INTERFACE_MESH)
            continue;
        MeshInterface *mesh = static_cast<MeshInterface *>(it->second->m_shapeInterface);
        plasma::SmoothMeshShape *shape = mesh->m_mesh.m_shape;
        if (!plasma::SmoothMeshShape::hasStroke(shape->m_flags))
            continue;
        int subdivisions = shape->m_subdivisions;
        int strokeSegments = shape->m_strokeSegments;
        shape->setStrokeSegments(StrokeConversionCapSegments);
        int level = SolidStrokeConversionSubdivisionLimit;
        if (shape->m_strokePattern) {
            if (shape->m_subdivisions - 1 < 0)
                level = 0;
            else
                level = shape->m_subdivisions - 1;
        }
        if (level > shape->m_subdivisions)
            level = shape->m_subdivisions;
        shape->setSubdivisions(level, true);
        plasma::ContinuousArrayAttribute<plasma::Vector<2, float>> positions(std::string("", 0u),
                                                                             std::vector<plasma::Vector<2, float>>());
        plasma::ContinuousArrayAttribute<plasma::Vector<4, float>> colors(std::string("", 0u),
                                                                          std::vector<plasma::Vector<4, float>>());
        plasma::ContinuousArrayAttribute<plasma::Vector<2, float>> texturePositions(
            std::string("", 0u), std::vector<plasma::Vector<2, float>>());
        std::vector<int> vertexTypes;
        std::vector<std::vector<int>> faces;
        if (shape->m_strokePattern) {
            shape->m_flags |= (1 << plasma::MESH_CONTOUR);
            for (int contour = 0; contour < (int)shape->m_strokeDrawing->m_contours.size(); ++contour) {
                std::vector<int> face;
                std::vector<int> &indices = shape->m_strokeDrawing->m_contours[contour];
                for (int i = 0; i < (int)indices.size(); ++i) {
                    face.push_back(positions.m_values[0]->size());
                    positions.append(shape->m_strokeDrawing->m_pathVertices[indices[i]]);
                    colors.append(shape->m_strokeDrawing->m_pathColors[indices[i]]);
                    texturePositions.append(shape->m_strokeDrawing->m_pathTextureCoordinates[indices[i]]);
                    vertexTypes.push_back(plasma::VERTEX_SMOOTH);
                }
                faces.push_back(face);
            }
        } else {
            for (int contour = 0; contour < (int)shape->m_strokePairs.size(); ++contour) {
                int count = shape->m_strokeOutlinePositions[contour].size();
                if (shape->isClosed())
                    count -= 2;
                int base = positions.m_values[0]->size();
                for (int i = 0; i < count; ++i) {
                    positions.append(shape->m_strokeOutlinePositions[contour][i]);
                    colors.append(shape->m_strokeOutlineColors[contour][i]);
                    texturePositions.append(shape->m_strokeOutlineTexCoords[contour][i]);
                    vertexTypes.push_back(plasma::VERTEX_SMOOTH);
                }
                int pairs = count;
                if (!shape->isClosed())
                    pairs = count - 2;
                for (int i = 0; i < pairs; i += 2) {
                    std::vector<int> face(4);
                    face[0] = base + i % count;
                    face[1] = base + (i + 2) % count;
                    face[2] = base + (i + 3) % count;
                    face[3] = base + (i + 1) % count;
                    faces.push_back(face);
                }
                if ((!shape->isClosed()) && shape->m_strokeCapType == plasma::STROKE_CAP_ROUND) {
                    // Round caps on an open stroke: a quad beyond each end.
                    std::vector<int> cap;
                    cap.resize(4);
                    plasma::Vector<2, float> second = positions.value()[faces.back()[2]];
                    plasma::Vector<2, float> first = positions.value()[faces.back()[1]];
                    cap[0] = faces.back()[1];
                    cap[1] = positions.m_values[0]->size();
                    cap[2] = positions.m_values[0]->size() + 1;
                    cap[3] = faces.back()[2];
                    plasma::Vector<2, float> direction = second - first;
                    vertexTypes[faces.back()[1]] = plasma::VERTEX_SHARP;
                    vertexTypes[faces.back()[2]] = plasma::VERTEX_SHARP;
                    plasma::Vector<2, float> normal(direction.m_elements[1], -direction.m_elements[0]);
                    normal = normal * 0.7f;
                    positions.append(first + normal);
                    colors.append(colors.value()[faces.back()[1]]);
                    texturePositions.append(texturePositions.value()[faces.back()[1]]);
                    vertexTypes.push_back(plasma::VERTEX_BEZIER);
                    positions.append(second + normal);
                    colors.append(colors.value()[faces.back()[2]]);
                    texturePositions.append(texturePositions.value()[faces.back()[2]]);
                    vertexTypes.push_back(plasma::VERTEX_BEZIER);
                    faces.push_back(cap);
                    vertexTypes[base] = plasma::VERTEX_SHARP;
                    vertexTypes[base + 1] = plasma::VERTEX_SHARP;
                    first = positions.value()[base];
                    second = positions.value()[base + 1];
                    direction = first - second;
                    normal = plasma::Vector<2, float>(direction.m_elements[1], -direction.m_elements[0]);
                    cap[0] = base;
                    cap[1] = base + 1;
                    normal = normal * 0.7f;
                    cap[2] = positions.m_values[0]->size();
                    cap[3] = positions.m_values[0]->size() + 1;

                    // The colours and texture positions of the first cap's new vertices (faces.back is that cap).
                    positions.append(second + normal);
                    colors.append(colors.value()[faces.back()[1]]);
                    texturePositions.append(texturePositions.value()[faces.back()[1]]);
                    vertexTypes.push_back(plasma::VERTEX_BEZIER);
                    positions.append(first + normal);
                    colors.append(colors.value()[faces.back()[2]]);
                    texturePositions.append(texturePositions.value()[faces.back()[2]]);
                    vertexTypes.push_back(plasma::VERTEX_BEZIER);
                    faces.push_back(cap);
                }
            }
        }
        shape->m_vertexPositions = positions;
        shape->m_vertexColors = colors;
        shape->m_vertexTexCoords = texturePositions;
        shape->m_vertexFlags = vertexTypes;
        shape->m_strokeColors.resize(positions.m_values[0]->size());
        shape->m_strokeWidths.resize(positions.m_values[0]->size());
        shape->m_extrusionFrontColors.resize(positions.m_values[0]->size());
        shape->m_extrusionBackColors.resize(positions.m_values[0]->size());
        for (int i = 0; i < (int)positions.m_values[0]->size(); ++i) {
            shape->m_strokeColors.value()[i] = m_colorModule->m_strokeColor;

            // The front colours twice; the back colours keep their values.
            shape->m_extrusionFrontColors.value()[i] = m_colorModule->m_extrusionColor;
            shape->m_extrusionFrontColors.value()[i] = m_colorModule->m_extrusionColor;
            shape->m_strokeWidths.value()[i] = m_strokeWidthModule->getWidth();
        }
        shape->m_vertexParameters.clear();
        shape->m_flags &= ~((1 << plasma::MESH_STROKE) | (1 << plasma::MESH_NO_FILL) | (1 << plasma::MESH_OPEN));
        shape->m_strokePattern = plasma::STROKE_PATTERN_SOLID;
        shape->setSubdivisions(subdivisions - 1, false);
        shape->setStrokeSegments(strokeSegments);
        shape->setFaces(faces, true);
        mesh->m_mesh.rebuild();
        mesh->m_hasChanges = true;
        mesh->refresh();
    }
    updateModules();
    endModifySelected();
    redraw();
}

// Mesh menu "Create Inner Edges" for the selected meshes.
void Workspace::createInnerEdges()
{
    resetTools();
    beginModifySelected();
    for (std::map<plasma::Node *, NodeInterface *>::iterator it = getOpenDesignSheet()->sheetNodesBegin();
         it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (it->first->isShown() && it->second->m_selected && it->second->m_shapeInterface &&
            it->second->m_shapeInterface->m_type == INTERFACE_MESH)
            static_cast<MeshInterface *>(it->second->m_shapeInterface)->createInnerEdges();
    }
    endModifySelected();
    redraw();
}

// Cancels the special colour module's preview worker and waits for it.
void Workspace::waitForColorWorker()
{
    if (m_specialColorModule->m_previewRunning) {
        m_specialColorModule->m_cancel = true;
        while (m_specialColorModule->m_previewRunning)
            Sleep(ColorWorkerPollIntervalMilliseconds);
    }
}

void Workspace::hideSoftwareCursor()
{
    if (m_softwareCursor && m_softwareCursor->m_display->m_visibility.value()) {
        m_softwareCursor->m_display->m_visibility.value() = 0;
        m_redrawRequested = true;
    }
}

// TraceMeshes' polygon data: GLU traces the selected meshes' outlines (boundary only), each vertex index into the
// attribute arrays, which the combine callback extends with the interpolated new vertices.
struct TraceData {
    std::vector<plasma::Vector<2, float>> positions;
    std::vector<plasma::Vector<4, float>> vertexColors;
    std::vector<plasma::Vector<4, float>> strokeColors;
    std::vector<float> widths;
    std::vector<int> vertexTypes;
    std::vector<std::vector<int>> contours; // The traced contours' vertex indices.
};

// GLU_TESS_VERTEX_DATA: the vertex joins the current contour.
static void CALLBACK traceVertex(void *vertex, void *traceData)
{
    static_cast<TraceData *>(traceData)->contours.back().push_back((int)(PLASMA_POINTER_INT)vertex);
}

// GLU_TESS_BEGIN_DATA: a new contour.
static void CALLBACK traceBegin(GLenum type, void *traceData)
{
    static_cast<TraceData *>(traceData)->contours.push_back(std::vector<int>());
}

// Adds the value to the sum, element by element.
template<unsigned N> static inline void accumulate(plasma::Vector<N, float> &sum, const plasma::Vector<N, float> &value)
{
    for (unsigned i = 0; i < N; i++)
        sum.m_elements[i] = value.m_elements[i] + sum.m_elements[i];
}

// GLU_TESS_COMBINE_DATA: a new vertex where outlines cross, its attributes weighted from the four.
static void CALLBACK traceCombine(GLdouble coords[3], void *vertices[4], GLfloat weights[4], void **out,
                                  void *traceData)
{
    TraceData *trace = static_cast<TraceData *>(traceData);
    plasma::Vector<2, float> position(0.0f, 0.0f);
    plasma::Vector<4, float> vertexColor(0.0f, 0.0f, 0.0f, 0.0f);
    plasma::Vector<4, float> strokeColor(0.0f, 0.0f, 0.0f, 0.0f);
    float width = 0.0f;
    for (int i = 0; i < 4; i++) {
        int vertex = (int)(PLASMA_POINTER_INT)vertices[i];
        accumulate(position, trace->positions[vertex] * weights[i]);
        accumulate(vertexColor, trace->vertexColors[vertex] * weights[i]);
        accumulate(strokeColor, trace->strokeColors[vertex] * weights[i]);
        width = trace->widths[vertex] * weights[i] + width;
    }
    int index = trace->positions.size();
    trace->positions.push_back(position);
    trace->vertexColors.push_back(vertexColor);
    trace->strokeColors.push_back(strokeColor);
    trace->widths.push_back(width);
    trace->vertexTypes.push_back(plasma::VERTEX_SHARP);
    *out = (void *)index;
}

// GLU_TESS_EDGE_FLAG: set, so that GLU emits separate primitives (empty).
static void CALLBACK traceEdgeFlag(GLboolean flag) {}

// Mesh menu "Trace Meshes" ("unify meshes"): the outline of the selected shown meshes' union, in the edit root's
// coordinates, becomes a new mesh node "Union" (vertices without edges removed), selected and current.
void Workspace::traceMeshes()
{
    resetTools();
    TraceData trace;
    GLUtesselator *tessellator = gluNewTess();
    gluTessCallback(tessellator, GLU_TESS_BEGIN_DATA, (void(CALLBACK *)())traceBegin);
    gluTessCallback(tessellator, GLU_TESS_VERTEX_DATA, (void(CALLBACK *)())traceVertex);
    gluTessCallback(tessellator, GLU_TESS_COMBINE_DATA, (void(CALLBACK *)())traceCombine);
    gluTessCallback(tessellator, GLU_TESS_EDGE_FLAG, (void(CALLBACK *)())traceEdgeFlag);
    gluTessProperty(tessellator, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
    gluTessProperty(tessellator, GLU_TESS_BOUNDARY_ONLY, GL_TRUE);
    gluTessBeginPolygon(tessellator, &trace);
    DesignSheet *designSheet = getOpenDesignSheet();
    std::map<plasma::Node *, NodeInterface *>::iterator it = designSheet->sheetNodesBegin();
    for (; it != getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
        if (!it->first->isShown() || !it->second->m_selected)
            continue;
        ShapeInterface *shapeInterface = it->second->m_shapeInterface;
        if (!shapeInterface || shapeInterface->m_type != INTERFACE_MESH)
            continue;
        plasma::SmoothMeshShape *shape = static_cast<MeshInterface *>(shapeInterface)->m_mesh.m_shape;
        plasma::Node *node = it->first;
        plasma::Matrix<float> matrix = getOpenDesignSheet()->getEditRoot()->m_inverseMatrix;
        matrix *= node->m_matrix;
        for (int contour = 0; contour < (int)shape->m_contours.size(); contour++) {
            gluTessBeginContour(tessellator);
            for (int i = 0; i < (int)shape->m_contours[contour].size(); i++) {
                void *index = (void *)trace.positions.size();
                plasma::Vector<2, float> position =
                    matrix * shape->m_vertexPositions.value()[shape->m_contours[contour][i]];
                trace.positions.push_back(position);
                trace.vertexColors.push_back(shape->m_vertexColors.value()[shape->m_contours[contour][i]]);
                trace.strokeColors.push_back(shape->m_strokeColors.value()[shape->m_contours[contour][i]]);
                trace.widths.push_back(shape->m_strokeWidths.value()[shape->m_contours[contour][i]]);
                trace.vertexTypes.push_back(shape->m_vertexFlags[shape->m_contours[contour][i]]);
                GLdouble coords[3];
                coords[0] = position[0];
                coords[1] = position[1];
                coords[2] = 0.0;
                gluTessVertex(tessellator, coords, index);
            }
            gluTessEndContour(tessellator);
        }
    }
    gluTessEndPolygon(tessellator);
    if (trace.contours.empty())
        return;

    plasma::SmoothMeshShape *shape = m_engine.createSmoothMeshShape(L"");
    shape->m_vertexPositions.resize(trace.positions.size());
    shape->m_vertexColors.resize(trace.vertexColors.size());
    shape->m_strokeColors.resize(trace.strokeColors.size());
    shape->m_strokeWidths.resize(trace.widths.size());
    shape->m_vertexFlags.resize(trace.vertexTypes.size());
    for (int i = 0; i < (int)trace.positions.size(); i++) {
        shape->m_vertexPositions.value()[i] = trace.positions[i];
        shape->m_vertexColors.value()[i] = trace.vertexColors[i];
        shape->m_strokeColors.value()[i] = trace.strokeColors[i];
        shape->m_strokeWidths.value()[i] = trace.widths[i];
        shape->m_vertexFlags[i] = trace.vertexTypes[i];
    }
    shape->m_flags |= (1 << plasma::MESH_CONTOUR);
    shape->setFaces(trace.contours, true);
    plasma::Node *node = m_engine.createNode(0, shape, 0, getOpenDesignSheet()->getEditRoot(), L"Union");
    NodeInterface *sheetNode = getOpenDesignSheet()->createSheetNode(node);
    ShapeInterface *shapeInterface = sheetNode->m_shapeInterface;
    MeshInterface *meshInterface =
        shapeInterface && shapeInterface->m_type == INTERFACE_MESH ? static_cast<MeshInterface *>(shapeInterface) : 0;
    std::list<MeshVertex *> vertices = meshInterface->m_mesh.getVertices();
    for (std::list<MeshVertex *>::iterator v = vertices.begin(); v != vertices.end(); ++v) {
        if ((*v)->m_edges.empty())
            meshInterface->m_mesh.removeVertex(*v);
    }
    meshInterface->m_mesh.updateShape();
    meshInterface->refresh();
    sheetNode->setSelected(true);
    getOpenDesignSheet()->addOperation(new CreateNodeOperation(getOpenDesignSheet(), "unify meshes", node));
    getOpenDesignSheet()->commit();
    getOpenDesignSheet()->m_currentNode = node;
    updateModules();
    selectionChanged();
    markDesignSheetChanged();
}

void Workspace::writeWebNode(std::ostream &file, plasma::Node *node, const plasma::Matrix<float> &parentMatrix)
{
    if (!node->isShown() || !node->m_display->m_visibility.value())
        return;
    plasma::Matrix<float> matrix = parentMatrix;
    matrix *= node->m_transformation->m_matrix;
    if (node->m_display->m_clipping.value())
        file << "M" << std::endl;
    plasma::SmoothMeshShape *shape = static_cast<plasma::SmoothMeshShape *>(node->getShape());
    if (shape && shape->getType() == plasma::SHAPE_SMOOTH_MESH) {
        // The stroke's indices follow the fill's vertices.
        int fillVertexCount = 0;
        if (shape->m_fillDrawing) {
            file << "p";
            for (int i = 0; i < (int)shape->m_fillDrawing->m_pathVertices.size(); i++) {
                plasma::Vector<2, float> point = matrix * shape->m_fillDrawing->m_pathVertices[i];
                file << " " << point[0] << " " << point[1];
            }
            file << std::endl;
            file << "c";
            for (int i = 0; i < (int)shape->m_fillDrawing->m_pathColors.size(); i++) {
                file << " " << shape->m_fillDrawing->m_pathColors[i][0] << " "
                     << shape->m_fillDrawing->m_pathColors[i][1] << " " << shape->m_fillDrawing->m_pathColors[i][2]
                     << " " << shape->m_fillDrawing->m_pathColors[i][3];
            }
            file << std::endl;
            file << "i";
            for (int i = 0; i < (int)shape->m_fillDrawing->m_pathIndices.size(); i++)
                file << " " << (unsigned)shape->m_fillDrawing->m_pathIndices[i];
            file << std::endl;
            fillVertexCount = shape->m_fillDrawing->m_pathVertices.size();
        }
        if (shape->m_strokeDrawing) {
            file << "p";
            for (int i = 0; i < (int)shape->m_strokeDrawing->m_pathVertices.size(); i++) {
                plasma::Vector<2, float> point = matrix * shape->m_strokeDrawing->m_pathVertices[i];
                file << " " << point[0] << " " << point[1];
            }
            file << std::endl;
            file << "c";
            for (int i = 0; i < (int)shape->m_strokeDrawing->m_pathColors.size(); i++) {
                file << " " << shape->m_strokeDrawing->m_pathColors[i][0] << " "
                     << shape->m_strokeDrawing->m_pathColors[i][1] << " " << shape->m_strokeDrawing->m_pathColors[i][2]
                     << " " << shape->m_strokeDrawing->m_pathColors[i][3];
            }
            file << std::endl;
            file << "i";
            for (int i = 0; i < (int)shape->m_strokeDrawing->m_pathIndices.size(); i++)
                file << " " << (unsigned)(fillVertexCount + shape->m_strokeDrawing->m_pathIndices[i]);
            file << std::endl;
        }
        file << "F" << std::endl;
    }
    for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end(); ++it)
        writeWebNode(file, *it, matrix);
    if (node->m_display->m_clipping.value())
        file << "m" << std::endl;
}

// File menu "Export Web...": writes the design's meshes as a Picroma Plasma Web Graphics file on a GL thread; the
// sheet keeps its own path.
void Workspace::exportWeb()
{
    if (!getOpenDesignSheet())
        return;
    std::wstring path = saveFileBox(L"Export Web", 0, L"Picroma Plasma Web Graphics (*.pwg)\0*.pwg\0", 1);
    if (path.size() == 0)
        return;
    waitForColorWorker();
    DesignSheet *sheet = getOpenDesignSheet();
    std::wstring previousPath = sheet->m_path;
    sheet->setPath(path);
    sheet->setBusy(true, true);
    m_designSheet->m_thread = startGLThread([this, sheet, previousPath]() {
        std::ofstream file(PLASMA_FILE_PATH(sheet->m_path));
        plasma::Matrix<float> identity;
        identity.identity();
        for (std::list<plasma::Node *>::iterator it = sheet->m_root->m_children.begin();
             it != sheet->m_root->m_children.end(); ++it)
            writeWebNode(file, *it, identity);
        sheet->setPath(previousPath);
        sheet->setBusy(false, false);
        sheet->m_thread = 0;
    });
}
