#pragma once

#include "../Texture.h"
#include <cstdint>
#include <glad/glad.h>

namespace Goonya {
namespace Graphics {

class GLTextureBase{
public:
    ~GLTextureBase(){
        glDeleteTextures(1, &texture_id);
    }
    void bind(uint32_t binding) const noexcept{
        glBindTextureUnit(binding, texture_id);
    }
protected:
    friend class OpenGLGraphicsAPI;
    friend class GLMaterial;
    GLTextureBase(GLuint texture_id): texture_id(texture_id){}
    
    GLuint texture_id;
};


class GLTexture: public Texture, public GLTextureBase {
public:
    ~GLTexture() = default;
    virtual void bind(uint32_t binding) const noexcept override{
        this->GLTextureBase::bind(binding);
    }
protected:
    friend class OpenGLGraphicsAPI;
    friend class GLMaterial;
    GLTexture(GLuint texture_id): GLTextureBase(texture_id){}
};

class GLTextureCube: public TextureCube, public GLTextureBase {
public:
    ~GLTextureCube() = default;
    virtual void bind(uint32_t binding) const noexcept override{
        this->GLTextureBase::bind(binding);
    }
protected:
    friend class OpenGLGraphicsAPI;
    GLTextureCube(GLuint texture_id): GLTextureBase(texture_id){}
};

class GLTexture2D: public Texture2D, public GLTextureBase  {
public:
    ~GLTexture2D() = default;
    virtual void bind(uint32_t binding) const noexcept override{
        this->GLTextureBase::bind(binding);
    }
    virtual std::tuple<uint32_t, uint32_t> get_size() const noexcept override{
        return size;
    }
protected:
    friend class OpenGLGraphicsAPI;
    GLTexture2D(GLuint texture_id, uint32_t width, uint32_t height): GLTextureBase(texture_id), size(width, height){}
private:
    std::tuple<uint32_t, uint32_t> size;
};


}
}