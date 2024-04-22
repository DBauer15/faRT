#include "renderer.h"

#include <chrono>
#include <optix_function_table_definition.h>

using namespace stage;

namespace fart {

extern "C" const char embedded_ptx[];

OptiXRenderer::~OptiXRenderer() {
}

void
OptiXRenderer::init(std::shared_ptr<Scene>& scene, std::shared_ptr<Window>& window) {
    m_scene = scene;
    m_window = window;

    initGL();
    initOptiX();
    initContexts();
    initModule();
    initRaygenProgramGroups();
    initMissProgramGroups();
    initHitProgramGroups();
    initPipeline();
    initBuffers();
    initAccelerationStructure();
    initSBT();
}

void
OptiXRenderer::initGL() {
    glfwMakeContextCurrent(m_window->getGlfwWindow());
    glfwSwapInterval(0);
    gladLoadGL();
    glViewport(0, 0, m_window->getWidth(), m_window->getHeight());
    glClearColor(1, 0, 0, 1);

    glGenTextures(1, &m_framebuffer_texture);
}

void
OptiXRenderer::initOptiX() {
    cudaFree(0);
    int n_devices;
    cudaGetDeviceCount(&n_devices);
    if (n_devices == 0) {
        ERR("No CUDA device found");
        throw std::runtime_error("No CUDA device found");
    }
    LOG("Found " + std::to_string(n_devices) << " CUDA device(s)");

    OPTIX_CHECK( optixInit() );
}

void
OptiXRenderer::initContexts() {
 
    // We run everything on GPU0
    int device_id = 0;
    cudaDeviceProp device_prop;
    cudaGetDeviceProperties(&device_prop, device_id);
    LOG("Running on CUDA device " + std::string(device_prop.name));

    CUresult result = cuCtxGetCurrent(&m_cuda_context);
    if (result != CUDA_SUCCESS)
        ERR("Error getting CUDA context");

    OPTIX_CHECK(optixDeviceContextCreate(m_cuda_context, nullptr, &m_optix_context));
    OPTIX_CHECK(optixDeviceContextSetLogCallback(m_optix_context, [](unsigned int level,
                                                                     const char *tag,
                                                                     const char *message,
                                                                     void *) {
                    std::string msg(message);
                    switch (level) {
                        case 1:
                        case 2:
                            ERR("OptiX Error: " + msg);
                            break;
                        case 3:
                            WARN("OptiX Warning: " + msg);
                            break;
                        case 4:
                            LOG("Optix Log: " + msg);
                            break;
                    }
                }, nullptr, 4));
}

void
OptiXRenderer::initModule() {
    OptixModuleCompileOptions module_compile_options = {};
    module_compile_options.maxRegisterCount  = 50;
    module_compile_options.optLevel          = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
    module_compile_options.debugLevel        = OPTIX_COMPILE_DEBUG_LEVEL_NONE;

    m_optix_pipeline_compile_options = {};
    m_optix_pipeline_compile_options.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
    m_optix_pipeline_compile_options.usesMotionBlur     = false;
    m_optix_pipeline_compile_options.numPayloadValues   = 2;
    m_optix_pipeline_compile_options.numAttributeValues = 2;
    m_optix_pipeline_compile_options.exceptionFlags     = OPTIX_EXCEPTION_FLAG_NONE;
    m_optix_pipeline_compile_options.pipelineLaunchParamsVariableName = "launch_params";
      
    const std::string ptx_code = embedded_ptx;
      
    char log[2048];
    size_t sizeof_log = sizeof( log );
    OPTIX_CHECK(optixModuleCreateFromPTX(m_optix_context,
                                         &module_compile_options,
                                         &m_optix_pipeline_compile_options,
                                         ptx_code.c_str(),
                                         ptx_code.size(),
                                         log,
                                         &sizeof_log,
                                         &m_optix_module
                                         ));
    if (sizeof_log > 1) 
        LOG(log);

    LOG("Built OptiX module from PTX");
}

void
OptiXRenderer::initRaygenProgramGroups() {
    m_raygen_program_groups.resize(1);
      
    OptixProgramGroupOptions pg_options = {};
    OptixProgramGroupDesc pg_description = {};
    pg_description.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
    pg_description.raygen.module = m_optix_module;           
    pg_description.raygen.entryFunctionName = "__raygen__pathtrace";

    char log[2048];
    size_t sizeof_log = sizeof( log );
    OPTIX_CHECK(optixProgramGroupCreate(m_optix_context,
                                        &pg_description,
                                        1,
                                        &pg_options,
                                        log,
                                        &sizeof_log,
                                        &m_raygen_program_groups[0]
                                        ));
    if (sizeof_log > 1) 
        LOG(log);
}

void
OptiXRenderer::initMissProgramGroups() {
    m_miss_program_groups.resize(1);
      
    OptixProgramGroupOptions pg_options = {};
    OptixProgramGroupDesc pg_description = {};
    pg_description.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
    pg_description.miss.module = m_optix_module;           
    pg_description.miss.entryFunctionName = "__miss__pathtrace";

    char log[2048];
    size_t sizeof_log = sizeof( log );
    OPTIX_CHECK(optixProgramGroupCreate(m_optix_context,
                                        &pg_description,
                                        1,
                                        &pg_options,
                                        log,
                                        &sizeof_log,
                                        &m_miss_program_groups[0]
                                        ));
    if (sizeof_log > 1) 
        LOG(log);
}

void
OptiXRenderer::initHitProgramGroups() {
    m_hit_program_groups.resize(1);

    OptixProgramGroupOptions pg_options = {};
    OptixProgramGroupDesc pg_description = {};
    pg_description.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
    pg_description.hitgroup.moduleCH = m_optix_module;
    pg_description.hitgroup.entryFunctionNameCH = "__closesthit__pathtrace";

    char log[2048];
    size_t sizeof_log = sizeof( log );
    OPTIX_CHECK(optixProgramGroupCreate(m_optix_context,
                                        &pg_description,
                                        1,
                                        &pg_options,
                                        log,
                                        &sizeof_log,
                                        &m_hit_program_groups[0]
                                        ));
    if (sizeof_log > 1)
        LOG(log);
}

void
OptiXRenderer::initPipeline() {
    // Collect program groups
    std::vector<OptixProgramGroup> program_groups;
    for (auto pg : m_raygen_program_groups)
      program_groups.push_back(pg);
    for (auto pg : m_miss_program_groups)
      program_groups.push_back(pg);
    for (auto pg : m_hit_program_groups)
      program_groups.push_back(pg);

    OptixPipelineLinkOptions pipeline_link_options = {};
    pipeline_link_options.maxTraceDepth          = 2;
      
    char log[2048];
    size_t sizeof_log = sizeof( log );
    OPTIX_CHECK(optixPipelineCreate(m_optix_context,
                                    &m_optix_pipeline_compile_options,
                                    &pipeline_link_options,
                                    program_groups.data(),
                                    (int)program_groups.size(),
                                    log,
                                    &sizeof_log,
                                    &m_optix_pipeline
                                    ));
    if (sizeof_log > 1) 
        LOG(log);

    OPTIX_CHECK(optixPipelineSetStackSize
                (m_optix_pipeline, 
                 2*1024 /* direct stack size from traversal */,
                 2*1024 /* direct stack size from state */,
                 2*1024 /* continuation stack size */,
                 1 /* max traveral graph depth */ 
                 ));

    if (sizeof_log > 1)
        LOG(log);
}

void
OptiXRenderer::initAccelerationStructure() {
    OptixTraversableHandle as_handle { 0 };

    size_t n_geometries = 0;
    for (auto& object : m_objects) {
        n_geometries += object.getGeometries().size();
    }
    
    // Triangle Inputs
    std::vector<OptixBuildInput> triangle_inputs(n_geometries);
    std::vector<uint32_t> triangle_input_flags(n_geometries);
    std::vector<CUdeviceptr> triangle_vertex_buffers(n_geometries);
    std::vector<CUdeviceptr> triangle_index_buffers(n_geometries);

    size_t geometry_id = 0;
    for (auto& object : m_objects) {
        for (auto& geometry : object.getGeometries()) {
            triangle_vertex_buffers[geometry_id] = geometry.getVertices().device_ptr();
            triangle_index_buffers[geometry_id] = geometry.getIndices().device_ptr();

            triangle_inputs[geometry_id] = {};
            triangle_inputs[geometry_id].type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
            triangle_inputs[geometry_id].triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
            triangle_inputs[geometry_id].triangleArray.vertexStrideInBytes = sizeof(AlignedVertex);
            triangle_inputs[geometry_id].triangleArray.numVertices = geometry.getVertices().size<AlignedVertex>();
            triangle_inputs[geometry_id].triangleArray.vertexBuffers = &triangle_vertex_buffers[geometry_id];

            triangle_inputs[geometry_id].triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
            triangle_inputs[geometry_id].triangleArray.indexStrideInBytes = sizeof(stage_vec3i);
            triangle_inputs[geometry_id].triangleArray.numIndexTriplets = geometry.getIndices().size<stage_vec3i>();
            triangle_inputs[geometry_id].triangleArray.indexBuffer = triangle_index_buffers[geometry_id];

            triangle_input_flags[geometry_id] = 0;
            triangle_inputs[geometry_id].triangleArray.flags = &triangle_input_flags[geometry_id];
            triangle_inputs[geometry_id].triangleArray.numSbtRecords                = 1;
            triangle_inputs[geometry_id].triangleArray.sbtIndexOffsetBuffer         = 0;
            triangle_inputs[geometry_id].triangleArray.sbtIndexOffsetSizeInBytes    = 0;
            triangle_inputs[geometry_id].triangleArray.sbtIndexOffsetStrideInBytes  = 0;

            geometry_id++;
        }
    }

    // BLAS setup
    OptixAccelBuildOptions accel_options = {};
    accel_options.buildFlags             = OPTIX_BUILD_FLAG_NONE | OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
    accel_options.motionOptions.numKeys  = 1;
    accel_options.operation              = OPTIX_BUILD_OPERATION_BUILD;
    
    OptixAccelBufferSizes blas_buffer_sizes;
    OPTIX_CHECK(optixAccelComputeMemoryUsage
                (m_optix_context,
                 &accel_options,
                 triangle_inputs.data(),
                 (int)n_geometries,
                 &blas_buffer_sizes
                 ));
    
    // Prepare Compaction
    CudaBuffer compacted_size_buffer;
    compacted_size_buffer.alloc(sizeof(uint64_t));
    
    OptixAccelEmitDesc emit_desc;
    emit_desc.type   = OPTIX_PROPERTY_TYPE_COMPACTED_SIZE;
    emit_desc.result = compacted_size_buffer.device_ptr();
    
    // ==================================================================
    // execute build (main stage)
    // ==================================================================
    
    CudaBuffer temp_buffer;
    temp_buffer.alloc(blas_buffer_sizes.tempSizeInBytes);
    
    CudaBuffer output_buffer;
    output_buffer.alloc(blas_buffer_sizes.outputSizeInBytes);
      
    OPTIX_CHECK(optixAccelBuild(m_optix_context,
                                /* stream */0,
                                &accel_options,
                                triangle_inputs.data(),
                                (int)n_geometries,
                                temp_buffer.device_ptr(),
                                temp_buffer.sizeInBytes(),
                                output_buffer.device_ptr(),
                                output_buffer.sizeInBytes(),
                                &as_handle,
                                &emit_desc,
                                1
                                ));
    CUDA_SYNC_CHECK();
    
    // Perform Compaction
    uint64_t compacted_size;
    compacted_size_buffer.download(&compacted_size, 1);
    
    // Allocate memory to hold the final acceleration structure data
    m_as_buffer.alloc(compacted_size);
    OPTIX_CHECK(optixAccelCompact(m_optix_context,
                                  /*stream:*/0,
                                  as_handle,
                                  m_as_buffer.device_ptr(),
                                  m_as_buffer.sizeInBytes(),
                                  &as_handle));
    CUDA_SYNC_CHECK();
    
    output_buffer.free(); 
    temp_buffer.free();
    compacted_size_buffer.free();
    
    // Store the handle to the AS in the launch params
    m_launch_params.traversable = as_handle;
}

void
OptiXRenderer::initBuffers() {
    for (auto& object : m_scene->getObjects()) {
        OptixObject optix_object(object);
        m_objects.push_back(optix_object);
    }
    LOG("Uploaded " + std::to_string(m_objects.size()) + " objects to GPU");
}

void
OptiXRenderer::initSBT() {

    // Raygen Records
    std::vector<RaygenRecord> raygen_records;
    for (size_t i=0; i < m_raygen_program_groups.size(); i++) {
      RaygenRecord rec;
      OPTIX_CHECK(optixSbtRecordPackHeader(m_raygen_program_groups[i], &rec));
      rec.data = nullptr; 
      raygen_records.push_back(rec);
    }
    m_raygen_records_buffer.alloc_and_upload(raygen_records);
    m_optix_sbt.raygenRecord = m_raygen_records_buffer.device_ptr();

    // Miss Records
    std::vector<MissRecord> miss_records;
    for (size_t i=0; i < m_miss_program_groups.size(); i++) {
      MissRecord rec;
      OPTIX_CHECK(optixSbtRecordPackHeader(m_miss_program_groups[i], &rec));
      rec.data = nullptr;
      miss_records.push_back(rec);
    }
    m_miss_records_buffer.alloc_and_upload(miss_records);
    m_optix_sbt.missRecordBase          = m_miss_records_buffer.device_ptr();
    m_optix_sbt.missRecordStrideInBytes = sizeof(MissRecord);
    m_optix_sbt.missRecordCount         = (int)miss_records.size();

    // Hitgroup Records
    std::vector<HitgroupRecord> hitgroup_records;
    for (auto& object : m_objects) {
        for (auto& geometry : object.getGeometries()) {
            HitgroupRecord rec;
            OPTIX_CHECK(optixSbtRecordPackHeader(m_hit_program_groups[0], &rec));
            rec.data = geometry.getSBTData();
            hitgroup_records.push_back(rec);
        }
    }
    m_hitgroup_records_buffer.alloc_and_upload(hitgroup_records);
    m_optix_sbt.hitgroupRecordBase          = m_hitgroup_records_buffer.device_ptr();
    m_optix_sbt.hitgroupRecordStrideInBytes = sizeof(HitgroupRecord);
    m_optix_sbt.hitgroupRecordCount         = (int)hitgroup_records.size();
}

void
OptiXRenderer::render(const glm::vec3 eye, const glm::vec3 dir, const glm::vec3 up, RenderStats& render_stats) {
    auto t_start = std::chrono::high_resolution_clock::now();

    // Update Launch Parameters
    m_launch_params.viewport_size = stage_vec2i(m_window->getWidth(), m_window->getHeight());
    m_launch_params.scene_scale = m_scene->getSceneScale();
    m_launch_params.aspect_ratio = (float)m_window->getWidth() / m_window->getHeight();
    m_launch_params.camera.eye.x = eye.x;
    m_launch_params.camera.eye.y = eye.y;
    m_launch_params.camera.eye.z = eye.z;
    m_launch_params.camera.dir.x = dir.x;
    m_launch_params.camera.dir.y = dir.y;
    m_launch_params.camera.dir.z = dir.z;
    m_launch_params.camera.up.x = up.x;
    m_launch_params.camera.up.y = up.y;
    m_launch_params.camera.up.z = up.z;

    m_pixel_data_buffer.resize(m_launch_params.viewport_size.x * m_launch_params.viewport_size.y * sizeof(stage_vec4f));
    m_launch_params.framebuffer_rgba = (stage_vec4f*)m_pixel_data_buffer.device_ptr();

    // Upload Launch Parameters
    m_launch_params_buffer.alloc_and_upload(&m_launch_params, 1);

    // Launch Optix
    OPTIX_CHECK(optixLaunch(m_optix_pipeline,
                            /*stream=*/ 0,
                            m_launch_params_buffer.device_ptr(),
                            m_launch_params_buffer.sizeInBytes(),
                            &m_optix_sbt,
                            m_launch_params.viewport_size.x,
                            m_launch_params.viewport_size.y,
                            1
                ));
    CUDA_SYNC_CHECK();

    // Copy Results to Framebuffer
    draw();
    glfwSwapBuffers(m_window->getGlfwWindow());

    auto frame_time_mus = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - t_start);
    render_stats.frame_time_ms = frame_time_mus.count() * 0.001f;
}

void
OptiXRenderer::draw() {
    m_pixel_data.resize(m_launch_params.viewport_size.x * m_launch_params.viewport_size.y);
    m_pixel_data_buffer.download(m_pixel_data.data(), m_pixel_data.size());

    glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
    GLenum tex_format = GL_RGBA;
    GLenum tex_type = GL_FLOAT;
    glTexImage2D(GL_TEXTURE_2D, 0, tex_format, m_launch_params.viewport_size.x, m_launch_params.viewport_size.y, 0, GL_RGBA,
            tex_type, (float*)m_pixel_data.data());

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, m_framebuffer_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glDisable(GL_DEPTH_TEST);

    glViewport(0, 0, m_launch_params.viewport_size.x, m_launch_params.viewport_size.y);
    glClear(GL_COLOR_BUFFER_BIT);

    glBegin(GL_QUADS);
    {
        glTexCoord2f(0.f, 0.f);
        glVertex2f(1.f, -1.f);

        glTexCoord2f(0.f, 1.f);
        glVertex2f(1.f, 1.f);

        glTexCoord2f(1.f, 1.f);
        glVertex2f(-1.f, 1.f);

        glTexCoord2f(1.f, 0.f);
        glVertex2f(-1.f, -1.f);
    }
    glEnd();
}

}
