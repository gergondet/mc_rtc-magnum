#pragma once

#include "Arrow.h"

namespace mc_rtc::magnum
{

struct Force : public Arrow
{
  Force(Client & client, const ElementId & id, McRtcGui & gui, const ElementId & reqId) : Arrow(client, id, gui, reqId)
  {
  }

  void data(const sva::ForceVecd & force, const sva::PTransformd & pose, const mc_rtc::gui::ForceConfig config)
  {
    const auto & p0 = pose.translation();
    Eigen::Vector3d p1 = (sva::PTransformd(Eigen::Vector3d(config.scale * force.force())) * pose).translation();
    Arrow::data(p0, p1, config, true);
  }
};

} // namespace mc_rtc::magnum
