#pragma once
#include "../noop_3d_math.h"

void printVec4(const char* name, const vec4& v) {
  printf ("%s: [%4.2f, %4.2f, %4.2f, %4.2f]\n", name, v.x, v.y, v.z, v.w);
}

void printMat4(const char* name, const mat4& M) {
  printf("===%s (transposed)===", name);
  char* columnString = "column X";
  for(int i = 0; i < 4; i++) {
    columnString[7] = '0' + i;
    printVec4(columnString, M.col[0]);
  }
}

b32 equals(const vec4& v1, const vec4& v2) {
  b32 pass = true;
  for(int i = 0; i < 4; ++i) {
    pass = pass && (v1.c[i] == v2.c[i]);
  }
  return pass;
}

b32 equals(const vec4& v1, const vec4& v2, f32 epsilon) {
  b32 pass = true;
  for(int i = 0; i < 4; ++i) {
    f32 diff = (v1.c[i] - v2.c[i]);
    pass = pass && (diff <= epsilon && diff >= -epsilon);
  }
  return pass;
}

b32 equals(const vec3& v1, const vec3& v2, f32 epsilon) {
  b32 pass = true;
  for(int i = 0; i < 3; ++i) {
    f32 diff = (v1.c[i] - v2.c[i]);
    pass = pass && (diff <= epsilon && diff >= -epsilon);
  }
  return pass;
}

b32 equals(const mat4& A, const mat4& B) {
  b32 pass = true;
  for(int i = 0; i < 4; ++i)
  {
    pass = pass && equals(A.col[i], B.col[i]);
  }
  return pass;
}

b32 printIfNotEqual(const mat4& A, const mat4& B) {
  b32 equal = equals(A, B);
  if(!equal) {
    printf("Two mat4s are not equal");
    printMat4("mat1", A);
    printMat4("mat2", B);
  }
  return equal;
}

b32 printIfNotEqual(const vec4& v1, const vec4& v2) {
  b32 equal = equals(v1, v2);
  if(!equal) {
    printf("Two vec4s are not equal");
    printVec4("vec1", v1);
    printVec4("vec2", v2);
  }
  return equal;
}

void translateTest() {
  vec3 translation = vec3{12.8f, 25.6f, 51.2f};
  mat4 expectedResult = identity_mat4();
  expectedResult.xBasis = {1.0f, 0.0f, 0.0f, 0.0f};
  expectedResult.yBasis = {0.0f, 1.0f, 0.0f, 0.0f};
  expectedResult.zBasis = {0.0f, 0.0f, 1.0f, 0.0f};
  expectedResult.translation = {translation.x, translation.y, translation.z, 1.0f};

  mat4 translatedMat = identity_mat4();
  translatedMat = translate_mat4(translation);

  Assert(printIfNotEqual(expectedResult, translatedMat));
}

void mat4Vec4MultTest() {
  vec3 translation = vec3{7.0f, 8.0f, 9.0f};
  vec4 point = vec4{1, 2, 3, 1};
  vec4 expectedPoint = vec4{point.x + translation.x, point.y + translation.y, point.z + translation.z, point.w};
  vec4 vector = vec4{1, 2, 3, 0};
  vec4 expectedVector = vector;
  mat4 M = translate_mat4(translation);

  vec4 translatedPoint = M * point;
  vec4 translatedVector = M * vector;

  Assert(printIfNotEqual(translatedVector, expectedVector));
  Assert(printIfNotEqual(translatedPoint, expectedPoint));
}

void mat4MultTest() {
  vec3 scale = vec3{1.0f, 2.0f, 3.0f};
  mat4 scaleMat4 = scale_mat4(scale);
  vec3 translation = vec3{7.0f, 8.0f, 9.0f};
  mat4 translateMat4 = translate_mat4(translation);
  vec4 startingPos = vec4{1.0f, 10.0f, 100.0f, 1.0f};
  vec4 expectedPosTransThenScale = vec4{scale.x * (startingPos.x + translation.x),
                                        scale.y * (startingPos.y + translation.y),
                                        scale.z * (startingPos.z + translation.z),
                                        1.0f};
  vec4 expectedPosScaleThenTrans = vec4{(scale.x * startingPos.x) + translation.x,
                                        (scale.y * startingPos.y) + translation.y,
                                        (scale.z * startingPos.z) + translation.z,
                                        1.0f};

  vec4 transformedPosTransThenScale = scaleMat4 * translateMat4 * startingPos;
  vec4 transformedPosScaleThenTrans = translateMat4 * scaleMat4 * startingPos;

  Assert(printIfNotEqual(expectedPosTransThenScale, transformedPosTransThenScale));
  Assert(printIfNotEqual(expectedPosScaleThenTrans, transformedPosScaleThenTrans));
}

void mat4RotateTest() {
  f32 epsilon = 0.01f;
  f32 angle = 120.f * RadiansPerDegree;
  vec3 rotationAxis = vec3{1.0f, 1.0f, 1.0f};
  vec4 x = vec4{1.0f, 0.0f, 0.0f, 1.0f};
  vec4 y = vec4{0.0f, 1.0f, 0.0f, 1.0f};
  vec4 z = vec4{0.0f, 0.0f, 1.0f, 1.0f};

  mat4 rotationMat = rotate_mat4(angle, rotationAxis);

  vec4 rotatedX = rotationMat * x;
  vec4 rotatedY = rotationMat * y;
  vec4 rotatedZ = rotationMat * z;
  vec4 rotatedAxis = rotationMat * vec4{rotationAxis.x, rotationAxis.y, rotationAxis.y, 1.0f};

  Assert(equals(rotatedX, y, epsilon));
  Assert(equals(rotatedY, z, epsilon));
  Assert(equals(rotatedZ, x, epsilon));
  Assert(equals(rotatedAxis.xyz, rotationAxis, epsilon));
}

void quaternionVec3RotationTest(){
  f32 epsilon = 0.01f;
  f32 angle = 120.f * RadiansPerDegree;
  vec3 rotationAxis = vec3{1.0f, 1.0f, 1.0f};
  vec3 x = vec3{1.0f, 0.0f, 0.0f};
  vec3 y = vec3{0.0f, 1.0f, 0.0f};
  vec3 z = vec3{0.0f, 0.0f, 1.0f};

  quaternion q = Quaternion(angle, rotationAxis);

  vec3 rotatedX = q * x;
  vec3 rotatedY = q * y;
  vec3 rotatedZ = q * z;
  vec3 rotatedAxis = q * rotationAxis;

  Assert(equals(rotatedX, y, epsilon));
  Assert(equals(rotatedY, z, epsilon));
  Assert(equals(rotatedZ, x, epsilon));
  Assert(equals(rotatedAxis, rotationAxis, epsilon));
}

void runAllMathTests()
{
  translateTest();
  mat4Vec4MultTest();
  mat4MultTest();
  mat4RotateTest();
  quaternionVec3RotationTest();
}

void runMathTests() {
//  runAllMathTests();
  quaternionVec3RotationTest();
}