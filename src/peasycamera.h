#pragma once

namespace peasycamera {

   struct vec3 { float x, y, z; };
   struct quat { float x, y, z, w; };

   constexpr quat kIdentityRotation = {0.0f, 0.0f, 0.0f, 1.0f};

   struct Camera {
      float m_viewMatrix[16];

      quat m_rotation;
      vec3 m_lookAt;
      float m_distance;

      Camera(float distance, float lookAtX = 0.0f, float lookAtY = 0.0f, float lookAtZ = 0.0f);

      void CalculateViewMatrix();
   };

}
