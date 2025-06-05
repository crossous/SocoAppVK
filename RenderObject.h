#pragma once

#include "Mesh.h"
#include "Shader.h"
#include "Transform.hpp"

class RenderObject
{
public:
    RenderObject(Mesh* mesh, Shader* shader);

    
    
private:
    Mesh* mMesh;
    Shader* mShader;
    Transform mTransform;
};
