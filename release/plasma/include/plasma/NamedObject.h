// plasma::NamedObject: an Object with a wide-string name.
#pragma once
#include <string>
#include "plasma/Object.h"

namespace plasma {

class NamedObject : public Object {
public:
    NamedObject(Engine *engine, const std::wstring &name);
    NamedObject(const NamedObject &other);

    const std::wstring &getName() const { return m_name; }
    void setName(const std::wstring &name) { m_name = name; }

protected:
    std::wstring m_name;
};

}
