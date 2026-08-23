#pragma once
#include <cmath>

struct Camera {
    // Configurable constants — tuned to reference
    // yaw 225° (45°+180°) views interior from SW looking NE so inner walls are lit;
    // 45° would face outer walls.
    float yaw = 225.0f * 3.14159265f / 180.0f;   // around Y
    float pitch = 30.0f * 3.14159265f / 180.0f; // elevation — 30° gives 2:1 tile (64×32) crisp, POV from SW
    float zoom = 1.0f;
    float distance = 22.0f;
    float centerX = 4.0f; // room center
    float centerY = 0.0f;
    float centerZ = 3.0f;
    float orthoBaseHeight = 11.0f; // world units visible vertically at zoom=1 — matches Image 1 framing (70-80% fill)
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    void setAspect(float w, float h) { aspect = w / h; }
    void setYawPitch(float y, float p) { yaw=y; pitch=p; }
    void setZoom(float z) { zoom = z < 0.2f ? 0.2f : (z>5.0f?5.0f:z); }

    void getEye(float eye[3]) const;
    void getViewMatrix(float out[16]) const;
    void getProjMatrix(float out[16]) const;
    void getViewProj(float out[16]) const;

    // For picking: compute world ray from NDC
    void ndcToWorldRay(float ndcX, float ndcY, float rayOrigin[3], float rayDir[3]) const;
    bool pickTile(float ndcX, float ndcY, int& outX, int& outY, int roomW, int roomH) const;
};

// Math helpers (column-major)
void mat4Identity(float m[16]);
void mat4Multiply(const float a[16], const float b[16], float out[16]);
void mat4LookAt(const float eye[3], const float center[3], const float up[3], float out[16]);
void mat4Ortho(float left, float right, float bottom, float top, float nearv, float farv, float out[16]);
