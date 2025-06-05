#include "Material.h"

Material::Material(Shader* shader, RenderState* renderState)
    :mShader(shader)
{
    assert(shader != nullptr && "创建材质shader不能为空");

    
}
