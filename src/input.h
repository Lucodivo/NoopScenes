#pragma once

#define WINDOW_SIZE_CALLBACK(name) void name(void)
typedef WINDOW_SIZE_CALLBACK(windows_size_callback);

/*
 * These enums are used for array indices
 * No enum values should be edited
 * InputType_NumTypes must remain the last enum in the list
 */
enum InputType
{
#define KeyboardInput(name, input_code) KeyboardInput_##name,
#include "keyboard_input_list.inc"
#undef KeyboardInput

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

b32 hotPress(InputType key); // returns true if input was just activated
b32 hotRelease(InputType key); // returns true if input was just deactivated
b32 isActive(InputType key); // returns true if key is pressed or held down
InputState getInputState(InputType key); // Note: for special use cases (ex: double click), use hotPress/hotRelease/isActive in most cases

vec2_f64 getMousePosition();
vec2_f64 getMouseDelta();
f32 getMouseScrollY();
s8 getControllerTriggerRaw_Left(); // NOTE: values range from 0 - 225 (255 minus trigger threshold)
s8 getControllerTriggerRaw_Right(); // NOTE: values range from 0 - 225 (255 minus trigger threshold)
f32 getControllerTrigger_Left(); // NOTE: values range from 0.0 - 1.0
f32 getControllerTrigger_Right(); // NOTE: values range from 0.0 - 1.0
vec2_u32 getWindowExtent();
vec2_s16 getControllerAnalogStickLeft();
vec2_s16 getControllerAnalogStickRight();

void enableCursor(GLFWwindow* window, b32 enable);
b32 isCursorEnabled(GLFWwindow* window);

// NOTE: Call with NULL to unsubscribe
void subscribeWindowSizeCallback(windows_size_callback callback);