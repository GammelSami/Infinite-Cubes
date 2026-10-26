// NodeModule: the editor's node panel ("module:node", the bottom bar captioned "Object"): the current node's name,
// transformation and display values, and the buttons that show the mesh, text and texture panels. Each value
// applies to every shown selected node of the open design sheet.
#pragma once
#include "Module.h"

class NodeModule : public Module {
public:
    NodeModule();

    virtual void build();
    virtual void updatePanel(); // Shows the node settings.
    virtual void showSelectedValues();
    // Refreshes the panel.
    virtual void currentNodeChanged();
    virtual void unitChanged();

    // Event 3 of the drag buttons (the drag ends): the selected nodes' changes become one history step.
    void endDrag();
    // Shows the current node's translation, scale, shear, rotation and blur.
    void showTransformation();

    void editName();                // "node:name".
    void toggleVisible();           // "node:visible".
    void setNoClipping();           // "node:noclipping".
    void setClipping();             // "node:clipping".
    void toggleActive();            // "node:active".
    void toggleSource();            // "node:generic".
    void toggleSnapable();          // "node:snapable".
    void setNoDeformer();           // "node:nodeformer".
    void setDeformer();             // "node:deformer".
    void toggleBlur();              // "node:blur".
    void setModulation();           // "node:modulation".
    void setScreen();               // "node:screen".
    void setAlignOutline();         // "node:align:outline".
    void setAlignOutlineRotation(); // "node:align:outline:rotation".
    void setAdapt();                // "node:adapt".
    void toggleFixed();             // "node:fixed".

    // "node:blur:edit" (event 19) and "node:blur:drag" (events 12, 2 and 4).
    void editBlur();
    void dragBlur(plasma::Widget *sender);
    void stepBlur(plasma::Widget *sender);

    // "node:translation:x" (event 19) and "node:translation:x:drag" (events 12, 2 and 4), likewise for y, scale, shear
    // and rotation. Apply* sets the selected nodes' value from the edit box.
    void editTranslationX();
    void dragTranslationX(plasma::Widget *sender);
    void stepTranslationX(plasma::Widget *sender);
    void editTranslationY();
    void dragTranslationY(plasma::Widget *sender);
    void stepTranslationY(plasma::Widget *sender);
    void editScaleX();
    void dragScaleX(plasma::Widget *sender);
    void stepScaleX(plasma::Widget *sender);
    void editScaleY();
    void dragScaleY(plasma::Widget *sender);
    void stepScaleY(plasma::Widget *sender);
    void editShearX();
    void dragShearX(plasma::Widget *sender);
    void stepShearX(plasma::Widget *sender);
    void editShearY();
    void dragShearY(plasma::Widget *sender);
    void stepShearY(plasma::Widget *sender);
    void editRotation();
    void dragRotation(plasma::Widget *sender);
    void stepRotation(plasma::Widget *sender);
    void applyTranslationX();
    void applyTranslationY();
    void applyScaleX();
    void applyScaleY();
    void applyShearX();
    void applyShearY();
    void applyRotation();

    // "node:module:mesh", "node:module:text", "node:module:texture": show or hide the mesh, text and texture panels
    // with the buttons.
    void toggleMeshModule();
    void toggleTextModule();
    void toggleTextureModule();
};
