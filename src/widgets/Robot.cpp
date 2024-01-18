#include "Robot.h"

#include <mc_rbdyn/RobotLoader.h>
#include <mc_rbdyn/Robots.h>

#include <mc_rtc/version.h>

namespace mc_rtc::magnum
{

namespace details
{

template<typename T>
void setConfiguration(T & robot, const std::vector<std::vector<double>> & q)
{
  static_assert(std::is_same_v<T, mc_rbdyn::Robot>);
  robot.mbc().q = q;
}

inline mc_rbdyn::RobotModulePtr fromParams(const std::vector<std::string> & p)
{
  mc_rbdyn::RobotModulePtr rm{nullptr};
  if(p.size() == 1) { rm = mc_rbdyn::RobotLoader::get_robot_module(p[0]); }
  if(p.size() == 2) { rm = mc_rbdyn::RobotLoader::get_robot_module(p[0], p[1]); }
  if(p.size() == 3) { rm = mc_rbdyn::RobotLoader::get_robot_module(p[0], p[1], p[2]); }
  if(p.size() > 3)
  {
    mc_rtc::log::warning("Too many parameters provided to load the robot, complain to the developpers of this package");
  }
  return rm;
}

struct RobotCache
{
  inline static std::shared_ptr<mc_rbdyn::Robots> get_robot(const std::vector<std::string> & params)
  {
    if(robots_.count(params)) { use_cnt_[params] += 1; }
    else
    {
      use_cnt_[params] = 1;
      robots_[params] = mc_rbdyn::loadRobot(*fromParams(params));
    }
    auto out = mc_rbdyn::Robots::make(
        [](mc_rbdyn::Robots * robots)
        {
          remove_robot(robots->robot().module().parameters());
          delete robots;
        });
    auto & robot = robots_[params]->robot();
    out->robotCopy(robot, robot.name());
    return out;
  }

  inline static void remove_robot(const std::vector<std::string> & params)
  {
    use_cnt_[params] -= 1;
    if(use_cnt_[params] == 0)
    {
      use_cnt_.erase(params);
      robots_.erase(params);
    }
  }

private:
  inline static std::map<std::vector<std::string>, std::shared_ptr<mc_rbdyn::Robots>> robots_ = {};
  inline static std::map<std::vector<std::string>, size_t> use_cnt_ = {};
};

struct RobotBody : public Object3D, public SceneGraph::Drawable3D
{
  RobotBody(Object3D * parent, SceneGraph::DrawableGroup3D * group)
  : Object3D{parent}, SceneGraph::Drawable3D{*this, group}, group_(group)
  {
  }

  inline void loadVisual(McRtcGui & gui, const std::string & rm_path, const rbd::parsers::Visual & visual)
  {
    std::shared_ptr<CommonDrawable> object;
    using Geometry = rbd::parsers::Geometry;
    switch(visual.geometry.type)
    {
      case Geometry::MESH:
      {
        const auto & mesh = boost::get<rbd::parsers::Geometry::Mesh>(visual.geometry.data);
        auto path = convertURI(mesh.filename, rm_path);
        object = gui.loadMesh(path.string(), color(visual.material), this, group_);
        // FIXME Bake scale in mesh?
        auto scale = mesh.scaleV;
        object->setTransformation(convert(visual.origin) * Matrix4::scaling(translation(scale)));
        break;
      }
      case Geometry::BOX:
      {
        const auto & box = boost::get<rbd::parsers::Geometry::Box>(visual.geometry.data);
        object = gui.makeBox(translation(visual.origin.translation()), convert(visual.origin.rotation()),
                             translation(box.size), color(visual.material), this, group_);
        break;
      }
      case Geometry::CYLINDER:
      {
        // FIXME Need to make Cylinder proper objects
        // const auto & cylinder = boost::get<rbd::parsers::Geometry::Cylinder>(visual.geometry.data);
        // const auto & start = sva::PTransformd(Eigen::Vector3d{0.0, 0.0, -cylinder.length / 2}) * visual.origin
        //                     * robot().mbc().bodyPosW[bIdx];
        // const auto & end = sva::PTransformd(Eigen::Vector3d{0.0, 0.0, cylinder.length}) * start;
        // gui().drawArrow(translation(start), translation(end), 2 * cylinder.radius, 0.0f, 0.0f,
        //                Color4(color(visual.material).rgb(), alpha));
        break;
      }
      case Geometry::SPHERE:
      {
        const auto & sphere = boost::get<rbd::parsers::Geometry::Sphere>(visual.geometry.data);
        object = gui.makeSphere(translation(visual.origin), static_cast<float>(sphere.radius), color(visual.material),
                                this, group_);
        break;
      }
      default:
        break;
    }
    if(object) { objects_.push_back(object); }
  }

  inline void draw(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera) final
  {
    for(auto & o : objects_) { o->draw(transformationMatrix * o->transformation(), camera); }
  }

  inline void alpha(float alpha) noexcept
  {
    for(auto & o : objects_) { o->alpha(alpha); }
  }

  SceneGraph::DrawableGroup3D * group_;
  std::vector<std::shared_ptr<CommonDrawable>> objects_;
};

struct RobotObject : public Object3D, public SceneGraph::Drawable3D
{
  RobotObject(Scene3D & scene, SceneGraph::DrawableGroup3D & group)
  : Object3D(&scene), SceneGraph::Drawable3D{*this, &group}, parent_group_(&group)
  {
  }

  inline void update(const mc_rbdyn::Robot & robot) noexcept
  {
    auto posW = robot.posW();
    setTransformation(convert(posW));
    for(size_t i = 0; i < robot.mb().bodies().size(); ++i)
    {
      bodies_[i]->setTransformation(convert(robot.mbc().bodyPosW[i] * posW.inv()));
    }
  }

  inline void loadBody(McRtcGui & gui, const std::string & rm_path, const std::vector<rbd::parsers::Visual> & visuals)
  {
    bodies_.push_back(std::make_shared<RobotBody>(this, &group_));
    for(const auto & v : visuals) { bodies_.back()->loadVisual(gui, rm_path, v); }
  }

  inline void draw(const Matrix4 & transformationMatrix, SceneGraph::Camera3D & camera) final
  {
    if(visible_)
    {
      for(auto & b : bodies_) { b->draw(transformationMatrix * b->transformation(), camera); }
    }
  }

  inline bool visible() const noexcept { return visible_; }

  void visible(bool v) noexcept
  {
    if(v) { parent_group_->add(*this); }
    else { parent_group_->remove(*this); }
    visible_ = v;
  }

  inline void alpha(float alpha) noexcept
  {
    alpha_ = alpha;
    for(auto & b : bodies_) { b->alpha(alpha); }
  }

  inline float alpha() const noexcept { return alpha_; }

  inline void clear() noexcept { bodies_.clear(); }

  SceneGraph::DrawableGroup3D * parent_group_;
  SceneGraph::DrawableGroup3D group_;
  std::vector<std::shared_ptr<RobotBody>> bodies_;
  bool visible_ = true;
  float alpha_ = 1.0f;
};

struct RobotImpl
{
  RobotImpl(Robot & robot, Scene3D & scene, SceneGraph::DrawableGroup3D & group)
  : self_(robot), visualRobot_(scene, group), collisionRobot_(scene, group)
  {
    collisionRobot_.visible(false);
    visualRobot_.visible(self_.id.category.size() <= 1 || self_.id.category[0] != "Robots");
  }

  inline mc_rbdyn::Robot & robot() { return robots_->robot(); }

  inline McRtcGui & gui() { return self_.gui(); }

  void data(const mc_control::RobotMsg & msg)
  {
    if(!robots_ || robot().module().parameters() != msg.parameters)
    {
      visualRobot_.clear();
      collisionRobot_.clear();
      robots_ = RobotCache::get_robot(msg.parameters);
      const auto & rm = robots_->robot().module();
      const auto & bodies = robot().mb().bodies();
      auto loadVisuals = [this, &rm](auto & object, const auto & visuals, const std::string & name)
      {
        auto it = visuals.find(name);
        object.loadBody(gui(), rm.path, it != visuals.end() ? it->second : std::vector<rbd::parsers::Visual>{});
      };
      for(size_t i = 0; i < bodies.size(); ++i)
      {
        const auto & b = bodies[i];
        loadVisuals(visualRobot_, rm._visual, b.name());
        loadVisuals(collisionRobot_, rm._collision, b.name());
      }
      visualRobot_.alpha(1.0f);
      collisionRobot_.alpha(1.0f);
    }
    setConfiguration(robot(), msg.q);
    robot().posW(msg.posW);
  }

  void draw2D()
  {
    if(!robots_) { return; }
    auto drawRobotControl = [this](RobotObject & robot, const char * type)
    {
      bool visible = robot.visible();
      ImGui::BeginTable(self_.label(fmt::format("##Table{}", type), self_.id.name).c_str(), 2,
                        ImGuiTableFlags_SizingStretchProp);
      ImGui::TableNextColumn();
      if(ImGui::Checkbox(self_.label(fmt::format("Draw {} {} model", self_.id.name, type)).c_str(), &visible))
      {
        robot.visible(visible);
      }
      ImGui::TableNextColumn();
      ImGui::Text("Alpha");
      ImGui::SameLine();
      float alpha = robot.alpha();
      if(ImGui::SliderFloat(self_.label(fmt::format("##Alpha{}", type), self_.id.name).c_str(), &alpha, 0.0, 1.0))
      {
        robot.alpha(alpha);
      }
      ImGui::EndTable();
    };
    drawRobotControl(visualRobot_, "visual");
    drawRobotControl(collisionRobot_, "collision");
  }

  void draw3D()
  {
    if(!robots_) { return; }
    visualRobot_.update(robot());
    collisionRobot_.update(robot());
  }

private:
  Robot & self_;
  std::shared_ptr<mc_rbdyn::Robots> robots_;
  RobotObject visualRobot_;
  RobotObject collisionRobot_;
};

} // namespace details

Robot::Robot(Client & client, const ElementId & id, McRtcGui & gui)
: Widget(client, id, gui), impl_(new details::RobotImpl{*this, gui.scene(), gui.drawables()})
{
}

Robot::~Robot() = default;

void Robot::data(const mc_control::RobotMsg & msg)
{
  impl_->data(msg);
}

void Robot::draw2D()
{
  impl_->draw2D();
}

void Robot::draw3D()
{
  impl_->draw3D();
}

} // namespace mc_rtc::magnum
