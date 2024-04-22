#include "common/types.cuh"
#include "common/helper_math.cuh"

static __forceinline__ __device__
void *unpackPointer( uint32_t i0, uint32_t i1 )
{
const uint64_t uptr = static_cast<uint64_t>( i0 ) << 32 | i1;
void*           ptr = reinterpret_cast<void*>( uptr ); 
return ptr;
}

static __forceinline__ __device__
void  packPointer( void* ptr, uint32_t& i0, uint32_t& i1 )
{
const uint64_t uptr = reinterpret_cast<uint64_t>( ptr );
i0 = uptr >> 32;
i1 = uptr & 0x00000000ffffffff;
}

template<typename T>
static __forceinline__ __device__ T *getPRD()
{ 
const uint32_t u0 = optixGetPayload_0();
const uint32_t u1 = optixGetPayload_1();
return reinterpret_cast<T*>( unpackPointer( u0, u1 ) );
}

extern "C" {

__constant__ fart::LaunchParams launch_params;


__global__ void __miss__pathtrace() {
    float3 &prd = *(float3*)getPRD<float3>();
    // set to constant white as background color
    prd = make_float3(1.f, 1.f, 1.f);
}

__global__ void __closesthit__pathtrace() {
    float3 &prd = *(float3*)getPRD<float3>();
    // set to constant white as background color
    prd = make_float3(optixGetTriangleBarycentrics().x, optixGetTriangleBarycentrics().y, 0.f);
}

__global__ void __raygen__pathtrace() {
    const int ix = optixGetLaunchIndex().x;
    const int iy = optixGetLaunchIndex().y;
    const int pid = iy * launch_params.viewport_size.x + ix;
    launch_params.framebuffer_rgba[pid].x = (float) ix / launch_params.viewport_size.x;
    launch_params.framebuffer_rgba[pid].y = (float) iy / launch_params.viewport_size.y;
    launch_params.framebuffer_rgba[pid].z = 1.f;
    launch_params.framebuffer_rgba[pid].a = 1.f;

    float3 pixelColorPRD;

    // the values we store the PRD pointer in:
    uint32_t u0, u1;
    packPointer( &pixelColorPRD, u0, u1 );

    float2 screen = make_float2(((float)ix+0.5f)/launch_params.viewport_size.x, 
                                ((float)iy+0.5f)/launch_params.viewport_size.y);

    // generate ray direction
    float3 rayDir = make_float3(launch_params.camera.dir.x, launch_params.camera.dir.y, launch_params.camera.dir.z);
    float3 camUp = make_float3(launch_params.camera.up.x, launch_params.camera.up.y, launch_params.camera.up.z);
    float3 camRight = cross(camUp, rayDir);

    rayDir = normalize(rayDir
                        + (screen.x - 0.5f) * camRight * launch_params.aspect_ratio
                        + (screen.y - 0.5f) * camUp);

    float3 rayOrg;
    rayOrg.x = launch_params.camera.eye.x;
    rayOrg.y = launch_params.camera.eye.y;
    rayOrg.z = launch_params.camera.eye.z;

    optixTrace(launch_params.traversable,
               rayOrg,
               rayDir,
               0.f,    // tmin
               1e20f,  // tmax
               0.0f,   // rayTime
               OptixVisibilityMask( 255 ),
               OPTIX_RAY_FLAG_DISABLE_ANYHIT,//OPTIX_RAY_FLAG_NONE,
               0,             // SBT offset
               1,             // SBT stride
               0,             // missSBTIndex 
               u0, u1 );
            
    launch_params.framebuffer_rgba[pid].x = pixelColorPRD.x;
    launch_params.framebuffer_rgba[pid].y = pixelColorPRD.y;
    launch_params.framebuffer_rgba[pid].z = pixelColorPRD.z;
    launch_params.framebuffer_rgba[pid].a = 1.f;
}

}
