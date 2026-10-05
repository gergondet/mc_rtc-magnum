#include "Primitives.h"

#include "Corrade/Containers/GrowableArray.h"
#include "Magnum/MeshTools/GenerateNormals.h"
#include "widgets/utils.h"

namespace mc_rtc::magnum
{

static void set_children_hidden(Object3D * object, bool hidden)
{
  for(auto & c : object->children())
  {
    auto * cd = dynamic_cast<CommonDrawable *>(&c);
    if(cd) { cd->hidden(hidden); }
    else
    {
      set_children_hidden(&c, hidden);
    }
  }
}

void CommonDrawable::hidden(bool hidden) noexcept
{
  if(hidden_ == hidden) { return; }
  hidden_ = hidden;
  if(hidden) { group_->remove(*this); }
  else
  {
    group_->add(*this);
  }
  set_children_hidden(this, hidden);
}

ColoredDrawable::ColoredDrawable(Object3D * object,
                                 SceneGraph::DrawableGroup3D * group,
                                 Shaders::PhongGL & shader,
                                 GL::Mesh & mesh,
                                 const Color4 & color,
                                 const Containers::Optional<Color4> & ambient)
: CommonDrawable(object, group), shader_(shader), mesh_(mesh), color_(color)
{
  if(ambient) { ambient_ = *ambient; }
  else
  {
    colorWithAmbient(color_);
  }
  ambient_.a() = 0.0f;
}

void ColoredDrawable::draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera)
{
  shader_.setDiffuseColor(Color4(color_))
      .setAmbientColor(ambient_)
      .setTransformationMatrix(transformationMatrix)
      .setNormalMatrix(transformationMatrix.normalMatrix())
      .setProjectionMatrix(camera.projectionMatrix())
      .draw(mesh_);
}

TexturedDrawable::TexturedDrawable(Object3D * object,
                                   SceneGraph::DrawableGroup3D * group,
                                   Shaders::PhongGL & shader,
                                   GL::Mesh & mesh,
                                   GL::Texture2D & texture)
: CommonDrawable{object, group}, shader_(shader), mesh_(mesh), texture_(texture)
{
}

void TexturedDrawable::draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera)
{
  shader_.setTransformationMatrix(transformationMatrix)
      .setNormalMatrix(transformationMatrix.normalMatrix())
      .setProjectionMatrix(camera.projectionMatrix())
      .bindDiffuseTexture(texture_)
      .setDiffuseColor(Color4(1.0, 1.0, 1.0, alpha_))
      .draw(mesh_);
}

Sphere::Sphere(Object3D * parent,
               SceneGraph::DrawableGroup3D * group,
               Shaders::PhongGL & shader,
               GL::Mesh & mesh,
               Vector3 center,
               float radius,
               Color4 color)
: ColoredDrawable(parent, group, shader, mesh, color), center_(center), radius_(radius)
{ update(); }

void Sphere::update() noexcept
{ setTransformation(Matrix4::from(Matrix3{Math::IdentityInit, radius_}, center_)); }

Box::Box(Object3D * parent,
         SceneGraph::DrawableGroup3D * group,
         Shaders::PhongGL & shader,
         GL::Mesh & mesh,
         Matrix4 pose,
         Vector3 size,
         Color4 color)
: ColoredDrawable(parent, group, shader, mesh, color), pose_(pose), size_(size)
{ update(); }

void Box::update() noexcept
{ setTransformation(pose_ * Matrix4::scaling(size_ / 2.0)); }

void PolyhedronDrawable::draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera)
{
  if(vertices_.isEmpty()) { return; }
  if(draw_wireframe_)
  {
    mesh_shader_.setColor(default_color_)
        .setWireframeColor(0x000000ff_rgbaf)
        .setWireframeWidth(2.0f)
        .setViewportSize(Vector2{GL::defaultFramebuffer.viewport().size()})
        .setTransformationMatrix(transformationMatrix)
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(mesh_);
  }
  else
  {
    shader_.setShininess(200.0f)
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(mesh_);
  }
}

void PolyhedronDrawable::alpha(float) noexcept {}

void PolyhedronDrawable::update(const std::vector<Eigen::Vector3d> & vertices,
                                const std::vector<std::array<size_t, 3>> & indices,
                                const std::vector<mc_rtc::gui::Color> & colors,
                                const mc_rtc::gui::PolyhedronConfig & config)
{
  Containers::arrayResize(vertices_, vertices.size());
  default_color_ = convert(config.triangle_color);
  for(size_t i = 0; i < vertices.size(); ++i)
  {
    const auto & color = i < colors.size() ? colors[i] : config.triangle_color;
    vertices_[i] = {translation(vertices[i]), {}, convert(color)};
  }
  auto vertices_view = Containers::StridedArrayView1D<Vector3>(vertices_, &vertices_[0].position,
                                                               Containers::arraySize(vertices_), sizeof(Vertex));
  auto normals_view = Containers::StridedArrayView1D<Vector3>(vertices_, &vertices_[0].normal,
                                                              Containers::arraySize(vertices_), sizeof(Vertex));
  Containers::arrayResize(indices_, 3 * indices.size());
  for(size_t i = 0; i < indices.size(); ++i)
  {
    indices_[3 * i + 0] = static_cast<uint16_t>(indices[i][0]);
    indices_[3 * i + 1] = static_cast<uint16_t>(indices[i][1]);
    indices_[3 * i + 2] = static_cast<uint16_t>(indices[i][2]);
  }
  indices_buffer_.setData(indices_, GL::BufferUsage::DynamicDraw);

  MeshTools::generateSmoothNormalsInto(indices_, vertices_view, normals_view);
  vertices_buffer_.setData(vertices_, GL::BufferUsage::DynamicDraw);

  mesh_.setCount(Containers::arraySize(indices_));
  mesh_.addVertexBuffer(vertices_buffer_, 0, Shaders::PhongGL::Position{}, Shaders::PhongGL::Normal{},
                        Shaders::PhongGL::Color4{});
  static_assert(std::is_same_v<uint16_t, unsigned short>);
  mesh_.setIndexBuffer(indices_buffer_, 0, MeshIndexType::UnsignedShort);
}

} // namespace mc_rtc::magnum
