// Movie creation helper used by workspace initialization and movie population.
#include "plasma/Engine.h"
#include "plasma/Node.h"
#include "plasma/Transformation.h"
#include "plasma/Display.h"
#include "plasma/GLEngine.h"

using namespace plasma;

static Node *populate(Engine *engine, const std::wstring &name)
{
    engine->createMovie(name);
    return 0;
}

#pragma comment(linker, "/INCLUDE:_StandIn_WorkspaceMovie")
extern "C" int StandIn_WorkspaceMovie(const std::wstring *name)
{
    Engine *engine = new GLEngine;
    populate(engine, *name);
    delete engine;
    return 0;
}
