#pragma once

#include "optix/optixdefs.h"

#include "stage.h"
using namespace stage;

namespace fart {

struct LaunchParams {
    /* Output image is written to this pointer */
    stage_vec4f* framebuffer_rgba;

    /* TLAS/BLAS Acceleration Structures */
    OptixTraversableHandle traversable;

    /* Camera */
    struct {
        stage_vec3f eye; 
        stage_vec3f dir; 
        stage_vec3f up; 
    } camera;

    /* Additional Parameters */
    float scene_scale;
    float aspect_ratio;
    stage_vec2i viewport_size;
};

}
