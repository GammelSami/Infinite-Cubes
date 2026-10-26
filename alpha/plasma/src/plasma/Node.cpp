// Node implements scene-tree drawing, hit testing, transforms and text variables.
#include <algorithm>
#include <cmath>
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Widget.h"
#include "plasma/RenderSurface.h"
#include "plasma/Attribute.h"
#include "plasma/TextShape.h"

namespace plasma {
// The cached and blurred drawings (anonymous: this file's own): a cache surface is SurfacePadding
// pixels larger than the drawing; a blur's radius is at most MaximumBlurRadius (larger ones are
// drawn downsampled), and a blurred drawing's surface is BlurMarginRadii radii larger.
namespace {
enum {
    SurfacePadding = 2
};
enum {
    MaximumBlurRadius = 8
};
enum {
    BlurMarginRadii = 4
};
}

Node::Node(Engine *engine, Transformation *transformation, Shape *shape, Display *display, const std::wstring &name)
    : NamedObject(engine, name),
      m_userData(0),
      m_parent(0),
      m_shape(shape),
      m_transformation(transformation),
      m_display(display),
      m_widget(0),
      m_enclosingWidget(0),
      m_flags(NODE_FLAGS_NONE),
      m_changed(false),
      m_cacheSurface(0)
{
    if (shape)
        shape->m_nodes.insert(this);
    if (transformation)
        transformation->m_nodes.insert(this);
    if (display)
        display->m_nodes.insert(this);
    m_engineEntry = engine->addNode(this);
    m_matrix.identity();
    m_inverseMatrix.identity();
}

Node::~Node()
{
    if (m_cacheSurface && !m_engine->isDestroying())
        m_engine->deleteRenderSurface(m_cacheSurface);
}

void Node::addChild(Node *child)
{
    if (child->m_parent)
        child->m_parent->removeChild(child);
    m_children.push_back(child);
    child->m_parent = this;
    child->m_matrix = m_matrix * child->m_transformation->m_matrix;
    child->m_inverseMatrix = child->m_transformation->m_inverseMatrix * m_inverseMatrix;
}

int Node::moveIntoWidget(const std::wstring &name, Widget *widget, bool fit)
{
    int count = 0;
    if (m_name == name) {
        addChild(widget->m_node);
        if (fit && m_widget)
            widget->setViewRect(Vector<2, float>(0.0f, 0.0f), m_widget->getViewSize(), true);
        count = 1;
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        count += (*it)->moveIntoWidget(name, widget, fit);
    return count;
}

Vector<2, float> Node::getMaximum(int flags)
{
    Vector<2, float> maximum(0.0f, 0.0f);
    const Matrix<float> &matrix = m_transformation->m_matrix;
    bool empty = true;
    if (m_shape) {
        Vector<2, float> low = m_shape->getMinimum();
        Vector<2, float> high = m_shape->getMaximum();
        if (m_enclosingWidget) {
            low = m_enclosingWidget->deform(low, this);
            high = m_enclosingWidget->deform(high, this);
        }
        if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
            float blur = m_display->m_blurRadius.value();
            low[0] -= blur;
            low[1] -= blur;
            high[0] += blur;
            high[1] += blur;
        }
        empty = false;
        maximum = matrix.transform(low);
        Vector<2, float> point = matrix.transform(Vector<2, float>(low[0], high[1]));
        if (point[0] > maximum[0])
            maximum[0] = point[0];
        if (point[1] > maximum[1])
            maximum[1] = point[1];
        point = matrix.transform(Vector<2, float>(high[0], high[1]));
        if (point[0] > maximum[0])
            maximum[0] = point[0];
        if (point[1] > maximum[1])
            maximum[1] = point[1];
        point = matrix.transform(Vector<2, float>(high[0], low[1]));
        if (point[0] > maximum[0])
            maximum[0] = point[0];
        if (point[1] > maximum[1])
            maximum[1] = point[1];
    } else if (m_widget && m_widget->hasBounds()) {
        empty = false;
        maximum = matrix.transform(m_widget->getMaximum());
    } else if (m_display->m_clipping.value() || m_children.empty()) {
        return matrix.transform(Vector<2, float>(0.0f, 0.0f));
    }
    if (!m_display->m_clipping.value() && !(flags & (1 << BOUNDS_NO_CHILDREN))) {
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
            Node *child = *it;
            if (!child->isShown())
                continue;
            if (!(flags & (1 << BOUNDS_INCLUDE_HIDDEN)) && !child->m_display->m_visibility.value())
                continue;
            if ((flags & (1 << BOUNDS_SKIP_INACTIVE)) && !child->isHitTestable())
                continue;
            if ((flags & (1 << BOUNDS_SKIP_WIDGETS)) && child->m_widget)
                continue;
            Vector<2, float> childMinimum = child->getMinimum(flags);
            Vector<2, float> childMaximum = (*it)->getMaximum(flags);
            if (childMinimum != childMaximum) {
                Vector<2, float> point = matrix.transform(Vector<2, float>(childMinimum[0], childMinimum[1]));
                if (empty) {
                    maximum = point;
                    empty = false;
                } else {
                    if (point[0] > maximum[0])
                        maximum[0] = point[0];
                    if (point[1] > maximum[1])
                        maximum[1] = point[1];
                }
                point = matrix.transform(Vector<2, float>(childMaximum[0], childMinimum[1]));
                if (point[0] > maximum[0])
                    maximum[0] = point[0];
                if (point[1] > maximum[1])
                    maximum[1] = point[1];
                point = matrix.transform(Vector<2, float>(childMaximum[0], childMaximum[1]));
                if (point[0] > maximum[0])
                    maximum[0] = point[0];
                if (point[1] > maximum[1])
                    maximum[1] = point[1];
                point = matrix.transform(Vector<2, float>(childMinimum[0], childMaximum[1]));
                if (point[0] > maximum[0])
                    maximum[0] = point[0];
                if (point[1] > maximum[1])
                    maximum[1] = point[1];
            }
        }
    }
    return maximum;
}

Vector<2, float> Node::getMinimum(int flags)
{
    Vector<2, float> minimum(0.0f, 0.0f);
    const Matrix<float> &matrix = m_transformation->m_matrix;
    bool empty = true;
    if (m_shape) {
        Vector<2, float> low = m_shape->getMinimum();
        Vector<2, float> high = m_shape->getMaximum();
        if (m_enclosingWidget) {
            low = m_enclosingWidget->deform(low, this);
            high = m_enclosingWidget->deform(high, this);
        }
        if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
            float blur = m_display->m_blurRadius.value();
            low[0] -= blur;
            low[1] -= blur;
            high[0] += blur;
            high[1] += blur;
        }
        empty = false;
        minimum = matrix.transform(low);
        Vector<2, float> point = matrix.transform(Vector<2, float>(low[0], high[1]));
        if (point[0] < minimum[0])
            minimum[0] = point[0];
        if (point[1] < minimum[1])
            minimum[1] = point[1];
        point = matrix.transform(Vector<2, float>(high[0], high[1]));
        if (point[0] < minimum[0])
            minimum[0] = point[0];
        if (point[1] < minimum[1])
            minimum[1] = point[1];
        point = matrix.transform(Vector<2, float>(high[0], low[1]));
        if (point[0] < minimum[0])
            minimum[0] = point[0];
        if (point[1] < minimum[1])
            minimum[1] = point[1];
    } else if (m_widget && m_widget->hasBounds()) {
        empty = false;
        minimum = matrix.transform(m_widget->getMinimum());
    } else if (m_display->m_clipping.value() || m_children.empty()) {
        return matrix.transform(Vector<2, float>(0.0f, 0.0f));
    }
    if (!m_display->m_clipping.value() && !(flags & (1 << BOUNDS_NO_CHILDREN))) {
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
            Node *child = *it;
            if (!child->isShown())
                continue;
            if (!(flags & (1 << BOUNDS_INCLUDE_HIDDEN)) && !child->m_display->m_visibility.value())
                continue;
            if ((flags & (1 << BOUNDS_SKIP_INACTIVE)) && !child->isHitTestable())
                continue;
            if ((flags & (1 << BOUNDS_SKIP_WIDGETS)) && child->m_widget)
                continue;
            Vector<2, float> childMinimum = child->getMinimum(flags);
            Vector<2, float> childMaximum = (*it)->getMaximum(flags);
            if (childMinimum != childMaximum) {
                Vector<2, float> point = matrix.transform(Vector<2, float>(childMinimum[0], childMinimum[1]));
                if (empty) {
                    minimum = point;
                    empty = false;
                } else {
                    if (point[0] < minimum[0])
                        minimum[0] = point[0];
                    if (point[1] < minimum[1])
                        minimum[1] = point[1];
                }
                point = matrix.transform(Vector<2, float>(childMaximum[0], childMinimum[1]));
                if (point[0] < minimum[0])
                    minimum[0] = point[0];
                if (point[1] < minimum[1])
                    minimum[1] = point[1];
                point = matrix.transform(Vector<2, float>(childMaximum[0], childMaximum[1]));
                if (point[0] < minimum[0])
                    minimum[0] = point[0];
                if (point[1] < minimum[1])
                    minimum[1] = point[1];
                point = matrix.transform(Vector<2, float>(childMinimum[0], childMaximum[1]));
                if (point[0] < minimum[0])
                    minimum[0] = point[0];
                if (point[1] < minimum[1])
                    minimum[1] = point[1];
            }
        }
    }
    return minimum;
}

bool Node::isAncestorOf(const Node *node) const
{
    if (!node)
        return false;
    do {
        if (this == node)
            return true;
        node = node->m_parent;
    } while (node);
    return false;
}

Node *Node::clone(Node *parent)
{
    Shape *shape = m_shape ? m_shape->clone() : 0;
    Transformation *transformation = m_transformation ? m_transformation->clone() : 0;
    Display *display = m_display ? m_display->clone() : 0;
    Node *copy = m_engine->createNode(transformation, shape, display, parent, m_name);
    if (m_widget) {
        m_widget->clone(copy);
        copy->m_enclosingWidget = copy->m_widget;
    } else if (parent && parent->m_enclosingWidget) {
        copy->m_enclosingWidget = parent->m_enclosingWidget;
    }
    if (copy->m_enclosingWidget && copy->m_shape)
        copy->m_shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isShown())
            (*it)->clone(copy);
    }
    copy->m_flags = m_flags;
    copy->m_variableKeys = m_variableKeys;
    copy->m_variables = m_variables;
    return copy;
}

void Node::destroyChildren()
{
    std::list<Node *> children = m_children; // the engine detaches them
    for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
        m_engine->deleteNode(*it);
}

// Renders the subtree and returns the number of shapes drawn.
int Node::draw(int mode)
{
    if (!m_display->m_visibility.value() || !isShown())
        return 0;
    m_engine->pushNodeMatrix(this);
    int rendered = 0;
    RenderSurface *surface = 0;
    bool widgetSurface = false;
    if (m_engine->usesWidgetSurfaces() && m_widget && m_widget->rendersToSurface()) {
        m_widget->updateSurface();
        widgetSurface = true;
        surface = m_widget->m_surface;
    }
    if (!surface && (m_flags & (1 << NODE_CACHED))) {
        if (!m_cacheSurface) {
            m_cacheSurface = m_engine->createRenderSurface(false);
            m_changed = true;
        }
        surface = m_cacheSurface;
    }
    bool redraw = surface && (m_changed || (m_widget && m_widget->m_changed));
    bool antialiasing = m_engine->m_renderFlags & (1 << RENDER_ANTIALIAS);
    // Surface position and size are rounded to whole pixels, with integer surface offsets and floating window offsets applied afterwards.
    Vector<2, float> windowOffset(0.0f, 0.0f);
    Vector<2, float> textureOffset(0.0f, 0.0f);
    if (redraw) {
        if (widgetSurface) {
            if (m_widget->m_surfaceScale < 1.0f)
                m_engine->m_renderFlags &= ~(1 << RENDER_ANTIALIAS);
            Vector<2, int> offset(textureOffset);
            Vector<2, float> screenSize = m_widget->screenSize() + Vector<2, float>(0.5f, 0.5f);
            Vector<2, float> size(Vector<2, int>(screenSize) + offset);
            Vector<2, float> screenPosition = m_widget->getContentPosition() + Vector<2, float>(0.5f, 0.5f);
            Vector<2, float> position = Vector<2, float>(Vector<2, int>(screenPosition)) + windowOffset;
            m_engine->beginSurface(surface, position, size, 1.0f, false);
            Matrix<float> matrix;
            m_engine->getModelViewMatrix(matrix);
            matrix.scale(m_widget->m_surfaceScale, m_widget->m_surfaceScale);
            matrix.translate(m_widget->m_surfaceOffset);
            m_engine->loadMatrix(matrix);
        } else {
            Vector<2, float> minimum;
            Vector<2, float> maximum;
            getScreenBounds(minimum, maximum);
            float height = maximum[1] - minimum[1];
            float width = maximum[0] - minimum[0];
            surface->resize((int)(width + 0.5f) + SurfacePadding, (int)(height + 0.5f) + SurfacePadding);
            Vector<2, int> offset(textureOffset);
            Vector<2, int> origin(windowOffset);
            Vector<2, float> roundedExtent(width, height);
            roundedExtent[0] += 0.5f;
            roundedExtent[1] += 0.5f;
            Vector<2, float> size(Vector<2, int>(roundedExtent) + offset);
            Vector<2, float> roundedMinimum = minimum;
            roundedMinimum[0] += 0.5f;
            roundedMinimum[1] += 0.5f;
            Vector<2, float> position(Vector<2, int>(roundedMinimum) + origin);
            m_engine->beginSurface(surface, position, size, 1.0f, false);
        }
        surface->clear();
    }
    Matrix<float> modelView;
    m_engine->getModelViewMatrix(modelView);
    bool clip =
        m_display->m_clipping.value() && !(mode & ((1 << NODE_RENDER_STOP_AT_EFFECT) | (1 << NODE_RENDER_NO_CHILDREN)));
    bool blur = m_engine->usesBlur() && (m_display->m_flags & (1 << DISPLAY_BLUR)) &&
                m_display->m_blurRadius.value() > 0.0f && !(mode & (1 << NODE_RENDER_SKIP_BLUR));
    if (m_shape)
        m_shape->updateBounds(modelView);
    float blurRadius = 0.0f;
    float blurScale = 1.0f;
    // Clamp each blur pass to MaximumBlurRadius; larger blurs downsample by blurScale.
    float passRadius = 1.0f;
    if (blur) {
        // In pixels: the display's radius times the mean scale of the two axes, over 1.4.
        blurRadius = (std::sqrt(modelView.m_elements[5] * modelView.m_elements[5] +
                                modelView.m_elements[1] * modelView.m_elements[1]) +
                      std::sqrt(modelView.m_elements[4] * modelView.m_elements[4] +
                                modelView.m_elements[0] * modelView.m_elements[0])) *
                     0.5f * m_display->m_blurRadius.value() / 1.4f;
        passRadius = blurRadius;
        if (blurRadius > MaximumBlurRadius) {
            blurScale = MaximumBlurRadius / blurRadius;
            passRadius = MaximumBlurRadius;
        }
    }
    bool visible = !(mode & (1 << NODE_RENDER_CULL_SCREEN)) || isOnScreen();
    if (redraw || !surface) {
        if (visible || (blur && !clip)) {
            Vector<2, float> minimum;
            Vector<2, float> maximum;
            if (blur) {
                getScreenBounds(minimum, maximum);
                minimum[0] -= blurRadius;
                minimum[1] -= blurRadius;
                maximum[0] += blurRadius;
                maximum[1] += blurRadius;
            }
            RenderSurface *surfaces[2];
            if (!clip && !blur) {
                m_engine->m_filterSurface = 0;
            } else {
                // Clipping or blurred: the subtree is drawn off screen first.
                m_engine->beginOffscreenDrawing(blurScale);
                if (blur) {
                    RenderSurface *target = m_engine->m_renderSurfaces[m_engine->m_surfaceDepth];
                    Vector<2, float> size(maximum[0] - minimum[0] + SurfacePadding + blurRadius * BlurMarginRadii,
                                          maximum[1] - minimum[1] + SurfacePadding + blurRadius * BlurMarginRadii);
                    Vector<2, float> position(minimum[0] - (blurRadius + blurRadius),
                                              minimum[1] - (blurRadius + blurRadius));
                    target->clearRect(position, size, true);
                    if (clip) {
                        m_engine->beginOffscreenDrawing(1.0f);
                        m_engine->clearSurfaceBounds(this, 1.0f);
                    } else if (visible) {
                        if (m_shape)
                            m_shape->draw(this);
                        rendered = 1;
                    }
                } else {
                    m_engine->clearSurfaceBounds(this, blurScale);
                }
                for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
                    rendered += (*it)->draw(mode & ~(1 << NODE_RENDER_SKIP_BLUR));
                if (blur) {
                    if (clip) {
                        m_engine->endOffscreenDrawing();
                        if (m_shape)
                            m_shape->draw(this);
                        ++rendered;
                    }
                    Engine *engine = m_engine;
                    surfaces[0] = engine->m_renderSurfaces[engine->m_surfaceDepth];
                    if (!engine->m_blurSurface) {
                        engine->m_blurSurface = engine->createRenderSurface(false);
                        engine->m_blurSurface->resize(engine->m_viewportWidth, engine->m_viewportHeight);
                    }
                    surfaces[1] = engine->m_blurSurface;
                    RenderSurface *current = m_engine->m_renderSurfaces[m_engine->m_surfaceDepth];
                    if (current)
                        current->end();
                    surfaces[1]->begin();
                    Vector<2, float> size(maximum[0] - minimum[0] + SurfacePadding + blurRadius * BlurMarginRadii,
                                          maximum[1] - minimum[1] + SurfacePadding + blurRadius * BlurMarginRadii);
                    Vector<2, float> position(minimum[0] - (blurRadius + blurRadius),
                                              minimum[1] - (blurRadius + blurRadius));
                    surfaces[1]->clearRect(position, size, true);
                    surfaces[1]->end();
                    // Two passes of a horizontal and a vertical blur between the two surfaces.
                    for (int pass = 2; pass > 0; --pass) {
                        for (int axis = 0; axis < 2; ++axis) {
                            RenderSurface *target = surfaces[(axis + 1) % 2];
                            target->begin();
                            Vector<2, float> extent(maximum[0] - minimum[0], maximum[1] - minimum[1]);
                            surfaces[axis]->drawBlurred(minimum, extent, passRadius / pass, axis == 0, blurScale);
                            target->end();
                        }
                    }
                    if (current)
                        current->begin();
                }
                m_engine->endOffscreenDrawing();
            }
            if (blur) {
                Vector<2, float> size(maximum[0] - minimum[0] + SurfacePadding,
                                      maximum[1] - minimum[1] + SurfacePadding);
                surfaces[0]->draw(minimum, size, minimum, blurScale);
            } else {
                if (m_shape)
                    m_shape->draw(this);
                ++rendered;
            }
        }
        if (m_widget)
            m_widget->draw();
        if (!clip && !blur && !(mode & (1 << NODE_RENDER_NO_CHILDREN))) {
            for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
                Node *child = *it;
                if ((mode & (1 << NODE_RENDER_STOP_AT_EFFECT)) && child->isShown() &&
                    child->m_display->m_visibility.value() &&
                    (child->m_display->m_clipping.value() || ((child->m_display->m_flags & (1 << DISPLAY_BLUR)) &&
                                                              child->m_display->m_blurRadius.value() > 0.0f)))
                    break;
                rendered += child->draw(mode & ~(1 << NODE_RENDER_SKIP_BLUR));
            }
        }
        if (redraw) {
            if (m_widget)
                m_widget->surfaceRendered();
            m_engine->endSurface();
        }
    }
    if (surface) {
        // Composite the surface into the window.
        if (widgetSurface) {
            Vector<2, int> offset(textureOffset);
            Vector<2, float> screenSize = m_widget->screenSize() + Vector<2, float>(0.5f, 0.5f);
            Vector<2, float> size(Vector<2, int>(screenSize) + offset);
            Vector<2, int> origin(windowOffset);
            Vector<2, float> screenPosition = m_widget->getContentPosition() + Vector<2, float>(0.5f, 0.5f);
            Vector<2, float> position(Vector<2, int>(screenPosition) + origin);
            Widget *widget = m_widget;
            Vector<2, float> texturePosition(
                widget->m_surfaceOffset[0] * widget->m_surfaceScale / (float)widget->m_surfaceSamples,
                widget->m_surfaceOffset[1] * widget->m_surfaceScale / (float)widget->m_surfaceSamples);
            surface->drawDownsampled(position, size, texturePosition,
                                     widget->m_surfaceScale / (float)widget->m_surfaceSamples,
                                     widget->m_surfaceSamples);
            if (antialiasing)
                m_engine->m_renderFlags |= (1 << RENDER_ANTIALIAS);
            else
                m_engine->m_renderFlags &= ~(1 << RENDER_ANTIALIAS);
        } else {
            Vector<2, float> minimum;
            Vector<2, float> maximum;
            getScreenBounds(minimum, maximum);
            // The surface is drawn at the rounded minimum.
            Vector<2, int> offset(textureOffset);
            Vector<2, int> origin(windowOffset);
            Vector<2, float> roundedMinimum = minimum;
            roundedMinimum[0] += 0.5f;
            roundedMinimum[1] += 0.5f;
            Vector<2, float> position(Vector<2, int>(roundedMinimum) + origin);
            Vector<2, float> extent = maximum - minimum;
            extent[0] += 0.5f;
            extent[1] += 0.5f;
            Vector<2, float> size(Vector<2, int>(extent) + offset);
            surface->draw(position, size, Vector<2, float>(0.0f, 0.0f), -1.0f);
        }
    }
    m_engine->popNodeMatrix(this);
    return rendered;
}

Node *Node::findNode(const std::wstring &name)
{
    if (m_name == name)
        return this;
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if (Node *node = (*it)->findNode(name))
            return node;
    }
    return 0;
}

void Node::findNodes(std::list<Node *> &nodes, const std::wstring &name)
{
    if (m_name == name)
        nodes.push_back(this);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->findNodes(nodes, name);
}

bool Node::extendBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix, bool &empty,
                        bool withBlur)
{
    Vector<2, float> low;
    Vector<2, float> high;
    bool subtreeBoundsEmpty = true;
    if (m_shape) {
        // Shapes that bound themselves through the matrix return true.
        if (!m_shape->getTransformedBounds(low, high, matrix, subtreeBoundsEmpty)) {
            m_shape->updateBounds(matrix);
            const Vector<2, float> shapeMinimum = m_shape->getMinimum();
            const Vector<2, float> shapeMaximum = m_shape->getMaximum();
            Vector<2, float> corners[4] = {matrix.transform(Vector<2, float>(shapeMinimum[0], shapeMinimum[1])),
                                           matrix.transform(Vector<2, float>(shapeMaximum[0], shapeMinimum[1])),
                                           matrix.transform(Vector<2, float>(shapeMaximum[0], shapeMaximum[1])),
                                           matrix.transform(Vector<2, float>(shapeMinimum[0], shapeMaximum[1]))};
            low = corners[0];
            high = corners[0];
            for (int i = 0; i < 4; i++) {
                low[0] = (std::min)(low[0], corners[i][0]);
                low[1] = (std::min)(low[1], corners[i][1]);
                high[0] = (std::max)(high[0], corners[i][0]);
                high[1] = (std::max)(high[1], corners[i][1]);
            }
        }
        subtreeBoundsEmpty = false;
    }
    if (!m_display->m_clipping.value()) {
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); it++) {
            if ((*it)->isShown() && (*it)->m_display->m_visibility.value()) {
                const Matrix<float> &transformation = (*it)->m_transformation->m_matrix;
                Matrix<float> childMatrix = matrix;
                childMatrix *= transformation;
                (*it)->extendBounds(low, high, childMatrix, subtreeBoundsEmpty, withBlur);
            }
        }
    }
    // Display flag bit 0: the blur is enabled; the radius is scaled by the mean length of the matrix's
    // axes.
    if (withBlur && (m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
        Vector<2, float> xAxis = matrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
        Vector<2, float> yAxis = matrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
        float blur = (std::sqrt(xAxis[0] * xAxis[0] + xAxis[1] * xAxis[1]) +
                      std::sqrt(yAxis[0] * yAxis[0] + yAxis[1] * yAxis[1])) *
                     (m_display->m_blurRadius.value() * 0.5f);
        low[0] -= blur;
        low[1] -= blur;
        high[0] += blur;
        high[1] += blur;
    }
    if (!subtreeBoundsEmpty) {
        if (empty) {
            minimum = low;
            maximum = high;
            empty = false;
        } else {
            minimum[0] = (std::min)(low[0], minimum[0]);
            minimum[1] = (std::min)(low[1], minimum[1]);
            maximum[0] = (std::max)(high[0], maximum[0]);
            maximum[1] = (std::max)(high[1], maximum[1]);
        }
    }
    return !subtreeBoundsEmpty;
}

// The text of the first text shape in the subtree.
std::wstring Node::getText()
{
    if (m_shape && m_shape->getType() == SHAPE_TEXT)
        return static_cast<TextShape *>(m_shape)->m_string.value();
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        std::wstring text = (*it)->getText();
        if (text.size())
            return text;
    }
    return L"";
}

bool Node::getScreenBounds(Vector<2, float> &minimum, Vector<2, float> &maximum)
{
    Matrix<float> matrix;
    m_engine->getModelViewMatrix(matrix);
    if (!getBounds(minimum, maximum, matrix))
        return false;
    Vector<2, float> corners[4] = {matrix.transform(Vector<2, float>(minimum[0], minimum[1])),
                                   matrix.transform(Vector<2, float>(maximum[0], minimum[1])),
                                   matrix.transform(Vector<2, float>(maximum[0], maximum[1])),
                                   matrix.transform(Vector<2, float>(minimum[0], maximum[1]))};
    minimum = corners[0];
    maximum = minimum;
    for (int i = 1; i < 4; i++) {
        minimum[0] = (std::min)(minimum[0], corners[i][0]);
        minimum[1] = (std::min)(minimum[1], corners[i][1]);
        maximum[0] = (std::max)(maximum[0], corners[i][0]);
        maximum[1] = (std::max)(maximum[1], corners[i][1]);
    }
    return true;
}

bool Node::getBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix)
{
    float blur = 0.0f;
    bool empty = true;
    // Display flag bit 0: the blur is enabled; the radius is scaled by the mean length of the matrix's
    // axes.
    if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
        Matrix<float> axes = matrix;
        Vector<2, float> xAxis = axes.transformDirection(Vector<2, float>(1.0f, 0.0f));
        Vector<2, float> yAxis = axes.transformDirection(Vector<2, float>(0.0f, 1.0f));
        blur = (std::sqrt(xAxis[0] * xAxis[0] + xAxis[1] * xAxis[1]) +
                std::sqrt(yAxis[1] * yAxis[1] + yAxis[0] * yAxis[0])) *
               (m_display->m_blurRadius.value() * 0.5f);
        // Applied to the bounds before they are computed and taken back out at the end, .
        minimum += Vector<2, float>(blur, blur);
        maximum = maximum - Vector<2, float>(blur, blur);
    }
    if (m_shape) {
        m_shape->updateBounds(matrix);
        Vector<2, float> low = m_shape->getMinimum();
        Vector<2, float> high = m_shape->getMaximum();
        if (!(low != high))
            return false;
        if (m_enclosingWidget) {
            low = m_enclosingWidget->deform(low, this);
            high = m_enclosingWidget->deform(high, this);
        }
        minimum = low;
        maximum = high;
        empty = false;
    } else if (m_widget && m_widget->hasBounds()) {
        minimum = m_widget->getMinimum();
        maximum = m_widget->getMaximum();
        empty = false;
    } else if (m_display->m_clipping.value() || m_children.empty()) {
        minimum = Vector<2, float>(0.0f, 0.0f);
        maximum = Vector<2, float>(0.0f, 0.0f);
        return false;
    }
    if (!m_display->m_clipping.value()) {
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
            if (!(*it)->isShown() || !(*it)->m_display->m_visibility.value())
                continue;
            const Matrix<float> &childTransform = (*it)->m_transformation->m_matrix;
            Matrix<float> childMatrix = matrix;
            childMatrix *= childTransform;
            Vector<2, float> childMinimum;
            Vector<2, float> childMaximum;
            if (!(*it)->getBounds(childMinimum, childMaximum, childMatrix))
                continue;
            Vector<2, float> point = childTransform.transform(Vector<2, float>(childMinimum[0], childMinimum[1]));
            if (empty) {
                minimum = point;
                maximum = point;
                empty = false;
            } else {
                if (minimum[0] > point[0])
                    minimum[0] = point[0];
                if (minimum[1] > point[1])
                    minimum[1] = point[1];
                if (point[0] > maximum[0])
                    maximum[0] = point[0];
                if (point[1] > maximum[1])
                    maximum[1] = point[1];
            }
            point = childTransform.transform(Vector<2, float>(childMaximum[0], childMinimum[1]));
            if (minimum[0] > point[0])
                minimum[0] = point[0];
            if (minimum[1] > point[1])
                minimum[1] = point[1];
            if (point[0] > maximum[0])
                maximum[0] = point[0];
            if (point[1] > maximum[1])
                maximum[1] = point[1];
            point = childTransform.transform(Vector<2, float>(childMaximum[0], childMaximum[1]));
            if (minimum[0] > point[0])
                minimum[0] = point[0];
            if (minimum[1] > point[1])
                minimum[1] = point[1];
            if (point[0] > maximum[0])
                maximum[0] = point[0];
            if (point[1] > maximum[1])
                maximum[1] = point[1];
            point = childTransform.transform(Vector<2, float>(childMinimum[0], childMaximum[1]));
            if (minimum[0] > point[0])
                minimum[0] = point[0];
            if (minimum[1] > point[1])
                minimum[1] = point[1];
            if (point[0] > maximum[0])
                maximum[0] = point[0];
            if (point[1] > maximum[1])
                maximum[1] = point[1];
        }
    }
    if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
        minimum = minimum - Vector<2, float>(blur, blur);
        maximum += Vector<2, float>(blur, blur);
    }
    return !empty;
}

// The text of the node named name: of its own text shape, or with recursive of the first text
// shape below it.
std::wstring Node::getText(const std::wstring &name, bool recursive)
{
    if (m_name == name) {
        if (recursive)
            return getText();
        if (m_shape && m_shape->getType() == SHAPE_TEXT)
            return static_cast<TextShape *>(m_shape)->m_string.value();
        return L"";
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        std::wstring text = (*it)->getText(name, recursive);
        if (text.size())
            return text;
    }
    return L"";
}

void Node::initializeWidgets()
{
    if (isShown()) {
        if (m_widget) {
            m_widget->initialize();
            m_enclosingWidget = m_widget;
        } else if (m_parent) {
            m_enclosingWidget = m_parent->m_enclosingWidget;
        }
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->initializeWidgets();
    }
}

void Node::insertChild(Node *child, std::list<Node *>::iterator where)
{
    if (child->m_parent)
        child->m_parent->removeChild(child);
    m_children.insert(where, child);
    child->m_parent = this;
    child->m_matrix = m_matrix * child->m_transformation->m_matrix;
    child->m_inverseMatrix = child->m_transformation->m_inverseMatrix * m_inverseMatrix;
}

bool Node::insertChild(Node *child, Node *before)
{
    if (!before) {
        addChild(child);
        return true;
    }
    if (child->m_parent)
        child->m_parent->removeChild(child);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if (*it == before) {
            insertChild(child, it);
            return true;
        }
    }
    return false;
}

Node *Node::instance(Node *parent)
{
    Node *copy = m_engine->createNode(m_transformation, m_shape, m_display, parent, m_name);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isShown())
            (*it)->instance(copy);
    }
    copy->m_flags = m_flags;
    copy->m_variableKeys = m_variableKeys;
    copy->m_variables = m_variables;
    if (m_widget)
        m_widget->clone(copy);
    return copy;
}

bool Node::isOnScreen()
{
    if (!m_shape)
        return true;
    Vector<2, float> low = m_shape->getMinimum();
    Vector<2, float> high = m_shape->getMaximum();
    if (m_enclosingWidget) {
        low = m_enclosingWidget->deform(low, this);
        high = m_enclosingWidget->deform(high, this);
    }
    if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->getBlurRadius() > 0.0f) {
        float blur = m_display->getBlurRadius();
        low[0] -= blur;
        low[1] -= blur;
        high[0] += blur;
        high[1] += blur;
    }
    Vector<2, float> minimum = m_matrix.transform(low);
    Vector<2, float> maximum = m_matrix.transform(high);
    if (minimum[0] > maximum[0])
        std::swap(minimum[0], maximum[0]);
    if (minimum[1] > maximum[1])
        std::swap(minimum[1], maximum[1]);
    Vector<2, float> point = m_matrix.transform(Vector<2, float>(low[0], high[1]));
    if (point[0] < minimum[0])
        minimum[0] = point[0];
    if (point[1] < minimum[1])
        minimum[1] = point[1];
    if (point[0] > maximum[0])
        maximum[0] = point[0];
    if (point[1] > maximum[1])
        maximum[1] = point[1];
    point = m_matrix.transform(Vector<2, float>(high[0], low[1]));
    if (point[0] < minimum[0])
        minimum[0] = point[0];
    if (point[1] < minimum[1])
        minimum[1] = point[1];
    if (point[0] > maximum[0])
        maximum[0] = point[0];
    if (point[1] > maximum[1])
        maximum[1] = point[1];
    return maximum[0] >= 0.0f && minimum[0] < m_engine->m_viewportWidth && maximum[1] >= 0.0f &&
           minimum[1] < m_engine->m_viewportHeight;
}

bool Node::isPlaying()
{
    if (m_shape && m_shape->m_playing)
        return true;
    if (m_transformation && m_transformation->m_playing)
        return true;
    if (m_display && m_display->m_playing)
        return true;
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isShown() && (*it)->isPlaying())
            return true;
    }
    return false;
}

Node *Node::hitTest(const Vector<2, float> &position, int flags)
{
    if (!m_display->m_visibility.value() || !isShown() || ignoresPointer())
        return 0;
    if (m_widget && m_widget->m_surface && m_engine->usesWidgetSurfaces() && !m_widget->isAlwaysHit()) {
        Vector<2, float> origin = m_widget->getContentPosition();
        Vector<2, float> size = m_widget->screenSize();
        if (position.m_elements[0] < origin.m_elements[0] || position.m_elements[1] < origin.m_elements[1] ||
            position.m_elements[0] > (float)(origin.m_elements[0] + size.m_elements[0]) ||
            position.m_elements[1] > (float)(origin.m_elements[1] + size.m_elements[1]))
            return 0;
    }
    Vector<2, float> local = Widget::transformPoint(m_inverseMatrix, position);
    Vector<2, float> deformed = local;
    Shape *shape;
    if (m_enclosingWidget && (shape = m_shape) && shape->isDeformable())
        deformed = m_enclosingWidget->undeform(deformed, this);
    bool contains = (shape = m_shape) && shape->contains(deformed) ||
                    m_widget && (m_widget->contains(local) || m_widget->isAlwaysHit());
    if (m_display->m_clipping.value() && !contains)
        return 0;
    for (std::list<Node *>::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it) {
        if (Node *node = (*it)->hitTest(position, flags))
            return node;
    }
    if ((flags & (1 << HIT_SKIP_INTERNAL)) && isInternal() ||
        (flags & (1 << HIT_SKIP_LOCKED)) && (m_flags & (1 << NODE_LOCKED)) ||
        !(flags & (1 << HIT_INCLUDE_INACTIVE)) && !isHitTestable())
        return 0;
    return contains ? this : 0;
}

void Node::play(const std::wstring &sequenceName, int loopLength)
{
    if (m_shape)
        m_shape->play(sequenceName, loopLength);
    if (m_transformation)
        m_transformation->play(sequenceName, loopLength);
    if (m_display)
        m_display->play(sequenceName, loopLength);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->play(sequenceName, loopLength);
}

void Node::removeChild(Node *child)
{
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if (*it == child) {
            m_children.remove(child);
            child->m_parent = 0;
            return;
        }
    }
}

void Node::setDisplay(Display *display)
{
    if (m_display != display) {
        if (m_display) {
            if (m_display->countNodes() == 1)
                m_engine->deleteDisplay(m_display);
            else
                m_display->m_nodes.erase(this);
        }
        if (display)
            display->m_nodes.insert(this);
        m_display = display;
    }
}

void Node::setParent(Node *parent)
{
    if (m_parent)
        m_parent->removeChild(this);
    if (parent) {
        removeChild(parent);
        parent->removeChild(this);
        parent->addChild(this);
    }
}

void Node::setShape(Shape *shape)
{
    if (m_shape != shape) {
        if (m_shape) {
            if (m_shape->countNodes() == 1)
                m_engine->deleteShape(m_shape);
            else
                m_shape->m_nodes.erase(this);
        }
        if (shape)
            shape->m_nodes.insert(this);
        m_shape = shape;
    }
}

// Sets the text of the node named name: its own text shape's, or with recursive every text shape
// below it.
void Node::setText(const std::wstring &name, const std::wstring &text, bool recursive)
{
    if (isShown()) {
        if (m_name == name) {
            if (recursive) {
                setText(text);
            } else if (m_shape && m_shape->getType() == SHAPE_TEXT) {
                TextShape *shape = static_cast<TextShape *>(m_shape);
                if (shape->m_string.value() != text) {
                    shape->m_string = text;
                    shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
                }
            }
        } else {
            for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
                (*it)->setText(name, text, recursive);
        }
    }
}

// Sets the text of every text shape in the subtree.
void Node::setText(const std::wstring &text)
{
    if (isShown()) {
        if (m_shape && m_shape->getType() == SHAPE_TEXT) {
            TextShape *shape = static_cast<TextShape *>(m_shape);
            if (shape->m_string.value() != text) {
                shape->m_string.value() = text;
                shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            }
        }
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setText(text);
    }
}

void Node::setTransformation(Transformation *transformation)
{
    if (m_transformation != transformation) {
        if (m_transformation) {
            if (m_transformation->countNodes() == 1)
                m_engine->deleteTransformation(m_transformation);
            else
                m_transformation->m_nodes.erase(this);
        }
        if (transformation)
            transformation->m_nodes.insert(this);
        m_transformation = transformation;
    }
}

void Node::resetStepped()
{
    if (isShown()) {
        if (m_shape)
            m_shape->m_stepped = false;
        if (m_transformation)
            m_transformation->m_stepped = false;
        if (m_display)
            m_display->m_stepped = false;
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->resetStepped();
    }
}

void Node::resetChanged()
{
    if (isShown() && m_changed) {
        if (m_shape)
            m_shape->m_changed = false;
        if (m_transformation)
            m_transformation->m_changed = false;
        if (m_display)
            m_display->m_changed = false;
        if (m_widget)
            m_widget->m_changed = false;
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->resetChanged();
    }
}

void Node::setFrame(int frame)
{
    resetStepped();
    setFrame(frame, false, false);
}

void Node::setFrame(int frame, bool matricesChanged, bool force)
{
    if (m_flags & (1 << NODE_ALWAYS_ANIMATE))
        force = true;
    if (isShown() && (m_display->m_visibility.value() || force)) {
        if (m_widget)
            m_enclosingWidget = m_widget;
        else if (m_parent)
            m_enclosingWidget = m_parent->m_enclosingWidget;
        else
            m_enclosingWidget = 0;
        if (m_shape && !m_shape->m_stepped)
            m_shape->setFrame(frame);
        if (m_transformation && !m_transformation->m_stepped)
            m_transformation->setFrame(frame);
        if (m_display && !m_display->m_stepped)
            m_display->setFrame(frame);
        updateMatrices();
        if (m_widget)
            m_widget->update();
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setFrame(frame, matricesChanged, force);
    }
}

void Node::setVariable(const std::string &key, const std::string &value)
{
    for (std::list<std::string>::iterator it = m_variableKeys.begin(); it != m_variableKeys.end(); ++it) {
        if (*it == key) {
            m_variables[key] = value;
            return;
        }
    }
    m_variableKeys.push_back(key);
    m_variables[key] = value;
}

void Node::setWidget(Widget *widget)
{
    if (widget)
        widget->setNode(this);
    else
        m_widget = 0;
}

void Node::stop()
{
    if (m_shape)
        m_shape->stop();
    if (m_transformation)
        m_transformation->stop();
    if (m_display)
        m_display->stop();
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isShown())
            (*it)->stop();
    }
}

int Node::getSequenceEnd(const std::wstring &sequenceName)
{
    if (!isShown())
        return 0;
    int end = 0;
    if (m_shape) {
        for (std::list<Attribute *>::iterator it = m_shape->m_attributes.begin(); it != m_shape->m_attributes.end();
             ++it) {
            Sequence *sequence = (*it)->findSequence(sequenceName);
            if (sequence && sequence->count() > 0) {
                int frame = (*sequence)[sequence->count() - 1].frame;
                if (frame > end)
                    end = frame;
            }
        }
    }
    if (m_transformation) {
        for (std::list<Attribute *>::iterator it = m_transformation->m_attributes.begin();
             it != m_transformation->m_attributes.end(); ++it) {
            Sequence *sequence = (*it)->findSequence(sequenceName);
            if (sequence && sequence->count() > 0) {
                int frame = (*sequence)[sequence->count() - 1].frame;
                if (frame > end)
                    end = frame;
            }
        }
    }
    if (m_display) {
        for (std::list<Attribute *>::iterator it = m_display->m_attributes.begin(); it != m_display->m_attributes.end();
             ++it) {
            Sequence *sequence = (*it)->findSequence(sequenceName);
            if (sequence && sequence->count() > 0) {
                int frame = (*sequence)[sequence->count() - 1].frame;
                if (frame > end)
                    end = frame;
            }
        }
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        int childEnd = (*it)->getSequenceEnd(sequenceName);
        if (childEnd > end)
            end = childEnd;
    }
    return end;
}

void Node::makeUnique()
{
    if (m_shape && m_shape->countNodes() > 1)
        setShape(m_shape->clone());
    if (m_transformation && m_transformation->countNodes() > 1)
        setTransformation(m_transformation->clone());
    if (m_display && m_display->countNodes() > 1)
        setDisplay(m_display->clone());
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isShown() && !(*it)->isInternal())
            (*it)->makeUnique();
    }
}

void Node::updateMatrices()
{
    if (!m_parent) {
        m_matrix = m_transformation->m_matrix;
        m_inverseMatrix = m_transformation->m_inverseMatrix;
    } else {
        m_matrix = m_parent->m_matrix * m_transformation->m_matrix;
        m_inverseMatrix = m_transformation->m_inverseMatrix * m_parent->m_inverseMatrix;
    }
}

void Node::updateMatricesRecursively()
{
    if (isShown()) {
        updateMatrices();
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->updateMatricesRecursively();
    }
}

bool Node::animate(int frames)
{
    resetStepped();
    bool changed = animate(frames, false, false);
    resetChanged();
    return changed;
}

bool Node::animate(int frames, bool matricesChanged, bool force)
{
    if (!isShown())
        return false;
    m_changed = false;
    if (!m_display->m_visibility.value() && !force)
        return false;
    if (m_flags & (1 << NODE_ALWAYS_ANIMATE))
        force = true;
    if (m_widget)
        m_enclosingWidget = m_widget;
    else if (m_parent)
        m_enclosingWidget = m_parent->m_enclosingWidget;
    else
        m_enclosingWidget = 0;
    if (m_shape && !m_shape->m_stepped)
        m_shape->advance(frames);
    if (m_transformation && !m_transformation->m_stepped)
        m_transformation->advance(frames);
    if (m_display && !m_display->m_stepped)
        m_display->advance(frames);
    if (m_transformation && m_transformation->m_changed)
        matricesChanged = true;
    if (matricesChanged)
        updateMatrices();
    if (m_widget)
        m_widget->update();
    if (m_shape && m_shape->m_changed)
        m_changed = true;
    if (m_transformation && m_transformation->m_changed)
        m_changed = true;
    if (m_display && m_display->m_changed)
        m_changed = true;
    if (m_widget && m_widget->m_changed)
        m_changed = true;
    if (m_engine->getFocusWidget() && m_engine->getFocusWidget() == m_widget)
        m_changed = true;
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->animate(frames, matricesChanged, force))
            m_changed = true;
    }
    if (fitsShapeToChildren() && m_changed && m_shape) {
        Vector<2, float> minimum;
        Vector<2, float> maximum;
        bool empty = true;
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
            if ((*it)->isShown())
                (*it)->extendBounds(minimum, maximum, (*it)->m_transformation->m_matrix, empty, false);
        }
        if (!empty) {
            m_shape->fitBounds(minimum, maximum);
            m_shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
        }
    }
    return m_changed;
}

}
