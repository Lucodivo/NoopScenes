#pragma once
#include <math.h>
#include <stdio.h>

#include "../noop_types.h"
#include "../noop_math.h"


void printVec4(const char* name, const vec4& v) {
  printf("%s: [%4.2f, %4.2f, %4.2f, %4.2f]\n", name, v.x, v.y, v.z, v.w);
}

void printMat4(const char* name, const mat4& M) {
  printf("===%s (transposed)===", name);
  char* columnString = "column X";
  for(u32 i = 0; i < 4; i++) {
    columnString[7] = '0' + i;
    printVec4(columnString, M.col[0]);
  }
}

b32 printIfNotEqual(const mat4& A, const mat4& B) {
  b32 equal = A == B;
  if(!equal) {
    printf("Two mat4s are not equal");
    printMat4("mat1", A);
    printMat4("mat2", B);
  }
  return equal;
}

b32 printIfNotEqual(const vec4& v1, const vec4& v2) {
  b32 equal = v1 == v2;
  if(!equal) {
    printf("Two vec4s are not equal");
    printVec4("vec1", v1);
    printVec4("vec2", v2);
  }
  return equal;
}

void translateTest() {
  vec3 translation{12.8f, 25.6f, 51.2f};
  mat4 expectedResult = identity_mat4();
  expectedResult.xTransform = {1.0f, 0.0f, 0.0f, 0.0f};
  expectedResult.yTransform = {0.0f, 1.0f, 0.0f, 0.0f};
  expectedResult.zTransform = {0.0f, 0.0f, 1.0f, 0.0f};
  expectedResult.translation = {translation.x, translation.y, translation.z, 1.0f};

  mat4 translatedMat = identity_mat4();
  translatedMat = translate_mat4(translation);

  Assert(printIfNotEqual(expectedResult, translatedMat));
}

void mat4Vec4MultTest() {
  vec3 translation{7.0f, 8.0f, 9.0f};
  vec4 point{1, 2, 3, 1};
  vec4 expectedPoint{point.x + translation.x, point.y + translation.y, point.z + translation.z, point.w};
  vec4 vector{1, 2, 3, 0};
  vec4 expectedVector = vector;
  mat4 M = translate_mat4(translation);

  vec4 translatedPoint = M * point;
  vec4 translatedVector = M * vector;

  Assert(printIfNotEqual(translatedVector, expectedVector));
  Assert(printIfNotEqual(translatedPoint, expectedPoint));
}

void mat4MultTest() {
  vec3 scale{1.0f, 2.0f, 3.0f};
  mat4 scaleMat4 = scale_mat4(scale);
  vec3 translation{7.0f, 8.0f, 9.0f};
  mat4 translateMat4 = translate_mat4(translation);
  vec4 startingPos{1.0f, 10.0f, 100.0f, 1.0f};
  vec4 expectedPosTransThenScale{scale.x * (startingPos.x + translation.x),
                                  scale.y * (startingPos.y + translation.y),
                                  scale.z * (startingPos.z + translation.z),
                                  1.0f};
  vec4 expectedPosScaleThenTrans{(scale.x * startingPos.x) + translation.x,
                                  (scale.y * startingPos.y) + translation.y,
                                  (scale.z * startingPos.z) + translation.z,
                                  1.0f};

  vec4 transformedPosTransThenScale = scaleMat4 * translateMat4 * startingPos;
  vec4 transformedPosScaleThenTrans = translateMat4 * scaleMat4 * startingPos;

  Assert(printIfNotEqual(expectedPosTransThenScale, transformedPosTransThenScale));
  Assert(printIfNotEqual(expectedPosScaleThenTrans, transformedPosScaleThenTrans));
}

void mat4RotateTest() {
  f32 angle = 120.f * RadiansPerDegree;
  vec3 rotationAxis{1.0f, 1.0f, 1.0f};
  vec4 x{1.0f, 0.0f, 0.0f, 1.0f};
  vec4 y{0.0f, 1.0f, 0.0f, 1.0f};
  vec4 z{0.0f, 0.0f, 1.0f, 1.0f};

  mat4 rotationMat = rotate_mat4(angle, rotationAxis);

  vec4 rotatedX = rotationMat * x;
  vec4 rotatedY = rotationMat * y;
  vec4 rotatedZ = rotationMat * z;
  vec4 rotatedAxis = rotationMat * vec4{rotationAxis.x, rotationAxis.y, rotationAxis.y, 1.0f};

  Assert(rotatedX == y);
  Assert(rotatedY == z);
  Assert(rotatedZ == x);
  Assert(rotatedAxis.xyz == rotationAxis);
}

void complexVec2RotationTest(){
  vec2 x{1.0f, 0.0f};
  vec2 y{0.0f, 1.0f};
  complex c0 = Complex(0.0f);
  complex c30 = Complex(30.0f * RadiansPerDegree);
  complex c45 = Complex(45.0f * RadiansPerDegree);
  complex c60 = Complex(60.0f * RadiansPerDegree);
  complex c90 = Complex(90.0f * RadiansPerDegree);
  vec2 expectedRotatedX0 = x;
  vec2 expectedRotatedX30 = {cos30, sin30};
  vec2 expectedRotatedX45 = {cos45, sin45};
  vec2 expectedRotatedX60 = {cos60, sin60};
  vec2 expectedRotatedX90 = y;
  vec2 expectedRotatedY0 = y;
  vec2 expectedRotatedY30 = {-sin30, cos30};
  vec2 expectedRotatedY45 = {-sin45, cos45};
  vec2 expectedRotatedY60 = {-sin60, cos60};
  vec2 expectedRotatedY90 = -x;

  vec2 rotatedX0 = c0 * x;
  vec2 rotatedX30 = c30 * x;
  vec2 rotatedX45 = c45 * x;
  vec2 rotatedX60 = c60 * x;
  vec2 rotatedX90 = c90 * x;
  vec2 rotatedY0 = c0 * y;
  vec2 rotatedY30 = c30 * y;
  vec2 rotatedY45 = c45 * y;
  vec2 rotatedY60 = c60 * y;
  vec2 rotatedY90 = c90 * y;

  Assert(rotatedX0 == expectedRotatedX0);
  Assert(rotatedX30 == expectedRotatedX30);
  Assert(rotatedX45 == expectedRotatedX45);
  Assert(rotatedX60 == expectedRotatedX60);
  Assert(rotatedX90 == expectedRotatedX90);
  Assert(rotatedY0 == expectedRotatedY0);
  Assert(rotatedY30 == expectedRotatedY30);
  Assert(rotatedY45 == expectedRotatedY45);
  Assert(rotatedY60 == expectedRotatedY60);
  Assert(rotatedY90 == expectedRotatedY90);
}

void quaternionVec3RotationTest(){
  f32 angle = 120.f * RadiansPerDegree;
  vec3 rotationAxis{1.0f, 1.0f, 1.0f};
  vec3 x{1.0f, 0.0f, 0.0f};
  vec3 y{0.0f, 1.0f, 0.0f};
  vec3 z{0.0f, 0.0f, 1.0f};

  quaternion q = Quaternion(rotationAxis, angle);

  vec3 rotatedX = q * x;
  vec3 rotatedY = q * y;
  vec3 rotatedZ = q * z;
  vec3 rotatedAxis = q * rotationAxis;

  Assert(rotatedX == y);
  Assert(rotatedY == z);
  Assert(rotatedZ == x);
  Assert(rotatedAxis == rotationAxis);
}

void crossProductTest() {
  vec3 x{1.0f, 0.0f, 0.0f};
  vec3 y{0.0f, 1.0f, 0.0f};
  vec3 z{0.0f, 0.0f, 1.0f};

  vec3 xCrossY = cross(x, y);
  vec3 xCrossZ = cross(x, z);
  vec3 yCrossZ = cross(y, z);
  vec3 yCrossX = cross(y, x);
  vec3 zCrossX = cross(z, x);
  vec3 zCrossY = cross(z, y);

  Assert(xCrossY == z);
  Assert(xCrossZ == -y);
  Assert(yCrossZ == x);
  Assert(yCrossX == -z);
  Assert(zCrossX == y);
  Assert(zCrossY == -x);
}

void slerpTest() {
  vec3 rotationAxis{0.0f, 0.0f, 1.0f};
  quaternion q1 = identity_quaternion();
  quaternion q2 = Quaternion(rotationAxis, RadiansPerDegree * 90);
  vec3 vector{1.0f, 0.0f, 0.0f};
  vec3 expectedVectorZero = vector;
  vec3 expectedVectorThirtyOverNinety{cos30, sin30, 0.0f};
  vec3 expectedVectorHalf{cos45, sin45, 0.0f};
  vec3 expectedSixetyOverNinety{cos60, sin60, 0.0f};
  vec3 expectedVectorOne{0.0f, 1.0f, 0.0f};

  quaternion qZero = slerp(q1, q2, 0.0f);
  quaternion qThirtyOverNinety = slerp(q1, q2, 30.0f / 90.0f);
  quaternion qHalf = slerp(q1, q2, 0.5f);
  quaternion qSixetyOverNinety = slerp(q1, q2, 60.0f / 90.0f);
  quaternion qOne = slerp(q1, q2, 1.0f);
  vec3 vectorZero = qZero * vector;
  vec3 vectorThirtyOver90 = qThirtyOverNinety * vector;
  vec3 vectorHalf = qHalf * vector;
  vec3 vectorSixetyOverNinety = qSixetyOverNinety * vector;
  vec3 vectorOne = qOne * vector;

  Assert(vectorZero == expectedVectorZero);
  Assert(vectorThirtyOver90 == expectedVectorThirtyOverNinety);
  Assert(vectorHalf == expectedVectorHalf);
  Assert(vectorSixetyOverNinety == expectedSixetyOverNinety);
  Assert(vectorOne == expectedVectorOne);

  // also test counter-clockwise slerp works as intended
  quaternion q3 = Quaternion(rotationAxis, RadiansPerDegree * -90);
  expectedVectorZero = vector;
  expectedVectorThirtyOverNinety = {cos30, -sin30, 0.0f};
  expectedVectorHalf = {cos45, -sin45, 0.0f};
  expectedSixetyOverNinety = {cos60, -sin60, 0.0f};
  expectedVectorOne = {0.0f, -1.0f, 0.0f};

  qZero = slerp(q1, q3, 0.0f);
  qThirtyOverNinety = slerp(q1, q3, 30.0f / 90.0f);
  qHalf = slerp(q1, q3, 0.5f);
  qSixetyOverNinety = slerp(q1, q3, 60.0f / 90.0f);
  qOne = slerp(q1, q3, 1.0f);
  vectorZero = qZero * vector;
  vectorThirtyOver90 = qThirtyOverNinety * vector;
  vectorHalf = qHalf * vector;
  vectorSixetyOverNinety = qSixetyOverNinety * vector;
  vectorOne = qOne * vector;

  Assert(vectorZero == expectedVectorZero);
  Assert(vectorThirtyOver90 == expectedVectorThirtyOverNinety);
  Assert(vectorHalf == expectedVectorHalf);
  Assert(vectorSixetyOverNinety == expectedSixetyOverNinety);
  Assert(vectorOne == expectedVectorOne);
}

void orthographicTest() {
  vec4 point{ 15.0f, 70.0f, 300.0f, 1.0f};
  f32 l = -20.0f;
  f32 r =  40.0f;
  f32 b = 50.0f;
  f32 t = 110.0f;
  f32 n = 230.0f;
  f32 f = 500.0f;
  vec4 expectedCanonicalViewPoint{
          (point.x - ((r + l) / 2.0f)) // move origin to center
              * (2.0f / (r - l)), // dimens desired dimens between -1 and 1
          (point.y - ((t + b) / 2.0f)) // move origin to center
              * (2.0f / (t - b)), // dimens desired dimens between -1 and 1
          (point.z - ((f + n) / 2.0f)) // move origin to center
              * (2.0f / (f - n)), // dimens desired dimens between -1 and 1
          1.0f
  };
  mat4 ortho = orthographic(l, r, b, t, n, f);

  vec4 transformedPoint = ortho * point;

  Assert(transformedPoint == expectedCanonicalViewPoint);
}

void bracketAssignmentOperatorsSanityCheck() {
  f32 zanyWhackyNum = 123.456f;
  u32 indexOfInterest = 1;
  vec2 v2{};
  vec3 v3{};
  vec4 v4{};
  mat3 m3{};
  mat4 m4{};
  quaternion q{};
  complex c{};

  v2[indexOfInterest] = zanyWhackyNum;
  v3[indexOfInterest] = zanyWhackyNum;
  v4[indexOfInterest] = zanyWhackyNum;
  m3[indexOfInterest][indexOfInterest] = zanyWhackyNum;
  m4[indexOfInterest][indexOfInterest] = zanyWhackyNum;
  q[indexOfInterest] = zanyWhackyNum;
  c[indexOfInterest] = zanyWhackyNum;

  Assert(v2[indexOfInterest] == zanyWhackyNum);
  Assert(v3[indexOfInterest] == zanyWhackyNum);
  Assert(v4[indexOfInterest] == zanyWhackyNum);
  Assert(m3[indexOfInterest][indexOfInterest] == zanyWhackyNum);
  Assert(m4[indexOfInterest][indexOfInterest] == zanyWhackyNum);
  Assert(q[indexOfInterest] == zanyWhackyNum);
  Assert(c[indexOfInterest] == zanyWhackyNum);
}

void quaternionOrientTest() {
  vec3 orientation1 = normalize(1.0f, 1.0f, 1.0f);
  vec3 orientation2 = normalize(1.0, 3.0f, -6.0f);
  quaternion orientQuaternion1to2 = orient(orientation1, orientation2);
  quaternion orientQuaternion2to1 = orient(orientation2, orientation1);
  mat3 matrix1to2 = rotate_mat3(orientQuaternion1to2);
  mat3 matrix2to1 = rotate_mat3(orientQuaternion2to1);

  vec3 resultOrientationQuaternion1to2 = orientQuaternion1to2 * orientation1;
  vec3 resultOrientationQuaternion2to1 = orientQuaternion2to1 * orientation2;
  vec3 resultOrientationMat1to2 = matrix1to2 * orientation1;
  vec3 resultOrientationMat2to1 = matrix2to1 * orientation2;

  Assert(resultOrientationQuaternion1to2 == orientation2);
  Assert(resultOrientationQuaternion2to1 == orientation1);
  Assert(resultOrientationMat1to2 == orientation2);
  Assert(resultOrientationMat2to1 == orientation1);
}

void runAllMathTests()
{
  translateTest();
  mat4Vec4MultTest();
  mat4MultTest();
  mat4RotateTest();
  complexVec2RotationTest();
  quaternionVec3RotationTest();
  crossProductTest();
  slerpTest();
  orthographicTest();
  bracketAssignmentOperatorsSanityCheck();
}

void runMathTests() {
//  runAllMathTests();
  quaternionOrientTest();
}