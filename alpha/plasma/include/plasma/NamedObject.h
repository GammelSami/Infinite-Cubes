// NamedObject adds a name to an engine-owned object.
#pragma once
#include <string>
#include "plasma/Object.h"

namespace plasma {

class NamedObject : public Object {
    friend class PLXStreamReader;
    friend class PLXStreamWriter;

public:
    NamedObject(Engine *engine, const std::wstring &name)
        : Object(engine),
          m_name(name)
    {}

    const std::wstring &getName() const { return m_name; }
    void setName(const std::wstring &name) { m_name = name; }

protected:
    std::wstring m_name;
};

}
