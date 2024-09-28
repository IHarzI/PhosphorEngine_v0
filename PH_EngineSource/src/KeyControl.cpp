#include "KeyControl.h"

namespace PhosphorEngine {

    const PH_StaticArray<int32, PH_MAX_KEYS_CODE> keyControl::KeyMappings::Keys{};

    void keyControl::moveXYZ(GLFWwindow* window, float dt, PH_GameObject& gameobject)
    {
        if (glfwGetKey(window, KeyMappings::KeyCodes::Esc) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
            return;
        }

        glfwGetCursorPos(window, &xyMouseLastPos[0], &xyMouseLastPos[1]);

        if (glfwGetKey(window, KeyMappings::KeyCodes::ArrowRigth) == GLFW_PRESS) rotateTo.y += turnSpd;
        if (glfwGetKey(window, KeyMappings::KeyCodes::ArrowLeft) == GLFW_PRESS) rotateTo.y -= turnSpd;
        if (glfwGetKey(window, KeyMappings::KeyCodes::ArrowUp) == GLFW_PRESS) rotateTo.x += turnSpd;
        if (glfwGetKey(window, KeyMappings::KeyCodes::ArrowDown) == GLFW_PRESS) rotateTo.x -= turnSpd;

        if (Math::DotProduct(rotateTo, rotateTo) > std::numeric_limits<float>::epsilon())
            gameobject.transform.rotation = gameobject.transform.rotation + Math::Normalize(rotateTo) * dt * turnSpd;

        gameobject.transform.rotation.x = Math::clamp(gameobject.transform.rotation.x, -1.5f, 1.5f);
        gameobject.transform.rotation.y = Math::mod(gameobject.transform.rotation.y, Math::two_pi<float>());

        float yaw = gameobject.transform.rotation.y;
        const Math::vec3 forwardDir{ Math::sin(yaw), 0.f, Math::cos(yaw) };
        const Math::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
        const Math::vec3 upDir{ 0.f, -1.f, 0.f };

        Math::vec3 moveDir{ 0.f };
        if (glfwGetKey(window, KeyMappings::KeyCodes::W) == GLFW_PRESS) moveDir += forwardDir;
        if (glfwGetKey(window, KeyMappings::KeyCodes::S) == GLFW_PRESS) moveDir -= forwardDir;
        if (glfwGetKey(window, KeyMappings::KeyCodes::D) == GLFW_PRESS) moveDir += rightDir;
        if (glfwGetKey(window, KeyMappings::KeyCodes::A) == GLFW_PRESS) moveDir -= rightDir;
        if (glfwGetKey(window, KeyMappings::KeyCodes::E) == GLFW_PRESS) moveDir += upDir;
        if (glfwGetKey(window, KeyMappings::KeyCodes::Q) == GLFW_PRESS) moveDir -= upDir;

        if (Math::DotProduct(moveDir, moveDir) > std::numeric_limits<float>::epsilon())
            gameobject.transform.translation += Math::Normalize(moveDir) * dt * moveSpd;

        rotateTo = Math::vec3{ 0.f };
    }
    void keyControl::cursorCntrl(GLFWwindow* window)
    {
        const int MouseSensetivity = 10;
        int button{ KeyMappings::KeyCodes::MouseMiddle};
        if (glfwGetMouseButton(window, button) == GLFW_PRESS)
        {
            double CurrentPos[2]{0.};
            glfwGetCursorPos(window, &CurrentPos[0], &CurrentPos[1]);

            double xRot = CurrentPos[1] - xyMouseLastPos[1] ;
            double yRot = CurrentPos[0] - xyMouseLastPos[0];
            yRot *= -MouseSensetivity;
            xRot *= MouseSensetivity;
            rotateTo.x += turnSpd * (xRot);
            rotateTo.y += turnSpd * (yRot);
        };
    }
}