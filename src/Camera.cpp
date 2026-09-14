#include "Camera.h"

#include <Magnum/GL/PixelFormat.h>
#include <Magnum/Image.h>

#include <iostream>

namespace mc_rtc::magnum
{

Camera::Camera(Platform::Application & app, Scene3D & scene)
{
  object_ = new Object3D{&scene};
  camera_ = new SceneGraph::Camera3D{*object_};
  resetTransform(app);
}

void Camera::resetTransform(Platform::Application & app)
{
  cameraPosition_ = {2.5f, 2.5f, 2.5f};
  focusPoint_ = {0.0f, 0.0f, 0.75f};
  setTransform(app);
}

void Camera::setTransform(Platform::Application & app)
{
  Vector3 up{0.0f, 0.0f, 1.0f};
  if(cross(focusPoint_ - cameraPosition_, up).isZero()) { up = {-1.0f, 0.0f, 0.0f}; }
  object_->setTransformation(Matrix4::lookAt(cameraPosition_, focusPoint_, up));
  setProjection(app.windowSize());
}

void Camera::swapProjectionType(Platform::Application & app)
{
  orthographic_ = !orthographic_;
  setProjection(app.windowSize());
}

void Camera::setProjection(const Vector2i & windowSize)
{
  if(orthographic_)
  {
    Float ratio = Vector2{windowSize}.aspectRatio();
    Float distance = (cameraPosition_ - focusPoint_).length() / 2.0f;
    camera_->setProjectionMatrix(Matrix4::orthographicProjection(Vector2{ratio * distance, distance}, 0.01, 100.0f));
  }
  else
  {
    camera_->setProjectionMatrix(
        Matrix4::perspectiveProjection(60.0_degf, Vector2{windowSize}.aspectRatio(), 0.01f, 100.0f));
  }
}

bool Camera::keyPressEvent(Platform::Application & app, KeyEvent & event)
{
  /* Reset the transformation to the original view */
  if(event.key() == KeyEvent::Key::NumZero)
  {
    resetTransform(app);
    app.redraw();
    return true;
  }
  /** Switch between orthographic and perspective projection */
  else if(event.key() == KeyEvent::Key::NumFive)
  {
    swapProjectionType(app);
    app.redraw();
    return true;
  }
  /* Axis-aligned view */
  else if(event.key() == KeyEvent::Key::NumOne || event.key() == KeyEvent::Key::NumThree
          || event.key() == KeyEvent::Key::NumSeven)
  {
    /* Front/back */
    const Float multiplier = event.modifiers() & KeyEvent::Modifier::Ctrl ? -1.0f : 1.0f;

    if(event.key() == KeyEvent::Key::NumSeven) /* Top/bottom */
    {
      cameraPosition_ = {0.0f, 0.0f, multiplier * 5.0f};
      focusPoint_ = {0.0f, 0.0f, 0.0f};
    }
    else if(event.key() == KeyEvent::Key::NumOne) /* Front/back */
    {
      cameraPosition_ = {multiplier * 5.0f, 0.0f, 1.0f};
      focusPoint_ = {0.0f, 0.0f, 1.0f};
    }
    else if(event.key() == KeyEvent::Key::NumThree) /* Right/left */
    {
      cameraPosition_ = {0.0f, multiplier * 5.0f, 1.0f};
      focusPoint_ = {0.0f, 0.0f, 1.0f};
    }
    else
    {
      CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    setTransform(app);
    app.redraw();
    return true;
  }
  return false;
}

bool Camera::mousePressEvent(Platform::Application &, MouseEvent & event)
{
  /* Due to compatibility reasons, scroll is also reported as a press event,
     so filter that out. Could be removed once MouseEvent::Button::Wheel is
     gone from Magnum. */
  if(event.button() != MouseEvent::Button::Left && event.button() != MouseEvent::Button::Middle) return false;

  lastPosition_ = event.position();
  return true;
}

bool Camera::mouseMoveEvent(Platform::Application & app, MouseMoveEvent & event)
{
  const Vector2i delta = event.position() - lastPosition_;
  lastPosition_ = event.position();

  if(!event.buttons()) { return false; }

  Vector3 direction = (cameraPosition_ - focusPoint_).normalized();
  Vector3 right = Magnum::Math::cross(Vector3{0, 0, 1}, direction);
  if(right.isZero()) { right = Magnum::Math::cross(Vector3{-1.0, 0, 0}, direction); }
  Vector3 up = Magnum::Math::cross(direction, right);
  const Float scale = 0.005;

  /* Translate */
  if(event.modifiers() & MouseMoveEvent::Modifier::Shift || event.buttons() == MouseMoveEvent::Button::Middle)
  {
    Vector3 diff = scale * (delta.y() * up - delta.x() * right);
    cameraPosition_ += diff;
    focusPoint_ += diff;
  }
  /* Rotate */
  else
  {
    Vector3 s = (cameraPosition_ - focusPoint_);
    Float r = s.length();
    Float phi = atan2f(s.y(), s.x());
    Float theta = acosf(s.z() / r);
    Float dphi = -scale * delta.x();
    Float dtheta = -scale * delta.y();
    Vector3 ns = {r * sinf(theta + dtheta) * cosf(phi + dphi), r * sinf(theta + dtheta) * sinf(phi + dphi),
                  r * cosf(theta + dtheta)};
    cameraPosition_ = focusPoint_ + ns;
  }
  setTransform(app);
  app.redraw();
  return true;
}

bool Camera::mouseScrollEvent(Platform::Application & app, MouseScrollEvent & event)
{
  const Float move = event.offset().y();
  if(!move) { return false; }

  const Float scale = 0.1;

  Vector3 direction = (cameraPosition_ - focusPoint_);
  if(move < 0) { cameraPosition_ += scale * direction; }
  else
  {
    cameraPosition_ -= scale * direction;
  }
  setTransform(app);

  event.setAccepted();
  app.redraw();
  return true;
}

void Camera::viewportEvent(ViewportEvent & event)
{ setProjection(event.windowSize()); }

} // namespace mc_rtc::magnum
