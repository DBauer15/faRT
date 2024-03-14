/**The MIT License (MIT)

Copyright (c) 2016 Will Usher

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "camera.h"
#include <cmath>
#include <iostream>
#include <glm/ext.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/component_wise.hpp>

#include "defs.h"

namespace fart {

// Project the point in [-1, 1] screen space onto the arcball sphere
static glm::quat screen_to_arcball(const glm::vec2 &p);

ArcballCamera::ArcballCamera(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up)
{
    float max_up = glm::compMax(up);
    m_world_up.x = (up.x == max_up ? 1.f : 0.f);
    m_world_up.y = (up.y == max_up ? 1.f : 0.f);
    m_world_up.z = (up.z == max_up ? 1.f : 0.f);

    const glm::vec3 dir = center - eye;
    glm::vec3 z_axis = glm::normalize(dir);
    glm::vec3 x_axis = glm::normalize(glm::cross(z_axis, glm::normalize(up)));
    glm::vec3 y_axis = glm::normalize(glm::cross(x_axis, z_axis));
    x_axis = glm::normalize(glm::cross(z_axis, y_axis));

    center_translation = glm::inverse(glm::translate(center));
    translation = glm::translate(glm::vec3(0.f, 0.f, -glm::length(dir)));
    rotation = glm::normalize(glm::quat_cast(glm::transpose(glm::mat3(x_axis, y_axis, -z_axis))));

    updateCamera();
}

ArcballCamera::ArcballCamera(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up, const glm::vec3 world_up)
{
    m_world_up = world_up;
    const glm::vec3 dir = center - eye;
    glm::vec3 z_axis = glm::normalize(dir);
    glm::vec3 x_axis = glm::normalize(glm::cross(z_axis, glm::normalize(up)));
    glm::vec3 y_axis = glm::normalize(glm::cross(x_axis, z_axis));
    x_axis = glm::normalize(glm::cross(z_axis, y_axis));

    center_translation = glm::inverse(glm::translate(center));
    translation = glm::translate(glm::vec3(0.f, 0.f, -glm::length(dir)));
    rotation = glm::normalize(glm::quat_cast(glm::transpose(glm::mat3(x_axis, y_axis, -z_axis))));

    updateCamera();
}

void ArcballCamera::rotate(glm::vec2 prev_mouse, glm::vec2 cur_mouse)
{
    // Clamp mouse positions to stay in NDC
    cur_mouse = glm::clamp(cur_mouse, glm::vec2{-1, -1}, glm::vec2{1, 1});
    prev_mouse = glm::clamp(prev_mouse, glm::vec2{-1, -1}, glm::vec2{1, 1});

    const glm::quat mouse_cur_ball = screen_to_arcball(cur_mouse);
    const glm::quat mouse_prev_ball = screen_to_arcball(prev_mouse);

    rotation = mouse_cur_ball * mouse_prev_ball * rotation;
    updateCamera();
}

void ArcballCamera::pan(glm::vec2 mouse_delta)
{
    const float zoom_amount = std::abs(translation[3][2]);
    glm::vec4 motion(mouse_delta.x * zoom_amount, mouse_delta.y * zoom_amount, 0.f, 0.f);
    // Find the panning amount in the world space
    motion = inv_camera * motion;

    center_translation = glm::translate(glm::vec3(motion)) * center_translation;
    updateCamera();
}

void ArcballCamera::zoom(const float zoom_amount)
{
    const glm::vec3 motion(0.f, 0.f, zoom_amount);

    translation = glm::translate(motion) * translation;
    updateCamera();
}

glm::vec3 ArcballCamera::center() const {
    return glm::vec3(glm::inverse(center_translation) * glm::vec4{ 0, 0, 0, 1});
}

void ArcballCamera::updateCamera()
{
    camera = translation * glm::mat4_cast(rotation) * center_translation;
    inv_camera = glm::inverse(camera);
}

glm::quat screen_to_arcball(const glm::vec2 &p)
{
    const float dist = glm::dot(p, p);
    // If we're on/in the sphere return the point on it
    if (dist <= 1.f) {
        return glm::quat(0.0, p.x, p.y, std::sqrt(1.f - dist));
    } else {
        // otherwise we project the point onto the sphere
        const glm::vec2 proj = glm::normalize(p);
        return glm::quat(0.0, proj.x, proj.y, 0.f);
    }
}

FirstPersonCamera::FirstPersonCamera(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up) {
    float max_up = glm::compMax(up);
    m_world_up.x = (up.x == max_up ? 1.f : 0.f);
    m_world_up.y = (up.y == max_up ? 1.f : 0.f);
    m_world_up.z = (up.z == max_up ? 1.f : 0.f);

    const glm::vec3 dir = center - eye;
    glm::vec3 z_axis = glm::normalize(dir);
    glm::vec3 x_axis = glm::normalize(glm::cross(z_axis, glm::normalize(up)));
    glm::vec3 y_axis = glm::normalize(glm::cross(x_axis, z_axis));
    x_axis = glm::normalize(glm::cross(z_axis, y_axis));

    translation = glm::translate(-eye);
    rotation = glm::normalize(glm::quat_cast(glm::transpose(glm::mat3(x_axis, y_axis, -z_axis))));

    updateCamera();
}

FirstPersonCamera::FirstPersonCamera(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up, const glm::vec3 world_up)
{
    m_world_up = world_up;
    const glm::vec3 dir = center - eye;
    glm::vec3 z_axis = glm::normalize(dir);
    glm::vec3 x_axis = glm::normalize(glm::cross(z_axis, glm::normalize(up)));
    glm::vec3 y_axis = glm::normalize(glm::cross(x_axis, z_axis));
    x_axis = glm::normalize(glm::cross(z_axis, y_axis));

    translation = glm::translate(-eye);
    rotation = glm::normalize(glm::quat_cast(glm::transpose(glm::mat3(x_axis, y_axis, -z_axis))));

    updateCamera();
}

void 
FirstPersonCamera::rotate(glm::vec2 prev_mouse, glm::vec2 cur_mouse) {
    glm::vec2 delta { cur_mouse.x - prev_mouse.x, prev_mouse.y - cur_mouse.y };
    delta *= 5.f;
    glm::quat yaw = glm::angleAxis(delta.x, world_up());
    glm::quat pitch = glm::angleAxis(delta.y, glm::cross(dir(), world_up()));

    rotation = rotation * pitch * yaw;
    updateCamera();
}

void 
FirstPersonCamera::move(glm::vec3 delta) {
    delta = dir() * delta.z + up() * delta.y + glm::cross(dir(), up()) * delta.x;
    translation = glm::translate(delta) * translation;
    updateCamera();
}

void
FirstPersonCamera::updateCamera() {
    camera = glm::mat4_cast(rotation) * translation;
    inv_camera = glm::inverse(camera);
}

}
