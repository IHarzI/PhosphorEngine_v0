#pragma once

#include "Core/PH_CORE.h"
#include "Core/PH_GameObject.h"
#include "Core/PH_Window.h"
#include "Core/PH_FrameInfo.h"
#include "Containers/PH_StaticArray.h"
#include <array>
#include <limits>

#define PH_MAX_KEYS_CODE 14

namespace PhosphorEngine {
    using namespace Containers;
    class  keyControl {
    public:
        PH_STATIC_LOCAL union KeyMappings
        {
            PH_STATIC_LOCAL PH_StaticArray<int32, PH_MAX_KEYS_CODE> const Keys;

            PH_STATIC_LOCAL struct KeyCodes {
                PH_STATIC_LOCAL const int32 A = GLFW_KEY_A;
                PH_STATIC_LOCAL const int32 D = GLFW_KEY_D;
                PH_STATIC_LOCAL const int32 W = GLFW_KEY_W;
                PH_STATIC_LOCAL const int32 S = GLFW_KEY_S;
                PH_STATIC_LOCAL const int32 E = GLFW_KEY_E;
                PH_STATIC_LOCAL const int32 Q = GLFW_KEY_Q;
                PH_STATIC_LOCAL const int32 ArrowLeft = GLFW_KEY_LEFT;
                PH_STATIC_LOCAL const int32 ArrowRigth = GLFW_KEY_RIGHT;
                PH_STATIC_LOCAL const int32 ArrowUp = GLFW_KEY_UP;
                PH_STATIC_LOCAL const int32 ArrowDown = GLFW_KEY_DOWN;
                PH_STATIC_LOCAL const int32 MouseMiddle = GLFW_MOUSE_BUTTON_MIDDLE;
                PH_STATIC_LOCAL const int32 Esc = GLFW_KEY_ESCAPE;
                PH_STATIC_LOCAL const int32 CtrlLeft = GLFW_KEY_LEFT_CONTROL;
                PH_STATIC_LOCAL const int32 ShiftLeft = GLFW_KEY_LEFT_SHIFT;
                PH_STATIC_LOCAL const int32 K = GLFW_KEY_K;
                PH_STATIC_LOCAL const int32 R = GLFW_KEY_R;
                PH_STATIC_LOCAL const int32 AltLeft = GLFW_KEY_LEFT_ALT;
            };
        };

        void moveXYZ(GLFWwindow* window, float dt, PH_GameObject& gameobject);

        void cursorCntrl(GLFWwindow* window);

        void update(GLFWwindow* window) { glfwGetCursorPos(window, &xyMouseLastPos[0], &xyMouseLastPos[1]); };
    private:
        KeyMappings keys{};
        float moveSpd{ 2.f };
        float turnSpd{ 2.f };
        double xyMouseLastPos[2]= {0,0};
        Math::vec3 rotateTo{ 0.f };
    };

}