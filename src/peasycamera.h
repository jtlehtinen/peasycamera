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

   struct Camera {
      float m_viewMatrix[16];

      DampedAction m_zoom;
      float m_wheelZoomScale = 1.0f;

      float m_minDistance = 1.0f;
      float m_maxDistance = 500.0f;

      quat m_rotation;
      vec3 m_lookAt;
      float m_distance;

      Camera(float distance, float lookAtX = 0.0f, float lookAtY = 0.0f, float lookAtZ = 0.0f);

      void CalculateViewMatrix();
      void Update(bool rightMouseButtonDown, int mouseX, int mouseY, int mouseDX, int mouseDY, int mouseWheelDelta);
   };

}
