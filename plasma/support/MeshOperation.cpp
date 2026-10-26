// Applies and reverses mesh edits by restoring the recorded mesh state.
#include "MeshInterface.h"

void MeshOperation::execute()
{
    m_mesh->restoreState(m_after);
}

void MeshOperation::undo()
{
    m_mesh->restoreState(m_before);
}
