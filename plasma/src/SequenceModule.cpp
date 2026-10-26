#include "SequenceModule.h"
#include "OnlineModule.h"
#include "TextModule.h"
#include "plasma/Engine.h"
#include "plasma/GenericWidget.h"
#include "FormatModule.h"
#include <sstream>
#include <cstdlib>

#include "plasma/ListWidget.h"
#include "Operations.h"
#include "plasma/Edit.h"
#include "NodeInterface.h"
#include "plasma/Attribute.h"
#include "plasma/Sequence.h"

void SequenceModule::update()
{
    plasma::ListWidget *list =
        dynamic_cast<plasma::ListWidget *>(m_workspace->m_engine.findWidget(L"sequence:listbox:list"));
    plasma::Widget *source = m_workspace->m_engine.findWidget(L"listitem");
    if (list && source) {
        list->clear();
        if (m_workspace->getOpenDesignSheet()) {
            std::set<std::wstring> names;
            m_workspace->getOpenDesignSheet()->getRoot()->getSequenceNames(names);
            for (std::set<std::wstring>::iterator it = names.begin(); it != names.end(); ++it) {
                plasma::Widget *item = list->addItem(source, true);
                item->setCaption(*it, true);
                item->connect(plasma::WIDGET_EVENT_POINTER_PRESSED, this, &SequenceModule::chooseSequence, true, true);
                item->connect(plasma::WIDGET_EVENT_POINTER_DOUBLE_CLICKED, this, &SequenceModule::chooseSequence, true,
                              true);
                item->connect(plasma::WIDGET_EVENT_RIGHT_BUTTON_PRESSED, this, &SequenceModule::editSequence, true,
                              true);
                if (it == names.begin()) {
                    m_sequenceName = *it;
                    item->getNode()->setChecked(true, false);
                }
            }
            list->layout();
        }
    }
}

void SequenceModule::chooseSequence(plasma::Widget *sender)
{
    m_sequenceName = sender->getNode()->getText();
    m_workspace->m_animationModule->refresh(true);
}

void SequenceModule::editSequence(plasma::Widget *sender)
{
    m_workspace->getGuiNode()->addChild(m_editWidget->getNode());
    m_editWidget->setViewPosition(sender->getContentPosition(), true);
    m_editWidget->resize(sender->getViewSize());
    m_editWidget->getNode()->setText(sender->getNode()->getText());
    plasma::Edit *edit = dynamic_cast<plasma::Edit *>(m_editWidget->getNode()->findWidget(L"edit"));
    if (edit)
        edit->selectAll();
    m_editSequenceName = m_editWidget->getNode()->getText();
}

void TimelineKeyWidget::collectKeyFrames()
{
    m_keyFrames.clear();
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->second->m_selected && it->first->isShown()) {
                if (it->first->getShape()) {
                    for (std::list<plasma::Attribute *>::iterator attribute =
                             it->first->getShape()->m_attributes.begin();
                         attribute != it->first->getShape()->m_attributes.end(); ++attribute) {
                        plasma::Sequence *sequence =
                            (*attribute)->findSequence(m_workspace->getSequenceModule()->getSequenceName());
                        if (sequence) {
                            for (int i = 0; i < sequence->count(); i++) {
                                const plasma::Key &key = (*sequence)[i];
                                m_keyFrames.insert(key.frame);
                            }
                        }
                    }
                }
                if (it->first->transformation()) {
                    for (std::list<plasma::Attribute *>::iterator attribute =
                             it->first->transformation()->m_attributes.begin();
                         attribute != it->first->transformation()->m_attributes.end(); ++attribute) {
                        plasma::Sequence *sequence =
                            (*attribute)->findSequence(m_workspace->getSequenceModule()->getSequenceName());
                        if (sequence) {
                            for (int i = 0; i < sequence->count(); i++) {
                                const plasma::Key &key = (*sequence)[i];
                                m_keyFrames.insert(key.frame);
                            }
                        }
                    }
                }
            }
        }
    }
}

void TimelineMarkWidget::collectKeyMarks()
{
    m_marks.clear();
    if (m_workspace->getOpenDesignSheet()) {
        DesignSheet *sheet = m_workspace->getOpenDesignSheet();
        std::map<plasma::Node *, NodeInterface *>::iterator it;
        if (sheet->isBusy())
            it = sheet->m_sheetNodes.end();
        else
            it = sheet->m_sheetNodes.begin();
        for (; it != m_workspace->getOpenDesignSheet()->m_sheetNodes.end(); ++it) {
            if (it->first->isShown() && it->second->m_selected) {
                float y = 0.0f;
                addKeyMarks(m_workspace->m_animationModule->m_tracks[0], y, it->first->m_transformation);
                addKeyMarks(m_workspace->m_animationModule->m_tracks[1], y, it->first->m_display);
                addKeyMarks(m_workspace->m_animationModule->m_tracks[2], y, it->first->m_shape);
                addKeyMarks(m_workspace->m_animationModule->m_tracks[3], y, it->first->m_shape);
            }
        }
        m_changed = true;
    }
}

void TimelineMarkWidget::addKeyMarks(const TimelineTrack &track, float &y, plasma::Keyable *keyable)
{
    if (keyable) {
        float headerCenter = track.m_header->getViewHeight() * 0.5 + y;
        y += track.m_header->getViewHeight();
        for (std::list<TimelineRow>::const_iterator row = track.m_rows.begin(); row != track.m_rows.end(); ++row) {
            float rowHeight = row->m_widget->getViewHeight();
            for (std::list<plasma::Attribute *>::iterator attribute = keyable->m_attributes.begin();
                 attribute != keyable->m_attributes.end(); ++attribute) {
                if ((*attribute)->m_name == row->m_attributeName) {
                    plasma::Sequence *sequence =
                        (*attribute)->findSequence(m_workspace->m_sequenceModule->m_sequenceName);
                    if (sequence) {
                        for (int i = 0; i < sequence->count(); i++) {
                            m_marks.push_back(
                                plasma::Vector<2, float>((float)(*sequence)[i].frame, y + rowHeight * 0.5));
                            m_marks.push_back(plasma::Vector<2, float>((float)(*sequence)[i].frame, headerCenter));
                        }
                    }
                }
            }
            y += rowHeight;
        }
    }
}
