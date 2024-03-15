#pragma once

#include <directx/d3dx12.h>
#include <dxgi1_4.h>
#include <winrt/base.h>

#include "DescriptorHeap.h"

namespace Goonya {
namespace Graphics {
namespace Detail{

#define BS_RESOURCE_HEAP_SIZE 1024 * 1024

struct Devices{
    winrt::com_ptr<IDXGIFactory4> dxgi_factory;
    winrt::com_ptr<ID3D12Device4> p_device;

    winrt::com_ptr<ID3D12Heap> p_static_heap;
    winrt::com_ptr<ID3D12Heap> p_dynamic_heap;

    DescriptorHeap rtv_srv_heap; // 除了sampler以外的描述符堆

    winrt::com_ptr<ID3D12CommandQueue> p_command_queue;
    winrt::com_ptr<ID3D12Fence1> p_fence;

    winrt::com_ptr<IDXGISwapChain1> p_swap_chain;
    
    void init();
    void drop();
};

extern Devices devices;
}}}