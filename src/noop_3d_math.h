#pragma once
#include <glm/glm.hpp>
#include <math.h>
#include <xmmintrin.h>
#include "noop_types.h"

/*
 * Insulates glm types from being stamped into the project
 * for easier replacement when desired
 * */
#define Vec2 glm::vec2
#define Vec3 glm::vec3
#define Vec4 glm::vec4
#define Mat3 glm::mat3
#define Mat4 glm::mat4

const Mat4 mat4_identity = Mat4(1.0f);

struct BoundingBox {
  Vec3 min;
  Vec3 dimensionInMeters;
};

void adjustNearFarProjection(Mat4* projectionMatrix, f32 zNear, f32 zFar) {
  // Note: logic pulled straight from glm::perspective -> perspectiveRH_NO
  (*projectionMatrix)[2][2] = - (zFar + zNear) / (zFar - zNear);
  (*projectionMatrix)[3][2] = - (2.0f * zFar * zNear) / (zFar - zNear);
}

union vec2 {
  struct {
    f32 x, y;
  };
  struct {
    f32 r, g;
  };
  f32 c[2];
};

union vec3 {
  struct {
    f32 x, y, z;
  };
  struct {
    vec2 xy;
    f32 _w_;
  };
  struct {
    f32 i, j, k;
  };
  struct {
    f32 r, g, b;
  };
  struct {
    vec2 rg;
    f32 _b_;
  };
  f32 c[3];
};

union vec4 {
  struct {
    f32 x, y, z, w;
  };
  struct {
    vec2 xy;
    vec2 zw;
  };
  struct{
    vec3 xyz;
    f32 _w_;
  };
  struct {
    f32 r, g, b, a;
  };
  struct {
    vec2 rg;
    vec2 ba;
  };
  struct{
    vec3 rgb;
    f32 _a_;
  };
  f32 c[4];
};

// NOTE: column-major
union mat4 {
  struct{
    vec4 xBasis;
    vec4 yBasis;
    vec4 zBasis;
    vec4 translation;
  };
  f32 c[4][4];
  vec4 col[4];
};

//inline vec4 operator*(const mat4& M, const vec4& v) {
//  __m128 vX = _mm_shuffle_ps(v.m128, v.m128, 0x00); //{x, x, x, x}
//  __m128 vY = _mm_shuffle_ps(v.m128, v.m128, 0x55); //{y, y, y, y}
//  __m128 vZ = _mm_shuffle_ps(v.m128, v.m128, 0xAA); //{z, z, z, z}
//  __m128 vW = _mm_shuffle_ps(v.m128, v.m128, 0xFF); //{w, w, w, w}
//
//  vec4 result;
//  result.m128 =                         _mm_mul_ps(vX, M.col[0].m128); // going x steps in the new x basis vector
//  result.m128 = _mm_add_ps(result.m128, _mm_mul_ps(vY, M.col[1].m128));  // going y steps in the new y basis vector
//  result.m128 = _mm_add_ps(result.m128, _mm_mul_ps(vZ, M.col[2].m128));  // going z steps in the new z basis vector
//  result.m128 = _mm_add_ps(result.m128, _mm_mul_ps(vW, M.col[3].m128));  // going w steps in the new w basis vector
//  return result;
//}

//inline mat4 operator*(const mat4& A, const mat4& B) {
//  mat4 result;
//  result.col[0] =
//}

// Vec2
inline f32 dot(vec2 xy1, vec2 xy2) {
  return (xy1.x * xy2.x) + (xy1.y * xy2.y);
}

inline f32 magnitudeSquared(vec2 xy) {
  return (xy.x * xy.x) + (xy.y * xy.y);
}

inline f32 magnitude(vec2 xy) {
  return sqrt((xy.x * xy.x) + (xy.y * xy.y));
}

inline vec2 normalize(const vec2& xy) {
  f32 mag = magnitude(xy);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / magnitude(xy);
  return vec2{xy.x * magInv, xy.y * magInv};
}

inline vec2 operator-(const vec2& xy1) {
  return vec2{-xy1.x, -xy1.y};
}

inline vec2 operator-(const vec2& xy1, const vec2& xy2) {
  return vec2{xy1.x - xy2.x, xy1.y - xy2.y };
}

inline vec2 operator+(const vec2& xy1, const vec2& xy2) {
  return vec2{xy1.x + xy2.x, xy1.y + xy2.y };
}

inline vec2 operator*(f32 s, vec2 xy) {
  return vec2{xy.x * s, xy.y * s};
}

inline vec2 operator*(vec2 xy, f32 s) {
  return vec2{xy.x * s, xy.y * s};
}

// Vec3
inline f32 dot(vec3 xyz1, vec3 xyz2) {
  return (xyz1.x * xyz2.x) + (xyz1.y * xyz2.y) + (xyz1.z * xyz2.z);
}

inline vec3 hadamard(vec3 xyz1, vec3 xyz2) {
  return vec3{ xyz1.x * xyz2.x, xyz2.y * xyz2.y, xyz1.z * xyz2.z };
}

inline vec3 cross(vec3 xyz1, vec3 xyz2) {
  return vec3{ (xyz1.y * xyz2.z - xyz2.y * xyz1.z),
               (xyz1.z * xyz2.x - xyz2.z * xyz1.x),
               (xyz1.x * xyz2.y - xyz2.x * xyz1.y) };
}

inline f32 magnitudeSquared(Vec3 xyz) {
  return (xyz.x * xyz.x) + (xyz.y * xyz.y) + (xyz.z * xyz.z);
}

inline f32 magnitude(vec3 xyz) {
  return sqrt((xyz.x * xyz.x) + (xyz.y * xyz.y) + (xyz.z * xyz.z));
}

inline vec3 normalize(const vec3& xyz) {
  f32 mag = magnitude(xyz);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / magnitude(xyz);
  return vec3{xyz.x * magInv, xyz.y * magInv, xyz.z * magInv};
}

inline vec3 operator-(const vec3& xyz1) {
  return vec3{-xyz1.x, -xyz1.y, -xyz1.z};
}

inline vec3 operator-(const vec3& xyz1, const vec3& xyz2) {
  return vec3{xyz1.x - xyz2.x, xyz1.y - xyz2.y, xyz1.z - xyz2.z };
}

inline vec3 operator+(const vec3& xyz1, const vec3& xyz2) {
  return vec3{xyz1.x + xyz2.x, xyz1.y + xyz2.y, xyz1.z + xyz2.z };
}

inline vec3 operator*(f32 s, vec3 xyz) {
  return vec3{xyz.x * s, xyz.y * s, xyz.z * s};
}

inline vec3 operator*(vec3 xyz, f32 s) {
  return vec3{xyz.x * s, xyz.y * s, xyz.z * s};
}

// Vec4
inline f32 dot(vec4 xyzw1, vec4 xyzw2) {
  return (xyzw1.x * xyzw2.x) + (xyzw1.y * xyzw2.y) + (xyzw1.z * xyzw2.z) + (xyzw1.w * xyzw2.w);
}

inline f32 magnitudeSquared(vec4 xyzw) {
  return (xyzw.x * xyzw.x) + (xyzw.y * xyzw.y) + (xyzw.z * xyzw.z) + (xyzw.w * xyzw.w);
}

inline f32 magnitude(vec4 xyzw) {
  return sqrt((xyzw.x * xyzw.x) + (xyzw.y * xyzw.y) + (xyzw.z * xyzw.z) + (xyzw.w * xyzw.w));
}

inline vec4 normalize(const vec4& xyzw) {
  f32 mag = magnitude(xyzw);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / magnitude(xyzw);
  return vec4{xyzw.x * magInv, xyzw.y * magInv, xyzw.z * magInv, xyzw.w * magInv};
}

inline vec4 operator-(const vec4& xyzw1) {
  return vec4{ -xyzw1.x, -xyzw1.y, -xyzw1.z, -xyzw1.w };
}

inline vec4 operator-(const vec4& xyzw1, const vec4& xyzw2) {
  return vec4{ xyzw1.x - xyzw2.x, xyzw1.y - xyzw2.y, xyzw1.z - xyzw2.z, xyzw1.w - xyzw2.w };
}

inline vec4 operator+(const vec4& xyzw1, const vec4& xyzw2) {
  return vec4{ xyzw1.x + xyzw2.x, xyzw1.y + xyzw2.y, xyzw1.z + xyzw2.z, xyzw1.w + xyzw2.w };
}

inline vec4 operator*(f32 s, vec4 xyzw) {
  return vec4{xyzw.x * s, xyzw.y * s, xyzw.z * s, xyzw.w * s};
}

inline vec4 operator*(vec4 xyzw, f32 s) {
  return vec4{xyzw.x * s, xyzw.y * s, xyzw.z * s, xyzw.w * s};
}

inline void operator+=(vec4& xyzw1, const vec4& xyzw2){
  xyzw1.x += xyzw2.x;
  xyzw1.y += xyzw2.y;
  xyzw1.z += xyzw2.z;
  xyzw1.w += xyzw2.w;
}

// mat4
inline mat4 identity_mat4() {
  return mat4 {
          1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f,
  };
}

inline mat4 scale_mat4(f32 scale) {
  return mat4 {
          scale, 0.0f, 0.0f, 0.0f,
          0.0f, scale, 0.0f, 0.0f,
          0.0f, 0.0f, scale, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f,
  };
}

inline mat4 scale_mat4(vec3 scale) {
  return mat4 {
          scale.x, 0.0f, 0.0f, 0.0f,
          0.0f, scale.y, 0.0f, 0.0f,
          0.0f, 0.0f, scale.z, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f,
  };
}

inline mat4 translate_mat4(vec3 translation) {
  return mat4 {
          1.0f, 0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f, 0.0f,
          0.0f, 0.0f, 1.0f, 0.0f,
          translation.x, translation.y, translation.z, 1.0f,
  };
}

inline mat4 transpose_mat4(const mat4& A) {
  return mat4 {
          A.c[0][0], A.c[1][0], A.c[2][0], A.c[3][0],
          A.c[0][1], A.c[1][1], A.c[2][1], A.c[3][1],
          A.c[0][2], A.c[1][2], A.c[2][2], A.c[3][2],
          A.c[0][3], A.c[1][3], A.c[2][3], A.c[3][3],
  };
}

// angle in radians
inline mat4 rotate_mat4(f32 angle, vec3 v) {
  vec3 axis(normalize(v));

  f32 const cosA = cos(angle);
  f32 const sinA = sin(angle);
  vec3 const axisTimesOneMinusCos = axis * (1.0f - cosA);

  mat4 rotate;
  rotate.c[0][0] = axis.x * axisTimesOneMinusCos.x + cosA;
  rotate.c[0][1] = axis.x * axisTimesOneMinusCos.y + sinA * axis.z;
  rotate.c[0][2] = axis.x * axisTimesOneMinusCos.z - sinA * axis.y;
  rotate.c[0][3] = 0;

  rotate.c[1][0] = axis.y * axisTimesOneMinusCos.x - sinA * axis.z;
  rotate.c[1][1] = axis.y * axisTimesOneMinusCos.y + cosA;
  rotate.c[1][2] = axis.y * axisTimesOneMinusCos.z + sinA * axis.x;
  rotate.c[1][3] = 0;

  rotate.c[2][0] = axis.z * axisTimesOneMinusCos.x + sinA * axis.y;
  rotate.c[2][1] = axis.z * axisTimesOneMinusCos.y - sinA * axis.x;
  rotate.c[2][2] = axis.z * axisTimesOneMinusCos.z + cosA;
  rotate.c[2][3] = 0;

  rotate.col[3] = {0.0f, 0.0f, 0.0f, 1.0f};

  return rotate;
}

inline vec4 operator*(const mat4& M, const vec4& v) {
  vec4 result =  M.col[0] * v.x;
  result      += M.col[1] * v.y;
  result      += M.col[2] * v.z;
  result      += M.col[3] * v.w;
  return result;
}

mat4 operator*(const mat4& A, const mat4& B) {
  mat4 result;

  mat4 transposeA = transpose_mat4(A); // cols <=> rows
  result.c[0][0] = dot(transposeA.col[0], B.col[0]);
  result.c[0][1] = dot(transposeA.col[1], B.col[0]);
  result.c[0][2] = dot(transposeA.col[2], B.col[0]);
  result.c[0][3] = dot(transposeA.col[3], B.col[0]);
  result.c[1][0] = dot(transposeA.col[0], B.col[1]);
  result.c[1][1] = dot(transposeA.col[1], B.col[1]);
  result.c[1][2] = dot(transposeA.col[2], B.col[1]);
  result.c[1][3] = dot(transposeA.col[3], B.col[1]);
  result.c[2][0] = dot(transposeA.col[0], B.col[2]);
  result.c[2][1] = dot(transposeA.col[1], B.col[2]);
  result.c[2][2] = dot(transposeA.col[2], B.col[2]);
  result.c[2][3] = dot(transposeA.col[3], B.col[2]);
  result.c[3][0] = dot(transposeA.col[0], B.col[3]);
  result.c[3][1] = dot(transposeA.col[1], B.col[3]);
  result.c[3][2] = dot(transposeA.col[2], B.col[3]);
  result.c[3][3] = dot(transposeA.col[3], B.col[3]);

  return result;
}

// Quaternions
union quaternion {
  struct {
    f32 r, i, j, k;
  };
  struct {
    f32 real;
    vec3 ijk;
  };
};

quaternion Quaternion(f32 angle, vec3 v) {
  vec3 n = normalize(v);

  f32 halfA = angle * 0.5f;
  f32 cosHalfA = cos(halfA);
  f32 sinHalfA = sin(halfA);

  quaternion result;
  result.real = cosHalfA;
  result.ijk = sinHalfA * n;

  return result;
}

/*
 * ii = jj = kk = ijk = -1
 * ij = -ji = k
 * jk = -kj = i
 * ki = -ik = j
 */
vec3 operator*(const quaternion& q, vec3 v) {
  Assert(magnitudeSquared())

  quaternion result;
  quaternion qv;
  quaternion qInv = {q.real, -q.i, -q.j, -q.k};

  qv.r = -(q.i * v.i) + -(q.j * v.j) + -(q.k * v.k);
  qv.i =  (q.r * v.i) + -(q.k * v.j) +  (q.j * v.k);
  qv.j =  (q.k * v.i) +  (q.r * v.j) + -(q.i * v.k);
  qv.k = -(q.j * v.i) +  (q.i * v.j) +  (q.r * v.k);

  result.r = (qv.r * qInv.r) + -(qv.i * qInv.i) + -(qv.j * qInv.j) + -(qv.k * qInv.k);
  result.i = (qv.i * qInv.r) +  (qv.r * qInv.i) + -(qv.k * qInv.j) +  (qv.j * qInv.k);
  result.j = (qv.j * qInv.r) +  (qv.k * qInv.i) +  (qv.r * qInv.j) + -(qv.i * qInv.k);
  result.k = (qv.k * qInv.r) + -(qv.j * qInv.i) +  (qv.i * qInv.j) +  (qv.r * qInv.k);

  return result.ijk;
}