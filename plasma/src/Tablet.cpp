#include "Tablet.h"
#include "main.h"

// The context maps the tablet onto a square of this many output units.
static const LONG OUTPUT_EXTENT = 10000;

// Update normalises the reported pressure by this fixed range, not by the device maximum Init records in
// m_pressureScale.
static const double PRESSURE_RANGE = 1024.0;

Tablet::Tablet()
    : m_present(false),
      m_context(0),
      m_window(0),
      m_unused10(0.0f),
      m_unused14(0.0f),
      m_pressure(1.0f),
      m_penDown(false)
{}

Tablet::~Tablet() {}

bool Tablet::init()
{
    HWND window = g_window;
    if (!WTInfoW(0, 0, NULL))
        return false;

    LOGCONTEXTW context;
    WTInfoW(WTI_DEFCONTEXT, 0, &context);
    context.lcOptions |= CXO_SYSTEM;
    context.lcPktData = PACKETDATA;
    context.lcMoveMask = PACKETDATA;
    context.lcMsgBase = WT_DEFBASE;
    context.lcPktMode = PACKETMODE;
    context.lcBtnUpMask = context.lcBtnDnMask;
    context.lcOutExtX = OUTPUT_EXTENT;
    context.lcOutExtY = OUTPUT_EXTENT;
    m_context = WTOpenW(window, &context, TRUE);
    if (!m_context)
        return false;

    AXIS pressure;
    WTInfoW(WTI_DEVICES + context.lcDevice, DVC_NPRESSURE, &pressure);
    m_window = window;
    m_present = true;
    m_pressureScale = 1.0f / pressure.axMax;
    m_outputScale = plasma::Vector<2, float>(1.0f / context.lcOutExtX, 1.0f / context.lcOutExtY);
    return true;
}

void Tablet::resetPressure()
{
    m_pressure = 1.0f;
}

float Tablet::update()
{
    if (!m_present)
        return 1.0f;

    PACKET packet;
    while (WTPacketsGet(m_context, 1, &packet)) {
        m_pressure = (float)packet.pkNormalPressure / PRESSURE_RANGE;
        if (HIWORD(packet.pkButtons) == TBN_UP) {
            m_penDown = false;
            m_pressure = 1.0f;
        }
        if (HIWORD(packet.pkButtons) == TBN_DOWN)
            m_penDown = true;
    }
    if (!m_penDown)
        m_pressure = 1.0f;
    return m_pressure;
}
