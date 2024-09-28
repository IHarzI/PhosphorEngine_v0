#include "PH_Camera.h"
#include "PH_MATH.h"

#undef far
#undef near

namespace PhosphorEngine {

    void PH_Camera::setOrthographicProjection(float left, float right, float top, float bottom, float near, float far)
    {
        projectionMatrix = Math::orthoLH_ZO(left, right, top, bottom, near, far);
    }


    void PH_Camera::setPerspectiveProjection(float fovy, float aspect, float near, float far)
    {   
        assert(Math::abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
        const float tanHalfFovy = tan(fovy / 2.f);
        projectionMatrix = Math::mat4x4{ 0.0f };
        projectionMatrix[0][0] = 1.f / (aspect * tanHalfFovy);
        projectionMatrix[1][1] = 1.f / (tanHalfFovy);
        projectionMatrix[2][2] = far / (far - near);
        projectionMatrix[2][3] = 1.f;
        projectionMatrix[3][2] = -(far * near) / (far - near);
    }

    void PH_Camera::setViewDirection(Math::vec3 position, Math::vec3 direction, Math::vec3 up)
    {
        const Math::vec3 w{ Math::Normalize(direction) };
        const Math::vec3 u{ Math::Normalize(Math::CrossProduct(w, up)) };
        const Math::vec3 v{ Math::CrossProduct(w, u) };

        viewMatrix = Math::mat4x4{ 1.f };
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -Math::DotProduct(u, position);
        viewMatrix[3][1] = -Math::DotProduct(v, position);
        viewMatrix[3][2] = -Math::DotProduct(w, position);

        inverseViewMatrix = Math::mat4x4{ 1.f };
        inverseViewMatrix[0][0] = u.x;
        inverseViewMatrix[0][1] = u.y;
        inverseViewMatrix[0][2] = u.z;
        inverseViewMatrix[1][0] = v.x;
        inverseViewMatrix[1][1] = v.y;
        inverseViewMatrix[1][2] = v.z;
        inverseViewMatrix[2][0] = w.x;
        inverseViewMatrix[2][1] = w.y;
        inverseViewMatrix[2][2] = w.z;
        inverseViewMatrix[3][0] = position.x;
        inverseViewMatrix[3][1] = position.y;
        inverseViewMatrix[3][2] = position.z;
    }

    void PH_Camera::setViewTarget(Math::vec3 position, Math::vec3 target, Math::vec3 up)
    {
        setViewDirection(position, target - position, up);
    }

    void PH_Camera::setViewYXZ(Math::vec3 position, Math::vec3 rotation)
    {
        const float c3 = Math::cos(rotation.z);
        const float s3 = Math::sin(rotation.z);
        const float c2 = Math::cos(rotation.x);
        const float s2 = Math::sin(rotation.x);
        const float c1 = Math::cos(rotation.y);
        const float s1 = Math::sin(rotation.y);
        const Math::vec3 u{ (c1 * c3 + s1 * s2 * s3), (c2 * s3), (c1 * s2 * s3 - c3 * s1) };
        const Math::vec3 v{ (c3 * s1 * s2 - c1 * s3), (c2 * c3), (c1 * c3 * s2 + s1 * s3) };
        const Math::vec3 w{ (c2 * s1), (-s2), (c1 * c2) };
        viewMatrix = Math::mat4x4{ 1.f };
        viewMatrix[0][0] = u.x;
        viewMatrix[1][0] = u.y;
        viewMatrix[2][0] = u.z;
        viewMatrix[0][1] = v.x;
        viewMatrix[1][1] = v.y;
        viewMatrix[2][1] = v.z;
        viewMatrix[0][2] = w.x;
        viewMatrix[1][2] = w.y;
        viewMatrix[2][2] = w.z;
        viewMatrix[3][0] = -Math::DotProduct(u, position);
        viewMatrix[3][1] = -Math::DotProduct(v, position);
        viewMatrix[3][2] = -Math::DotProduct(w, position);

        inverseViewMatrix = Math::mat4x4{ 1.f };
        inverseViewMatrix[0][0] = u.x;
        inverseViewMatrix[0][1] = u.y;
        inverseViewMatrix[0][2] = u.z;
        inverseViewMatrix[1][0] = v.x;
        inverseViewMatrix[1][1] = v.y;
        inverseViewMatrix[1][2] = v.z;
        inverseViewMatrix[2][0] = w.x;
        inverseViewMatrix[2][1] = w.y;
        inverseViewMatrix[2][2] = w.z;
        inverseViewMatrix[3][0] = position.x;
        inverseViewMatrix[3][1] = position.y;
        inverseViewMatrix[3][2] = position.z;
    }

}