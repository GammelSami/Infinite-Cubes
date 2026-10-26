#include "plasma/Movie.h"
#include "plasma/EditFilter.h"

namespace plasma {

Movie::Movie(Engine *engine, const std::wstring &name)
    : NamedObject(engine, name),
      m_unused50(0)
{}

void Movie::addEntry(const std::wstring &name, int value)
{
    for (int i = 0; i < (int)m_entries.size(); i++) {
        if (m_entries[i].value == value && m_entries[i].name == name)
            return;
    }
    m_entries.push_back(Entry(name, value));
}

}
