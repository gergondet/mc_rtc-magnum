#include "InteractiveMarker.h"

#include "imgui.h"

#include "ImGuizmo.h"

#include "../utils.h"

#include <Magnum/Math/Algorithms/GramSchmidt.h>

namespace mc_rtc::magnum
{

static inline bool has(ControlAxis mask, ControlAxis value)
{ return static_cast<std::underlying_type_t<ControlAxis>>(mask & value) != 0; }

static inline ImGuizmo::OPERATION convert(ControlAxis mask)
{
  ImGuizmo::OPERATION out = static_cast<ImGuizmo::OPERATION>(0);
#define HANDLE(CA, OP) \
  if(has(mask, CA)) { out = out | OP; }
  HANDLE(ControlAxis::TX, ImGuizmo::OPERATION::TRANSLATE_X)
  HANDLE(ControlAxis::TY, ImGuizmo::OPERATION::TRANSLATE_Y)
  HANDLE(ControlAxis::TZ, ImGuizmo::OPERATION::TRANSLATE_Z)
  HANDLE(ControlAxis::RX, ImGuizmo::OPERATION::ROTATE_X)
  HANDLE(ControlAxis::RY, ImGuizmo::OPERATION::ROTATE_Y)
  HANDLE(ControlAxis::RZ, ImGuizmo::OPERATION::ROTATE_Z)
#undef HANDLE
  return out;
}

int InteractiveMarkerImpl::next_id_ = 0;

InteractiveMarkerImpl::InteractiveMarkerImpl(const Camera & camera, const sva::PTransformd & pose, ControlAxis mask)
: mc_rtc::imgui::InteractiveMarker(pose, mask), camera_(camera), id_(next_id_++)
{ this->mask(mask); }

void InteractiveMarkerImpl::mask(ControlAxis mask)
{
  mask_ = mask;
  operation_ = convert(mask);
}

void InteractiveMarkerImpl::pose(const sva::PTransformd & pose)
{
  if(!active_) { pose_ = pose; }
}

bool InteractiveMarkerImpl::draw(const Camera & camera)
{
  if(operation_ == 0) { return false; }
  ImGuizmo::SetID(id_);
  ImGuizmo::SetOrthographic(camera.isOrthographic());
  ImGuizmo::SetGizmoSizeWorldSpace(0.15f);
  auto view = camera.camera()->cameraMatrix();
  auto projection = camera.camera()->projectionMatrix();
  Magnum::Matrix4 mat = convert(pose_);
  float m[16];
  memcpy(&m, mat.data(), 16 * sizeof(float));
  auto op = static_cast<ImGuizmo::OPERATION>(operation_);
  bool changed = ImGuizmo::Manipulate(view.data(), projection.data(), op, ImGuizmo::MODE::LOCAL, m);
  active_ = ImGuizmo::IsUsing();
  if(changed)
  {
    auto npose = Magnum::Matrix4::from(m);
    auto t = npose.translation();
    Eigen::Vector3f t_map = Eigen::Map<Eigen::Vector3f>(t.data());
    pose_.translation() = t_map.cast<double>();
    Magnum::Math::Algorithms::gramSchmidtOrthonormalizeInPlace(npose);
    auto rot = npose.rotation().transposed();
    Eigen::Matrix3f rot_map = Eigen::Map<Eigen::Matrix3f>(rot.data());
    pose_.rotation() = rot_map.cast<double>();
  }
  return changed;
}

} // namespace mc_rtc::magnum
