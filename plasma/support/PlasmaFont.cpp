// Returns node variable keys by value for the PLX writer.
#include "plasma/PlasmaFont.h"
#include "plasma/Engine.h"
#include "plasma/Node.h"

namespace plasma {

std::list<std::string> nodeVariableKeys(const Node *node)
{
    return node->m_variableKeys;
}

}
