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

   template <typename T>
   struct Interpolator {
      float timeInSeconds = 0.0f;
      float timeConsumedInSeconds = 0.0f;
      T startValue = { };
      T endValue = { };
   };

   struct Input {
      int viewport[4];
      int mouseX;
      int mouseY;
      int mouseDX;
      int mouseDY;
      int mouseWheelDelta;
      float deltaTimeInSeconds;

      bool leftMouseButtonDown;
      bool middleMouseButtonDown;
      bool rightMouseButtonDown;
      bool shiftKeyDown;
   };

   struct Camera {
      Interpolator<float> m_distanceInterpolator;
      Interpolator<vec3> m_lookAtInterpolator;
      Interpolator<quat> m_rotationInterpolator;

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
      CameraState m_resetState;

      Constraint m_dragConstraint = Constraint::None;
      Constraint m_permaConstraint = Constraint::None;

      float m_viewMatrix[16];

      Camera(float distance, float lookAtX = 0.0f, float lookAtY = 0.0f, float lookAtZ = 0.0f);

      void CalculateViewMatrix();
      void Update(const Input& input);

      void Pan(float dx, float dy);

      float GetDistance() const { return m_state.m_distance; }
      float GetWheelZoomScale() const { return m_wheelZoomScale; }
      void GetLookAt(float* outX, float* outY, float* outZ) const { *outX = m_state.m_lookAt.x; *outY = m_state.m_lookAt.y; *outZ = m_state.m_lookAt.z; }
      void GetPosition(float* outX, float* outY, float* outZ) const;
      float GetMaxDistance() const { return m_maxDistance; }
      float GetMinDistance() const { return m_minDistance; }

      void SetDistance(float distance, float animationTimeInSeconds = 0.0f);
      void SetWheelZoomScale(float wheelZoomScale) { m_wheelZoomScale = wheelZoomScale; }
      void SetLookAt(float x, float y, float z, float animationTimeInSeconds = 0.0f);
      void SetMaxDistance(float maxDistance) { m_maxDistance = maxDistance; }
      void SetMinDistance(float minDistance) { m_minDistance = minDistance; }

      CameraState GetState() const { return m_state; }
      void SetState(const CameraState& state, float animationTimeInSeconds = 0.0f);

      void Reset(float animationTimeInSeconds = 0.0f);

      void SetFreeRotationMode();
      void SetYawRotationMode();
      void SetPitchRotationMode();
      void SetRollRotationMode();
      void SetSuppressRollRotationMode();

      void RotateX(float radians);
      void RotateY(float radians);
      void RotateZ(float radians);

      void SetRotations(float rx, float ry, float rz);
      void GetRotations(float* outRX, float* outRY, float* outRZ) const;

      // @TODO: Custom impulse providers
   };

}
