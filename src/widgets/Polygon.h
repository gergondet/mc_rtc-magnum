#pragma once

#include "Widget.h"

#include <Magnum/Shaders/LineGL.h>

namespace mc_rtc::magnum
{

struct Polygon : public Widget
{
  Polygon(Client & client, const ElementId & id, McRtcGui & gui) : Widget(client, id, gui) {}

  void data(const std::vector<std::vector<Eigen::Vector3d>> & points, const mc_rtc::gui::LineConfig & config);

  void draw3D() override;

private:
  std::vector<std::vector<Eigen::Vector3d>> points_;
  mc_rtc::gui::LineConfig config_;
  struct PolygonData
  { std::optional<GL::Mesh> mesh; };
  std::vector<PolygonData> polygons_;
  Shaders::LineGL3D lineShader_;
};

} // namespace mc_rtc::magnum
