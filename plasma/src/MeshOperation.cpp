#include "MeshOperation.h"
#include "plasma/Texture.h"

MeshOperation::MeshOperation(DesignSheet *sheet, const std::string &name, MeshInterface *mesh, const MeshState &before,
                             const MeshState &after)
    : Operation(sheet, name, true),
      m_before(before),
      m_after(after),
      m_mesh(mesh)
{}

// Do and Undo are implemented in support/MeshOperation.cpp.
void MeshOperation::forgetTexture(plasma::Texture *texture)
{
    for (int i = 0; i < m_after.m_texture.size(); ++i) {
        int &id = m_after.m_texture.m_values[i];
        if (texture->m_id == id)
            id = -1;
    }
    for (int i = 0; i < m_after.m_strokeTexture.size(); ++i) {
        int &id = m_after.m_strokeTexture.m_values[i];
        if (texture->m_id == id)
            id = -1;
    }
    for (int i = 0; i < m_before.m_texture.size(); ++i) {
        int &id = m_before.m_texture.m_values[i];
        if (texture->m_id == id)
            id = -1;
    }
    for (int i = 0; i < m_before.m_strokeTexture.size(); ++i) {
        int &id = m_before.m_strokeTexture.m_values[i];
        if (texture->m_id == id)
            id = -1;
    }
}
