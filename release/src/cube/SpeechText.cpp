// Text syntax, as the parser reads it: words separated by blanks and punctuation; "{a|b|c}" is a choice (a node of
// kind 2 whose children are the alternatives), "[...]" an optional group (kind 3); a word starting with '#' or '$' is
// not text but is recorded in the enclosing node's conditions or styles set; a word starting with '@' is a macro call
// and may carry arguments "@name(a, key=value, *)".
#include "cube/SpeechText.h"

namespace cube {

// Whether c ends a word and is kept as a node of its own.
static inline bool isPunctuation(wchar_t c)
{
    return c == L'.' || c == L',' || c == L';' || c == L'!' || c == L'?' || c == L':' || c == L'\'' || c == L'-' ||
           c == L'=' || c == L'(' || c == L')' || c == L'/';
}

SpeechText::SpeechText()
    : m_root(nullptr)
{}

SpeechTextNode::SpeechTextNode()
    : inheritTags(false),
      parent(nullptr),
      kind(0)
{}

SpeechText::~SpeechText()
{
    if (m_root) {
        delete m_root;
    }
    for (std::vector<SpeechText *>::iterator it = m_alternatives.begin(); it != m_alternatives.end(); ++it) {
        if (*it) {
            delete *it;
        }
    }
}

SpeechTextNode::~SpeechTextNode()
{
    for (std::list<SpeechTextNode *>::iterator it = children.begin(); it != children.end(); ++it) {
        if (*it) {
            delete *it;
        }
    }
}

void SpeechText::gatherTags(SpeechTextNode *node)
{
    if (!node) {
        return;
    }
    m_nodeTags.insert(node->conditions.begin(), node->conditions.end());
    node->allConditions = node->conditions;
    for (std::list<SpeechTextNode *>::iterator it = node->children.begin(); it != node->children.end(); ++it) {
        gatherTags(*it);
        node->allConditions.insert((*it)->allConditions.begin(), (*it)->allConditions.end());
    }
}

bool SpeechText::parse(const std::wstring &text)
{
    if (m_root) {
        delete m_root;
        m_root = nullptr;
    }
    m_root = new SpeechTextNode;
    m_nodeTags.clear();

    // The open nodes, innermost last.
    std::list<SpeechTextNode *> open;
    open.push_back(m_root);

    size_t length = text.size();
    // The index of the last delimiter; a word lies between two.
    int last = -1;
    bool balanced = true;
    for (int i = 0; i <= (int)length; i++) {
        if (text[i] == L' ' || text[i] == L'\n' || text[i] == L'\t' || text[i] == 0 || isPunctuation(text[i]) ||
            text[i] == L'{' || text[i] == L'}' || text[i] == L'[' || text[i] == L']' || text[i] == L'|') {
            if (text[i] == L'{') {
                SpeechTextNode *choice = new SpeechTextNode;
                choice->kind = 2;
                open.back()->children.push_back(choice);
                choice->parent = open.back();
                open.push_back(choice);

                SpeechTextNode *alternative = new SpeechTextNode;
                open.back()->children.push_back(alternative);
                alternative->parent = open.back();
                open.push_back(alternative);
            }
            if (text[i] == L'[') {
                SpeechTextNode *optional = new SpeechTextNode;
                optional->kind = 3;
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
                    SpeechTextNode *node = new SpeechTextNode;
                    node->word = word;
                    open.back()->children.push_back(node);
                    node->parent = open.back();
                    if (word[0] == L'@') {
                        if (text[i] == L'(') {
                            // The arguments, separated by commas or blanks up to the closing parenthesis. The scan
                            // resumes behind the last character read.
                            int start = i + 1;
                            int resume = i;
                            for (int position = i + 1; position <= (int)length; position++) {
                                if (text[position] == L')' || text[position] == L',' || text[position] == L' ') {
                                    if (position - start > 0) {
                                        std::wstring argument = text.substr(start, position - start);
                                        size_t equals = argument.find_first_of(L"=");
                                        if (equals != std::wstring::npos) {
                                            std::wstring key = L"@" + argument.substr(0, equals);
                                            std::wstring value =
                                                L"@" + argument.substr(equals + 1, argument.size() - equals - 1);
                                            node->arguments[key] = value;
                                        } else if (argument == L"*") {
                                            node->inheritTags = true;
                                        } else {
                                            node->speechTags.insert(L"#" + argument);
                                        }
                                    }
                                    start = position + 1;
                                }
                                resume = position + 1;
                                if (text[position] == L')') {
                                    break;
                                }
                            }
                            if (node->speechTags.empty()) {
                                node->speechTags.insert(L"#default");
                            }
                            i = resume;
                        }
                    }
                    if (isPunctuation(text[i])) {
                        wchar_t mark[2];
                        mark[0] = text[i];
                        mark[1] = 0;
                        SpeechTextNode *punctuation = new SpeechTextNode;
                        punctuation->word = mark;
                        punctuation->kind = 1;
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
                SpeechTextNode *alternative = new SpeechTextNode;
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

}
