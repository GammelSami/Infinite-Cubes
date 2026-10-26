// Clipboard support for copying, pasting and adjusting node trees.
#include "GridModule.h"
#include "ClipboardModule.h"
#include "ZoomModule.h"
#include "StrokeWidthModule.h"
#include "LayerModule.h"
#include "ScaleBoxModule.h"
#include "TextModule.h"
#include "ToolModule.h"
#include "DesignSheet.h"
#include "plasma/GenericWidget.h"
#include "plasma/Transformation.h"
#include "plasma/SmoothMeshShape.h"
#include "plasma/TextShape.h"
#include "PaletteModule.h"
#include <sstream>
#include <cmath>
#include "Operations.h"
#include "CreateNodeOperation.h"
#include "NodeTreeWidget.h"

void ClipboardModule::copyNodes(plasma::Node *node)
{
    if (!node) {
        node = m_workspace->getOpenDesignSheet()->m_root;
        for (std::list<plasma::Node *>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
            m_workspace->m_engine.destroyNode(*it);
        for (std::list<plasma::Node *>::iterator it = m_instances.begin(); it != m_instances.end(); ++it)
            m_workspace->m_engine.destroyNode(*it);
        m_nodes.clear();
        m_instances.clear();
    }
    if (!(node->m_flags & (1 << plasma::NODE_HIDDEN)) && !(node->m_flags & (1 << plasma::NODE_INTERNAL))) {
        NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->findSheetNode(node);

        // The code assumes every traversed Node has a NodeInterface.
        if (sheetNode->m_selected) {
            m_nodes.push_back(node->instance(0));
            m_instances.push_back(node->instance(0));
            plasma::Node *copy = m_nodes.back();
            copy->setTransformation(copy->transformation()->clone());
            plasma::Matrix<float> transform = m_workspace->getOpenDesignSheet()->m_root->m_inverseMatrix;
            transform *= node->m_parent->m_matrix;

            // The code assumes non-null objects and equal pivot/translation frame counts.
            for (int frame = 0; frame < copy->transformation()->m_translation.size(); ++frame) {
                plasma::Transformation *transformation = copy->transformation();
                std::vector<plasma::Vector<2, float>> &translations = transformation->m_translation.m_values;
                std::vector<plasma::Vector<2, float>> &pivots = transformation->m_pivot.m_values;
                translations[frame] = transform.transform(translations[frame] + pivots[frame]) - pivots[frame];
            }
            for (int frame = 0; frame < copy->transformation()->m_deformation.size(); ++frame) {
                plasma::Transformation *transformation = copy->transformation();
                std::vector<plasma::Matrix<float>> &deformations = transformation->m_deformation.m_values;
                plasma::ContinuousAttribute<plasma::Vector<3, float>> &rotationAttribute = transformation->m_rotation;
                plasma::Matrix<float> rotation;
                rotation.identity();

                // Every deformation frame uses the current rotation value.
                rotation.rotateX(rotationAttribute.value().m_elements[0]);
                rotation.rotateY(rotationAttribute.value().m_elements[1]);
                rotation.rotateZ(rotationAttribute.value().m_elements[2]);
                plasma::Matrix<float> inverseRotation = rotation;
                inverseRotation.invert();
                plasma::Matrix<float> deformation = inverseRotation * transform * rotation;
                deformation.m_elements[12] = 0.0f;
                deformation.m_elements[13] = 0.0f;
                deformation.m_elements[14] = 0.0f;
                deformations[frame] = deformation * deformations[frame];
            }
            copy->transformation()->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            m_instances.back()->setTransformation(copy->transformation()->clone());
        } else {
            for (std::list<plasma::Node *>::iterator it = node->m_children.begin(); it != node->m_children.end();
                 ++it) {
                copyNodes(*it);
            }
            if (node == m_workspace->getOpenDesignSheet()->m_root) {
                std::map<void *, void *> shapes;
                std::map<void *, void *> transformations;
                std::map<void *, void *> displays;
                for (std::list<plasma::Node *>::iterator it = m_nodes.begin(); it != m_nodes.end(); ++it)
                    cloneNodeObjects(*it, shapes, transformations, displays);

                // The code walks this second list with an empty body.
                for (std::list<plasma::Node *>::iterator it = m_instances.begin(); it != m_instances.end(); ++it) {
                }
            }
        }
    }
}

void ClipboardModule::pasteNodes(bool instance, const plasma::Matrix<float> &matrix, bool record)
{
    if (!m_nodes.empty()) {
        if (record) {
            for (std::map<plasma::Node *, NodeInterface *>::iterator it =
                     m_workspace->getOpenDesignSheet()->m_sheetNodes.begin();
                 it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
                if (!(it->first->m_flags & (1 << plasma::NODE_HIDDEN)) && it->second->m_selected) {
                    it->second->setSelected(false);
                    m_workspace->getOpenDesignSheet()->addOperation(new NodeSelectionOperation(
                        m_workspace->getOpenDesignSheet(), "deselectNode", it->second, false));
                }
            }
        }
        std::map<void *, void *> shapes;
        std::map<void *, void *> transformations;
        std::map<void *, void *> displays;
        plasma::Matrix<float> transform = m_workspace->getOpenDesignSheet()->m_editRoot->m_inverseMatrix;
        transform *= m_workspace->getOpenDesignSheet()->m_root->m_matrix;
        transform *= matrix;
        std::list<plasma::Node *> &nodes = instance ? m_instances : m_nodes;
        for (std::list<plasma::Node *>::iterator it = nodes.begin(); it != nodes.end(); ++it) {
            plasma::Node *node = (*it)->instance(m_workspace->getOpenDesignSheet()->m_editRoot);
            if (!instance)
                cloneNodeObjects(node, shapes, transformations, displays);

            // The code assumes both objects and equally sized pivot/translation frames.
            node->setTransformation(node->m_transformation->clone());
            node->setDisplay(node->m_display->clone());

            // The code assumes non-null objects and equal pivot/translation frame counts.
            for (int frame = 0; frame < node->m_transformation->m_translation.size(); ++frame) {
                plasma::Transformation *transformation = node->m_transformation;
                std::vector<plasma::Vector<2, float>> &translations = transformation->m_translation.m_values;
                std::vector<plasma::Vector<2, float>> &pivots = transformation->m_pivot.m_values;
                translations[frame] = transform.transform(translations[frame] + pivots[frame]) - pivots[frame];
            }
            for (int frame = 0; frame < node->m_transformation->m_deformation.size(); ++frame) {
                plasma::Transformation *transformation = node->m_transformation;
                std::vector<plasma::Matrix<float>> &deformations = transformation->m_deformation.m_values;
                plasma::ContinuousAttribute<plasma::Vector<3, float>> &rotationAttribute = transformation->m_rotation;
                plasma::Matrix<float> rotation;
                rotation.identity();

                // The code uses the current rotation for every deformation frame.
                rotation.rotateX(rotationAttribute.value().m_elements[0]);
                rotation.rotateY(rotationAttribute.value().m_elements[1]);
                rotation.rotateZ(rotationAttribute.value().m_elements[2]);
                plasma::Matrix<float> inverseRotation = rotation;
                inverseRotation.invert();
                plasma::Matrix<float> deformation = inverseRotation * transform * rotation;
                deformation.m_elements[12] = 0.0f;
                deformation.m_elements[13] = 0.0f;
                deformation.m_elements[14] = 0.0f;
                deformations[frame] = deformation * deformations[frame];
            }
            node->m_transformation->attributesChanged(plasma::ATTRIBUTES_CHANGED_ALL);
            m_workspace->m_engine.resolveSources(node);
            m_workspace->getOpenDesignSheet()->m_currentNode = node;
            NodeInterface *sheetNode = m_workspace->getOpenDesignSheet()->createSheetNodes(node);
            sheetNode->setSelected(true);
            m_workspace->getOpenDesignSheet()->addOperation(
                new CreateNodeOperation(m_workspace->getOpenDesignSheet(), "paste node", node));
        }
        m_workspace->realignChildren(m_workspace->getOpenDesignSheet()->m_editRoot);
    }
}

void reconnectTextWidth(plasma::Widget *widget);
void applyTextAlignment(plasma::TextShape *shape, int alignment);
void applyMeshExtrusion(plasma::MeshShape *shape, bool extruded);
void appendPaletteColor(PaletteModule *module, const plasma::Vector<4, float> &hsv);

#include "plasma/TextShape.h"
#include "PaletteModule.h"
