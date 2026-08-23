#include "Camera.h"
#include <cstring>

void mat4Identity(float m[16]) {
    memset(m,0,16*sizeof(float));
    m[0]=m[5]=m[10]=m[15]=1.f;
}
void mat4Multiply(const float a[16], const float b[16], float out[16]) {
    // column-major: out = a * b
    float tmp[16];
    for(int col=0;col<4;col++) for(int row=0;row<4;row++) {
        tmp[col*4+row] = a[0*4+row]*b[col*4+0] + a[1*4+row]*b[col*4+1] + a[2*4+row]*b[col*4+2] + a[3*4+row]*b[col*4+3];
    }
    memcpy(out, tmp, sizeof(tmp));
}

void mat4LookAt(const float eye[3], const float center[3], const float up[3], float out[16]) {
    float f[3] = {center[0]-eye[0], center[1]-eye[1], center[2]-eye[2]};
    float flen = sqrtf(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);
    f[0]/=flen; f[1]/=flen; f[2]/=flen;
    // s = f x up
    float s[3] = { f[1]*up[2]-f[2]*up[1], f[2]*up[0]-f[0]*up[2], f[0]*up[1]-f[1]*up[0] };
    float slen = sqrtf(s[0]*s[0]+s[1]*s[1]+s[2]*s[2]);
    s[0]/=slen; s[1]/=slen; s[2]/=slen;
    float u[3] = { s[1]*f[2]-s[2]*f[1], s[2]*f[0]-s[0]*f[2], s[0]*f[1]-s[1]*f[0] };
    // column-major
    out[0]= s[0]; out[1]= u[0]; out[2]= -f[0]; out[3]=0;
    out[4]= s[1]; out[5]= u[1]; out[6]= -f[1]; out[7]=0;
    out[8]= s[2]; out[9]= u[2]; out[10]=-f[2]; out[11]=0;
    out[12]= -(s[0]*eye[0]+s[1]*eye[1]+s[2]*eye[2]);
    out[13]= -(u[0]*eye[0]+u[1]*eye[1]+u[2]*eye[2]);
    out[14]=  (f[0]*eye[0]+f[1]*eye[1]+f[2]*eye[2]);
    out[15]=1;
}
void mat4Ortho(float l,float r,float b,float t,float n,float f,float out[16]) {
    mat4Identity(out);
    out[0]=2.f/(r-l);
    out[5]=2.f/(t-b);
    out[10]=-2.f/(f-n);
    out[12]=-(r+l)/(r-l);
    out[13]=-(t+b)/(t-b);
    out[14]=-(f+n)/(f-n);
}

void Camera::getEye(float eye[3]) const {
    float cy = cosf(yaw), sy = sinf(yaw);
    float cp = cosf(pitch), sp = sinf(pitch);
    // Spherical: yaw around Y, pitch up from horizontal
    float dx = cy * cp;
    float dy = sp;
    float dz = sy * cp;
    eye[0] = centerX + dx * distance;
    eye[1] = centerY + dy * distance;
    eye[2] = centerZ + dz * distance;
}
void Camera::getViewMatrix(float out[16]) const {
    float eye[3]; getEye(eye);
    float center[3] = {centerX, centerY, centerZ};
    float up[3] = {0,1,0};
    // handle singularity when pitch ~90deg: not needed (35deg)
    mat4LookAt(eye, center, up, out);
}
void Camera::getProjMatrix(float out[16]) const {
    float h = orthoBaseHeight / zoom;
    float w = h * aspect;
    float left = -w*0.5f;
    float right = w*0.5f;
    float bottom = -h*0.5f;
    float top = h*0.5f;
    mat4Ortho(left,right,bottom,top, nearPlane, farPlane, out);
}
void Camera::getViewProj(float out[16]) const {
    float v[16], p[16];
    getViewMatrix(v);
    getProjMatrix(p);
    mat4Multiply(p, v, out);
}

void Camera::ndcToWorldRay(float ndcX, float ndcY, float rayOrigin[3], float rayDir[3]) const {
    float eye[3]; getEye(eye);
    float center[3] = {centerX, centerY, centerZ};
    float f[3] = {center[0]-eye[0], center[1]-eye[1], center[2]-eye[2]};
    float flen = sqrtf(f[0]*f[0]+f[1]*f[1]+f[2]*f[2]);
    f[0]/=flen; f[1]/=flen; f[2]/=flen;
    float upWorld[3]={0,1,0};
    float right[3]={ f[1]*upWorld[2]-f[2]*upWorld[1], f[2]*upWorld[0]-f[0]*upWorld[2], f[0]*upWorld[1]-f[1]*upWorld[0]};
    float rlen= sqrtf(right[0]*right[0]+right[1]*right[1]+right[2]*right[2]);
    right[0]/=rlen; right[1]/=rlen; right[2]/=rlen;
    float up[3]={ right[1]*f[2]-right[2]*f[1], right[2]*f[0]-right[0]*f[2], right[0]*f[1]-right[1]*f[0]};

    float h = orthoBaseHeight / zoom;
    float w = h * aspect;
    float ox = ndcX * w * 0.5f;
    float oy = ndcY * h * 0.5f;
    rayOrigin[0] = eye[0] + right[0]*ox + up[0]*oy;
    rayOrigin[1] = eye[1] + right[1]*ox + up[1]*oy;
    rayOrigin[2] = eye[2] + right[2]*ox + up[2]*oy;
    rayDir[0]=f[0]; rayDir[1]=f[1]; rayDir[2]=f[2];
}

bool Camera::pickTile(float ndcX, float ndcY, int& outX, int& outY, int roomW, int roomH) const {
    float origin[3], dir[3];
    ndcToWorldRay(ndcX, ndcY, origin, dir);
    if (fabsf(dir[1]) < 1e-6f) return false;
    float t = -origin[1] / dir[1];
    if (t < 0) return false; // behind camera
    float hitX = origin[0] + dir[0]*t;
    float hitZ = origin[2] + dir[2]*t;
    int tx = (int)floorf(hitX);
    int tz = (int)floorf(hitZ);
    if (tx <0 || tx >= roomW || tz <0 || tz >= roomH) return false;
    outX = tx; outY = tz;
    return true;
}
