#pragma once

#include <vector>
#include <cassert>
#include "optixdefs.h"

namespace fart {

/*
 * Adapted from: https://github.com/ingowald/optix7course/blob/master/example02_pipelineAndRayGen/CUDABuffer.h 
 *
 */
struct CudaBuffer {

    void free() {
        if (m_device_ptr == nullptr) return;
        CUDA_CHECK(cudaFree(m_device_ptr));
        m_device_ptr = nullptr;
        m_size_in_bytes = 0;
    }

    void resize(size_t size) {
        if (size == m_size_in_bytes) return;
        if (m_device_ptr != nullptr)
            free();
        alloc(size);
    }

    void alloc(size_t size) {
        assert(m_device_ptr == nullptr);
        m_size_in_bytes = size;
        CUDA_CHECK(cudaMalloc(&m_device_ptr, size));
    }


    template<typename T>
    void alloc_and_upload(const std::vector<T>& data) {
        resize(data.size() * sizeof(T));
        CUDA_CHECK(cudaMemcpy(m_device_ptr, data.data(), m_size_in_bytes, cudaMemcpyHostToDevice));
    }

    template<typename T>
    void alloc_and_upload(const T* data, const size_t size) {
        resize(size * sizeof(T));
        CUDA_CHECK(cudaMemcpy(m_device_ptr, data, m_size_in_bytes, cudaMemcpyHostToDevice));
    }

    template<typename T>
    void upload(const std::vector<T>& data) {
        assert(m_device_ptr != nullptr);
        assert(data.size() * sizeof(T) == m_size_in_bytes);
        CUDA_CHECK(cudaMemcpy(m_device_ptr, data.data(), m_size_in_bytes, cudaMemcpyHostToDevice));
    }

    template<typename T>
    void download(T *data, size_t count)
    {
      assert(m_device_ptr != nullptr);
      assert(m_size_in_bytes == count * sizeof(T));
      CUDA_CHECK(cudaMemcpy((void *)data, m_device_ptr, count * sizeof(T), cudaMemcpyDeviceToHost));
    }

    template<typename T>
    size_t size() {
        return m_size_in_bytes / sizeof(T);
    }

    size_t sizeInBytes() {
        return m_size_in_bytes;
    }

    inline CUdeviceptr device_ptr() const { return (CUdeviceptr)m_device_ptr; }

private:
    size_t m_size_in_bytes { 0 };
    void* m_device_ptr { nullptr };

};

}
