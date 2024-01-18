#include "MagnumClient.h"

#include "McRtcGui.h"

#include "widgets/Arrow.h"
#include "widgets/Force.h"
#include "widgets/Point3D.h"
#include "widgets/Polygon.h"
#include "widgets/Robot.h"
#include "widgets/Rotation.h"
#include "widgets/Trajectory.h"
#include "widgets/Transform.h"
#include "widgets/Visual.h"
#include "widgets/XYTheta.h"

namespace mc_rtc::magnum
{

InteractiveMarkerPtr MagnumClient::make_marker(const sva::PTransformd & pose, ControlAxis mask)
{
  return std::make_unique<InteractiveMarkerImpl>(gui_.camera(), pose, mask);
}

void MagnumClient::point3d(const ElementId & id,
                           const ElementId & requestId,
                           bool ro,
                           const Eigen::Vector3d & pos,
                           const mc_rtc::gui::PointConfig & config)
{
  widget<Point3D>(id, gui_, requestId).data(ro, pos, config);
}

void MagnumClient::trajectory(const ElementId & id,
                              const std::vector<Eigen::Vector3d> & points,
                              const mc_rtc::gui::LineConfig & config)
{
  widget<Trajectory<Eigen::Vector3d>>(id, gui_).data(points, config);
}

void MagnumClient::trajectory(const ElementId & id,
                              const std::vector<sva::PTransformd> & points,
                              const mc_rtc::gui::LineConfig & config)
{
  widget<Trajectory<sva::PTransformd>>(id, gui_).data(points, config);
}

void MagnumClient::trajectory(const ElementId & id,
                              const Eigen::Vector3d & point,
                              const mc_rtc::gui::LineConfig & config)
{
  widget<Trajectory<Eigen::Vector3d>>(id, gui_).data(point, config);
}

void MagnumClient::trajectory(const ElementId & id,
                              const sva::PTransformd & point,
                              const mc_rtc::gui::LineConfig & config)
{
  widget<Trajectory<sva::PTransformd>>(id, gui_).data(point, config);
}

void MagnumClient::polygon(const ElementId & id,
                           const std::vector<std::vector<Eigen::Vector3d>> & points,
                           const mc_rtc::gui::LineConfig & config)
{
  widget<Polygon>(id, gui_).data(points, config);
}

void MagnumClient::force(const ElementId & id,
                         const ElementId & requestId,
                         const sva::ForceVecd & force,
                         const sva::PTransformd & pos,
                         const mc_rtc::gui::ForceConfig & forceConfig,
                         bool /* ro */)
{
  widget<Force>(id, gui_, requestId).data(force, pos, forceConfig);
}

void MagnumClient::arrow(const ElementId & id,
                         const ElementId & requestId,
                         const Eigen::Vector3d & start,
                         const Eigen::Vector3d & end,
                         const mc_rtc::gui::ArrowConfig & config,
                         bool ro)
{
  widget<Arrow>(id, gui_, requestId).data(start, end, config, ro);
}

void MagnumClient::rotation(const ElementId & id, const ElementId & requestId, bool ro, const sva::PTransformd & pos)
{
  widget<Rotation>(id, gui_, requestId).data(ro, pos);
}

void MagnumClient::transform(const ElementId & id, const ElementId & requestId, bool ro, const sva::PTransformd & pos)
{
  widget<TransformWidget>(id, gui_, requestId).data(ro, pos);
}

void MagnumClient::xytheta(const ElementId & id,
                           const ElementId & requestId,
                           bool ro,
                           const Eigen::Vector3d & xytheta,
                           double altitude)
{
  widget<XYTheta>(id, gui_, requestId).data(ro, xytheta, altitude);
}

void MagnumClient::robot(const ElementId & id,
                         const mc_control::RobotMsg & msg)
{
  widget<Robot>(id, gui_).data(msg);
}

void MagnumClient::visual(const ElementId & id, const rbd::parsers::Visual & visual, const sva::PTransformd & pos)
{
  widget<Visual>(id, gui_).data(visual, pos);
}

} // namespace mc_rtc::magnum
