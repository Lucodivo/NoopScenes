#pragma once

// TODO: No optimizations have been made in this file. Ideas: intrinsics, sse, better usage of temporary memory.

#define Min(x, y) (x < y ? x : y)
#define Max(x, y) (x > y ? x : y)

#define cos30 0.86602540f
#define cos45 0.70710678f
#define cos60 0.5f

#define sin30 0.5f
#define sin45 0.70710678f
#define sin60 0.86602540f

union vec2;
union vec3;
union vec4;
union mat3;
union mat4;
union quaternion;

union vec2 {
  struct {
    f32 x, y;
  };
  struct {
    f32 real, i;
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
    f32 _;
  };
  struct {
    f32 i, j, k;
  };
  struct {
    f32 r, g, b;
  };
  struct {
    vec2 rg;
    f32 __;
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
    f32 _;
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
    f32 __;
  };
  f32 c[4];
};

// NOTE: column-major
union mat3 {
  struct{
    vec3 xTransform;
    vec3 yTransform;
    vec3 zTransform;
  };
  f32 c[3][3]; // ij component
  f32 values[9]; // single array values
  vec3 col[3];
};

// NOTE: column-major
union mat4 {
  struct{
    vec4 xTransform;
    vec4 yTransform;
    vec4 zTransform;
    vec4 translation;
  };
  f32 c[4][4]; // ij component
  f32 values[16]; // single array values
  vec4 col[4];
};

union quaternion {
  struct {
    f32 r, i, j, k;
  };
  struct {
    f32 real;
    vec3 ijk;
  };
};

union complex {
  struct {
    f32 r, i;
  };
  struct {
    f32 real, _;
  };
};

struct BoundingRect {
  vec2 min;
  vec2 diagonal;
};

struct BoundingBox {
  vec3 min;
  vec3 diagonal;
};

// floating point
inline f32 step(f32 edge, f32 x) {
  return x < edge ? 0.0f : 1.0f;
}

inline f32 clamp(f32 minVal, f32 maxVal, f32 x) {
  return Min(maxVal, Max(minVal, x));
}

inline f32 smoothStep(f32 edge1, f32 edge2, f32 x) {
  f32 t = clamp((x - edge1) / (edge2 - edge1), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

inline f32 lerp(f32 a, f32 b, f32 t) {
  Assert(t >= 0.0f && t <= 1.0f)
  return a - ((a + b) * t);
}

// vec2
inline f32 dot(vec2 xy1, vec2 xy2) {
  return (xy1.x * xy2.x) + (xy1.y * xy2.y);
}

inline f32 magnitudeSquared(vec2 xy) {
  return (xy.x * xy.x) + (xy.y * xy.y);
}

inline f32 magnitude(vec2 xy) {
  return sqrtf((xy.x * xy.x) + (xy.y * xy.y));
}

inline vec2 normalize(const vec2& xy) {
  f32 mag = magnitude(xy);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / mag;
  return vec2{xy.x * magInv, xy.y * magInv};
}

inline vec2 normalize(f32 x, f32 y) {
  f32 mag = sqrtf(x * x + y * y);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / mag;
  return vec2{x * magInv, y * magInv};
}

inline vec2 operator-(const vec2& xy) {
  return vec2{-xy.x, -xy.y};
}

inline vec2 operator-(const vec2& xy1, const vec2& xy2) {
  return vec2{xy1.x - xy2.x, xy1.y - xy2.y };
}

inline vec2 operator+(const vec2& xy1, const vec2& xy2) {
  return vec2{xy1.x + xy2.x, xy1.y + xy2.y };
}

inline void operator-=(vec2& xy1, const vec2& xy2) {
  xy1.x -= xy2.x;
  xy1.y -= xy2.y;
}

inline void operator+=(vec2& xy1, const vec2& xy2) {
  xy1.x += xy2.x;
  xy1.y += xy2.y;
}

inline vec2 operator*(f32 s, vec2 xy) {
  return vec2{xy.x * s, xy.y * s};
}

inline vec2 operator*(vec2 xy, f32 s) {
  return vec2{xy.x * s, xy.y * s};
}

inline void operator*=(vec2& xy, f32 s) {
  xy.x *= s;
  xy.y *= s;
}

inline vec2 operator/(const vec2& xy, const f32 s) {
  f32 scaleInv = 1.0f / s;
  return {xy.x * scaleInv, xy.y * scaleInv};
}

inline vec2 lerp(const vec2& a, const vec2& b, f32 t) {
  Assert(t >= 0.0f && t <= 1.0f)
  return a - ((a + b) * t);
}

// vec3
inline vec3 Vec3(vec2 xy, f32 z) {
  return vec3{xy.x, xy.y, z};
}

inline f32 dot(vec3 xyz1, vec3 xyz2) {
  return (xyz1.x * xyz2.x) + (xyz1.y * xyz2.y) + (xyz1.z * xyz2.z);
}

inline vec3 hadamard(const vec3& xyz1, const vec3& xyz2) {
  return vec3{ xyz1.x * xyz2.x, xyz1.y * xyz2.y, xyz1.z * xyz2.z };
}

inline vec3 cross(const vec3& xyz1, const vec3& xyz2) {
  return vec3{ (xyz1.y * xyz2.z - xyz2.y * xyz1.z),
               (xyz1.z * xyz2.x - xyz2.z * xyz1.x),
               (xyz1.x * xyz2.y - xyz2.x * xyz1.y) };
}

inline f32 magnitudeSquared(vec3 xyz) {
  return (xyz.x * xyz.x) + (xyz.y * xyz.y) + (xyz.z * xyz.z);
}

inline f32 magnitude(vec3 xyz) {
  return sqrtf((xyz.x * xyz.x) + (xyz.y * xyz.y) + (xyz.z * xyz.z));
}

inline vec3 normalize(const vec3& xyz) {
  f32 mag = magnitude(xyz);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / mag;
  return vec3{xyz.x * magInv, xyz.y * magInv, xyz.z * magInv};
}

inline vec3 normalize(f32 x, f32 y, f32 z) {
  f32 mag = sqrtf(x * x + y * y + z * z);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / mag;
  return vec3{x * magInv, y * magInv, z * magInv};
}

inline vec3 operator-(const vec3& xyz) {
  return vec3{-xyz.x, -xyz.y, -xyz.z};
}

inline vec3 operator-(const vec3& xyz1, const vec3& xyz2) {
  return vec3{xyz1.x - xyz2.x, xyz1.y - xyz2.y, xyz1.z - xyz2.z };
}

inline vec3 operator+(const vec3& xyz1, const vec3& xyz2) {
  return vec3{xyz1.x + xyz2.x, xyz1.y + xyz2.y, xyz1.z + xyz2.z };
}

inline void operator-=(vec3& xyz1, const vec3& xyz2) {
  xyz1.x -= xyz2.x;
  xyz1.y -= xyz2.y;
  xyz1.z -= xyz2.z;
}

inline void operator+=(vec3& xyz1, const vec3& xyz2) {
  xyz1.x += xyz2.x;
  xyz1.y += xyz2.y;
  xyz1.z += xyz2.z;
}

inline vec3 operator*(f32 s, vec3 xyz) {
  return vec3{xyz.x * s, xyz.y * s, xyz.z * s};
}

inline vec3 operator*(vec3 xyz, f32 s) {
  return vec3{xyz.x * s, xyz.y * s, xyz.z * s};
}

inline void operator*=(vec3& xyz, f32 s) {
  xyz.x *= s;
  xyz.y *= s;
  xyz.z *= s;
}

inline vec3 operator/(const vec3& xyz, const f32 s) {
  f32 scaleInv = 1.0f / s;
  return {xyz.x * scaleInv, xyz.y * scaleInv, xyz.z * scaleInv};
}

inline vec3 lerp(const vec3& a, const vec3& b, f32 t) {
  Assert(t >= 0.0f && t <= 1.0f)
  return a - ((a + b) * t);
}

// vec4
inline vec4 Vec4(vec3 xyz, f32 w) {
  return vec4{xyz.x, xyz.y, xyz.z, w};
}

inline vec4 Vec4(f32 x, vec3 yzw) {
  return vec4{x, yzw.c[0], yzw.c[1], yzw.c[2]};
}

inline vec4 Vec4(vec2 xy, vec2 zw) {
  return vec4{xy.x, xy.y, zw.c[0], zw.c[1]};
}

inline f32 dot(vec4 xyzw1, vec4 xyzw2) {
  return (xyzw1.x * xyzw2.x) + (xyzw1.y * xyzw2.y) + (xyzw1.z * xyzw2.z) + (xyzw1.w * xyzw2.w);
}

inline f32 magnitudeSquared(vec4 xyzw) {
  return (xyzw.x * xyzw.x) + (xyzw.y * xyzw.y) + (xyzw.z * xyzw.z) + (xyzw.w * xyzw.w);
}

inline f32 magnitude(vec4 xyzw) {
  return sqrtf((xyzw.x * xyzw.x) + (xyzw.y * xyzw.y) + (xyzw.z * xyzw.z) + (xyzw.w * xyzw.w));
}

inline vec4 normalize(const vec4& xyzw) {
  f32 mag = magnitude(xyzw);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / mag;
  return vec4{xyzw.x * magInv, xyzw.y * magInv, xyzw.z * magInv, xyzw.w * magInv};
}

inline vec4 normalize(f32 x, f32 y, f32 z, f32 w) {
  f32 mag = sqrtf(x * x + y * y + z * z + w * w);
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / mag;
  return vec4{x * magInv, y * magInv, z * magInv, w * magInv};
}

inline vec4 operator-(const vec4& xyzw) {
  return vec4{-xyzw.x, -xyzw.y, -xyzw.z, -xyzw.w };
}

inline vec4 operator-(const vec4& xyzw1, const vec4& xyzw2) {
  return vec4{ xyzw1.x - xyzw2.x, xyzw1.y - xyzw2.y, xyzw1.z - xyzw2.z, xyzw1.w - xyzw2.w };
}

inline vec4 operator+(const vec4& xyzw1, const vec4& xyzw2) {
  return vec4{ xyzw1.x + xyzw2.x, xyzw1.y + xyzw2.y, xyzw1.z + xyzw2.z, xyzw1.w + xyzw2.w };
}

inline void operator-=(vec4& xyzw1, const vec4& xyzw2){
  xyzw1.x -= xyzw2.x;
  xyzw1.y -= xyzw2.y;
  xyzw1.z -= xyzw2.z;
  xyzw1.w -= xyzw2.w;
}

inline void operator+=(vec4& xyzw1, const vec4& xyzw2) {
  xyzw1.x += xyzw2.x;
  xyzw1.y += xyzw2.y;
  xyzw1.z += xyzw2.z;
  xyzw1.w += xyzw2.w;
}

inline vec4 operator*(f32 s, vec4 xyzw) {
  return vec4{xyzw.x * s, xyzw.y * s, xyzw.z * s, xyzw.w * s};
}

inline vec4 operator*(vec4 xyzw, f32 s) {
  return vec4{xyzw.x * s, xyzw.y * s, xyzw.z * s, xyzw.w * s};
}

inline void operator*=(vec4& xyzw, f32 s) {
  xyzw.x *= s;
  xyzw.y *= s;
  xyzw.z *= s;
  xyzw.w *= s;
}

inline vec4 operator/(const vec4& xyzw, const f32 s) {
  f32 scaleInv = 1.0f / s;
  return {xyzw.x * scaleInv, xyzw.y * scaleInv, xyzw.z * scaleInv, xyzw.w * scaleInv};
}

inline vec4 lerp(const vec4& a, const vec4& b, f32 t) {
  Assert(t >= 0.0f && t <= 1.0f)
  return a - ((a + b) * t);
}

// mat3
inline mat3 identity_mat3() {
  return mat3 {
          1.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f,
          0.0f, 0.0f, 1.0f,
  };
}

inline mat3 scale_mat3(f32 scale) {
  return mat3 {
          scale, 0.0f, 0.0f,
          0.0f, scale, 0.0f,
          0.0f, 0.0f, scale,
  };
}

inline mat3 scale_mat3(vec3 scale) {
  return mat3 {
          scale.x, 0.0f, 0.0f,
          0.0f, scale.y, 0.0f,
          0.0f, 0.0f, scale.z,
  };
}

inline mat3 transpose_mat3(const mat3& A) {
  return mat3 {
          A.c[0][0], A.c[1][0], A.c[2][0],
          A.c[0][1], A.c[1][1], A.c[2][1],
          A.c[0][2], A.c[1][2], A.c[2][2],
  };
}

// angle in radians
inline mat3 rotate_mat3(f32 angle, vec3 v) {
  vec3 axis(normalize(v));

  f32 const cosA = cosf(angle);
  f32 const sinA = sinf(angle);
  vec3 const axisTimesOneMinusCos = axis * (1.0f - cosA);

  mat3 rotate;
  rotate.c[0][0] = axis.x * axisTimesOneMinusCos.x + cosA;
  rotate.c[0][1] = axis.x * axisTimesOneMinusCos.y + sinA * axis.z;
  rotate.c[0][2] = axis.x * axisTimesOneMinusCos.z - sinA * axis.y;

  rotate.c[1][0] = axis.y * axisTimesOneMinusCos.x - sinA * axis.z;
  rotate.c[1][1] = axis.y * axisTimesOneMinusCos.y + cosA;
  rotate.c[1][2] = axis.y * axisTimesOneMinusCos.z + sinA * axis.x;

  rotate.c[2][0] = axis.z * axisTimesOneMinusCos.x + sinA * axis.y;
  rotate.c[2][1] = axis.z * axisTimesOneMinusCos.y - sinA * axis.x;
  rotate.c[2][2] = axis.z * axisTimesOneMinusCos.z + cosA;

  return rotate;
}

inline vec3 operator*(const mat3& M, const vec3& v) {
  vec3 result =  M.col[0] * v.x;
  result      += M.col[1] * v.y;
  result      += M.col[2] * v.z;
  return result;
}

mat3 operator*(const mat3& A, const mat3& B) {
  mat3 result;

  mat3 transposeA = transpose_mat3(A); // cols <=> rows
  result.c[0][0] = dot(transposeA.col[0], B.col[0]);
  result.c[0][1] = dot(transposeA.col[1], B.col[0]);
  result.c[0][2] = dot(transposeA.col[2], B.col[0]);
  result.c[1][0] = dot(transposeA.col[0], B.col[1]);
  result.c[1][1] = dot(transposeA.col[1], B.col[1]);
  result.c[1][2] = dot(transposeA.col[2], B.col[1]);
  result.c[2][0] = dot(transposeA.col[0], B.col[2]);
  result.c[2][1] = dot(transposeA.col[1], B.col[2]);
  result.c[2][2] = dot(transposeA.col[2], B.col[2]);

  return result;
}

// mat4
inline mat4 Mat4(mat3 M) {
  return mat4{
    M.c[0][1], M.c[0][1], M.c[0][1], 0.0f,
    M.c[0][1], M.c[0][1], M.c[0][1], 0.0f,
    M.c[0][1], M.c[0][1], M.c[0][1], 0.0f,
         0.0f,      0.0f,      0.0f, 1.0f,
  };
}

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
          scale,  0.0f,  0.0f, 0.0f,
           0.0f, scale,  0.0f, 0.0f,
           0.0f,  0.0f, scale, 0.0f,
           0.0f,  0.0f,  0.0f, 1.0f,
  };
}

inline mat4 scale_mat4(vec3 scale) {
  return mat4 {
          scale.x,    0.0f,    0.0f, 0.0f,
             0.0f, scale.y,    0.0f, 0.0f,
             0.0f,    0.0f, scale.z, 0.0f,
             0.0f,    0.0f,    0.0f, 1.0f,
  };
}

inline mat4 translate_mat4(vec3 translation) {
  return mat4 {
                   1.0f,          0.0f,          0.0f, 0.0f,
                   0.0f,          1.0f,          0.0f, 0.0f,
                   0.0f,          0.0f,          1.0f, 0.0f,
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

  f32 const cosA = cosf(angle);
  f32 const sinA = sinf(angle);
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

// real-time rendering 4.7.2
// ex: screenWidth = 20.0f, screenDist = 30.0f will provide the horizontal field of view
// for a person sitting 30 inches away from a 20 inch screen, assuming the screen is
// perpendicular to the line of sight.
// NOTE: Any units work as long as they are the same. Works for vertical and horizontal.
f32 fieldOfView(f32 screenWidth, f32 screenDist) {
  f32 phi = 2.0f * atanf(screenWidth/(2.0f * screenDist));
  return phi;
}

// real-time rendering 4.7.1
// This projection is for a canonical view volume goes from <-1,1>
inline mat4 orthographic(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
    // NOTE: Projection matrices are all about creating properly placing
    // objects in the canonical view volume, which in the case of OpenGL
    // is from <-1,-1,-1> to <1,1,1>.
    // The 3x3 matrix is dividing vectors/points by the specified dimensions
    // and multiplying by two. That is because the the canonical view volume
    // in our case actually has dimensions of <2,2,2>. So we map our specified
    // dimensions between the values of 0 and 2
    // The translation value is necessary in order to translate the values from
    // the range of 0 and 2 to the range of -1 and 1
    // The translation may look odd and that is because this matrix is a
    // combination of a Scale matrix, S, and a translation matrix, T, in
    // which the translation must be performed first. Taking the x translation
    // for example, we want to translate it by -(l + r) / 2. Which would
    // effectively move the origin's x value in the center of l & r. However,
    // the scaling changes that translation to [(2 / (r + l)) * (-(r + l) / 2)],
    // which simplifies to what is seen below for the x translation.
    return {
             2 / (r - l),                0,                0, 0,
                       0,      2 / (t - b),                0, 0,
                       0,                0,      2 / (f - n), 0,
        -(r + l)/(r - l), -(t + b)/(t - b), -(f + n)/(f - n), 1
    };
}

// real-time rendering 4.7.2
inline mat4 perspective(f32 l, f32 r, f32 b, f32 t, f32 n, f32 f) {
  return {
          (2.0f * n) / (r - l),                    0,      -(r + l) / (r - l),                         0,
                             0, (2.0f * n) / (t - b),      -(t + b) / (t - b),                         0,
                             0,                    0,      -(f + n) / (f - n), -(2.0f * f * n) / (f - n),
                             0,                    0,                  -(1/n),                         0
  };
}

// real-time rendering 4.7.2
// aspect ratio is equivalent to width / height
inline mat4 perspective(f32 fovVert, f32 aspect, f32 n, f32 f) {
  const f32 c = 1.0f / tanf(fovVert / 2.0f);
  return {
          (c / aspect), 0.0f,                      0.0f,                      0.0f,
                  0.0f,    c,                      0.0f,                      0.0f,
                  0.0f, 0.0f,        -(f + n) / (f - n),                     -1.0f,
                  0.0f, 0.0f, -(2.0f * f * n) / (f - n),                      0.0f,
  };
}

void adjustAspectPerspProj(mat4* projectionMatrix, f32 fovVert, f32 aspect) {
  const f32 c = 1.0f / tanf(fovVert / 2.0f);
  projectionMatrix->c[0][0] = c / aspect;
  projectionMatrix->c[1][1] = c;
}

void adjustNearFarPerspProj(mat4* projectionMatrix, f32 n, f32 f) {
  projectionMatrix->c[2][2] = -(f + n) / (f - n);
  projectionMatrix->c[3][2] = -(2.0f * f * n) / (f - n);
}

// Complex
// This angle represents a counter-clockwise rotation
complex Complex(f32 angle){
  return { cosf(angle), sinf(angle) };
}

inline f32 magnitudeSquared(complex c) {
  return (c.r * c.r) + (c.i * c.i);
}

inline f32 magnitude(complex c) {
  return sqrtf(magnitudeSquared(c));
}

// ii = -1
vec2 operator*(const complex& ri, vec2 xy) {
#ifndef NDEBUG
  // Assert that we are only ever dealing with unit quaternions when rotating a vector
  f32 qMagn = magnitudeSquared(ri);
  Assert(qMagn < 1.001f && qMagn > .999f);
#endif

  return {
          (ri.r * xy.r) + -(ri.i * xy.i), // real
          (ri.r * xy.i) + (ri.i * xy.r) // imaginary
  };
}

void operator*=(vec2& xy, const complex& ri) {
#ifndef NDEBUG
  // Assert that we are only ever dealing with unit quaternions when rotating a vector
  f32 qMagn = magnitudeSquared(ri);
  Assert(qMagn < 1.001f && qMagn > .999f);
#endif

  f32 imaginaryTmp = (ri.i * xy.r);
  xy.x = (ri.r * xy.r) + -(ri.i * xy.i);
  xy.y = (ri.r * xy.i) + imaginaryTmp;
}

// Quaternions
quaternion Quaternion(f32 angle, vec3 v) {
  vec3 n = normalize(v);

  f32 halfA = angle * 0.5f;
  f32 cosHalfA = cosf(halfA);
  f32 sinHalfA = sinf(halfA);

  quaternion result;
  result.real = cosHalfA;
  result.ijk = sinHalfA * n;

  return result;
}

inline quaternion identity_quaternion() {
  return {1.0f, 0.0f, 0.0f, 0.0f};
}

inline f32 magnitudeSquared(quaternion rijk) {
  return (rijk.r * rijk.r) + (rijk.i * rijk.i) + (rijk.j * rijk.j) + (rijk.k * rijk.k);
}

inline f32 magnitude(quaternion q) {
  return sqrtf(magnitudeSquared(q));
}

// NOTE: Conjugate is equal to the inverse when the quaternion is unit length
inline quaternion conjugate(quaternion q) {
  return {q.r, -q.i, -q.j, -q.k};
}

inline f32 dot(const quaternion& q1, const quaternion& q2) {
  return (q1.r * q2.r) + (q1.i * q2.i) + (q1.j * q2.j) + (q1.k * q2.k);
}

inline quaternion normalize(const quaternion& q) {
  f32 mag = sqrtf((q.r * q.r) + (q.i * q.i) + (q.j * q.j) + (q.k * q.k));
  Assert(mag != 0.0f);
  f32 magInv = 1.0f / mag;
  return {q.r * magInv, q.i * magInv, q.j * magInv, q.k * magInv};
}

// TODO: overloading star operator for this may lead to confusion
/*
 * ii = jj = kk = ijk = -1
 * ij = -ji = k
 * jk = -kj = i
 * ki = -ik = j
 */
vec3 operator*(const quaternion& q, vec3 v) {
#ifndef NDEBUG
  // Assert that we are only ever dealing with unit quaternions when rotating a vector
  f32 qMagn = magnitudeSquared(q);
  Assert(qMagn < 1.001f && qMagn > .999f);
#endif

  quaternion result;
  quaternion qv;
  quaternion qInv = conjugate(q);

  qv.r = -(q.i * v.i) + -(q.j * v.j) + -(q.k * v.k);
  qv.i =  (q.r * v.i) + -(q.k * v.j) +  (q.j * v.k);
  qv.j =  (q.k * v.i) +  (q.r * v.j) + -(q.i * v.k);
  qv.k = -(q.j * v.i) +  (q.i * v.j) +  (q.r * v.k);

  // result.r = (qv.r * qInv.r) + -(qv.i * qInv.i) + -(qv.j * qInv.j) + -(qv.k * qInv.k); \\ equates to zero
  result.i = (qv.i * qInv.r) +  (qv.r * qInv.i) + -(qv.k * qInv.j) +  (qv.j * qInv.k);
  result.j = (qv.j * qInv.r) +  (qv.k * qInv.i) +  (qv.r * qInv.j) + -(qv.i * qInv.k);
  result.k = (qv.k * qInv.r) + -(qv.j * qInv.i) +  (qv.i * qInv.j) +  (qv.r * qInv.k);

  return result.ijk;
}

quaternion operator*(const quaternion& q1, quaternion q2) {
  quaternion q1q2;
  q1q2.r = -(q1.i * q2.i) + -(q1.j * q2.j) + -(q1.k * q2.k);
  q1q2.i = (q1.r * q2.i) + -(q1.k * q2.j) + (q1.j * q2.k);
  q1q2.j = (q1.k * q2.i) + (q1.r * q2.j) + -(q1.i * q2.k);
  q1q2.k = -(q1.j * q2.i) + (q1.i * q2.j) + (q1.r * q2.k);
  return q1q2;
}

inline quaternion operator+(const quaternion& q1, const quaternion& q2) {
  return {q1.r + q2.r, q1.i + q2.i, q1.j + q2.j, q1.k + q2.k};
}

inline quaternion operator-(const quaternion& q1, const quaternion& q2) {
  return {q1.r - q2.r, q1.i - q2.i, q1.j - q2.j, q1.k - q2.k};
}

inline quaternion operator*(const quaternion& q1, const f32 s) {
  return {q1.r * s, q1.i * s, q1.j * s, q1.k * s};
}

inline quaternion operator*(const f32 s, const quaternion& q1) {
  return {q1.r * s, q1.i * s, q1.j * s, q1.k * s};
}

inline quaternion operator/(const quaternion& q1, const f32 s) {
  f32 scaleInv = 1.0f / s;
  return {q1.r * scaleInv, q1.i * scaleInv, q1.j * scaleInv, q1.k * scaleInv};
}

quaternion lerp(quaternion a, quaternion b, f32 t) {
  Assert(t >= 0.0f && t <= 1.0f)
  return normalize(a - ((a + b) * t));
}

// spherical linear interpolation
// this gives the shortest art on a 4d unit sphere
// great for smooth animations between two orientations defined by two quaternions
quaternion slerp(quaternion a, quaternion b, f32 t) {
  Assert(t >= 0.0f && t <= 1.0f)
  f32 theta = acosf(dot(a, b));

  return ((sinf(theta * (1.0f - t)) * a) + (sinf(theta * t) * b)) / sinf(theta);
}

// etc
bool insideRect(BoundingRect boundingRect, vec2 position) {
  const vec2 boundingRectMax = boundingRect.min + boundingRect.diagonal;
  return (position.x > boundingRect.min.x &&
          position.x < boundingRectMax.x &&
          position.y > boundingRect.min.y &&
          position.y < boundingRectMax.y);
}

bool insideBox(BoundingBox boundingBox, vec3 position) {
  const vec3 boundingBoxMax = boundingBox.min + boundingBox.diagonal;
  return (position.x > boundingBox.min.x &&
          position.x < boundingBoxMax.x &&
          position.y > boundingBox.min.y &&
          position.y < boundingBoxMax.y &&
          position.z > boundingBox.min.z &&
          position.z < boundingBoxMax.z);
}

bool overlap(BoundingRect bbA, BoundingRect bbB) {
  const vec2 bbAMax = bbA.min + bbA.diagonal;
  const vec2 bbBMax = bbB.min + bbB.diagonal;
  return ((bbBMax.x > bbA.min.x && bbB.min.x < bbAMax.x)  && // overlap in X
          (bbBMax.y > bbA.min.y && bbB.min.y < bbAMax.x));   // overlap in Y
}

bool overlap(BoundingBox bbA, BoundingBox bbB) {
  const vec3 bbAMax = bbA.min + bbA.diagonal;
  const vec3 bbBMax = bbB.min + bbB.diagonal;
  return ((bbBMax.x > bbA.min.x && bbB.min.x < bbAMax.x)  && // overlap in X
          (bbBMax.y > bbA.min.y && bbB.min.y < bbAMax.y)  && // overlap in X
          (bbBMax.z > bbA.min.z && bbB.min.z < bbAMax.z));   // overlap in Z
}