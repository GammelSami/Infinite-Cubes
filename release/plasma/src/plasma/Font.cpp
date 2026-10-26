#include "plasma/Font.h"
#include "plasma/Engine.h"
#include <utility>
#include "plasma/EditFilter.h"

namespace plasma {

Font::Font(Engine *engine, const std::wstring &name)
    : NamedObject(engine, name),
      m_engineEntry(),
      m_size(0.0f),
      m_spacing(0.0f),
      m_transient(false)
{
    engine->m_fontList.push_back(this);
    m_engineEntry = --engine->m_fontList.end();
    // A font gets an id one above the largest in the engine's id map.
    int largest = -1;
    for (std::map<int, Font *>::const_iterator entry = engine->m_fontsById.begin(); entry != engine->m_fontsById.end();
         ++entry) {
        if (entry->first > largest)
            largest = entry->first;
    }
    m_id = largest + 1;
    engine->m_fontsById[std::move(m_id)] = this;
}

Font::~Font() {}

void Font::renderText(const std::wstring &text, const Vector<2, float> &position, const float *fill,
                      const float *outline, int flags)
{}

float Font::getKerning(unsigned short left, unsigned short right)
{
    std::map<std::pair<unsigned short, unsigned short>, float>::iterator pair =
        m_kerning.find(std::make_pair(left, right));
    if (pair == m_kerning.end())
        return 0.0f;
    return pair->second;
}

}
