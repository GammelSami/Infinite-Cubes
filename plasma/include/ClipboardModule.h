// The "module:clipboard" toolbar and its two lists of copied node trees.
#pragma once
#include "Module.h"
#include "plasma/Matrix.h"
#include <map>

class ClipboardModule : public Module {
public:
    ClipboardModule();
    virtual void build();
    void cut();
    void copy();
    void paste();
    void duplicate();
    void pasteInstance();
    void duplicateInstance();
    void copyNodes(plasma::Node *node);
    void pasteNodes(bool instance, const plasma::Matrix<float> &matrix, bool record);

    std::list<plasma::Node *> m_nodes;     // Independent copies.
    std::list<plasma::Node *> m_instances; // Instance copies.

    // Left uninitialized by the constructor; Workspace::Init allocates the clipboard node.
    plasma::Matrix<float> m_matrix;
};

void cloneNodeObjects(plasma::Node *node, std::map<void *, void *> &shapes, std::map<void *, void *> &transformations,
                      std::map<void *, void *> &displays);

// HSV conversion uses saturation and value as percentages.
plasma::Vector<4, float> hsvToRgb(const plasma::Vector<4, float> &hsv);

