#include "InputManager.h"
#include <cstring>

bool InputManager::keys[1024] = { false };
bool InputManager::prevKeys[1024] = { false };

bool InputManager::mouseButtons[8] = { false };
bool InputManager::prevMouseButtons[8] = { false };

double InputManager::lastMouseX = 0.0;
double InputManager::lastMouseY = 0.0;
double InputManager::currentMouseX = 0.0;
double InputManager::currentMouseY = 0.0;
bool InputManager::firstMouse = true;

float InputManager::mouseWheelDelta = 0.0f;
float InputManager::mouseWheelCurrent = 0.0f;

GLFWwindow* InputManager::window = nullptr;

bool InputManager::lockCursor;

InputManager::InputManager()
{
    lockCursor = false;
}

InputManager::~InputManager()
{
}

void InputManager::Initialize(GLFWwindow* win)
{
    window = win;
    int width, height;
    glfwGetWindowSize(window, &width, &height);
    lastMouseX = width / 2.0;
    lastMouseY = height / 2.0;
    currentMouseX = lastMouseX;
    currentMouseY = lastMouseY;

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetScrollCallback(window, ScrollCallback);

   // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    SetMouseCursorVisible(true);
    //if (glfwRawMouseMotionSupported())
       // glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    memset(keys, 0, sizeof(keys));
    memset(prevKeys, 0, sizeof(prevKeys));
    memset(mouseButtons, 0, sizeof(mouseButtons));
    memset(prevMouseButtons, 0, sizeof(prevMouseButtons));
}

void InputManager::Update()
{
    memcpy(prevKeys, keys, sizeof(keys));
    memcpy(prevMouseButtons, mouseButtons, sizeof(mouseButtons));

    mouseWheelDelta = 0.0f;
}


void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)
        {
            keys[key] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            keys[key] = false;
        }
    }
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button >= 0 && button < 8)
    {
        if (action == GLFW_PRESS)
        {
            mouseButtons[button] = true;
        }
        else if (action == GLFW_RELEASE)
        {
            mouseButtons[button] = false;
        }
    }
}

glm::vec2 InputManager::GetMouseDelta()
{
    if (firstMouse)
    {
        firstMouse = false;
        return glm::vec2(0.0f, 0.0f);
    }

    float deltaX = (float)(currentMouseX - lastMouseX);
    float deltaY = (float)(currentMouseY - lastMouseY);

    lastMouseX = currentMouseX;
    lastMouseY = currentMouseY;

    return glm::vec2(deltaX, -deltaY);
}

void InputManager::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastMouseX = xpos;
        lastMouseY = ypos;
        currentMouseX = xpos;
        currentMouseY = ypos;
        firstMouse = false;
    }

    currentMouseX = xpos;
    currentMouseY = ypos;
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    mouseWheelDelta += (float)yoffset;
    mouseWheelCurrent += (float)yoffset;
}

bool InputManager::IsKeyPressed(int key)
{
    if (key >= 0 && key < 1024)
        return keys[key] && !prevKeys[key];
    return false;
}

bool InputManager::IsKeyReleased(int key)
{
    if (key >= 0 && key < 1024)
        return !keys[key] && prevKeys[key];
    return false;
}

bool InputManager::IsKeyDown(int key)
{
    if (key >= 0 && key < 1024)
        return keys[key];
    return false;
}

bool InputManager::IsKeyUp(int key)
{
    if (key >= 0 && key < 1024)
        return !keys[key];
    return true;
}

glm::vec2 InputManager::GetMousePosition()
{
    return glm::vec2((float)currentMouseX, (float)currentMouseY);
}



float InputManager::GetMouseWheelDelta()
{
    return mouseWheelDelta;
}

bool InputManager::IsMouseButtonDown(int button)
{
    if (button >= 0 && button < 8)
        return mouseButtons[button];
    return false;
}

bool InputManager::IsMouseButtonPressed(int button)
{
    if (button >= 0 && button < 8)
        return mouseButtons[button] && !prevMouseButtons[button];
    return false;
}

bool InputManager::IsMouseButtonReleased(int button)
{
    if (button >= 0 && button < 8)
        return !mouseButtons[button] && prevMouseButtons[button];
    return false;
}

void InputManager::SetMouseCursorVisible(bool visible)
{
    glfwSetInputMode(window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
}

void InputManager::SetMouseCursorMode(int mode)
{
    glfwSetInputMode(window, GLFW_CURSOR, mode);
}

void InputManager::UpdateCursorLock()
{
    static bool prevCtrlState = false;
    bool currentCtrlState = IsKeyDown(GLFW_KEY_LEFT_CONTROL) || IsKeyDown(GLFW_KEY_RIGHT_CONTROL);

    if (currentCtrlState && !prevCtrlState)
    {
        lockCursor = !lockCursor;
        SetMouseCursorVisible(!lockCursor);
    }

    prevCtrlState = currentCtrlState;
}

InputState InputManager::GetInputState()
{
    InputState state;

    memcpy(state.keys, keys, sizeof(keys));
    for (int i = 0; i < 1024; i++)
    {
        state.keysPressed[i] = keys[i] && !prevKeys[i];
        state.keysReleased[i] = !keys[i] && prevKeys[i];
    }

    memcpy(state.mouseButtons, mouseButtons, sizeof(mouseButtons));
    for (int i = 0; i < 8; i++)
    {
        state.mouseButtonsPressed[i] = mouseButtons[i] && !prevMouseButtons[i];
        state.mouseButtonsReleased[i] = !mouseButtons[i] && prevMouseButtons[i];
    }

    state.mousePosition = GetMousePosition();
    state.mouseDelta = GetMouseDelta();
    state.mouseWheelDelta = GetMouseWheelDelta();

    return state;
}