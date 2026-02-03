#pragma once

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class InputManager
{
public:
    InputManager();
    ~InputManager();

    static void Initialize(GLFWwindow* window);
    static void Update();

    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
    static void MouseCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

    static bool IsKeyPressed(int key);
    static bool IsKeyReleased(int key);
    static bool IsKeyDown(int key);
    static bool IsKeyUp(int key);

    static glm::vec2 GetMousePosition();
    static glm::vec2 GetMouseDelta();
    static float GetMouseWheelDelta();

    static bool IsMouseButtonDown(int button);
    static bool IsMouseButtonPressed(int button);
    static bool IsMouseButtonReleased(int button);

    static void SetMouseCursorVisible(bool visible);
    static void SetMouseCursorMode(int mode);

    static struct InputState GetInputState();

    static void UpdateCursorLock();
    static bool lockCursor;

protected:
    static bool keys[1024];
    static bool prevKeys[1024];

    static bool mouseButtons[8];
    static bool prevMouseButtons[8];

    static double lastMouseX;
    static double lastMouseY;
    static double currentMouseX;
    static double currentMouseY;
    static bool firstMouse;

    static float mouseWheelDelta;
    static float mouseWheelCurrent;

    static GLFWwindow* window;
};

struct InputState
{
    bool keys[1024];
    bool keysPressed[1024];
    bool keysReleased[1024];

    bool mouseButtons[8];
    bool mouseButtonsPressed[8];
    bool mouseButtonsReleased[8];

    glm::vec2 mousePosition;
    glm::vec2 mouseDelta;
    float mouseWheelDelta;
};