// IndexBuffer is the API-neutral index-buffer handle created by Graphics. D3D11IndexBuffer is its implementation.
#pragma once

namespace gfx {

class IndexBuffer {
public:
    virtual ~IndexBuffer() {}

    // D3D11IndexBuffer binds the buffer with its format.
    virtual bool bind() { return true; }

    virtual void prepare() {}
};

}
