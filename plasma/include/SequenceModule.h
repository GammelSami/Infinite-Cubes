// SequenceModule ("module:sequence", sequencemodule.plg): the sequence list of the open design sheet; its current
// sequence name is read by MeshInterface's "set... key" operations. Also declares the animation module and its
// rows.
#pragma once
#include <list>
#include <set>
#include "Module.h"
#include "DesignSheet.h"
#include "plasma/Node.h"
#include "plasma/Vector.h"

namespace plasma {
class Keyable;
}

class SequenceModule : public Module {
public:
    SequenceModule();     // Implemented in support/SequenceModule.cpp.
    virtual void build(); // Implemented in support/SequenceModule.cpp.
    void update();        // Selected document's sequence list.
    void chooseSequence(plasma::Widget *sender);
    void editSequence(plasma::Widget *sender);

    const std::wstring &getSequenceName() const { return m_sequenceName; }

    std::wstring m_sequenceName;     // List-item caption; MeshInterface.
    std::wstring m_editSequenceName; // Caption before renaming.
    plasma::Widget *m_editWidget;    // Floating sequence editor.
};

// One attribute row of a timeline track: the rows' attribute names select the keyable's attributes whose keys are
// marked.
// The animation module displays the selected nodes' attribute keys for SequenceModule::m_sequenceName. It uses
// "animationmodule.plg", "module:animation", "sequence:attribute:keys" and "sequence:time" widgets.
// Workspace::Init leaves the module uncreated, so this panel is currently inactive.
struct TimelineRow {
    unsigned char m_unused00[0x38]; // Unused.
    std::string m_attributeName;    // Compared with Attribute::m_name.
    plasma::Widget *m_widget;       // Its view height is the row's.
};

// A track groups the rows of one keyable of a node (its transformation, display or shape) under a header widget.
struct TimelineTrack {
    unsigned char m_unused00[0x38]; // Unused.
    std::list<TimelineRow> m_rows;
    plasma::Widget *m_header;
};

// The key frames of the selected shown nodes' shapes and transformations.
class TimelineKeyWidget : public plasma::Widget {
public:
    void collectKeyFrames();

    Workspace *m_workspace;
    std::set<int> m_keyFrames;
};

// A mark per key on the timeline rows: for each key, a point on its row and one on its track's header.
class TimelineMarkWidget : public plasma::Widget {
public:
    void collectKeyMarks();

    // Adds the marks of the keyable's attributes named by the track's rows; y is the top of the track and moves below
    // it.
    void addKeyMarks(const TimelineTrack &track, float &y, plasma::Keyable *keyable);

    Workspace *m_workspace;
    int m_unused174; // Unused.
    std::list<plasma::Vector<2, float>> m_marks;
};

class AnimationModule : public Module {
public:
    // Replays the current sequence on the open document from m_frame when restart is set, then updates the sequence's
    // end and widgets.
    void refresh(bool restart)
    {
        if (m_workspace->getOpenDesignSheet()) {
            if (restart && m_workspace->getOpenDesignSheet()) {
                m_workspace->getOpenDesignSheet()->getRoot()->stop();
                m_workspace->getOpenDesignSheet()->getRoot()->play(m_workspace->getSequenceModule()->getSequenceName(),
                                                                   0);
                m_workspace->getOpenDesignSheet()->getRoot()->setFrame(m_frame);
            }
            m_sequenceEnd = m_workspace->getOpenDesignSheet()->getRoot()->getSequenceEnd(
                m_workspace->getSequenceModule()->getSequenceName());
            if (m_keyWidget)
                m_keyWidget->collectKeyFrames();
            if (m_markWidget)
                m_markWidget->collectKeyMarks();
        }
    }

    // The tracks of a node's transformation, display and shape (the shape twice), in CollectKeyMarks' order.
    TimelineTrack m_tracks[4];
    TimelineMarkWidget *m_markWidget;
    TimelineKeyWidget *m_keyWidget;
    int m_unused16C;                          // Unused.
    int m_frame;                              // The frame shown.
    unsigned char m_unused174[0x180 - 0x174]; // Unused.
    int m_sequenceEnd;
};
