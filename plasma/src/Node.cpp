#include <algorithm>
#include <cmath>
#include <cstring>
#include "plasma/Node.h"
#include "plasma/Shape.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/Engine.h"
#include "plasma/Widget.h"
#include "plasma/RenderSurface.h"
#include "plasma/Button.h"
#include "plasma/Edit.h"
#include "plasma/TextShape.h"
#include "plasma/Attribute.h"

namespace plasma {

Node::Node(Engine *engine, Transformation *transformation, Shape *shape, Display *display, const std::wstring &name)
    : NamedObject(engine, name),
      m_userData(0),
      m_parent(0),
      m_shape(shape),
      m_transformation(transformation),
      m_display(display),
      m_widget(0),
      m_enclosingWidget(0),
      m_flags(0),
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

void Node::setWidget(Widget *widget)
{
    if (widget)
        widget->setNode(this);
    else
        m_widget = 0;
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

void Node::addChild(Node *child)
{
    if (child->m_parent)
        child->m_parent->removeChild(child);
    m_children.push_back(child);
    child->m_parent = this;
    Matrix<float> matrix = m_matrix;
    matrix *= child->m_transformation->m_matrix;
    child->m_matrix = matrix;
    Matrix<float> inverse = child->m_transformation->m_inverseMatrix;
    inverse *= m_inverseMatrix;
    child->m_inverseMatrix = inverse;
}

void Node::insertChild(std::list<Node *>::iterator where, Node *child)
{
    if (child->m_parent)
        child->m_parent->removeChild(child);
    m_children.insert(where, child);
    child->m_parent = this;
    Matrix<float> matrix = m_matrix;
    matrix *= child->m_transformation->m_matrix;
    child->m_matrix = matrix;
    Matrix<float> inverse = child->m_transformation->m_inverseMatrix;
    inverse *= m_inverseMatrix;
    child->m_inverseMatrix = inverse;
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
            insertChild(it, child);
            return true;
        }
    }
    return false;
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

void Node::destroyChildren()
{
    std::list<Node *> children = m_children; // The engine detaches them.
    for (std::list<Node *>::iterator it = children.begin(); it != children.end(); ++it)
        m_engine->destroyNode(*it);
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

void Node::setShown(bool shown)
{
    if (!shown)
        m_flags |= (1 << NODE_HIDDEN);
    else
        m_flags &= ~(1 << NODE_HIDDEN);
    if (m_shape)
        m_shape->shownChanged(shown);
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

    // Display flag bit 0: the blur is enabled.
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

bool Node::isAncestorOf(const Node *node) const
{
    for (; node; node = node->m_parent) {
        if (this == node)
            return true;
    }
    return false;
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

        // Display flag bit 0: the blur is enabled.
        if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
            float blur = m_display->m_blurRadius.value();
            low[0] -= blur;
            low[1] -= blur;
            high[0] += blur;
            high[1] += blur;
        }
        empty = false;
        minimum = matrix * low;
        Vector<2, float> point = matrix * Vector<2, float>(low[0], high[1]);
        if (point[0] < minimum[0])
            minimum[0] = point[0];
        if (point[1] < minimum[1])
            minimum[1] = point[1];
        point = matrix * Vector<2, float>(high[0], high[1]);
        if (point[0] < minimum[0])
            minimum[0] = point[0];
        if (point[1] < minimum[1])
            minimum[1] = point[1];
        point = matrix * Vector<2, float>(high[0], low[1]);
        if (point[0] < minimum[0])
            minimum[0] = point[0];
        if (point[1] < minimum[1])
            minimum[1] = point[1];
    } else if (m_widget && m_widget->hasBounds()) {
        empty = false;
        minimum = matrix.transform(m_widget->getMinimum());
    } else if (m_display->m_clipping.value() || m_children.empty()) {
        return matrix * Vector<2, float>(0.0f, 0.0f);
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
                Vector<2, float> point = matrix * Vector<2, float>(childMinimum[0], childMinimum[1]);
                if (empty) {
                    minimum = point;
                    empty = false;
                } else {
                    if (point[0] < minimum[0])
                        minimum[0] = point[0];
                    if (point[1] < minimum[1])
                        minimum[1] = point[1];
                }
                point = matrix * Vector<2, float>(childMaximum[0], childMinimum[1]);
                if (point[0] < minimum[0])
                    minimum[0] = point[0];
                if (point[1] < minimum[1])
                    minimum[1] = point[1];
                point = matrix * Vector<2, float>(childMaximum[0], childMaximum[1]);
                if (point[0] < minimum[0])
                    minimum[0] = point[0];
                if (point[1] < minimum[1])
                    minimum[1] = point[1];
                point = matrix * Vector<2, float>(childMinimum[0], childMaximum[1]);
                if (point[0] < minimum[0])
                    minimum[0] = point[0];
                if (point[1] < minimum[1])
                    minimum[1] = point[1];
            }
        }
    }
    return minimum;
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

        // Display flag bit 0: the blur is enabled.
        if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
            float blur = m_display->m_blurRadius.value();
            low[0] -= blur;
            low[1] -= blur;
            high[0] += blur;
            high[1] += blur;
        }
        empty = false;
        maximum = matrix * low;
        Vector<2, float> point = matrix * Vector<2, float>(low[0], high[1]);
        if (point[0] > maximum[0])
            maximum[0] = point[0];
        if (point[1] > maximum[1])
            maximum[1] = point[1];
        point = matrix * Vector<2, float>(high[0], high[1]);
        if (point[0] > maximum[0])
            maximum[0] = point[0];
        if (point[1] > maximum[1])
            maximum[1] = point[1];
        point = matrix * Vector<2, float>(high[0], low[1]);
        if (point[0] > maximum[0])
            maximum[0] = point[0];
        if (point[1] > maximum[1])
            maximum[1] = point[1];
    } else if (m_widget && m_widget->hasBounds()) {
        empty = false;
        maximum = matrix.transform(m_widget->getMaximum());
    } else if (m_display->m_clipping.value() || m_children.empty()) {
        return matrix * Vector<2, float>(0.0f, 0.0f);
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
                Vector<2, float> point = matrix * Vector<2, float>(childMinimum[0], childMinimum[1]);
                if (empty) {
                    maximum = point;
                    empty = false;
                } else {
                    if (point[0] > maximum[0])
                        maximum[0] = point[0];
                    if (point[1] > maximum[1])
                        maximum[1] = point[1];
                }
                point = matrix * Vector<2, float>(childMaximum[0], childMinimum[1]);
                if (point[0] > maximum[0])
                    maximum[0] = point[0];
                if (point[1] > maximum[1])
                    maximum[1] = point[1];
                point = matrix * Vector<2, float>(childMaximum[0], childMaximum[1]);
                if (point[0] > maximum[0])
                    maximum[0] = point[0];
                if (point[1] > maximum[1])
                    maximum[1] = point[1];
                point = matrix * Vector<2, float>(childMinimum[0], childMaximum[1]);
                if (point[0] > maximum[0])
                    maximum[0] = point[0];
                if (point[1] > maximum[1])
                    maximum[1] = point[1];
            }
        }
    }
    return maximum;
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

bool Node::extendShapeBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix,
                             bool &empty)
{
    bool found = false;
    if (m_shape) {
        // Shapes that bound themselves through the matrix return true.
        if (!m_shape->getVertexBounds(minimum, maximum, matrix, empty)) {
            m_shape->updateBounds(matrix);
            const Vector<2, float> shapeMinimum = m_shape->getMinimum();
            const Vector<2, float> shapeMaximum = m_shape->getMaximum();
            Vector<2, float> corners[4] = {matrix.transform(Vector<2, float>(shapeMinimum[0], shapeMinimum[1])),
                                           matrix.transform(Vector<2, float>(shapeMaximum[0], shapeMinimum[1])),
                                           matrix.transform(Vector<2, float>(shapeMaximum[0], shapeMaximum[1])),
                                           matrix.transform(Vector<2, float>(shapeMinimum[0], shapeMaximum[1]))};
            if (empty) {
                minimum = corners[0];
                maximum = corners[0];
                empty = false;
            }
            for (int i = 0; i < 4; i++) {
                minimum[0] = (std::min)(minimum[0], corners[i][0]);
                minimum[1] = (std::min)(minimum[1], corners[i][1]);
                maximum[0] = (std::max)(maximum[0], corners[i][0]);
                maximum[1] = (std::max)(maximum[1], corners[i][1]);
            }
        }
        found = true;
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); it++) {
        if ((*it)->isShown() && (*it)->m_display->m_visibility.value()) {
            const Matrix<float> &transformation = (*it)->m_transformation->m_matrix;
            Matrix<float> childMatrix = matrix;
            childMatrix *= transformation;
            if ((*it)->extendShapeBounds(minimum, maximum, childMatrix, empty))
                found = true;
        }
    }
    return found;
}

bool Node::getShapeBounds(Vector<2, float> &minimum, Vector<2, float> &maximum)
{
    bool empty = true;
    return extendShapeBounds(minimum, maximum, m_matrix, empty);
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

    // Display flag bit 0: the blur is enabled; the radius is scaled by the mean length of the matrix's axes.
    if (withBlur && (m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->m_blurRadius.value() > 0.0f) {
        Vector<2, float> xAxis = matrix.transformDirection(Vector<2, float>(1.0f, 0.0f));
        Vector<2, float> yAxis = matrix.transformDirection(Vector<2, float>(0.0f, 1.0f));
        float blur = m_display->m_blurRadius.value() * (double)0.5f *
                     (std::sqrt(xAxis[0] * xAxis[0] + xAxis[1] * xAxis[1]) +
                      std::sqrt(yAxis[0] * yAxis[0] + yAxis[1] * yAxis[1]));
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

bool Node::getBounds(Vector<2, float> &minimum, Vector<2, float> &maximum, const Matrix<float> &matrix)
{
    float blur = 0.0f;
    bool empty = true;

    // Display flag bit 0: the blur is enabled; the radius is scaled by the mean length of the matrix's axes.
    if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->getBlurRadius() > 0.0f) {
        Matrix<float> axes = matrix;
        Vector<2, float> xAxis = axes.transformDirection(Vector<2, float>(1.0f, 0.0f));
        Vector<2, float> yAxis = axes.transformDirection(Vector<2, float>(0.0f, 1.0f));
        blur = (std::sqrt(xAxis[0] * xAxis[0] + xAxis[1] * xAxis[1]) +
                std::sqrt(yAxis[1] * yAxis[1] + yAxis[0] * yAxis[0])) *
               (m_display->getBlurRadius() * (double)0.5f);

        // Applied to the bounds before they are computed and taken back out at the end.
        minimum[0] += blur;
        minimum[1] += blur;
        maximum[0] -= blur;
        maximum[1] -= blur;
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
        empty = false;
        maximum = high;
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
            Node *child = *it;
            if (!child->isShown() || !child->m_display->m_visibility.value())
                continue;
            const Matrix<float> &childTransform = child->m_transformation->m_matrix;
            Matrix<float> childMatrix = matrix;
            childMatrix *= childTransform;
            Vector<2, float> childMinimum;
            Vector<2, float> childMaximum;
            if (!child->getBounds(childMinimum, childMaximum, childMatrix))
                continue;
            Vector<2, float> point = childTransform.transform(childMinimum);
            if (empty) {
                minimum = point;
                maximum = point;
                empty = false;
            } else {
                if (minimum[0] > point[0])
                    minimum[0] = point[0];
                if (minimum[1] > point[1])
                    minimum[1] = point[1];
                if (maximum[0] < point[0])
                    maximum[0] = point[0];
                if (maximum[1] < point[1])
                    maximum[1] = point[1];
            }
            point = childTransform * Vector<2, float>(childMaximum[0], childMinimum[1]);
            if (minimum[0] > point[0])
                minimum[0] = point[0];
            if (minimum[1] > point[1])
                minimum[1] = point[1];
            if (maximum[0] < point[0])
                maximum[0] = point[0];
            if (maximum[1] < point[1])
                maximum[1] = point[1];
            point = childTransform.transform(childMaximum);
            if (minimum[0] > point[0])
                minimum[0] = point[0];
            if (minimum[1] > point[1])
                minimum[1] = point[1];
            if (maximum[0] < point[0])
                maximum[0] = point[0];
            if (maximum[1] < point[1])
                maximum[1] = point[1];
            point = childTransform * Vector<2, float>(childMinimum[0], childMaximum[1]);
            if (minimum[0] > point[0])
                minimum[0] = point[0];
            if (minimum[1] > point[1])
                minimum[1] = point[1];
            if (maximum[0] < point[0])
                maximum[0] = point[0];
            if (maximum[1] < point[1])
                maximum[1] = point[1];
        }
    }
    if ((m_display->m_flags & (1 << DISPLAY_BLUR)) && m_display->getBlurRadius() > 0.0f) {
        minimum[0] -= blur;
        minimum[1] -= blur;
        maximum[0] += blur;
        maximum[1] += blur;
    }
    return !empty;
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

void Node::makeAnimatedUnique()
{
    if (m_shape && m_shape->countNodes() > 1 && (m_shape->countSequences() > 0 || m_shape->m_type == SHAPE_TEXT))
        setShape(m_shape->clone());
    if (m_transformation && m_transformation->countNodes() > 1 && (m_transformation->countSequences() > 0 || m_widget))
        setTransformation(m_transformation->clone());
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isShown())
            (*it)->makeAnimatedUnique();
    }
}

int Node::moveIntoWidget(const std::wstring &name, Widget *widget, bool fit)
{
    int count = 0;
    if (m_name == name) {
        addChild(widget->m_node);
        if (fit && m_widget)
            widget->setViewRect(Vector<2, float>(0.0f, 0.0f), m_widget->getViewSize());
        count = 1;
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        count += (*it)->moveIntoWidget(name, widget, fit);
    return count;
}

Widget *Node::findWidget(const std::wstring &name)
{
    if (m_widget && m_widget->getName() == name)
        return m_widget;
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if (Widget *widget = (*it)->findWidget(name))
            return widget;
    }
    return 0;
}

Node *Node::getNextSibling() const
{
    if (m_parent) {
        const std::list<Node *> &siblings = m_parent->m_children;
        for (std::list<Node *>::const_iterator it = siblings.begin(); it != siblings.end(); ++it) {
            if (*it == this) {
                ++it;
                return it != siblings.end() ? *it : 0;
            }
        }
    }
    return 0;
}

void Node::updateMatrices()
{
    if (!m_parent) {
        m_matrix = m_transformation->m_matrix;
        m_inverseMatrix = m_transformation->m_inverseMatrix;
    } else {
        Matrix<float> matrix = m_parent->m_matrix;
        matrix *= m_transformation->m_matrix;
        m_matrix = matrix;
        Matrix<float> inverse = m_transformation->m_inverseMatrix;
        inverse *= m_parent->m_inverseMatrix;
        m_inverseMatrix = inverse;
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

const std::string &Node::getVariable(const std::string &key) const
{
    std::map<std::string, std::string>::const_iterator it = m_variables.find(key);
    if (it == m_variables.end()) {
        static const std::string empty = "";
        return empty;
    }
    return it->second;
}

void Node::setVariable(const std::string &key, const std::string &value)
{
    std::list<std::string>::iterator it = std::find(m_variableKeys.begin(), m_variableKeys.end(), key);
    if (it == m_variableKeys.end())
        m_variableKeys.push_back(key);
    m_variables[key] = value;
}

void Node::findNodes(std::list<Node *> &nodes, const std::wstring &name)
{
    if (m_name == name)
        nodes.push_back(this);
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
        (*it)->findNodes(nodes, name);
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

int Node::countVisibleChildren() const
{
    int count = 0;
    for (std::list<Node *>::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isShown() && !(*it)->isInternal())
            count++;
    }
    return count;
}

void Node::getSequenceNames(std::set<std::wstring> &names)
{
    if (isShown() && !isInternal()) {
        if (m_transformation)
            m_transformation->getSequenceNames(names);
        if (m_display)
            m_display->getSequenceNames(names);
        if (m_shape)
            m_shape->getSequenceNames(names);
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->getSequenceNames(names);
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

bool Node::animate(int frames)
{
    resetStepped();
    bool changed = animate(frames, false, false);
    resetChanged();
    return changed;
}

void Node::setFrame(int frame)
{
    resetStepped();
    setFrame(frame, false, false);
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

// Blur surfaces use two pixels of extent padding. Clear margins use four radii with an offset of two radii.
int Node::draw(int mode)
{
    if (!m_display->m_visibility.value() || !isShown())
        return 0;
    m_engine->pushNodeMatrix(this);
    int rendered = 0;
    RenderSurface *surface = 0;
    bool widgetSurface = false;

    // RENDER_CLIP_WIDGET_SURFACES: widgets with WIDGET_RENDER_SURFACE draw into their own surface; NODE_CACHED: the
    // subtree is cached in the node's surface.
    if (m_engine->usesWidgetSurfaces() && m_widget && m_widget->rendersToSurface()) {
        m_widget->updateSurface();
        surface = m_widget->m_surface;
        widgetSurface = true;
    }
    if (!surface && (m_flags & (1 << NODE_CACHED))) {
        if (!m_cacheSurface) {
            m_cacheSurface = m_engine->createRenderSurface(0);
            m_changed = true;
        }
        surface = m_cacheSurface;
    }
    bool redraw = surface && (m_changed || (m_widget && m_widget->m_changed));
    bool antialiasing = m_engine->m_renderFlags & (1 << RENDER_ANTIALIAS);
    Vector<2, float> windowOffset(0.0f, 0.0f);
    Vector<2, float> textureOffset(0.0f, 0.0f);
    if (redraw) {
        if (widgetSurface) {
            // A downscaled widget draws its surface without the anti-aliasing offset (RENDER_ANTIALIAS).
            if (m_widget->m_surfaceScale < 1.0f)
                m_engine->m_renderFlags &= ~(1 << RENDER_ANTIALIAS);

            // Window positions and sizes are rounded to whole pixels: 0.5 is added in double precision, then the elements are
            // truncated.
            Vector<2, int> offset(textureOffset);
            Vector<2, float> screenSize = m_widget->screenSize();
            screenSize.m_elements[0] += (double)0.5f;
            screenSize.m_elements[1] += (double)0.5f;
            Vector<2, float> size(offset + Vector<2, int>(screenSize));
            Vector<2, float> screenPosition = m_widget->getContentPosition();
            screenPosition.m_elements[0] += (double)0.5f;
            screenPosition.m_elements[1] += (double)0.5f;
            Vector<2, float> position = Vector<2, float>(Vector<2, int>(screenPosition)) + windowOffset;
            m_engine->beginSurface(surface, position, size, 1.0f, false);
            Matrix<float> matrix;
            m_engine->getModelViewMatrix(matrix);
            matrix.scale(m_widget->m_surfaceScale, m_widget->m_surfaceScale);
            matrix.translate(m_widget->m_surfaceOffset);
            m_engine->loadMatrix(matrix);
            surface->clear();
        } else {
            Vector<2, float> minimum;
            Vector<2, float> maximum;
            getScreenBounds(minimum, maximum);

            // The extent stays unrounded (double) for the surface size.
            double height = maximum.m_elements[1] - minimum.m_elements[1];
            double width = maximum.m_elements[0] - minimum.m_elements[0];
            surface->resize((int)(width + (double)0.5f) + 2, (int)(height + (double)0.5f) + 2);
            Vector<2, int> offset(textureOffset);
            Vector<2, int> origin(windowOffset);
            Vector<2, float> roundedExtent((float)width, (float)height);
            roundedExtent.m_elements[0] += (double)0.5f;
            roundedExtent.m_elements[1] += (double)0.5f;
            Vector<2, float> size(offset + Vector<2, int>(roundedExtent));
            Vector<2, float> roundedMinimum = minimum;
            roundedMinimum.m_elements[0] += (double)0.5f;
            roundedMinimum.m_elements[1] += (double)0.5f;
            Vector<2, float> position(origin + Vector<2, int>(roundedMinimum));
            m_engine->beginSurface(surface, position, size, 1.0f, false);
            surface->clear();
        }
    }
    Matrix<float> modelView;
    m_engine->getModelViewMatrix(modelView);

    // NODE_RENDER_CULL_SCREEN culls offscreen shapes; STOP_AT_EFFECT stops at a clipping or blurred child. SKIP_BLUR
    // omits blur, NO_CHILDREN omits children; STOP_AT_EFFECT and NO_CHILDREN also suppress clipping.
    bool clip =
        m_display->m_clipping.value() && !(mode & ((1 << NODE_RENDER_STOP_AT_EFFECT) | (1 << NODE_RENDER_NO_CHILDREN)));

    // RENDER_BLUR and DISPLAY_BLUR: the blur is enabled.
    bool blur = (m_engine->m_renderFlags & (1 << RENDER_BLUR)) && (m_display->m_flags & (1 << DISPLAY_BLUR)) &&
                m_display->m_blurRadius.value() > 0.0f && !(mode & (1 << NODE_RENDER_SKIP_BLUR));
    if (m_shape)
        m_shape->updateBounds(modelView);
    float blurRadius = 0.0f;
    float blurScale = 1.0f;
    float passRadius = 1.0f;
    if (blur) {
        // The radius in window pixels; above 8 the blur is drawn at a smaller scale.
        const float *m = modelView.m_elements;
        passRadius = (std::sqrt(m[5] * m[5] + m[1] * m[1]) + std::sqrt(m[0] * m[0] + m[4] * m[4])) * (double)0.5f *
                     m_display->m_blurRadius.value();
        blurRadius = passRadius / (double)1.4f;
        passRadius = blurRadius;
        if (blurRadius > 8.0f) {
            blurScale = (double)8.0f / blurRadius;
            passRadius = 8.0f;
        }
    }
    bool visible = !(mode & (1 << NODE_RENDER_CULL_SCREEN)) || isOnScreen();
    if (redraw || !surface) {
        if (visible || (blur && !clip)) {
            Vector<2, float> minimum;
            Vector<2, float> maximum;
            if (blur) {
                getScreenBounds(minimum, maximum);
                minimum.m_elements[0] -= blurRadius;
                minimum.m_elements[1] -= blurRadius;
                maximum.m_elements[0] += blurRadius;
                maximum.m_elements[1] += blurRadius;
            }
            if (!clip && !blur) {
                m_engine->m_filterSurface = 0;
                renderShape();
                ++rendered;
            } else {
                // Clipping or blurred: the subtree is drawn off screen first.
                m_engine->beginOffscreenDrawing(blurScale);
                RenderSurface *surfaces[2];
                if (blur) {
                    RenderSurface *target = m_engine->m_renderSurfaces[m_engine->m_surfaceDepth];
                    float margin = 4.0f * blurRadius;
                    Vector<2, float> size =
                        maximum - minimum + Vector<2, float>(2.0f, 2.0f) + Vector<2, float>(margin, margin);
                    float border = 2.0f * blurRadius;
                    Vector<2, float> position = minimum - Vector<2, float>(border, border);
                    target->clearRect(position, size, true);
                    if (clip) {
                        m_engine->beginOffscreenDrawing(1.0f);
                        m_engine->clearSurfaceBounds(this);
                    } else if (visible) {
                        renderShape();
                        rendered = 1;
                    }
                } else {
                    m_engine->clearSurfaceBounds(this);
                }
                for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
                    rendered += (*it)->draw(mode & ~(1 << NODE_RENDER_SKIP_BLUR));
                if (blur) {
                    // The clipped subtree is drawn into the shape's mask.
                    if (clip) {
                        m_engine->endOffscreenDrawing();
                        renderShape();
                        ++rendered;
                    }
                    Engine *engine = m_engine;
                    surfaces[0] = engine->m_renderSurfaces[engine->m_surfaceDepth];
                    if (!engine->m_blurSurface) {
                        engine->m_blurSurface = engine->createRenderSurface(0);
                        engine->m_blurSurface->resize(engine->m_viewportWidth, engine->m_viewportHeight);
                    }
                    surfaces[1] = engine->m_blurSurface;
                    float margin = 4.0f * blurRadius;
                    Vector<2, float> size =
                        maximum - minimum + Vector<2, float>(2.0f, 2.0f) + Vector<2, float>(margin, margin);
                    float border = blurRadius * 2.0f;
                    Vector<2, float> position = minimum - Vector<2, float>(border, border);
                    surfaces[1]->clearRect(position, size, true);

                    // Two passes of a horizontal and a vertical blur between the two surfaces.
                    for (int pass = 2; pass > 0; --pass) {
                        float radius = passRadius / pass;
                        for (int axis = 0; axis < 2; ++axis) {
                            surfaces[(axis + 1) % 2]->begin();
                            Vector<2, float> extent = maximum - minimum;
                            surfaces[axis]->drawBlurred(minimum, extent, radius, axis == 0, blurScale);
                        }
                    }
                }
                m_engine->endOffscreenDrawing();
                if (!blur) {
                    renderShape();
                    ++rendered;
                } else {
                    Vector<2, float> size = maximum - minimum + Vector<2, float>(2.0f, 2.0f);
                    surfaces[0]->draw(minimum, size, minimum, blurScale);
                }
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
        if (!surface) {
            m_engine->popNodeMatrix(this);
            return rendered;
        }
    }

    // Composite the surface into the window.
    if (!widgetSurface) {
        Vector<2, float> minimum;
        Vector<2, float> maximum;
        getScreenBounds(minimum, maximum);
        Vector<2, int> offset(textureOffset);
        Vector<2, int> origin(windowOffset);
        Vector<2, float> roundedMinimum = minimum;
        roundedMinimum.m_elements[0] += (double)0.5f;
        roundedMinimum.m_elements[1] += (double)0.5f;
        Vector<2, float> position(Vector<2, int>(roundedMinimum) + origin);
        Vector<2, float> extent = maximum - minimum;
        extent.m_elements[0] += (double)0.5f;
        extent.m_elements[1] += (double)0.5f;
        Vector<2, float> size(Vector<2, int>(extent) + offset);
        surface->draw(position, size, Vector<2, float>(0.0f, 0.0f), -1.0f);
        m_engine->popNodeMatrix(this);
        return rendered;
    }
    Vector<2, int> offset(textureOffset);
    Vector<2, float> screenSize = m_widget->screenSize();
    screenSize.m_elements[0] += (double)0.5f;
    screenSize.m_elements[1] += (double)0.5f;
    Vector<2, float> size(Vector<2, int>(screenSize) + offset);
    Vector<2, int> origin(windowOffset);
    Vector<2, float> screenPosition = m_widget->getContentPosition();
    screenPosition.m_elements[0] += (double)0.5f;
    screenPosition.m_elements[1] += (double)0.5f;
    Vector<2, float> position(Vector<2, int>(screenPosition) + origin);
    Widget *widget = m_widget;
    float scale = widget->m_surfaceScale;
    Vector<2, float> texturePosition = widget->m_surfaceOffset * scale / (float)widget->m_surfaceSamples;
    surface->drawDownsampled(position, size, texturePosition, scale / widget->m_surfaceSamples,
                             widget->m_surfaceSamples);
    if (antialiasing)
        m_engine->m_renderFlags |= (1 << RENDER_ANTIALIAS);
    else
        m_engine->m_renderFlags &= ~(1 << RENDER_ANTIALIAS);
    m_engine->popNodeMatrix(this);
    return rendered;
}

void Node::renderShape()
{
    if (m_shape)
        m_shape->draw(this);
}

bool Node::isChecked()
{
    if (!isShown())
        return false;
    if (m_widget && m_widget->m_kind == WIDGET_BUTTON && static_cast<Button *>(m_widget)->m_checked)
        return true;
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isChecked())
            return true;
    }
    return false;
}

bool Node::isChecked(const std::wstring &name, bool recursive)
{
    if (!isShown())
        return false;
    if (m_name == name) {
        if (recursive)
            return isChecked();
        return m_widget && m_widget->m_kind == WIDGET_BUTTON && static_cast<Button *>(m_widget)->m_checked;
    }
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        if ((*it)->isChecked(name, recursive))
            return true;
    }
    return false;
}

void Node::setChecked(bool checked, bool fromGroup)
{
    if (isShown()) {
        if (m_widget && m_widget->m_kind == WIDGET_BUTTON)
            static_cast<Button *>(m_widget)->setChecked(checked, fromGroup);
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); it++)
            (*it)->setChecked(checked, fromGroup);
    }
}

void Node::setChecked(const std::wstring &name, bool checked, bool fromGroup, bool recursive)
{
    if (isShown()) {
        if (m_name == name) {
            if (recursive) {
                setChecked(checked, fromGroup);
            } else if (m_widget && m_widget->m_kind == WIDGET_BUTTON) {
                Button *button = static_cast<Button *>(m_widget);
                if (button->m_checked != checked)
                    button->setChecked(checked, fromGroup);
            }
        } else {
            for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); it++)
                (*it)->setChecked(name, checked, fromGroup, recursive);
        }
    }
}

void Node::setEnabled(bool enabled)
{
    if (isShown()) {
        if (m_widget && m_widget->m_enabled != enabled)
            m_widget->setEnabled(enabled);
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setEnabled(enabled);
    }
}

void Node::setEnabled(const std::wstring &name, bool enabled, bool recursive)
{
    if (isShown()) {
        if (m_name == name) {
            if (recursive)
                setEnabled(enabled);
            else if (m_widget && m_widget->m_enabled != enabled)
                m_widget->setEnabled(enabled);
        } else {
            for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); it++)
                (*it)->setEnabled(name, enabled, recursive);
        }
    }
}

void Node::setText(const std::wstring &text)
{
    if (isShown()) {
        if (m_shape && m_shape->m_type == SHAPE_TEXT) {
            TextShape *shape = static_cast<TextShape *>(m_shape);
            if (shape->m_string.value() != text) {
                shape->m_string = text;
                shape->attributesChanged(ATTRIBUTES_CHANGED_ALL);
            }
        }
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setText(text);
    }
}

void Node::setText(const std::wstring &name, const std::wstring &text, bool recursive)
{
    if (isShown()) {
        if (m_name == name) {
            if (recursive) {
                setText(text);
            } else if (m_shape && m_shape->m_type == SHAPE_TEXT) {
                TextShape *shape = static_cast<TextShape *>(m_shape);
                if (shape->m_string.value().compare(text) != 0) {
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

void Node::setEditFilter(EditFilter *filter)
{
    if (isShown()) {
        if (m_widget && m_widget->m_kind == WIDGET_EDIT)
            static_cast<Edit *>(m_widget)->m_filter = filter;
        for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
            (*it)->setEditFilter(filter);
    }
}

void Node::setEditFilter(const std::wstring &name, EditFilter *filter, bool recursive)
{
    if (isShown()) {
        if (m_name == name) {
            if (recursive)
                setEditFilter(filter);
            else if (m_widget && m_widget->m_kind == WIDGET_EDIT)
                static_cast<Edit *>(m_widget)->m_filter = filter;
        } else {
            for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it)
                (*it)->setEditFilter(name, filter, recursive);
        }
    }
}

std::wstring Node::getText()
{
    if (m_shape && m_shape->m_type == SHAPE_TEXT)
        return static_cast<TextShape *>(m_shape)->m_string.value();
    for (std::list<Node *>::iterator it = m_children.begin(); it != m_children.end(); ++it) {
        std::wstring text = (*it)->getText();
        if (text.size())
            return text;
    }
    return L"";
}

std::wstring Node::getText(const std::wstring &name, bool recursive)
{
    if (m_name == name) {
        if (recursive)
            return getText();
        if (m_shape && m_shape->m_type == SHAPE_TEXT)
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

Node *Node::hitTest(const Vector<2, float> &position, float radius, int flags)
{
    if (!m_display->m_visibility.value() || !isShown() || !(flags & (1 << HIT_INCLUDE_INACTIVE)) && !isHitTestable())
        return 0;
    if (m_widget && m_widget->m_surface && m_engine->usesWidgetSurfaces() && !m_widget->isAlwaysHit()) {
        Vector<2, float> origin = m_widget->getContentPosition();
        Vector<2, float> size = m_widget->screenSize();
        if (position.m_elements[0] < origin.m_elements[0] || position.m_elements[1] < origin.m_elements[1] ||
            position.m_elements[0] > (float)(origin.m_elements[0] + size.m_elements[0]) ||
            position.m_elements[1] > (float)(origin.m_elements[1] + size.m_elements[1]))
            return 0;
    }
    Vector<2, float> local = m_inverseMatrix * position;
    Vector<2, float> deformed = local;
    Shape *shape;
    if (m_enclosingWidget && (shape = m_shape) && shape->isDeformable())
        deformed = m_enclosingWidget->undeform(deformed, this);
    bool contains;
    if ((shape = m_shape) && shape->contains(deformed) ||
        m_widget && (m_widget->contains(local) || m_widget->isAlwaysHit())) {
        contains = true;
    } else {
        contains = false;
        if ((shape = m_shape) && shape->intersects(position, radius, m_matrix, m_inverseMatrix))
            contains = true;
    }
    if (m_display->m_clipping.value() && !contains)
        return 0;
    if (!(flags & (1 << HIT_INCLUDE_INACTIVE)))
        contains = contains && isHitTestable();
    for (std::list<Node *>::reverse_iterator it = m_children.rbegin(); it != m_children.rend(); ++it) {
        Node *node = (*it)->hitTest(position, radius, flags);
        if (node) {
            if ((flags & (1 << HIT_SKIP_INTERNAL)) && node->isInternal() &&
                (!(flags & (1 << HIT_SKIP_LOCKED)) || !isLocked()))
                return this;
            return node;
        }
    }
    if ((flags & (1 << HIT_SKIP_LOCKED)) && isLocked())
        return 0;
    return contains ? this : 0;
}

}
