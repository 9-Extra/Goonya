#include "graphics_device.h"

#include "../display/display.h"
#include "runtime/log/Log.h"
#include <dxgi.h>
#include <dxgi1_2.h>
#include <winrt/base.h>
#include <windows.h>
#include <runtime/GoonyaException.h>


namespace Goonya {

namespace Display {
    extern HWND hwnd;
}

namespace Graphics {

namespace Detail {

Devices devices;

void Devices::init() {
#ifndef NDEBUG
    // Enable the D3D12 debug layer.
    {
        winrt::com_ptr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
            debugController->EnableDebugLayer();
        }
    }
#endif

    CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory));

    winrt::com_ptr<IDXGIAdapter1> hardwareAdapter;
    dxgi_factory->EnumAdapters1(0, hardwareAdapter.put());
    DXGI_ADAPTER_DESC adapter_desc;
    hardwareAdapter->GetDesc(&adapter_desc);
    LOG_DEBUG(L"{}", adapter_desc.Description);

    D3D12CreateDevice(hardwareAdapter.get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(p_device.put()));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    p_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(p_command_queue.put()));

    // 创建堆
    {
        CD3DX12_HEAP_DESC static_heap_desc(BS_RESOURCE_HEAP_SIZE, D3D12_HEAP_TYPE_DEFAULT);
        p_device->CreateHeap(&static_heap_desc, IID_PPV_ARGS(p_static_heap.put()));

        CD3DX12_HEAP_DESC dynamic_heap_desc(BS_RESOURCE_HEAP_SIZE, D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD);
        p_device->CreateHeap(&dynamic_heap_desc, IID_PPV_ARGS(p_static_heap.put()));
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1000u;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        rtv_srv_heap.init(p_device.get(), &srvHeapDesc);
    }

    // p_device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
    //                              D3D12_COMMAND_LIST_FLAG_NONE,
    //                              IID_PPV_ARGS(p_command_list.put()));

    p_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(p_fence.put()));
    {
        if (Display::hwnd == nullptr) {
            throw std::runtime_error("必须先创建窗口");
        }
        auto [width, height] = ::Goonya::Display::get_size();
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
            .Width = width,
            .Height = height,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .Stereo = FALSE,
            .SampleDesc = {.Count = 1, .Quality = 0},
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_NONE,
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
            .AlphaMode = DXGI_ALPHA_MODE_IGNORE,
            .Flags = 0,
        };
        if (FAILED(dxgi_factory->CreateSwapChainForHwnd(p_command_queue.get(),Display::hwnd, &swapChainDesc,NULL, NULL, p_swap_chain.put()))){
            throw RuntimeError("创建交换链失败");
        }
    }
}

void Devices::drop() {
    p_fence.put_void();
    p_command_queue.put_void();
    rtv_srv_heap.drop();
    p_dynamic_heap.put_void();
    p_static_heap.put_void();
    p_swap_chain.put_void();

    p_device.put_void();
    dxgi_factory.put_void();
}

}
}
}