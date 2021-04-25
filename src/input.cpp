#include <windows.h>
#include "input.h"

#include <Xinput.h>

internal_func void setKeyState(GLFWwindow* window, s32 glfwKey, InputType keyboardInput);
internal_func void setMouseState(GLFWwindow* window, s32 glfwKey, InputType mouseInput);
internal_func void setControllerState(s16 gamepadFlags, u16 xInputButtonFlag, InputType controllerInput);
internal_func void loadXInput();

internal_func void glfw_mouse_scroll_callback(GLFWwindow* window, f64 xOffset, f64 yOffset);
internal_func void glfw_framebuffer_size_callback(GLFWwindow* window, s32 width, s32 height);

global_variable b32 windowModeChangeTossNextInput = false;
global_variable vec2_u32 globalWindowExtent = vec2_u32{0, 0 };
global_variable vec2_f64 globalMouseScroll = vec2_f64{0.0, 0.0 };
global_variable vec2_f64 mousePosition = {0.0, 0.0 };
global_variable vec2_f64 mouseDelta = {0.0, 0.0 };
global_variable vec2_s16 analogStickLeft = {0, 0 };
global_variable vec2_s16 analogStickRight = {0, 0 };
global_variable f32 mouseScrollY = 0.0f;
global_variable s8 controller1TriggerLeftValue = 0;
global_variable s8 controller1TriggerRightValue = 0;
global_variable windows_size_callback* windowSizeCallback = NULL;

// initialized to zero is equivalent to InputState_Inactive
// indices of this array will be accessed through InputType enums
global_variable InputState inputStates[InputType_NumTypes] = {};

// NOTE: Casey Muratori's efficient way of handling function pointers, Handmade Hero episode 6 @ 22:06 & 1:00:21
// NOTE: Allows us to quickly change the function parameters & return type in one place and cascade throughout the rest
// NOTE: of the code if need be.
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState) // macro to succinctly define a function with given params & return type
typedef X_INPUT_GET_STATE(x_input_get_state); // Define a function type called x_input_get_state using the above macro
X_INPUT_GET_STATE(XInputGetStateStub) // Create a stub function of the type defined by the macro
{
  return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub; // Use the typedef to create a function pointer initialized to our stub function
#define XInputGetState XInputGetState_ // Enable the use of XInputGetState method name without conflicting with definition in Xinput.h

void initializeInput(GLFWwindow* window)
{
  glfwSetScrollCallback(window, glfw_mouse_scroll_callback);
  glfwSetFramebufferSizeCallback(window, glfw_framebuffer_size_callback);

  s32 framebufferWidth, framebufferHeight;
  glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
  framebufferWidth = max(0, framebufferWidth);
  framebufferHeight = max(0, framebufferHeight);
  globalWindowExtent = vec2_u32{(u32)framebufferWidth, (u32)framebufferHeight};

  loadXInput();
}

b32 consumabool(b32* boolPtr) {
  b32 originalVal = *boolPtr;
  *boolPtr = false;
  return(originalVal);
}

InputState getInputState(InputType inputType) {
    return inputStates[inputType];
}

b32 hotPress(InputType inputType) {
  return inputStates[inputType] & INPUT_HOT_PRESS;
}

b32 hotRelease(InputType inputType) {
  return inputStates[inputType] & INPUT_HOT_RELEASE;
}

b32 isActive(InputType inputType) {
  return inputStates[inputType] & (INPUT_HOT_PRESS | INPUT_ACTIVE);
}

vec2_f64 getMousePosition() {
  return mousePosition;
}

vec2_f64 getMouseDelta() {
  return mouseDelta;
}

f32 getMouseScrollY() {
  return mouseScrollY;
}

vec2_u32 getWindowExtent() {
  return globalWindowExtent;
}

vec2_s16 getControllerAnalogStickLeft(){
  return analogStickLeft;
}

vec2_s16 getControllerAnalogStickRight() {
  return analogStickRight;
}

// NOTE: values range from 0 to 225 (255 minus trigger threshold)
s8 getControllerTriggerRaw_Right() {
    return controller1TriggerRightValue;
}

// NOTE: values range from 0 to 225 (255 minus trigger threshold)
s8 getControllerTriggerRaw_Left() {
    return controller1TriggerLeftValue;
}

// NOTE: values range from 0.0 - 1.0
f32 getControllerTrigger_Right() {
    return (f32)controller1TriggerRightValue / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

// NOTE: values range from 0.0 - 1.0
f32 getControllerTrigger_Left() {
    return (f32)controller1TriggerLeftValue / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

void setInputState(InputType inputType, b32 inputIsCurrentlyActive) {
    if (inputIsCurrentlyActive)
    {
        if(inputStates[inputType] & INPUT_HOT_PRESS) {
            inputStates[inputType] = INPUT_ACTIVE;
        } else if(inputStates[inputType] ^ INPUT_ACTIVE) {
            inputStates[inputType] = INPUT_HOT_PRESS;
        }
    } else if(inputStates[inputType] & (INPUT_HOT_PRESS | INPUT_ACTIVE)) {
        inputStates[inputType] = INPUT_HOT_RELEASE;
    } else if(inputStates[inputType] & INPUT_HOT_RELEASE) { // only erase if there is something to be erased
        inputStates[inputType] = INPUT_INACTIVE;
    }
}

void setKeyState(GLFWwindow* window, s32 glfwKey, InputType keyboardInput)
{
    b32 keyIsCurrentlyActive = glfwGetKey(window, glfwKey) == GLFW_PRESS;
    setInputState(keyboardInput, keyIsCurrentlyActive);
}

void setMouseState(GLFWwindow* window, s32 glfwKey, InputType mouseInput)
{
    b32 mouseInputIsCurrentlyActive = glfwGetMouseButton(window, glfwKey) == GLFW_PRESS;
    setInputState(mouseInput, mouseInputIsCurrentlyActive);
}

void setControllerState(s16 gamepadFlags, u16 xInputButtonFlag, InputType controllerInput)
{
    b32 controllerInputIsCurrentlyActive = gamepadFlags & xInputButtonFlag;
    setInputState(controllerInput, controllerInputIsCurrentlyActive);
}

void loadInputStateForFrame(GLFWwindow* window) {
  // keyboard state
  {
    setKeyState(window, GLFW_KEY_Q, KeyboardInput_Q);
    setKeyState(window, GLFW_KEY_W, KeyboardInput_W);
    setKeyState(window, GLFW_KEY_E, KeyboardInput_E);
    setKeyState(window, GLFW_KEY_R, KeyboardInput_R);
    setKeyState(window, GLFW_KEY_A, KeyboardInput_A);
    setKeyState(window, GLFW_KEY_S, KeyboardInput_S);
    setKeyState(window, GLFW_KEY_D, KeyboardInput_D);
    setKeyState(window, GLFW_KEY_F, KeyboardInput_F);
    setKeyState(window, GLFW_KEY_J, KeyboardInput_J);
    setKeyState(window, GLFW_KEY_K, KeyboardInput_K);
    setKeyState(window, GLFW_KEY_L, KeyboardInput_L);
    setKeyState(window, GLFW_KEY_SEMICOLON, KeyboardInput_Semicolon);
    setKeyState(window, GLFW_KEY_LEFT_SHIFT, KeyboardInput_Shift_Left);
    setKeyState(window, GLFW_KEY_LEFT_CONTROL, KeyboardInput_Ctrl_Left);
    setKeyState(window, GLFW_KEY_LEFT_ALT, KeyboardInput_Alt_Left);
    setKeyState(window, GLFW_KEY_TAB, KeyboardInput_Tab);
    setKeyState(window, GLFW_KEY_RIGHT_SHIFT, KeyboardInput_Shift_Right);
    setKeyState(window, GLFW_KEY_RIGHT_CONTROL, KeyboardInput_Ctrl_Right);
    setKeyState(window, GLFW_KEY_RIGHT_ALT, KeyboardInput_Alt_Right);
    setKeyState(window, GLFW_KEY_ENTER, KeyboardInput_Enter);
    setKeyState(window, GLFW_KEY_ESCAPE, KeyboardInput_Esc);
    setKeyState(window, GLFW_KEY_GRAVE_ACCENT, KeyboardInput_Backtick);
    setKeyState(window, GLFW_KEY_1, KeyboardInput_1);
    setKeyState(window, GLFW_KEY_2, KeyboardInput_2);
    setKeyState(window, GLFW_KEY_3, KeyboardInput_3);
    setKeyState(window, GLFW_KEY_UP, KeyboardInput_Up);
    setKeyState(window, GLFW_KEY_DOWN, KeyboardInput_Down);
    setKeyState(window, GLFW_KEY_LEFT, KeyboardInput_Left);
    setKeyState(window, GLFW_KEY_RIGHT, KeyboardInput_Right);
    setKeyState(window, GLFW_KEY_SPACE, KeyboardInput_Space);
  }

  // mouse state
  {
    setMouseState(window, GLFW_MOUSE_BUTTON_LEFT, MouseInput_Left);
    setMouseState(window, GLFW_MOUSE_BUTTON_RIGHT, MouseInput_Right);
    setMouseState(window, GLFW_MOUSE_BUTTON_MIDDLE, MouseInput_Middle);
    setMouseState(window, GLFW_MOUSE_BUTTON_4, MouseInput_Back);
    setMouseState(window, GLFW_MOUSE_BUTTON_5, MouseInput_Forward);

    // mouse movement state management
    {
      vec2_f64 newMouseCoord;
      glfwGetCursorPos(window, &newMouseCoord.x, &newMouseCoord.y);

      // NOTE: We do not consume mouse input on window size changes as it results in unwanted values
      mouseDelta = consumabool(&windowModeChangeTossNextInput) ? vec2_f64{0.0f, 0.0f} : vec2_f64{newMouseCoord.x - mousePosition.x, newMouseCoord.y - mousePosition.y};
      mousePosition = newMouseCoord;
      b32 mouseMovementIsCurrentlyActive = mouseDelta.x != 0.0f || mouseDelta.y != 0.0f;
      setInputState(MouseInput_Movement, mouseMovementIsCurrentlyActive);
    }

    // mouse scroll state management
    {
      mouseScrollY = (f32)globalMouseScroll.y;
      globalMouseScroll.y = 0.0f; // NOTE: Set to 0.0f to signify that the result has been consumed
      b32 mouseScrollIsCurrentlyActive = mouseScrollY != 0.0f;
      setInputState(MouseInput_Scroll, mouseScrollIsCurrentlyActive);
    }
  }

  // TODO: Add support for multiple controllers?
  const u32 controllerIndex = 0;
  XINPUT_STATE controllerState;
  if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
  {
    // the controller is plugged in
    s16 gamepadButtonFlags = controllerState.Gamepad.wButtons;
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_A, Controller1Input_A);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_B, Controller1Input_B);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_X, Controller1Input_X);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_Y, Controller1Input_Y);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_UP, Controller1Input_DPad_Up);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_DOWN, Controller1Input_DPad_Down);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_LEFT, Controller1Input_DPad_Left);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_DPAD_RIGHT, Controller1Input_DPad_Right);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_LEFT_SHOULDER, Controller1Input_Shoulder_Left);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_RIGHT_SHOULDER, Controller1Input_Shoulder_Right);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_START, Controller1Input_Start);
    setControllerState(gamepadButtonFlags, XINPUT_GAMEPAD_BACK, Controller1Input_Select);

    analogStickLeft = { controllerState.Gamepad.sThumbLX, controllerState.Gamepad.sThumbLY };
    if(analogStickLeft.x > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && analogStickLeft.x < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
      analogStickLeft.x = 0; // deadzone
    }
    if(analogStickLeft.y > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && analogStickLeft.y < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
      analogStickLeft.y = 0; // deadzone
    }
    b32 analogStickLeftIsCurrentlyActive = analogStickLeft.x != 0 || analogStickLeft.y != 0;
    setInputState(Controller1Input_Analog_Left, analogStickLeftIsCurrentlyActive);

    analogStickRight = { controllerState.Gamepad.sThumbRX, controllerState.Gamepad.sThumbRY };
    if(analogStickRight.x > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && analogStickRight.x < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
      analogStickRight.x = 0; // deadzone
    }
    if(analogStickRight.y > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && analogStickRight.y < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
      analogStickRight.y = 0; // deadzone
    }
    b32 analogStickRightIsCurrentlyActive = analogStickRight.x != 0 || analogStickRight.y != 0;
    setInputState(Controller1Input_Analog_Right, analogStickRightIsCurrentlyActive);

    b32 leftTriggerIsCurrentlyActive = controllerState.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    setInputState(Controller1Input_Trigger_Left, leftTriggerIsCurrentlyActive);
    controller1TriggerLeftValue = leftTriggerIsCurrentlyActive ? controllerState.Gamepad.bLeftTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD : 0;

    b32 rightTriggerIsCurrentlyActive = controllerState.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    setInputState(Controller1Input_Trigger_Right, rightTriggerIsCurrentlyActive);
    controller1TriggerRightValue = rightTriggerIsCurrentlyActive ? controllerState.Gamepad.bRightTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD : 0;
  }
}

// Callback function for when user scrolls with mouse wheel
void glfw_mouse_scroll_callback(GLFWwindow* window, f64 xOffset, f64 yOffset)
{
  globalMouseScroll.y = yOffset;
}

void subscribeWindowSizeCallback(windows_size_callback* callback)
{
  windowSizeCallback = callback;
}

// NOTE: returns (0,0) when no longer on screen
void glfw_framebuffer_size_callback(GLFWwindow* window, s32 width, s32 height)
{
  func_persist b32 globalWindowMinimized = false;
  // NOTE: Currently just ignoring minimize.
  if(width <= 0 || height <= 0) {
    globalWindowMinimized = true;
    return;
  } else if(globalWindowMinimized) {
    globalWindowMinimized = false;
    // return if no change in size has been made to the framebuffer since minimization
    if(globalWindowExtent.width == width && globalWindowExtent.height == height)
    {
      return;
    }
  }

  windowModeChangeTossNextInput = true;
  globalWindowExtent = {(u32)width, (u32)height };

  if(windowSizeCallback != NULL) {
    windowSizeCallback();
  }
}

void enableCursor(GLFWwindow* window, b32 enable)
{
  glfwSetInputMode(window, GLFW_CURSOR, enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
  windowModeChangeTossNextInput = true;
}

b32 isCursorEnabled(GLFWwindow* window)
{
  return glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_NORMAL;
}

internal_func void loadXInput()
{
  HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
  if (!XInputLibrary)
  {
    XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
  }
  if (!XInputLibrary)
  {
    XInputLibrary = LoadLibraryA("xinput1_3.dll");
  }
  if (XInputLibrary)
  {
    XInputGetState = (x_input_get_state*) GetProcAddress(XInputLibrary, "XInputGetState");
    if (!XInputGetState)
    {
      XInputGetState = XInputGetStateStub;
    }
  } else
  {
    std::cout << "Failed to load XInput" << std::endl;
    exit(-1);
  }
}