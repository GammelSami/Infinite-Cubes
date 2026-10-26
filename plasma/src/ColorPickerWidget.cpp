#include "ColorPickerWidget.h"
#include "ColorModule.h"
#include "ClipboardModule.h"
#include "Workspace.h"
#include "plasma/Node.h"
#include <GL/glew.h>

static const int DefaultPickerBarWidth = 15;
static const int AlphaCheckerCellSizePixels = 5;

ColorPickerWidget::ColorPickerWidget(plasma::Engine *engine, Workspace *workspace, plasma::Node *node)
    : plasma::Widget(engine, node, L"colorpickercontrol"),
      m_workspace(workspace),
      m_dragSaturationBrightness(false),
      m_dragHue(false),
      m_dragAlpha(false),
      m_hsv(240.0f, 100.0f, 100.0f, 1.0f),
      m_barWidth(DefaultPickerBarWidth)
{}

ColorPickerWidget::~ColorPickerWidget() {}

void ColorPickerWidget::colorChanged()
{
    m_changed = true;
}

void ColorPickerWidget::pointerReleased()
{
    m_engine->releasePointerCapture();
    m_dragSaturationBrightness = false;
    m_dragHue = false;
    m_dragAlpha = false;
}

plasma::Vector<4, float> ColorPickerWidget::getColor()
{
    return hsvToRgb(m_hsv);
}

bool ColorPickerWidget::contains(const plasma::Vector<2, float> &point)
{
    return point.m_elements[0] >= 0.0f && (double)point.m_elements[0] < getContentWidth() &&
           point.m_elements[1] >= 0.0f && (double)point.m_elements[1] < getContentHeight();
}

// Hue ramp: 101 vertices span 100 height intervals; the hue divisor 101 deliberately leaves the final hue above
// zero.
void ColorPickerWidget::draw()
{
    glDisable(GL_DEPTH_TEST);
    __glewUseProgram(0);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_INDEX_ARRAY);
    glPushAttrib(GL_SCISSOR_BIT);
    glBegin(GL_QUADS);
    int columns = (int)(getContentWidth() / (double)AlphaCheckerCellSizePixels);
    int rows = m_barWidth / AlphaCheckerCellSizePixels;
    for (int i = 0; i < columns; ++i) {
        for (int j = 0; j < rows; ++j) {
            double shade = 1.0f;
            if ((i + j) % 2)
                shade = 0.0f;
            glColor3f(shade, shade, shade);
            glVertex2f((float)(i * AlphaCheckerCellSizePixels),
                       getContentWidth() - (float)(j * AlphaCheckerCellSizePixels));
            glVertex2f((float)(i * AlphaCheckerCellSizePixels + AlphaCheckerCellSizePixels),
                       getContentWidth() - (float)(j * AlphaCheckerCellSizePixels));
            glVertex2f((float)(i * AlphaCheckerCellSizePixels + AlphaCheckerCellSizePixels),
                       getContentWidth() - (float)(j * AlphaCheckerCellSizePixels) -
                           (double)AlphaCheckerCellSizePixels);
            glVertex2f((float)(i * AlphaCheckerCellSizePixels), getContentWidth() -
                                                                    (float)(j * AlphaCheckerCellSizePixels) -
                                                                    (double)AlphaCheckerCellSizePixels);
        }
    }
    glEnd();
    glBegin(GL_TRIANGLE_STRIP);
    plasma::Vector<4, float> hsv(0.0f, 100.0f, 100.0f, 1.0f);
    for (int i = 0; i <= 100; ++i) {
        hsv.m_elements[0] = (float)(360 - i * 360 / 101);
        plasma::Vector<4, float> color = hsvToRgb(hsv);
        glColor3fv(color.m_elements);
        glVertex2f(getContentWidth() - (double)m_barWidth,
                   (getContentWidth() - (double)m_barWidth) * (float)i / (double)100.0f);
        glVertex2f(getContentWidth(), (getContentWidth() - (double)m_barWidth) * (float)i / (double)100.0f);
    }
    glEnd();
    hsv = plasma::Vector<4, float>(m_hsv.m_elements[0], 100.0f, 100.0f, 1.0f);
    plasma::Vector<4, float> color = hsvToRgb(hsv);
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glDisable(GL_BLEND);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, 0.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, getContentWidth() - (double)m_barWidth);
    glVertex2f(0.0f, getContentWidth() - (double)m_barWidth);
    glEnd();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    glBegin(GL_QUADS);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(0.0f, getContentWidth() - (double)m_barWidth);
    glVertex2f(0.0f, 0.0f);
    glColor3fv(color.m_elements);
    glVertex2f(getContentWidth() - (double)m_barWidth, 0.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, getContentWidth() - (double)m_barWidth);
    glEnd();
    color = hsvToRgb(m_hsv);
    __glewBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
    glBegin(GL_QUADS);
    glColor4f(color.m_elements[0], color.m_elements[1], color.m_elements[2], 0.0f);
    glVertex2f(0.0f, getContentWidth());
    glVertex2f(0.0f, getContentWidth() - (double)m_barWidth);
    glColor4f(color.m_elements[0], color.m_elements[1], color.m_elements[2], 1.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, getContentWidth() - (double)m_barWidth);
    glVertex2f(getContentWidth() - (double)m_barWidth, getContentWidth());
    glColor4fv(color.m_elements);
    glVertex2f(getContentWidth() - (double)m_barWidth, getContentWidth() - (double)m_barWidth);
    glVertex2f(getContentWidth(), getContentWidth() - (double)m_barWidth);
    glVertex2f(getContentWidth(), getContentWidth());
    glVertex2f(getContentWidth() - (double)m_barWidth, getContentWidth());
    glEnd();
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);
    plasma::Vector<2, float> crosshair(
        (getContentWidth() - (double)m_barWidth - (double)1.0f) * m_hsv.m_elements[1] / (double)100.0f + (double)1.0f,
        ((double)100.0f - m_hsv.m_elements[2]) * (getContentWidth() - (double)m_barWidth - (double)1.0f) /
                (double)100.0f +
            (double)1.0f);
    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_INVERT);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glLineWidth(1.0f);
    glColor3f(0.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);

    // The casts are unchecked for non-finite and out-of-range coordinates.
    glVertex2i((int)crosshair.m_elements[0], 0);
    glVertex2i((int)crosshair.m_elements[0], (int)(getContentWidth() - (double)m_barWidth));
    glVertex2i(0, (int)crosshair.m_elements[1]);
    glVertex2i((int)(getContentWidth() - (double)m_barWidth), (int)crosshair.m_elements[1]);
    glEnd();
    glDisable(GL_COLOR_LOGIC_OP);
    glPopAttrib();
    float huePosition = ((double)360.0f - m_hsv.m_elements[0]) *
                        (getContentWidth() - (double)m_barWidth - (double)2.0f) / (double)360.0f;
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, huePosition - (double)1.0f);
    glVertex2f(getContentWidth(), huePosition - (double)1.0f);
    glVertex2f(getContentWidth(), huePosition + (double)2.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, huePosition + (double)2.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, huePosition);
    glVertex2f(getContentWidth() - (double)1.0f, huePosition);
    glVertex2f(getContentWidth() - (double)1.0f, huePosition + (double)1.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, huePosition + (double)1.0f);
    glEnd();
    float alphaPosition = (getContentWidth() - (double)m_barWidth - (double)2.0f) * m_hsv.m_elements[3];
    glBegin(GL_QUADS);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(alphaPosition - (double)1.0f, getContentWidth() - (double)m_barWidth);
    glVertex2f(alphaPosition + (double)2.0f, getContentWidth() - (double)m_barWidth);
    glVertex2f(alphaPosition + (double)2.0f, getContentWidth());
    glVertex2f(alphaPosition - (double)1.0f, getContentWidth());
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex2f(alphaPosition, getContentWidth() - (double)m_barWidth);
    glVertex2f(alphaPosition + (double)1.0f, getContentWidth() - (double)m_barWidth);
    glVertex2f(alphaPosition + (double)1.0f, getContentWidth() - (double)1.0f);
    glVertex2f(alphaPosition, getContentWidth() - (double)1.0f);
    glEnd();
    glBegin(GL_LINES);
    glColor3f(0.0f, 0.0f, 0.0f);
    glVertex2f(0.0f, getContentWidth() - (double)m_barWidth);
    glVertex2f(getContentWidth(), getContentWidth() - (double)m_barWidth);
    glVertex2f(getContentWidth() - (double)m_barWidth, 0.0f);
    glVertex2f(getContentWidth() - (double)m_barWidth, getContentWidth());
    glEnd();
    glLineWidth(1.0f);
    glDisable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_LOOP);
    glColor3f(0.5f, 0.5f, 0.5f);
    glVertex2f(0.0f, 0.0f);
    glVertex2f(getContentWidth(), 0.0f);
    glVertex2f(getContentWidth(), getContentHeight());
    glVertex2f(0.0f, getContentHeight());
    glEnd();
    glPopAttrib();
}

void ColorPickerWidget::pointerPressed()
{
    m_engine->setPointerCapture(m_node);
    m_dragSaturationBrightness = false;
    m_dragHue = false;
    m_dragAlpha = false;
    plasma::Vector<2, float> pointer = m_engine->m_pointer;

    // Transforms the point with perspective division, adding m[7] * y before m[3] * x when computing w.
    const float *m = m_node->m_inverseMatrix.m_elements;
    float w = m[7] * pointer.m_elements[1] + m[3] * pointer.m_elements[0] + m[15];
    plasma::Vector<2, float> point =
        plasma::Vector<2, float>(m[4] * pointer.m_elements[1] + pointer.m_elements[0] * m[0] + m[12],
                                 m[1] * pointer.m_elements[0] + m[5] * pointer.m_elements[1] + m[13]) *
        (1 / w);
    if (point.m_elements[0] < getContentWidth() - (double)m_barWidth &&
        point.m_elements[1] < getContentWidth() - (double)m_barWidth) {
        m_dragSaturationBrightness = true;
        m_hsv.m_elements[1] = point.m_elements[0] / (getContentWidth() - (double)m_barWidth) * (double)100.0f;
        m_hsv.m_elements[2] =
            (double)100.0f - point.m_elements[1] / (getContentWidth() - (double)m_barWidth) * (double)100.0f;
    }
    if (point.m_elements[0] >= getContentWidth() - (double)m_barWidth &&
        point.m_elements[1] < getContentWidth() - (double)m_barWidth) {
        m_dragHue = true;
        m_hsv.m_elements[0] =
            (double)360.0f - point.m_elements[1] / (getContentWidth() - (double)m_barWidth) * (double)360.0f;
    }
    if (point.m_elements[1] >= getContentWidth() - (double)m_barWidth &&
        point.m_elements[0] < getContentWidth() - (double)m_barWidth) {
        m_dragAlpha = true;
        m_hsv.m_elements[3] = point.m_elements[0] / (getContentWidth() - (double)m_barWidth);
    }
    m_workspace->getColorModule()->setColor(getColor(), false, true, false);
    colorChanged();
}

void ColorPickerWidget::pointerMoved()
{
    if (m_engine->m_buttons & (1 << plasma::MOUSE_BUTTON_LEFT)) {
        plasma::Vector<2, float> pointer = m_engine->m_pointer;

        // Transforms the point with perspective division, adding m[7] * y before m[3] * x when computing w.
        const float *m = m_node->m_inverseMatrix.m_elements;
        float w = m[7] * pointer.m_elements[1] + m[3] * pointer.m_elements[0] + m[15];
        plasma::Vector<2, float> point =
            plasma::Vector<2, float>(m[4] * pointer.m_elements[1] + pointer.m_elements[0] * m[0] + m[12],
                                     m[1] * pointer.m_elements[0] + m[5] * pointer.m_elements[1] + m[13]) *
            (1 / w);
        if (m_dragSaturationBrightness) {
            m_hsv[1] = point.m_elements[0] / (getContentWidth() - (double)m_barWidth) * (double)100.0f;
            m_hsv[2] = (double)100.0f - point.m_elements[1] / (getContentWidth() - (double)m_barWidth) * (double)100.0f;
            if (m_hsv[1] < 0.0f)
                m_hsv[1] = 0.0f;
            if (m_hsv[1] > 100.0f)
                m_hsv[1] = 100.0f;
            if (m_hsv[2] < 0.0f)
                m_hsv[2] = 0.0f;
            if (m_hsv[2] > 100.0f)
                m_hsv[2] = 100.0f;
            m_workspace->getColorModule()->setColor(getColor(), false, true, false);
        }
        if (m_dragHue) {
            m_hsv[0] = (double)360.0f - point.m_elements[1] / (getContentWidth() - (double)m_barWidth) * (double)360.0f;
            if (m_hsv[0] < 0.0f)
                m_hsv[0] = 0.0f;
            if (m_hsv[0] > 360.0f)
                m_hsv[0] = 360.0f;
            m_workspace->getColorModule()->setColor(getColor(), false, true, false);
        }
        if (m_dragAlpha) {
            m_hsv[3] = point.m_elements[0] / (getContentWidth() - (double)m_barWidth);
            if (m_hsv[3] < 0.0f)
                m_hsv[3] = 0.0f;
            if (m_hsv[3] > 1.0f)
                m_hsv[3] = 1.0f;
            m_workspace->getColorModule()->setColor(getColor(), false, true, false);
        }
    }
    colorChanged();
}
