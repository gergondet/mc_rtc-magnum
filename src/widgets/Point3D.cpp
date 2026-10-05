#include "Point3D.h"

namespace mc_rtc::magnum
{

Point3D::Point3D(Client & client, const ElementId & id, McRtcGui & gui, const ElementId & requestId)
: TransformBase(client, id, gui, requestId)
{ sphere_ = gui_.makeSphere({}, 0.0f, {}); }

void Point3D::data(bool ro, const Eigen::Vector3d & pos, const mc_rtc::gui::PointConfig & config)
{
  TransformBase::data(ro, pos);
  sphere_->center(translation(marker_->pose()));
  sphere_->radius(config.scale);
  sphere_->color(convert(config.color));
}

void Point3D::draw3D()
{ TransformBase::draw3D(); }

} // namespace mc_rtc::magnum
