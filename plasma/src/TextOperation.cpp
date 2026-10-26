#include "TextInterface.h"

TextOperation::TextOperation(DesignSheet *sheet, const std::string &name, TextInterface *text, const TextState &before,
                             const TextState &after)
    : Operation(sheet, name, true),
      m_before(before),
      m_after(after),
      m_text(text)
{}

void TextOperation::execute()
{
    m_text->restoreState(m_after);
}

void TextOperation::undo()
{
    m_text->restoreState(m_before);
}
