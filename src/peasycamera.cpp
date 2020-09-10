#include "peasycamera.h"
#include <math.h>
#include <assert.h>

namespace peasycamera {
   namespace {
      constexpr vec3 XAxis = {1.0f, 0.0f, 0.0f};
      constexpr vec3 YAxis = {0.0f, 1.0f, 0.0f};
      constexpr vec3 ZAxis = {0.0f, 0.0f, 1.0f};

      vec3 operator +(const vec3& a, const vec3& b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
      vec3 operator -(const vec3& a, const vec3& b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
      vec3 operator *(float a, const vec3& b) { return {a * b.x, a * b.y, a * b.z}; }

      vec3 CrossProduct(const vec3& a, const vec3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
      float Length(const vec3& a) { return sqrtf(a.x * a.x + a.y * a.y + a.z * a.z); }
      vec3 Normalize(const vec3& a) { return (1.0f / Length(a)) * a; }

      float Clamp(float value, float min, float max) { return value < min ? min : value > max ? max : value; }

      quat operator *(const quat& a, const quat& b) {
         return {
            b.x * a.w + b.w * a.x + (b.y * a.z - b.z * a.y),
            b.y * a.w + b.w * a.y + (b.z * a.x - b.x * a.z),
            b.z * a.w + b.w * a.z + (b.x * a.y - b.y * a.x),
            b.w * a.w - (b.x * a.x + b.y * a.y + b.z * a.z)
         };
      }

      vec3 ApplyRotation(const quat& rotation, const vec3& vector) {
         const float x = vector.x;
         const float y = vector.y;
         const float z = vector.z;

         const float s = rotation.x * x + rotation.y * y + rotation.z * z;

         return {
            2.0f * (rotation.w * (x * rotation.w - (rotation.y * z - rotation.z * y)) + s * rotation.x) - x,
            2.0f * (rotation.w * (y * rotation.w - (rotation.z * x - rotation.x * z)) + s * rotation.y) - y,
            2.0f * (rotation.w * (z * rotation.w - (rotation.x * y - rotation.y * x)) + s * rotation.z) - z
         };
      }

      quat QuatFromAxisAndAngle(const vec3& axis, float angle) {
         float len = Length(axis);
         assert(len != 0.0f);

         const float half_angle = -0.5f * angle;
         const float coeff = sinf(half_angle) / len;

         return {coeff * axis.x, coeff * axis.y, coeff * axis.z, cosf(half_angle)};
      }

      void LookAtMatrix(const vec3& position, const vec3& lookAt, const vec3& up, float* outViewMatrix4x4) {
         const vec3 zAxis = Normalize(lookAt - position);
         const vec3 xAxis = Normalize(CrossProduct(zAxis, up));
         const vec3 yAxis = CrossProduct(xAxis, zAxis);

         float M[16] = {
            xAxis.x, yAxis.x, -zAxis.x, 0.0f,
            xAxis.y, yAxis.y, -zAxis.y, 0.0f,
            xAxis.z, yAxis.z, -zAxis.z, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f,
         };

         float T[16] = {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            -position.x, -position.y, -position.z, 1.0f,
         };

         outViewMatrix4x4[0] = M[0] * T[0] + M[4] * T[1] + M[8] * T[2] + M[12] * T[3];
         outViewMatrix4x4[1] = M[1] * T[0] + M[5] * T[1] + M[9] * T[2] + M[13] * T[3];
         outViewMatrix4x4[2] = M[2] * T[0] + M[6] * T[1] + M[10] * T[2] + M[14] * T[3];
         outViewMatrix4x4[3] = M[3] * T[0] + M[7] * T[1] + M[11] * T[2] + M[15] * T[3];

         outViewMatrix4x4[4] = M[0] * T[4] + M[4] * T[5] + M[8] * T[6] + M[12] * T[7];
         outViewMatrix4x4[5] = M[1] * T[4] + M[5] * T[5] + M[9] * T[6] + M[13] * T[7];
         outViewMatrix4x4[6] = M[2] * T[4] + M[6] * T[5] + M[10] * T[6] + M[14] * T[7];
         outViewMatrix4x4[7] = M[3] * T[4] + M[7] * T[5] + M[11] * T[6] + M[15] * T[7];

         outViewMatrix4x4[8] = M[0] * T[8] + M[4] * T[9] + M[8] * T[10] + M[12] * T[11];
         outViewMatrix4x4[9] = M[1] * T[8] + M[5] * T[9] + M[9] * T[10] + M[13] * T[11];
         outViewMatrix4x4[10] = M[2] * T[8] + M[6] * T[9] + M[10] * T[10] + M[14] * T[11];
         outViewMatrix4x4[11] = M[3] * T[8] + M[7] * T[9] + M[11] * T[10] + M[15] * T[11];

         outViewMatrix4x4[12] = M[0] * T[12] + M[4] * T[13] + M[8] * T[14] + M[12] * T[15];
         outViewMatrix4x4[13] = M[1] * T[12] + M[5] * T[13] + M[9] * T[14] + M[13] * T[15];
         outViewMatrix4x4[14] = M[2] * T[12] + M[6] * T[13] + M[10] * T[14] + M[14] * T[15];
         outViewMatrix4x4[15] = M[3] * T[12] + M[7] * T[13] + M[11] * T[14] + M[15] * T[15];
      }

      void AddMouseWheelZoomImpulse(DampedAction& zoom, float zoomScale, int mouseWheelDelta) {
         zoom.m_velocity += zoomScale * float(mouseWheelDelta);
      }

      void AddMouseMoveZoomImpulse(DampedAction& zoom, int mouseDY) {
         zoom.m_velocity += float(-mouseDY) / 10.0f;
      }

      void AddMouseMovePanImpulse(DampedAction& panX, DampedAction& panY, int dx, int dy) {
         panX.m_velocity += float(dx) / 8.0f;
         panY.m_velocity += float(dy) / 8.0f;
      }

      void AddMouseMoveRotateImpulse(DampedAction& rotateX, DampedAction& rotateY, DampedAction& rotateZ, Constraint constraint, float distance, int mouseX, int mouseY, int mouseDX, int mouseDY, int viewportLeft, int viewportTop, int viewportWidth, int viewportHeight) {
         float mult = -powf(log10f(1.0f + distance), 0.5f) * 0.0005f; //0.00125f;

         float dmx = float(mouseDX) * mult;
         float dmy = float(mouseDY) * mult;

         float viewX = float(viewportLeft);
         float viewY = float(viewportTop);
         float viewW = float(viewportWidth);
         float viewH = float(viewportHeight);

         // mouse [-1, +1]
         float mxNDC = Clamp((float(mouseX) - viewX) / viewW, 0.0f, 1.0f) * 2.0f - 1.0f;
         float myNDC = Clamp((float(mouseY) - viewY) / viewH, 0.0f, 1.0f) * 2.0f - 1.0f;

         if (constraint == Constraint::None || constraint == Constraint::Pitch || constraint == Constraint::SuppressRoll) {
            rotateX.m_velocity += dmy * (1.0f - mxNDC * mxNDC);
         }

         if (constraint == Constraint::None || constraint == Constraint::Yaw || constraint == Constraint::SuppressRoll) {
            rotateY.m_velocity += dmx * (1.0f - myNDC * myNDC);
         }

         if (constraint == Constraint::None || constraint == Constraint::Roll) {
            rotateZ.m_velocity += -dmx * myNDC;
            rotateZ.m_velocity += -dmy * mxNDC;
         }
      }

      void ApplyZoomToCamera(Camera& camera) {
         if (camera.m_zoom.m_velocity == 0.0f) {
            return;
         }

         const float newDistance = camera.m_state.m_distance + camera.m_zoom.m_velocity * camera.m_state.m_distance * 0.02f;

         if (newDistance < camera.m_minDistance || newDistance > camera.m_maxDistance) {
            camera.m_zoom.m_velocity = 0.0f;
         }
         camera.m_state.m_distance = Clamp(newDistance, camera.m_minDistance, camera.m_maxDistance);

         camera.m_zoom.m_velocity *= (1.0f - camera.m_zoom.m_friction);
         if (fabsf(camera.m_zoom.m_velocity) < 0.001f) {
            camera.m_zoom.m_velocity = 0.0f;
         }
      }

      void ApplyPanToCamera(Camera& camera) {
         auto MousePan = [](Camera& camera, float dx, float dy) {
            // @TODO: Make the pan scale dependent on the viewport size.
            const float panScale = camera.m_state.m_distance * 0.0025f;
            dx = (camera.m_dragConstraint == Constraint::Pitch ? 0.0f : -panScale * dx);
            dy = (camera.m_dragConstraint == Constraint::Yaw ? 0.0f : panScale * dy);
            camera.Pan(dx, dy);
         };

         DampedAction& panX = camera.m_panX;
         DampedAction& panY = camera.m_panY;

         if (panX.m_velocity != 0.0f) {
            MousePan(camera, panX.m_velocity, 0.0f);

            panX.m_velocity *= (1.0f - panX.m_friction);

            if (fabsf(panX.m_velocity) < 0.001f) {
               panX.m_velocity = 0.0f;
            }
         }

         if (panY.m_velocity != 0.0f) {
            MousePan(camera, 0.0f, panY.m_velocity);

            panY.m_velocity *= (1.0f - panY.m_friction);

            if (fabsf(panY.m_velocity) < 0.001f) {
               panY.m_velocity = 0.0f;
            }
         }
      }

      quat ApplyRotate(DampedAction& rotate, const vec3& rotationAxis, const quat& currentRotation) {
         if (rotate.m_velocity == 0.0f) {
            return currentRotation;
         }

         quat result = currentRotation * QuatFromAxisAndAngle(rotationAxis, rotate.m_velocity);
         rotate.m_velocity *= (1.0f - rotate.m_friction);

         if (fabsf(rotate.m_velocity) < 0.001f) {
            rotate.m_velocity = 0.0f;
         }

         return result;
      }

      void ApplyRotateToCamera(Camera& camera) {
         camera.m_state.m_rotation = ApplyRotate(camera.m_rotateX, XAxis, camera.m_state.m_rotation);
         camera.m_state.m_rotation = ApplyRotate(camera.m_rotateY, YAxis, camera.m_state.m_rotation);
         camera.m_state.m_rotation = ApplyRotate(camera.m_rotateZ, ZAxis, camera.m_state.m_rotation);
      }
   }

   Camera::Camera(float distance, float lookAtX, float lookAtY, float lookAtZ) { 
      m_state.m_distance = distance;
      m_state.m_lookAt = {lookAtX, lookAtY, lookAtZ};
      m_state.m_rotation = kIdentityRotation;
   }

   void Camera::CalculateViewMatrix() {
      vec3 position = m_state.m_distance * ApplyRotation(m_state.m_rotation, ZAxis) + m_state.m_lookAt;
      vec3 up = ApplyRotation(m_state.m_rotation, YAxis);
      LookAtMatrix(position, m_state.m_lookAt, up, m_viewMatrix);
   }

   void Camera::Update(bool shiftKeyDown, bool leftMouseButtonDown, bool rightMouseButtonDown, bool middleMouseButtonDown, int mouseX, int mouseY, int mouseDX, int mouseDY, int mouseWheelDelta,
                       int viewportLeft, int viewportTop, int viewportWidth, int viewportHeight) {
      if (shiftKeyDown) {
         const int dx = mouseDX;
         const int dy = mouseDY;

         if (m_dragConstraint == Constraint::None && abs(mouseDX - mouseDY) > 1) {
            m_dragConstraint = (abs(dx) > abs(dy) ? Constraint::Yaw : Constraint::Pitch);
         }
      } else {
         m_dragConstraint = Constraint::None;
      }
      
      if (mouseWheelDelta != 0) {
         AddMouseWheelZoomImpulse(m_zoom, m_wheelZoomScale, mouseWheelDelta);
      }

      if (rightMouseButtonDown) {
         AddMouseMoveZoomImpulse(m_zoom, mouseDY);
      }

      if (middleMouseButtonDown) {
         AddMouseMovePanImpulse(m_panX, m_panY, mouseDX, mouseDY);
      }

      if (leftMouseButtonDown) {
         AddMouseMoveRotateImpulse(m_rotateX, m_rotateY, m_rotateZ, m_dragConstraint, m_state.m_distance, mouseX, mouseY, mouseDX, mouseDY, viewportLeft, viewportTop, viewportWidth, viewportHeight);
      }

      ApplyZoomToCamera(*this);
      ApplyPanToCamera(*this);
      ApplyRotateToCamera(*this);
   }

   void Camera::Pan(float dx, float dy) {
      m_state.m_lookAt = m_state.m_lookAt + ApplyRotation(m_state.m_rotation, vec3 {dx, dy, 0.0f});
   }
}
