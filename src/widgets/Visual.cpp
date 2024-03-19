#include "Visual.h"

namespace mc_rtc::magnum
{

Visual::Visual(Client & client, const ElementId & id, McRtcGui & gui) : Widget(client, id, gui) {}

void Visual::data(const rbd::parsers::Visual & visual, const sva::PTransformd & pos)
{
  if(visual_.geometry.type != visual.geometry.type) { typeChanged_ = true; }
  visual_ = visual;
  // Bake the visual origin in the position
  pos_ = visual.origin * pos;
}

void Visual::draw2D()
{
  if(object_)
  {
    bool show = !object_->hidden();
    if(ImGui::Checkbox(label(fmt::format("Show {}", id.name)).c_str(), &show)) { object_->hidden(!show); }
  }
}

void Visual::draw3D()
{
  using Geometry = rbd::parsers::Geometry;
  using Type = rbd::parsers::Geometry::Type;
  auto handleMesh = [&]()
  {
    const auto & in = boost::get<Geometry::Mesh>(visual_.geometry.data);
    auto path = convertURI(in.filename, "");
    if(path != mesh_)
    {
      mesh_ = path;
      object_ = gui_.loadMesh(path.string(), color(visual_.material));
    }
    const auto & scaleV = in.scaleV;
    object_->setTransformation(convert(pos_)
                               * Matrix4::scaling({static_cast<float>(scaleV.x()), static_cast<float>(scaleV.y()),
                                                   static_cast<float>(scaleV.z())}));
  };
  auto handleBox = [&]()
  {
    const auto & box = boost::get<Geometry::Box>(visual_.geometry.data);
    if(!object_)
    {
      object_ = gui_.makeBox(translation(pos_), rotation(pos_), translation(box.size), color(visual_.material));
    }
    auto & b = static_cast<Box &>(*object_);
    b.pose(Matrix4::from(rotation(pos_), translation(pos_)));
    b.size(translation(box.size));
    b.color(color(visual_.material));
  };
  auto handleCylinder = [&]()
  {
    const auto & cyl = boost::get<Geometry::Cylinder>(visual_.geometry.data);
    const auto & start = sva::PTransformd{Eigen::Vector3d{0, 0, -cyl.length / 2}} * pos_;
    const auto & end = sva::PTransformd{Eigen::Vector3d{0, 0, cyl.length / 2}} * pos_;
    gui_.drawArrow(translation(start), translation(end), 2 * cyl.radius, 0.0f, 0.0f, color(visual_.material));
  };
  auto handleSphere = [&]()
  {
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
  auto handleSuperellipsoid = [&]()
  {
    const auto & se = boost::get<rbd::parsers::Geometry::Superellipsoid>(visual_.geometry.data);
    if(se.epsilon1 != 1.0 || se.epsilon2 != 1.0)
    {
      static bool warned_once = false;
      if(!warned_once)
      {
        mc_rtc::log::error("mc-rtc-magnum only support ellipsoid display");
        warned_once = true;
      }
      return;
    }
    if(!object_)
    {
      object_ = gui_.makeEllipsoid(translation(pos_), rotation(pos_), translation(se.size), color(visual_.material));
    }
    auto & e = static_cast<Ellipsoid &>(*object_);
    e.pose(Matrix4::from(rotation(pos_), translation(pos_)));
    e.size(translation(se.size));
    e.color(color(visual_.material));
  };
  if(object_ && typeChanged_) { object_.reset(); }
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
    case Type::SUPERELLIPSOID:
      handleSuperellipsoid();
      break;
    default:
      break;
  }
}

} // namespace mc_rtc::magnum
