#pragma once

#include <memory>

#include <SpaceVecAlg/SpaceVecAlg>

#include "../../mc_rtc-imgui/InteractiveMarker.h"

#include "../../Camera.h"

namespace mc_rtc::magnum
{

using ControlAxis = mc_rtc::imgui::ControlAxis;
using InteractiveMarkerPtr = mc_rtc::imgui::InteractiveMarkerPtr;

struct InteractiveMarkerImpl : public mc_rtc::imgui::InteractiveMarker
{
  InteractiveMarkerImpl(const Camera & camera,
                        const sva::PTransformd & pose = sva::PTransformd::Identity(),
                        ControlAxis mask = ControlAxis::NONE);

  ~InteractiveMarkerImpl() override {};

  void mask(ControlAxis mask) override;

  void pose(const sva::PTransformd & pose) override;

  inline bool draw() override { return draw(camera_); }

private:
  const Camera & camera_;
  int operation_;
  bool active_ = false;
  int id_;
  static int next_id_;

  bool draw(const Camera & camera);
};

} // namespace mc_rtc::magnum
