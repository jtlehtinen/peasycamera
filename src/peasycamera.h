#pragma once

namespace peasycamera {

   struct vec3 { float x, y, z; };
   struct quat { float x, y, z, w; };

   constexpr quat kIdentityRotation = {0.0f, 0.0f, 0.0f, 1.0f};
   constexpr float kDefaultFriction = 0.16f;

   struct DampedAction {
      float m_velocity = 0.0f;
      float m_friction = kDefaultFriction;
   };

   enum class Constraint { None, Yaw, Pitch, Roll, SuppressRoll };

   struct CameraState {
      quat m_rotation;
      vec3 m_lookAt;
      float m_distance;
   };

   struct Camera {
      DampedAction m_panX;
      DampedAction m_panY;
      DampedAction m_zoom;
      DampedAction m_rotateX;
      DampedAction m_rotateY;
      DampedAction m_rotateZ;

      float m_minDistance = 1.0f;
      float m_maxDistance = 500.0f;
      float m_wheelZoomScale = 1.0f;

      CameraState m_state;

      Constraint m_dragConstraint = Constraint::None;

      float m_viewMatrix[16];

      Camera(float distance, float lookAtX = 0.0f, float lookAtY = 0.0f, float lookAtZ = 0.0f);

      void CalculateViewMatrix();
      void Update(bool shiftKeyDown, bool leftMouseButtonDown, bool rightMouseButtonDown, bool middleMouseButtonDown, int mouseX, int mouseY, int mouseDX, int mouseDY, int mouseWheelDelta,
                  int viewportLeft, int viewportTop, int viewportWidth, int viewportHeight);

      void Pan(float dx, float dy);
   };

}
