#pragma once
#include <vector>
#include <webgpu/webgpu.h>

namespace fart 
{

template<typename T>
struct Buffer {
    public:

        Buffer(WGPUDevice device, size_t n_elements, WGPUBufferUsage usage) {
            m_usage = usage;
            resize(device, n_elements);
        }
        ~Buffer() {
            if (m_buffer) {
                wgpuBufferRelease(m_buffer);
            }
        }

        void resize(WGPUDevice device, size_t n_elements) {
                if (n_elements == m_n_elements) {
                    return;
                }

                if (m_buffer) {
                    wgpuBufferRelease(m_buffer);
                }

                m_n_elements = n_elements;
                m_size = m_n_elements * sizeof(T);

                WGPUBufferDescriptor descriptor = {};
                descriptor.mappedAtCreation = false;
                descriptor.size = m_size;
                descriptor.usage = m_usage;

                m_buffer = wgpuDeviceCreateBuffer(device, &descriptor);
        }

        void setData(WGPUDevice device, WGPUQueue queue, std::vector<T> data) {
            resize(device, data.size());
            wgpuQueueWriteBuffer(queue, m_buffer, 0, data.data(), data.size() * sizeof(T));
        }
        void setData(WGPUDevice device, WGPUQueue queue, T* data, size_t n_elements) {
            resize(device, n_elements);
            wgpuQueueWriteBuffer(queue, m_buffer, 0, data, n_elements * sizeof(T));
        }

        WGPUBuffer getBuffer() const    { return m_buffer; }

        size_t getNElements()  const    { return m_n_elements; }
        size_t getSize()       const    { return m_size; }
    
    private:
        WGPUBuffer m_buffer     { nullptr };
        WGPUBufferUsage m_usage;

        size_t m_n_elements     { 0 };
        size_t m_size           { 0 };
};

}