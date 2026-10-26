// Widget stretch setters shared by widget and sheet callers.
#include "plasma/Widget.h"

namespace plasma {

void Widget::setStretchesX(bool stretch)
{
    if (stretch)
        m_flags |= (1 << WIDGET_ADAPT_WIDTH);
    else
        m_flags &= ~(1 << WIDGET_ADAPT_WIDTH);
}

void Widget::setStretchesY(bool stretch)
{
    if (stretch)
        m_flags |= (1 << WIDGET_ADAPT_HEIGHT);
    else
        m_flags &= ~(1 << WIDGET_ADAPT_HEIGHT);
}

}
