#pragma once
#include <list>
#include <set>
#include <string>

namespace cube {

bool isPunctuation(wchar_t c);

// A parsed word or group.
struct QuestTextNode {
    QuestTextNode();
    virtual ~QuestTextNode();

    enum Kind {
        Text = 0,
        Punctuation = 1,
        Choice = 2,
        Optional = 3
    };
    std::set<std::wstring> conditions;
    std::set<std::wstring> allConditions;
    std::set<std::wstring> styles;
    std::wstring word;
    QuestTextNode *parent;               // not owned
    std::list<QuestTextNode *> children; // owned
    Kind kind;
};

class QuestText {
public:
    QuestText();
    virtual ~QuestText();
    void gatherTags(QuestTextNode *node);
    bool parse(std::wstring text);

    QuestTextNode *m_root; // owned
    std::set<std::wstring> m_nodeTags;
};

}
