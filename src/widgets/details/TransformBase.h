#pragma once

#include "../Widget.h"

namespace mc_rtc::magnum
{

template<ControlAxis ctl>
struct TransformBase : public Widget
{
  TransformBase(Client & client, const ElementId & id, McRtcGui & gui, const ElementId & requestId)
  : Widget(client, id, gui), requestId_(requestId),
    marker_(client.make_marker(sva::PTransformd::Identity(), ControlAxis::NONE))
  {
  }

  ~TransformBase() override = default;

  void data(bool ro, const sva::PTransformd & pos)
  {
    marker_->mask(ro ? ControlAxis::NONE : ctl);
    marker_->pose(pos);
  }

  void draw3D() override
  {
    const auto & pos = marker_->pose();
    if(marker_->draw())
    {
      if constexpr(ctl == ControlAxis::TRANSLATION) { client.send_request(requestId_, pos.translation()); }
      else if constexpr(ctl == ControlAxis::ROTATION) { client.send_request(requestId_, pos.rotation()); }
      else if constexpr(ctl == ControlAxis::ALL) { client.send_request(requestId_, pos); }
      else if constexpr(ctl == ControlAxis::XYTHETA || ctl == ControlAxis::XYZTHETA)
      {
        Eigen::VectorXd data = Eigen::VectorXd::Zero(4);
        const auto & t = pos.translation();
        auto yaw = mc_rbdyn::rpyFromMat(pos.rotation()).z();
        data(0) = t.x();
        data(1) = t.y();
        data(2) = yaw;
        data(3) = t.z();
        client.send_request(requestId_, data);
      }
    }
    gui_.drawFrame(convert(pos));
  }

protected:
  ElementId requestId_;
  InteractiveMarkerPtr marker_;
};

} // namespace mc_rtc::magnum
