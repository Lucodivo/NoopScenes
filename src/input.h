#pragma once

#define GLFW_INCLUDE_NONE // ensure GLFW doesn't load OpenGL headers
#include <glfw3.h>

#include "noop_types.h"

typedef void (*WindowSizeCallback)(void);

struct ControllerAnalogStick
{
  s16 x;
  s16 y;
};

/*
 * These enums are used for array indices
 * No enum values should be edited
 * InputType_NumTypes must remain the last enum in the list
 */
enum InputType
{
  KeyboardInput_Q = 0,

  KeyboardInput_W, KeyboardInput_E, KeyboardInput_R,
  KeyboardInput_A, KeyboardInput_S, KeyboardInput_D, KeyboardInput_F,
  KeyboardInput_J, KeyboardInput_K, KeyboardInput_L, KeyboardInput_Semicolon,
  KeyboardInput_Shift_Left, KeyboardInput_Ctrl_Left, KeyboardInput_Alt_Left, KeyboardInput_Tab,
  KeyboardInput_Shift_Right, KeyboardInput_Ctrl_Right, KeyboardInput_Alt_Right, KeyboardInput_Enter,
  KeyboardInput_Esc, KeyboardInput_Backtick, KeyboardInput_1, KeyboardInput_2, KeyboardInput_3,
  KeyboardInput_Up, KeyboardInput_Down, KeyboardInput_Left, KeyboardInput_Right, KeyboardInput_Space,

  MouseInput_Left, MouseInput_Right, MouseInput_Middle, MouseInput_Back, MouseInput_Forward,
  MouseInput_Scroll, MouseInput_Movement,

  Controller1Input_A, Controller1Input_B, Controller1Input_X, Controller1Input_Y,
  Controller1Input_DPad_Up, Controller1Input_DPad_Down, Controller1Input_DPad_Left, Controller1Input_DPad_Right,
  Controller1Input_Shoulder_Left, Controller1Input_Trigger_Left, Controller1Input_Shoulder_Right, Controller1Input_Trigger_Right,
  Controller1Input_Start, Controller1Input_Select, Controller1Input_Analog_Left, Controller1Input_Analog_Right,

  InputType_NumTypes
};

// NOTE: INPUT_INACTIVE being set to 0 means that zero-initialized InputState enums will have the desired value of inactive
enum InputState
{
    INPUT_INACTIVE = 0,
    INPUT_HOT_PRESS = 1 << 0,
    INPUT_ACTIVE = 1 << 1,
    INPUT_HOT_RELEASE = 1 << 2,
};

void initializeInput(GLFWwindow* window);
void deinitializeInput(GLFWwindow* window);
void loadInputStateForFrame(GLFWwindow* window);

inline b32 hotPress(InputType key); // returns true if input was just activated
inline b32 hotRelease(InputType key); // returns true if input was just deactivated
inline b32 isActive(InputType key); // returns true if key is pressed or held down
inline InputState getInputState(InputType key); // Note: for special use cases (ex: double click), use hotPress/hotRelease/isActive in most cases

inline Vec2_f64 getMousePosition();
inline Vec2_f64 getMouseDelta();
inline f32 getMouseScrollY();
inline s8 getControllerTriggerRaw_Left(); // NOTE: values range from 0 - 225 (255 minus trigger threshold)
inline s8 getControllerTriggerRaw_Right(); // NOTE: values range from 0 - 225 (255 minus trigger threshold)
inline f32 getControllerTrigger_Left(); // NOTE: values range from 0.0 - 1.0
inline f32 getControllerTrigger_Right(); // NOTE: values range from 0.0 - 1.0
inline Extent2D getWindowExtent();
inline ControllerAnalogStick getControllerAnalogStickLeft();
inline ControllerAnalogStick getControllerAnalogStickRight();

void enableCursor(GLFWwindow* window, b32 enable);
b32 isCursorEnabled(GLFWwindow* window);

// NOTE: Call with NULL to unsubscribe
void subscribeWindowSizeCallback(WindowSizeCallback callback);