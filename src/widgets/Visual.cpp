#include "Visual.h"

namespace mc_rtc::magnum
{

Visual::Visual(Client & client, const ElementId & id, McRtcGui & gui) : Widget(client, id, gui) {}

void Visual::data(const rbd::parsers::Visual & visual, const sva::PTransformd & pos)
{
  if(visual_.geometry.type != visual.geometry.type)
  {
    typeChanged_ = true;
  }
  visual_ = visual;
  // Bake the visual origin in the position
  pos_ = visual.origin * pos;
}

void Visual::draw2D()
{
  if(object_)
  {
    bool show = !object_->hidden();
    if(ImGui::Checkbox(label(fmt::format("Show {}", id.name)).c_str(), &show))
    {
      object_->hidden(!show);
    }
  }
}

void Visual::draw3D()
{
  using Geometry = rbd::parsers::Geometry;
  using Type = rbd::parsers::Geometry::Type;
  auto handleMesh = [&]() {
    const auto & in = boost::get<Geometry::Mesh>(visual_.geometry.data);
    auto path = convertURI(in.filename, "");
    if(path != mesh_)
    {
      mesh_ = path;
      object_ = gui_.loadMesh(path.string(), color(visual_.material));
    }
    auto scale = static_cast<float>(in.scale);
    object_->setTransformation(convert(pos_) * Matrix4::scaling({scale, scale, scale}));
  };
  auto handleBox = [&]() {
    const auto & box = boost::get<Geometry::Box>(visual_.geometry.data);
    gui_.drawCube(translation(pos_), rotation(pos_), translation(box.size), color(visual_.material));
  };
  auto handleCylinder = [&]() {
    const auto & cyl = boost::get<Geometry::Cylinder>(visual_.geometry.data);
    const auto & start = sva::PTransformd{Eigen::Vector3d{0, 0, -cyl.length / 2}} * pos_;
    const auto & end = sva::PTransformd{Eigen::Vector3d{0, 0, cyl.length / 2}} * pos_;
    gui_.drawArrow(translation(start), translation(end), 2 * cyl.radius, 0.0f, 0.0f, color(visual_.material));
  };
  auto handleSphere = [&]() {
    const auto & sphere = boost::get<rbd::parsers::Geometry::Sphere>(visual_.geometry.data);
    if(!object_)
    {
      object_ = gui_.makeSphere(translation(pos_), static_cast<float>(sphere.radius), color(visual_.material));
    }
    auto & s = static_cast<Sphere &>(*object_);
    s.center(translation(pos_));
    s.radius(static_cast<float>(sphere.radius));
    s.color(color(visual_.material));
  };
  if(object_ && typeChanged_)
  {
    object_.reset();
  }
  typeChanged_ = false;
  switch(visual_.geometry.type)
  {
    case Type::MESH:
      handleMesh();
      break;
    case Type::BOX:
      handleBox();
      break;
    case Type::CYLINDER:
      handleCylinder();
      break;
    case Type::SPHERE:
      handleSphere();
      break;
    default:
      break;
  }
}

} // namespace mc_rtc::magnum
