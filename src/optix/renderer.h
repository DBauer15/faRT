#pragma once

#include <memory>
#include <stage.h>
#include <glad/glad.h>

#include "optixdefs.h"
#include "buffer.h"
#include "mesh.h"

#include "cuda/common/types.cuh"

#include "common/defs.h"
#include "common/renderer.h"


namespace fart {

struct __align__( OPTIX_SBT_RECORD_ALIGNMENT ) RaygenRecord
{
    __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    void *data;
};

/*! SBT record for a miss program */
struct __align__( OPTIX_SBT_RECORD_ALIGNMENT ) MissRecord
{
    __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    void *data;
};

/*! SBT record for a hitgroup program */
struct __align__( OPTIX_SBT_RECORD_ALIGNMENT ) HitgroupRecord
{
    __align__( OPTIX_SBT_RECORD_ALIGNMENT ) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    OptixGeometrySBTData data;
};

struct OptiXRenderer : Renderer {

    public:
        ~OptiXRenderer();

        virtual void init(std::shared_ptr<Scene> &scene, std::shared_ptr<Window> &window) override; 

        virtual void render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) override;

        virtual std::string name() override {
            return "OptiX Renderer";
        }

    private:
        glm::vec3 m_prev_eye, m_prev_dir, m_prev_up;

        std::shared_ptr<Window>     m_window { nullptr };
        std::shared_ptr<Scene>      m_scene { nullptr };

        // Cuda Data
        CUcontext   m_cuda_context; 
        CUstream    m_cuda_stream;

        // OptiX Data
        OptixDeviceContext  m_optix_context;
        OptixModule         m_optix_module;
        OptixPipeline       m_optix_pipeline;
        OptixPipelineCompileOptions m_optix_pipeline_compile_options = {};

        std::vector<OptixProgramGroup> m_raygen_program_groups;
        std::vector<OptixProgramGroup> m_miss_program_groups;
        std::vector<OptixProgramGroup> m_hit_program_groups;

        OptixShaderBindingTable m_optix_sbt = {};
        CudaBuffer              m_raygen_records_buffer;
        CudaBuffer              m_miss_records_buffer;
        CudaBuffer              m_hitgroup_records_buffer;

        // Scene Data
        LaunchParams    m_launch_params;
        CudaBuffer      m_launch_params_buffer;
        std::vector<OptixObject>         m_objects;
        std::vector<OptixObjectInstance> m_instances;
        CudaBuffer      m_as_buffer;

        // Presentation Data
        std::vector<stage_vec4f>      m_pixel_data;
        CudaBuffer                    m_pixel_data_buffer;
        GLuint                        m_framebuffer_texture;

        // Private helper functions
        void initGL();

        void initOptiX();
        void initContexts();
        void initModule();
        void initRaygenProgramGroups();
        void initMissProgramGroups();
        void initHitProgramGroups();
        void initPipeline();

        void initBuffers();
        void initAccelerationStructure();
        void initSBT();

        void draw();
        bool shouldClear(const glm::vec3& eye, const glm::vec3& dir, const glm::vec3& up);

};


}
