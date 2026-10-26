// QuestText parses dialog text without macro arguments or a separate alternative-text vector.
#include "cube/QuestText.h"
#include "cube/DefaultBlocks.h"
#include "cube/World.h"

cube::QuestText::QuestText()
    : m_root(nullptr)
{}

cube::QuestTextNode::QuestTextNode()
    : parent(nullptr),
      kind(Text)
{}

cube::QuestText::~QuestText()
{
    if (m_root) {
        delete m_root;
    }
}

cube::QuestTextNode::~QuestTextNode()
{
    for (std::list<QuestTextNode *>::iterator it = children.begin(); it != children.end(); ++it) {
        if (*it) {
            delete *it;
        }
    }
}

void cube::QuestText::gatherTags(QuestTextNode *node)
{
    if (!node) {
        return;
    }
    m_nodeTags.insert(node->conditions.begin(), node->conditions.end());
    node->allConditions = node->conditions;
    for (std::list<QuestTextNode *>::iterator it = node->children.begin(); it != node->children.end(); ++it) {
        gatherTags(*it);
        node->allConditions.insert((*it)->allConditions.begin(), (*it)->allConditions.end());
    }
}

// Whether c ends a word and is kept as a node of its own.
bool cube::isPunctuation(wchar_t c)
{
    return c == L'.' || c == L':' || c == L'-' || c == L',' || c == L';' || c == L'!' || c == L'?' || c == L'/' ||
           c == L'(' || c == L')';
}

bool cube::QuestText::parse(std::wstring text)
{
    if (m_root) {
        delete m_root;
        m_root = nullptr;
    }
    m_root = new QuestTextNode;
    m_nodeTags.clear();

    // The open nodes, innermost last.
    std::list<QuestTextNode *> open;
    open.push_back(m_root);

    size_t length = text.size();
    // The index of the last delimiter; a word lies between two.
    int last = -1;
    bool balanced = true;
    for (int i = 0; i <= (int)length; i++) {
        if (text[i] == L' ' || text[i] == L'\n' || text[i] == L'\t' || text[i] == 0 || isPunctuation(text[i]) ||
            text[i] == L'{' || text[i] == L'}' || text[i] == L'[' || text[i] == L']' || text[i] == L'|') {
            if (text[i] == L'{') {
                QuestTextNode *choice = new QuestTextNode;
                choice->kind = QuestTextNode::Choice;
                open.back()->children.push_back(choice);
                choice->parent = open.back();
                open.push_back(choice);

                QuestTextNode *alternative = new QuestTextNode;
                open.back()->children.push_back(alternative);
                alternative->parent = open.back();
                open.push_back(alternative);
            }
            if (text[i] == L'[') {
                QuestTextNode *optional = new QuestTextNode;
                optional->kind = QuestTextNode::Optional;
                open.back()->children.push_back(optional);
                optional->parent = open.back();
                open.push_back(optional);
            }
            if (i > last + 1 || isPunctuation(text[i])) {
                std::wstring word = text.substr(last + 1, i - last - 1);
                if (word[0] == L'#') {
                    open.back()->conditions.insert(word);
                } else if (word[0] == L'$') {
                    open.back()->styles.insert(word);
                } else {
                    QuestTextNode *node = new QuestTextNode;
                    node->word = word;
                    open.back()->children.push_back(node);
                    node->parent = open.back();
                    if (isPunctuation(text[i])) {
                        wchar_t mark[2];
                        mark[0] = text[i];
                        mark[1] = 0;
                        QuestTextNode *punctuation = new QuestTextNode;
                        punctuation->word = mark;
                        punctuation->kind = QuestTextNode::Punctuation;
                        punctuation->parent = open.back();
                        open.back()->children.push_back(punctuation);
                    }
                }
            }
            last = i;
            if (text[i] == L'|') {
                if (open.size() < 2) {
                    balanced = false;
                    break;
                }
                open.pop_back();
                QuestTextNode *alternative = new QuestTextNode;
                open.back()->children.push_back(alternative);
                alternative->parent = open.back();
                open.push_back(alternative);
            }
            if (text[i] == L'}') {
                if (open.size() < 3) {
                    balanced = false;
                    break;
                }
                open.pop_back();
                open.pop_back();
            }
            if (text[i] == L']') {
                if (open.size() < 2) {
                    balanced = false;
                    break;
                }
                open.pop_back();
            }
        }
    }
    if (balanced) {
        gatherTags(m_root);
    }
    return balanced;
}
