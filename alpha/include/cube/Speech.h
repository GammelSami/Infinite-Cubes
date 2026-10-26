#pragma once
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "cube/QuestText.h"
#include "cube/Region.h"
#include "plasma/Vector.h"

namespace cube {

class World;

// A page of coloured words. Speech's generator appends to the last page.
class Dialog {
public:
    struct Phrase {
        Phrase(std::wstring text, plasma::Vector<4, float> color)
            : text(text),
              color(color)
        {}
        std::wstring text;
        plasma::Vector<4, float> color;
    };
    void write(std::wstring text, const plasma::Vector<4, float> &color);
    std::list<Phrase> phrases;
};

class Speech {
public:
    enum Expression {
        TalkTest1 = 1,
        TalkTest2,
        PetFood,
        Explored,
        KillTest,
        CreatureStrong,
        CreatureStupid,
        CreatureClever,
        TakeCare1,
        TakeCare2,
        TakeCare3,
        TakeCare4
    };
    struct Context {
        std::set<std::wstring> tags;
        std::map<std::wstring, std::wstring> variables;
    };
    struct Name {
        std::map<std::wstring, std::wstring> forms;
        std::list<std::wstring> tags;
    };
    struct Quest {
        std::map<std::wstring, QuestText> texts;
    };

    Speech(); // Reads dict_en.xml from data4.db.
    virtual ~Speech();
    bool generate(std::vector<Dialog> &dialogs, QuestText *text, QuestTextNode *node,
                  const std::set<std::wstring> &tags, const std::map<std::wstring, std::wstring> &variables);
    void generateDialogs(const std::wstring &key, const Context *context, std::vector<Dialog> &dialogs);

    void generateQuest(World *world, int expression, const plasma::Vector<3, int> &location,
                       std::vector<Dialog> &dialogs);
    std::wstring landscapeName(World *world, int x, int y);
    std::wstring regionName(World *world, Region::Record *region);
    std::wstring cellName(World *world, Region::Cell *cell, int x, int y);
    std::wstring cellName(World *world, int x, int y);
    // Returns the requested dictionary form, such as singular or item, or an empty string.
    std::wstring getForm(const std::wstring &key, std::wstring form)
    {
        std::map<std::wstring, Name>::iterator it = m_names.find(key);
        static Name none;
        if (it == m_names.end())
            return L"";
        std::map<std::wstring, std::wstring>::iterator text = it->second.forms.find(form);
        if (text == it->second.forms.end())
            return L"";
        return text->second;
    }
    // Returns the dictionary entry for a key, or an empty entry.
    const Name &getName(const std::wstring &key)
    {
        std::map<std::wstring, Name>::iterator it = m_names.find(key);
        static Name none;
        if (it == m_names.end())
            return none;
        return it->second;
    }

    std::map<std::wstring, Name> m_names;
    std::map<std::wstring, std::wstring> m_landscapeNormal;
    std::map<std::wstring, std::wstring> m_landscapeIn;
    std::map<std::wstring, std::wstring> m_landscapeTo;
    std::map<std::wstring, std::wstring> m_landscapeThe;
    std::map<std::wstring, std::wstring> m_unusedMap; // constructed and destroyed
    std::map<std::wstring, std::wstring> m_sentences;
    std::map<std::wstring, Quest *> m_quests;     // owns values
    std::map<std::wstring, QuestText *> m_speech; // owns values
    std::map<int, std::wstring> m_expressionKeys;
};

}
