internal_func void setKeyState(InputType keyboardInput, s32 glfwKey, GLFWwindow* window);
internal_func void setMouseState(InputType mouseInput, s32 glfwKey, GLFWwindow* window);
internal_func void setControllerState(InputType controllerInput, u16 xInputButtonFlag, s16 gamepadFlags);
internal_func void loadXInput();

internal_func void glfw_mouse_scroll_callback(GLFWwindow* window, f64 xOffset, f64 yOffset);
internal_func void glfw_framebuffer_size_callback(GLFWwindow* window, s32 width, s32 height);

global_variable b32 globalWindowModeChangeTossNextInput = false;
global_variable vec2_u32 globalWindowExtent = vec2_u32{0, 0 };
global_variable vec2_f64 globalMouseScroll = vec2_f64{0.0, 0.0 };
global_variable vec2_f64 globaleMousePosition = {0.0, 0.0 };
global_variable vec2_f64 globalMouseDelta = {0.0, 0.0 };
global_variable vec2_s16 globalAnalogStickLeft = {0, 0 };
global_variable vec2_s16 globalAnalogStickRight = {0, 0 };
global_variable f32 globalMouseScrollY = 0.0f;
global_variable s8 globalController1TriggerLeftValue = 0;
global_variable s8 globalController1TriggerRightValue = 0;
global_variable windows_size_callback* globalWindowSizeCallback = NULL;

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
  framebufferWidth = Max(0, framebufferWidth);
  framebufferHeight = Max(0, framebufferHeight);
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
  return globaleMousePosition;
}

vec2_f64 getMouseDelta() {
  return globalMouseDelta;
}

f32 getMouseScrollY() {
  return globalMouseScrollY;
}

vec2_u32 getWindowExtent() {
  return globalWindowExtent;
}

vec2_s16 getControllerAnalogStickLeft(){
  return globalAnalogStickLeft;
}

vec2_s16 getControllerAnalogStickRight() {
  return globalAnalogStickRight;
}

// NOTE: values range from 0 to 225 (255 minus trigger threshold)
s8 getControllerTriggerRaw_Right() {
    return globalController1TriggerRightValue;
}

// NOTE: values range from 0 to 225 (255 minus trigger threshold)
s8 getControllerTriggerRaw_Left() {
    return globalController1TriggerLeftValue;
}

// NOTE: values range from 0.0 - 1.0
f32 getControllerTrigger_Right() {
    return (f32)globalController1TriggerRightValue / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
}

// NOTE: values range from 0.0 - 1.0
f32 getControllerTrigger_Left() {
    return (f32)globalController1TriggerLeftValue / (255 - XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
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

void setKeyState(InputType keyboardInput, s32 glfwKey, GLFWwindow* window)
{
    b32 keyIsCurrentlyActive = glfwGetKey(window, glfwKey) == GLFW_PRESS;
    setInputState(keyboardInput, keyIsCurrentlyActive);
}

void setMouseState(InputType mouseInput, s32 glfwKey, GLFWwindow* window)
{
    b32 mouseInputIsCurrentlyActive = glfwGetMouseButton(window, glfwKey) == GLFW_PRESS;
    setInputState(mouseInput, mouseInputIsCurrentlyActive);
}

void setControllerState(InputType controllerInput, u16 xInputButtonFlag, s16 gamepadFlags)
{
    b32 controllerInputIsCurrentlyActive = gamepadFlags & xInputButtonFlag;
    setInputState(controllerInput, controllerInputIsCurrentlyActive);
}

void loadInputStateForFrame(GLFWwindow* window) {
  // keyboard state
#define KeyboardInput(name, input_code) setKeyState(KeyboardInput_##name, input_code, window);
#include "keyboard_input_list.inc"
#undef KeyboardInput

  // mouse state
  {
    setMouseState(MouseInput_Left, GLFW_MOUSE_BUTTON_LEFT, window);
    setMouseState(MouseInput_Right, GLFW_MOUSE_BUTTON_RIGHT, window);
    setMouseState(MouseInput_Middle, GLFW_MOUSE_BUTTON_MIDDLE, window);
    setMouseState(MouseInput_Back, GLFW_MOUSE_BUTTON_4, window);
    setMouseState(MouseInput_Forward, GLFW_MOUSE_BUTTON_5, window);

    // mouse movement state management
    {
      vec2_f64 newMouseCoord;
      glfwGetCursorPos(window, &newMouseCoord.x, &newMouseCoord.y);

      // NOTE: We do not consume mouse input on window size changes as it results in unwanted values
      globalMouseDelta = consumabool(&globalWindowModeChangeTossNextInput) ? vec2_f64{0.0f, 0.0f} : vec2_f64{newMouseCoord.x - globaleMousePosition.x, newMouseCoord.y - globaleMousePosition.y};
      globaleMousePosition = newMouseCoord;
      b32 mouseMovementIsCurrentlyActive = globalMouseDelta.x != 0.0f || globalMouseDelta.y != 0.0f;
      setInputState(MouseInput_Movement, mouseMovementIsCurrentlyActive);
    }

    // mouse scroll state management
    {
      globalMouseScrollY = (f32)globalMouseScroll.y;
      globalMouseScroll.y = 0.0f; // NOTE: Set to 0.0f to signify that the result has been consumed
      b32 mouseScrollIsCurrentlyActive = globalMouseScrollY != 0.0f;
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
    setControllerState(Controller1Input_A, XINPUT_GAMEPAD_A, gamepadButtonFlags);
    setControllerState(Controller1Input_B, XINPUT_GAMEPAD_B, gamepadButtonFlags);
    setControllerState(Controller1Input_X, XINPUT_GAMEPAD_X, gamepadButtonFlags);
    setControllerState(Controller1Input_Y, XINPUT_GAMEPAD_Y, gamepadButtonFlags);
    setControllerState(Controller1Input_DPad_Up, XINPUT_GAMEPAD_DPAD_UP, gamepadButtonFlags);
    setControllerState(Controller1Input_DPad_Down, XINPUT_GAMEPAD_DPAD_DOWN, gamepadButtonFlags);
    setControllerState(Controller1Input_DPad_Left, XINPUT_GAMEPAD_DPAD_LEFT, gamepadButtonFlags);
    setControllerState(Controller1Input_DPad_Right, XINPUT_GAMEPAD_DPAD_RIGHT, gamepadButtonFlags);
    setControllerState(Controller1Input_Shoulder_Left, XINPUT_GAMEPAD_LEFT_SHOULDER, gamepadButtonFlags);
    setControllerState(Controller1Input_Shoulder_Right, XINPUT_GAMEPAD_RIGHT_SHOULDER, gamepadButtonFlags);
    setControllerState(Controller1Input_Start, XINPUT_GAMEPAD_START, gamepadButtonFlags);
    setControllerState(Controller1Input_Select, XINPUT_GAMEPAD_BACK, gamepadButtonFlags);

    globalAnalogStickLeft = {controllerState.Gamepad.sThumbLX, controllerState.Gamepad.sThumbLY };
    if(globalAnalogStickLeft.x > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && globalAnalogStickLeft.x < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
      globalAnalogStickLeft.x = 0; // deadzone
    }
    if(globalAnalogStickLeft.y > -XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE && globalAnalogStickLeft.y < XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
    {
      globalAnalogStickLeft.y = 0; // deadzone
    }
    b32 analogStickLeftIsCurrentlyActive = globalAnalogStickLeft.x != 0 || globalAnalogStickLeft.y != 0;
    setInputState(Controller1Input_Analog_Left, analogStickLeftIsCurrentlyActive);

    globalAnalogStickRight = {controllerState.Gamepad.sThumbRX, controllerState.Gamepad.sThumbRY };
    if(globalAnalogStickRight.x > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && globalAnalogStickRight.x < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
      globalAnalogStickRight.x = 0; // deadzone
    }
    if(globalAnalogStickRight.y > -XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE && globalAnalogStickRight.y < XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
    {
      globalAnalogStickRight.y = 0; // deadzone
    }
    b32 analogStickRightIsCurrentlyActive = globalAnalogStickRight.x != 0 || globalAnalogStickRight.y != 0;
    setInputState(Controller1Input_Analog_Right, analogStickRightIsCurrentlyActive);

    b32 leftTriggerIsCurrentlyActive = controllerState.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    setInputState(Controller1Input_Trigger_Left, leftTriggerIsCurrentlyActive);
    globalController1TriggerLeftValue = leftTriggerIsCurrentlyActive ? controllerState.Gamepad.bLeftTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD : 0;

    b32 rightTriggerIsCurrentlyActive = controllerState.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    setInputState(Controller1Input_Trigger_Right, rightTriggerIsCurrentlyActive);
    globalController1TriggerRightValue = rightTriggerIsCurrentlyActive ? controllerState.Gamepad.bRightTrigger - XINPUT_GAMEPAD_TRIGGER_THRESHOLD : 0;
  }
}

// Callback function for when user scrolls with mouse wheel
void glfw_mouse_scroll_callback(GLFWwindow* window, f64 xOffset, f64 yOffset)
{
  globalMouseScroll.y = yOffset;
}

void subscribeWindowSizeCallback(windows_size_callback* callback)
{
  globalWindowSizeCallback = callback;
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

  globalWindowModeChangeTossNextInput = true;
  globalWindowExtent = {(u32)width, (u32)height };

  if(globalWindowSizeCallback != NULL) {
    globalWindowSizeCallback();
  }
}

void enableCursor(GLFWwindow* window, b32 enable)
{
  glfwSetInputMode(window, GLFW_CURSOR, enable ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
  globalWindowModeChangeTossNextInput = true;
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