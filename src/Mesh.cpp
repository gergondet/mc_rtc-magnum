#include "Mesh.h"

namespace mc_rtc::magnum
{

Mesh::Mesh(Object3D * parent,
           SceneGraph::DrawableGroup3D * group,
           ImportedMesh & data,
           Shaders::Phong & colorShader,
           Shaders::Phong & textureShader,
           Color4 color)
: CommonDrawable(parent, group)
{
  if(data.scene_)
  {
    for(UnsignedInt id : data.scene_->children3D())
    {
      addObject(this, group, data, id, colorShader, textureShader, color);
    }
  }
  else if(!data.meshes_.empty() && data.meshes_[0])
  {
    drawables_.push_back(new ColoredDrawable(this, group, colorShader, *data.meshes_[0], color));
  }
}

void Mesh::addObject(Object3D * parent,
                     SceneGraph::DrawableGroup3D * group,
                     ImportedMesh & data,
                     UnsignedInt idx,
                     Shaders::Phong & colorShader,
                     Shaders::Phong & textureShader,
                     Color4 color)
{
  const auto & objectData = data.objects_[idx];
  if(!objectData)
  {
    return;
  }

  /* Add the object to the scene and set its transformation */
  Object3D * object = nullptr;
  CommonDrawable * drawable = nullptr;

  /* Add a drawable if the object has a mesh and the mesh is loaded */
  if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh && objectData->instance() != -1
     && data.meshes_[objectData->instance()])
  {
    const Int materialId = static_cast<const Trade::MeshObjectData3D *>(objectData.get())->material();

    /* Material not available / not loaded, use a default material */
    if(materialId == -1 || !data.materials_[materialId])
    {
      drawable = new ColoredDrawable{parent, group, colorShader, *data.meshes_[objectData->instance()], color};
    }
    /* Textured material. If the texture failed to load, again just use a
       default colored material. */
    else if(data.materials_[materialId]->hasAttribute(Trade::MaterialAttribute::DiffuseTexture))
    {
      Containers::Optional<GL::Texture2D> & texture = data.textures_[data.materials_[materialId]->diffuseTexture()];
      if(texture)
      {
        drawable = new TexturedDrawable{parent, group, textureShader, *data.meshes_[objectData->instance()], *texture};
      }
      else
      {
        drawable = new ColoredDrawable{parent, group, colorShader, *data.meshes_[objectData->instance()], color};
      }
    }
    /* Color-only material */
    else
    {
      Containers::Optional<Color4> ambient = Containers::NullOpt;
      if(data.materials_[materialId]->hasAttribute(Trade::MaterialAttribute::DiffuseColor))
      {
        auto diffuse = data.materials_[materialId]->diffuseColor();
        if(diffuse != 0xffffffff_rgbaf)
        {
          color = diffuse;
        }
        if(data.materials_[materialId]->hasAttribute(Trade::MaterialAttribute::AmbientColor))
        {
          ambient = data.materials_[materialId]->ambientColor();
        }
      }
      drawable = new ColoredDrawable{parent, group, colorShader, *data.meshes_[objectData->instance()], color, ambient};
    }
  }

  if(!drawable)
  {
    object = new Object3D{parent};
  }
  else
  {
    drawables_.push_back(drawable);
    object = drawable;
  }
  object->setTransformation(objectData->transformation());

  /* Recursively add children */
  for(std::size_t id : objectData->children())
  {
    addObject(object, group, data, id, colorShader, textureShader, color);
  }
}

void Mesh::draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera)
{
  for(auto & d : drawables_)
  {
    d->draw_(transformationMatrix, camera);
  }
}

} // namespace mc_rtc::magnum
