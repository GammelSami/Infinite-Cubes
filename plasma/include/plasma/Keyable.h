// plasma::Keyable: a named object with animatable attributes. It plays one named sequence at a time on all its
// attributes.
#pragma once
#include <list>
#include <set>
#include <string>
#include "plasma/NamedObject.h"

class DesignSheet;
class NodeTool;
class ExportModule;
class Workspace;
class SpecialColorModule;
class ShadowModule;
class TextureModule;
class TimelineKeyWidget;
class TimelineMarkWidget;

namespace plasma {

class Attribute;

// The reason argument of Keyable::AttributesChanged. SmoothMeshShape re-uploads only the attributes whose changed
// flag is set for VALUES and everything for ALL.
enum AttributeChangeReason {
    ATTRIBUTES_CHANGED_VALUES = 0, // Advance/SetFrame moved animated values.
    ATTRIBUTES_CHANGED_ALL = 1     // An edit: rebuild the shape completely.
};

class Keyable : public NamedObject {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    Keyable(Engine *engine, const std::wstring &name);
    virtual ~Keyable();

    // Called with ATTRIBUTES_CHANGED_VALUES after playback changed an attribute, with ATTRIBUTES_CHANGED_ALL after an
    // edit.
    virtual void attributesChanged(int reason) {}

    void addAttribute(Attribute *attribute);

    // Both return whether an attribute changed.
    bool advance(int frames);
    bool setFrame(int frame);
    void play(const std::wstring &sequenceName, int loopLength);
    void stop();
    void getSequenceNames(std::set<std::wstring> &names);
    int countSequences();

protected:
    friend class Widget;               // Keeps m_changed across a move.
    friend class Node;                 // Clears m_stepped and m_changed.
    friend class ::Workspace;          // Likewise.
    friend class ::ExportModule;       // Sets the sheet root transformation's m_changed.
    friend class ::DesignSheet;        // Likewise after Undo/Redo.
    friend class ::NodeTool;           // Sets or clears it for the transformed nodes.
    friend class ::SpecialColorModule; // Marks its colour swatches' displays changed.
    friend class ::ShadowModule;       // Likewise for its colour panel.
    friend class ::TextureModule;      // Marks the sheets that use the texture changed.
    friend class ::TimelineKeyWidget;  // Collects the selected nodes' key frames.
    friend class ::TimelineMarkWidget; // Places the key marks on the timeline rows.
    std::list<Attribute *> m_attributes;
    bool m_playing;
    bool m_changed; // Set by AttributesChanged; Node::Animate redraws (and rebuilds the matrices) and clears it.
    bool m_stepped; // Set by Advance and SetFrame, never cleared here.
    std::wstring m_sequenceName; // The sequence last played.
};

}
