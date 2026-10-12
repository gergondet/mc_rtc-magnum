#pragma once

#include <mc_rtc/config.h>
#include <mc_rtc/gui/types.h>

#include <Magnum/Magnum.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>

#include <boost/filesystem.hpp>
namespace bfs = boost::filesystem;

#ifdef MC_RTC_HAS_ROS_SUPPORT
#  ifdef MC_RTC_ROS_IS_ROS2
#    include <ament_index_cpp/get_package_share_directory.hpp>
#  else
#    include <ros/package.h>
#  endif
#endif

#include <RBDyn/parsers/common.h>

namespace mc_rtc::magnum
{

inline Magnum::Color4 convert(const mc_rtc::gui::Color & c)
{ return {static_cast<float>(c.r), static_cast<float>(c.g), static_cast<float>(c.b), static_cast<float>(c.a)}; }

inline Magnum::Vector3 translation(const Eigen::Vector3d & v)
{ return {static_cast<float>(v.x()), static_cast<float>(v.y()), static_cast<float>(v.z())}; }

inline Magnum::Vector3 translation(const sva::PTransformd & pt)
{ return translation(pt.translation()); }

inline Magnum::Matrix3 rotation(const Eigen::Matrix3d & m)
{
  Eigen::Matrix3f m3f = m.transpose().cast<float>();
  return Magnum::Matrix3::from(m3f.data());
}

inline Magnum::Matrix3 rotation(const sva::PTransformd & pt)
{ return rotation(pt.rotation()); }

inline Magnum::Matrix3 convert(const Eigen::Matrix3d & m)
{ return rotation(m); }

inline Magnum::Matrix4 convert(const sva::PTransformd & pt)
{ return Magnum::Matrix4::from(convert(pt.rotation()), translation(pt.translation())); }

inline Magnum::Matrix4 convert(const Eigen::Vector3d & t)
{ return Magnum::Matrix4::translation(translation(t)); }

inline sva::PTransformd convert(const Magnum::Matrix4 & m)
{
  auto t = m.translation();
  Eigen::Vector3f t_map = Eigen::Map<Eigen::Vector3f>(t.data());
  auto r = m.rotation().transposed();
  Eigen::Matrix3f r_map = Eigen::Map<Eigen::Matrix3f>(r.data());
  return {r_map.cast<double>(), t_map.cast<double>()};
}

inline bfs::path convertURI(const std::string & uri, [[maybe_unused]] std::string_view default_dir = "")
{
  const std::string package = "package://";
  if(uri.size() >= package.size() && uri.find(package) == 0)
  {
    size_t split = uri.find('/', package.size());
    std::string pkg = uri.substr(package.size(), split - package.size());
    auto leaf = bfs::path(uri.substr(split + 1));
    bfs::path MC_ENV_DESCRIPTION_PATH(mc_rtc::MC_ENV_DESCRIPTION_PATH);
#ifndef __EMSCRIPTEN__
#  ifndef MC_RTC_HAS_ROS_SUPPORT
    // FIXME Prompt the user for unknown packages
    if(pkg == "jvrc_description") { pkg = (MC_ENV_DESCRIPTION_PATH / ".." / "jvrc_description").string(); }
    else if(pkg == "mc_env_description") { pkg = MC_ENV_DESCRIPTION_PATH.string(); }
    else if(pkg == "mc_int_obj_description")
    {
      pkg = (MC_ENV_DESCRIPTION_PATH / ".." / "mc_int_obj_description").string();
    }
    else
    {
      pkg = default_dir;
    }
#  else
#    ifdef MC_RTC_ROS_IS_ROS2
    pkg = ament_index_cpp::get_package_share_directory(pkg);
#    else
    pkg = ros::package::getPath(pkg);
#    endif
#  endif
#else
    pkg = "/assets/" + pkg;
#endif
    return pkg / leaf;
  }
  const std::string file = "file://";
  if(uri.size() >= file.size() && uri.find(file) == 0) { return bfs::path(uri.substr(file.size())); }
  return uri;
}

inline Magnum::Color4 color(const rbd::parsers::Material & m)
{
  if(m.type == rbd::parsers::Material::Type::COLOR)
  {
    const auto & c = boost::get<rbd::parsers::Material::Color>(m.data);
    return {static_cast<float>(c.r), static_cast<float>(c.g), static_cast<float>(c.b), static_cast<float>(c.a)};
  }
  return {1.0f, 1.0f, 1.0f, 1.0f};
}

} // namespace mc_rtc::magnum
