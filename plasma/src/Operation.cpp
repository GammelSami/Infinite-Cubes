#include "Operation.h"

Operation::Operation(DesignSheet *sheet, const std::string &name, bool changesDocument)
    : m_sheet(sheet),
      m_name(name),
      m_changesDocument(changesDocument)
{}

Operation::~Operation() {}
