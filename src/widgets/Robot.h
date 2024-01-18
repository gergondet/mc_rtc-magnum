#pragma once

#include "Widget.h"

namespace mc_rtc::magnum
{

namespace details
{

struct RobotImpl;
}

struct Robot : public Widget
{
  Robot(Client & client, const ElementId & id, McRtcGui & gui);

  ~Robot() override;

  void data(const mc_control::RobotMsg & msg);

  void draw2D() override;

  void draw3D() override;

  inline McRtcGui & gui() noexcept { return gui_; }

private:
  std::unique_ptr<details::RobotImpl> impl_;
};

} // namespace mc_rtc::magnum
