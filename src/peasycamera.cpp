#include "peasycamera.h"
#include <math.h>

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

      void ApplyZoomToCamera(Camera& camera) {
         if (camera.m_zoom.m_velocity == 0.0f) {
            return;
         }

         const float newDistance = camera.m_distance + camera.m_zoom.m_velocity * camera.m_distance * 0.02f;

         if (newDistance < camera.m_minDistance || newDistance > camera.m_maxDistance) {
            camera.m_zoom.m_velocity = 0.0f;
         }
         camera.m_distance = Clamp(newDistance, camera.m_minDistance, camera.m_maxDistance);

         camera.m_zoom.m_velocity *= (1.0f - camera.m_zoom.m_friction);
         if (fabsf(camera.m_zoom.m_velocity) < 0.001f) {
            camera.m_zoom.m_velocity = 0.0f;
         }
      }

      void ApplyPanToCamera(Camera& camera) {
         auto MousePan = [](Camera& camera, float dx, float dy) {
            // @TODO: Make the pan scale dependent on the viewport size.
            const float panScale = camera.m_distance * 0.0025f;
            camera.Pan(-panScale * dx, panScale * dy);
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
   }

   Camera::Camera(float distance, float lookAtX, float lookAtY, float lookAtZ) : m_distance(distance), m_lookAt({lookAtX, lookAtY, lookAtZ}), m_rotation(kIdentityRotation) { }

   void Camera::CalculateViewMatrix() {
      vec3 position = m_distance * ApplyRotation(m_rotation, ZAxis) + m_lookAt;
      vec3 up = ApplyRotation(m_rotation, YAxis);
      LookAtMatrix(position, m_lookAt, up, m_viewMatrix);
   }

   void Camera::Update(bool rightMouseButtonDown, bool middleMouseButtonDown, int mouseX, int mouseY, int mouseDX, int mouseDY, int mouseWheelDelta) {
      if (mouseWheelDelta != 0) {
         AddMouseWheelZoomImpulse(m_zoom, m_wheelZoomScale, mouseWheelDelta);
      }

      if (rightMouseButtonDown) {
         AddMouseMoveZoomImpulse(m_zoom, mouseDY);
      }

      if (middleMouseButtonDown) {
         AddMouseMovePanImpulse(m_panX, m_panY, mouseDX, mouseDY);
      }

      ApplyZoomToCamera(*this);
      ApplyPanToCamera(*this);
   }

   void Camera::Pan(float dx, float dy) {
      m_lookAt = m_lookAt + ApplyRotation(m_rotation, vec3 {dx, dy, 0.0f});
   }
}
