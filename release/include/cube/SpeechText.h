// SpeechText parses dialog markup into a tree of SpeechNode records, including conditional text, variables, macros and
// alternatives.
#pragma once
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace cube {

struct SpeechTextNode {
    SpeechTextNode();
    virtual ~SpeechTextNode();

    std::set<std::wstring> conditions;
    std::set<std::wstring> allConditions;
    std::set<std::wstring> styles;
    std::set<std::wstring> speechTags;
    bool inheritTags; // false (constructor)
    std::map<std::wstring, std::wstring> arguments;
    std::wstring word;                    // empty (constructor)
    SpeechTextNode *parent;               // the enclosing node (parser stores it); not owned
    std::list<SpeechTextNode *> children; // owned (destructor deletes them; recurses)
    int kind;                             // 0 (constructor)
};

class SpeechText {
public:
    SpeechText();
    virtual ~SpeechText();

    // Parses text into m_root, replacing any earlier tree and refilling m_nodeTags. Returns false when a closing '|',
    // '}' or ']' has no matching open group.
    bool parse(const std::wstring &text);
    // Collects condition strings from node and its descendants into m_nodeTags and each node's allConditions.
    void gatherTags(SpeechTextNode *node);

public:
    SpeechTextNode *m_root; // the parse tree (parser creates it); owned
    std::vector<SpeechText *> m_alternatives;
    std::list<std::wstring> m_tags;
    std::set<std::wstring> m_nodeTags;
};

}
