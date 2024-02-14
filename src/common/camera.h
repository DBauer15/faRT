/* ArcballCamera adapted from Will Usher "ChameleonRT" */
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

#pragma once

#include <glm/ext.hpp>
#include <glm/glm.hpp>

namespace fart {

struct Camera {
    public:
        Camera() = default;
        virtual ~Camera() = default;

        virtual void rotate(glm::vec2 prev_mouse, glm::vec2 cur_mouse) = 0;
        virtual void pan(glm::vec2 mouse_delta) = 0;
        virtual void zoom(const float zoom_amount) = 0;
        virtual void move(glm::vec3 delta) = 0;

        virtual const glm::mat4 &transform() const {
            return camera;
        }
        virtual const glm::mat4 &inv_transform() const {
            return inv_camera;
        }

        virtual glm::vec3 eye() const {
            return glm::vec3(inv_camera * glm::vec4{ 0,0,0,1 });
        }
        virtual glm::vec3 dir() const {
            return glm::normalize(glm::vec3(inv_camera * glm::vec4{ 0,0,-1,0 }));
        }
        virtual glm::vec3 up() const {
            return glm::normalize(glm::vec3(inv_camera * glm::vec4{ 0,1,0,0 }));
        }
        virtual glm::vec3 center() const {
            return eye() + dir();
        }
    
    protected:
        virtual void updateCamera() = 0;
        glm::mat4 camera, inv_camera;
};


/* A simple arcball camera that moves around the camera's focal point.
 * The mouse inputs to the camera should be in normalized device coordinates,
 * where the top-left of the screen corresponds to [-1, 1], and the bottom
 * right is [1, -1].
 */
struct ArcballCamera : public Camera {

    public:
        /* Create an arcball camera focused on some center point
        * screen: [win_width, win_height]
        */
        ArcballCamera(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up);
        ArcballCamera(const Camera& other) : ArcballCamera(other.eye(), other.center(), other.up()) {}
        virtual ~ArcballCamera() = default;

        /* Rotate the camera from the previous mouse position to the current
        * one. Mouse positions should be in normalized device coordinates
        */
        virtual void rotate(glm::vec2 prev_mouse, glm::vec2 cur_mouse) override;

        /* Pan the camera given the translation vector. Mouse
        * delta amount should be in normalized device coordinates
        */
        virtual void pan(glm::vec2 mouse_delta) override;

        /* Zoom the camera given the zoom amount to (i.e., the scroll amount).
        * Positive values zoom in, negative will zoom out.
        */
        virtual void zoom(const float zoom_amount) override;

        /* Moving the camera directly is not supported by the arcball camera model*/
        virtual void move(glm::vec3 delta) override {}
        
        virtual glm::vec3 center() const override;

    protected:
        virtual void updateCamera() override;

        // We store the unmodified look at matrix along with
        // decomposed translation and rotation components
        glm::mat4 center_translation, translation;
        glm::quat rotation;
};

struct FirstPersonCamera : public Camera {

    public:
        FirstPersonCamera(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up);
        FirstPersonCamera(const Camera& other) : FirstPersonCamera(other.eye(), other.center(), other.up()) {}
        virtual ~FirstPersonCamera() = default;

        virtual void rotate(glm::vec2 prev_mouse, glm::vec2 cur_mouse) override;
        virtual void pan(glm::vec2 mouse_delta) override {}
        virtual void zoom(const float zoom_amount) override {}
        virtual void move(glm::vec3 delta) override;

    protected:
        virtual void updateCamera() override;

        glm::mat4 translation;
        glm::quat rotation;
};
}
