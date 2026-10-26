// WidgetBindOperation records widget geometry before and after "bind widget". This file also contains the
// recursive freehand path simplification.
#include <vector>
#include "BoundingBoxTool.h"
#include "WidgetModule.h"
#include "plasma/Widget.h"
#include "DesignSheet.h"

WidgetBindOperation::WidgetBindOperation(
    DesignSheet *sheet, const std::string &name, plasma::Widget *widget, const plasma::Vector<2, float> &oldBindPos,
    const plasma::Vector<2, float> &oldBindSize, const plasma::Vector<2, float> &oldInnerBindPos,
    const plasma::Vector<2, float> &oldInnerBindSize, const plasma::Matrix<float> &oldBindMatrix,
    const plasma::Vector<2, float> &newBindPos, const plasma::Vector<2, float> &newBindSize,
    const plasma::Vector<2, float> &newInnerBindPos, const plasma::Vector<2, float> &newInnerBindSize,
    const plasma::Matrix<float> &newBindMatrix)
    : Operation(sheet, name, true),
      m_oldBindPos(oldBindPos),
      m_oldBindSize(oldBindSize),
      m_oldInnerBindPos(oldInnerBindPos),
      m_oldInnerBindSize(oldInnerBindSize),
      m_oldBindMatrix(oldBindMatrix),
      m_newBindPos(newBindPos),
      m_newBindSize(newBindSize),
      m_newInnerBindPos(newInnerBindPos),
      m_newInnerBindSize(newInnerBindSize),
      m_newBindMatrix(newBindMatrix),
      m_widget(widget)
{}

// SetGeometry takes the inner rectangle first, then the bind rectangle.
void WidgetBindOperation::execute()
{
    m_widget->setGeometry(m_newInnerBindPos, m_newInnerBindSize, m_newBindPos, m_newBindSize, m_newBindMatrix);
}

void WidgetBindOperation::undo()
{
    m_widget->setGeometry(m_oldInnerBindPos, m_oldInnerBindSize, m_oldBindPos, m_oldBindSize, m_oldBindMatrix);
}

// Douglas-Peucker simplification keeps the point farthest from the first-to-last segment if it exceeds the
// tolerance, then simplifies both halves. The freehand tool starts with the whole path.
void simplifyPathSegment(float tolerance, const std::vector<plasma::Vector<2, float>> &points, int first, int last,
                         std::vector<int> &kept)
{
    if (last <= first + 1)
        return;
    float toleranceSquared = tolerance * tolerance;
    float maxDistance = 0.0f;
    plasma::Vector<2, float> start = points[first];
    plasma::Vector<2, float> end = points[last];
    plasma::Vector<2, float> direction = end - start;
    float lengthSquared =
        direction.m_elements[0] * direction.m_elements[0] + direction.m_elements[1] * direction.m_elements[1];
    int farthest = first;
    plasma::Vector<2, float> offset, nearest;
    for (int i = first + 1; i < last; i++) {
        // The point's distance from the segment: from the nearer end, or from its projection onto the segment.
        offset = points[i] - start;
        float projection = offset.m_elements[0] * direction.m_elements[0];
        projection += offset.m_elements[1] * direction.m_elements[1];
        float distance;
        if (projection <= 0.0) {
            plasma::Vector<2, float> delta = points[i] - start;
            distance = delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
        } else if (projection >= lengthSquared) {
            plasma::Vector<2, float> delta = points[i] - end;
            distance = delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
        } else {
            float t = projection / lengthSquared;
            nearest = start + direction * t;
            plasma::Vector<2, float> delta = points[i] - nearest;
            distance = delta.m_elements[0] * delta.m_elements[0] + delta.m_elements[1] * delta.m_elements[1];
        }
        if (!(maxDistance >= distance)) {
            maxDistance = distance;
            farthest = i;
        }
    }
    if (maxDistance > toleranceSquared) {
        kept[farthest] = 1;
        simplifyPathSegment(tolerance, points, first, farthest, kept);
        simplifyPathSegment(tolerance, points, farthest, last, kept);
    }
}
