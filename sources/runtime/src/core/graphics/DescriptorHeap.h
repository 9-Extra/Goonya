#pragma once

#include <directx/d3dx12.h>
#include <winrt/base.h>

namespace Goonya {
namespace Graphics {
namespace Detail{
class DescriptorHeap {
	winrt::com_ptr<ID3D12DescriptorHeap> p_heap;
	unsigned int descriptor_size;
	unsigned int capability;

public:
	void init(ID3D12Device* device, const D3D12_DESCRIPTOR_HEAP_DESC* desc) {
		capability = desc->NumDescriptors;
		device->CreateDescriptorHeap(desc, IID_PPV_ARGS(p_heap.put()));
		descriptor_size = device->GetDescriptorHandleIncrementSize(desc->Type);
	}

	ID3D12DescriptorHeap* get() {
		return p_heap.get();
	}

	D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_slot_handle(unsigned int slot=0u) {
		assert(slot < capability);
		return CD3DX12_CPU_DESCRIPTOR_HANDLE(p_heap->GetCPUDescriptorHandleForHeapStart(), slot ,descriptor_size);
	}

	D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_slot_handle(unsigned int slot=0u) {
		assert(slot < capability);
		return CD3DX12_GPU_DESCRIPTOR_HANDLE(p_heap->GetGPUDescriptorHandleForHeapStart(), slot, descriptor_size);
    }

    void drop(){
        p_heap.put_void();
    }

};
}
}
}
