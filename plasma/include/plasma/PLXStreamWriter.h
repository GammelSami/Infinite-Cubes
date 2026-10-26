// Writes the engine's numbered PLX scene chunks and attribute sequences.
#pragma once
#include <ostream>
#include <set>
#include "plasma/StreamWriter.h"
#include "plasma/PLXStreamReader.h"
#include "plasma/GenericShape.h"
#include "plasma/ListWidget.h"
#include "plasma/ScrollButton.h"
#include "plasma/PopUpButton.h"
#include "plasma/Edit.h"

namespace plasma {

class PLXStreamWriter : public StreamWriter {
public:
    PLXStreamWriter(Engine *engine, Engine::Page *page, Node *root, std::ostream &stream, unsigned __int64 key);
    void write();
    void collectNodes(Node *node, std::list<Node *> &nodes);
    void writeTexture(Texture *texture);
    void writeSequences(Attribute *attribute);

    // Inline scalar serialization.
    std::ostream &writeInt(int value) { return write4(&value); }
    void writeTextShape(Shape *shape);
    void writeSmoothMeshShape(Shape *shape);
    void writeGenericShape(GenericShape *shape);
    void writeTransformation(Transformation *transformation);
    void writeDisplay(Display *display);
    bool writeShape(Shape *shape);
    void writeScrollButton(Widget *widget);
    void writeScrollSlider(Widget *widget);
    void writePopUpButton(Widget *widget);
    void writeButton(Button *button);
    void writeListWidget(Widget *widget);
    void writeWidget(Widget *widget);
    void writeWidgetObject(Widget *widget);

    void writeContinuousVector4(Attribute *attribute);
    void writeArrayVector2(ContinuousArrayAttribute<Vector<2, float>> *attribute);
    void writeArrayVector4(ContinuousArrayAttribute<Vector<4, float>> *attribute);
    void writeArrayFloat(ContinuousArrayAttribute<float> *attribute);
    void writeDiscreteInt(Attribute *attribute);
    void writeContinuousVector2(Attribute *attribute);
    void writeContinuousVector3(Attribute *attribute);
    void writeContinuousMatrix(Attribute *attribute);
    void writeContinuousFloat(Attribute *attribute);
    std::ostream &writeVector2(const Vector<2, float> &value);
    std::ostream &writeMatrix(const Matrix<float> &value);
    std::ostream &writeVector4(const Vector<4, float> &value);
    std::ostream &writeVector3(const Vector<3, float> &value);

    Engine *m_engine;
    Engine::Page *m_page;
    Node *m_root;
    std::map<Widget *, int> m_widgetIds;
};

void compressPLX(std::vector<char> &output, const std::vector<unsigned char> &input);

}
