#pragma once

#include "Camera.h"

#include <Magnum/Shaders/MeshVisualizerGL.h>

#include <mc_rtc/gui/types.h>

#include <memory>

namespace mc_rtc::magnum
{

class CommonDrawable : public Object3D, public SceneGraph::Drawable3D
{
public:
  inline CommonDrawable(Object3D * parent, SceneGraph::DrawableGroup3D * group)
  : Object3D{parent}, SceneGraph::Drawable3D{*this, group}, group_(group)
  {
  }

  inline bool hidden() const noexcept { return hidden_; }

  void hidden(bool hidden) noexcept;

  virtual void draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera) = 0;

  virtual void alpha(float alpha) noexcept = 0;

  inline void draw(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera) final
  {
    if(!hidden_) { draw_(transformationMatrix, camera); }
  }

private:
  SceneGraph::DrawableGroup3D * group_;
  bool hidden_ = false;
};

class ColoredDrawable : public CommonDrawable
{
public:
  explicit ColoredDrawable(Object3D * parent,
                           SceneGraph::DrawableGroup3D * group,
                           Shaders::PhongGL & shader,
                           GL::Mesh & mesh,
                           const Color4 & color,
                           const Containers::Optional<Color4> & ambient = Containers::NullOpt);

  inline void color(const Color4 & color) noexcept { color_ = color; }

  inline void ambient(const Color4 & ambient) noexcept
  {
    ambient_ = ambient;
    ambient_.a() = 0.0;
  }

  inline void colorWithAmbient(const Color4 & color) noexcept
  {
    color_ = color;
    if(color_.r() == color_.g() && color_.g() == color_.a())
    {
      ambient_ = 0x000000ff_rgbaf;
      ambient_.a() = 0.0f;
    }
    else
    {
      ambient_ = Color4::fromHsv({color_.hue(), 1.0f, 0.3f}, 0.0f);
    }
  }

  inline void alpha(float alpha) noexcept override
  {
    color_.a() = alpha;
    ambient_.a() = 0.0f;
  }

protected:
  void draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera) override;

  Shaders::PhongGL & shader_;
  GL::Mesh & mesh_;
  Color4 color_;
  Color4 ambient_;
};

class TexturedDrawable : public CommonDrawable
{
public:
  explicit TexturedDrawable(Object3D * object,
                            SceneGraph::DrawableGroup3D * group,
                            Shaders::PhongGL & shader,
                            GL::Mesh & mesh,
                            GL::Texture2D & texture);

  inline void alpha(float alpha) noexcept final { alpha_ = alpha; }

private:
  void draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera) override;

  Shaders::PhongGL & shader_;
  GL::Mesh & mesh_;
  GL::Texture2D & texture_;
  float alpha_ = 1.0f;
};

class Sphere : public ColoredDrawable
{
public:
  explicit Sphere(Object3D * parent,
                  SceneGraph::DrawableGroup3D * group,
                  Shaders::PhongGL & shader,
                  GL::Mesh & mesh,
                  Vector3 center,
                  float radius,
                  Color4 color);

  inline void center(const Vector3 & c) noexcept
  {
    center_ = c;
    update();
  }

  inline void radius(float radius) noexcept
  {
    radius_ = radius;
    update();
  }

private:
  Vector3 center_;
  float radius_;
  void update() noexcept;
};

using SpherePtr = std::shared_ptr<Sphere>;

class Box : public ColoredDrawable
{
public:
  explicit Box(Object3D * parent,
               SceneGraph::DrawableGroup3D * group,
               Shaders::PhongGL & shader,
               GL::Mesh & mesh,
               Matrix4 pose,
               Vector3 size,
               Color4 color);

  inline void pose(const Matrix4 & pose) noexcept
  {
    pose_ = pose;
    update();
  }

  inline void size(const Vector3 & size) noexcept
  {
    size_ = size;
    update();
  }

private:
  Matrix4 pose_;
  Vector3 size_;
  void update() noexcept;
};

using BoxPtr = std::shared_ptr<Box>;

using Ellipsoid = Box;
using EllipsoidPtr = std::shared_ptr<Ellipsoid>;

class PolyhedronDrawable : public CommonDrawable
{
public:
  using CommonDrawable::CommonDrawable;

  bool draw_wireframe() const noexcept { return draw_wireframe_; }
  void draw_wireframe(bool b) noexcept { draw_wireframe_ = b; }

  void draw_(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera) override;

  void alpha(float alpha) noexcept override;

  void update(const std::vector<Eigen::Vector3d> & vertices,
              const std::vector<std::array<size_t, 3>> & indices,
              const std::vector<mc_rtc::gui::Color> & colors,
              const mc_rtc::gui::PolyhedronConfig & config);

private:
  struct Vertex
  {
    Vector3 position;
    Vector3 normal;
    Color4 color;
  };
  bool draw_wireframe_ = false;
  Containers::Array<Vertex> vertices_;
  Containers::Array<uint16_t> indices_;
  GL::Buffer vertices_buffer_;
  GL::Buffer indices_buffer_;
  GL::Mesh mesh_;
  Color4 default_color_;
  Shaders::PhongGL shader_ = Shaders::PhongGL{Shaders::PhongGL::Configuration{}.setFlags(
      Shaders::PhongGL::Flag::VertexColor | Shaders::PhongGL::Flag::DoubleSided)};
  Shaders::MeshVisualizerGL3D mesh_shader_ = Shaders::MeshVisualizerGL3D{
      Shaders::MeshVisualizerGL3D::Configuration{}.setFlags(Shaders::MeshVisualizerGL3D::Flag::Wireframe)};
};

using PolyhedronPtr = std::shared_ptr<PolyhedronDrawable>;

} // namespace mc_rtc::magnum
