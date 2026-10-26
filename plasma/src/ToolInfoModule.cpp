#include "ToolInfoModule.h"
#include "NodeToolModule.h"
#include "ToolModule.h"
#include "plasma/Node.h"

ToolInfo::ToolInfo() {}

ToolInfo::~ToolInfo() {}

ToolInfoModule::~ToolInfoModule()
{
    for (std::map<std::string, ToolInfo *>::iterator it = m_toolInfos.begin(); it != m_toolInfos.end(); ++it) {
        delete it->second;
    }
}
