#pragma once

#include <directx/d3dx12.h>
#include <winrt/base.h>

namespace Goonya {
namespace Graphics {
namespace Detail{
template<typename T>
class ConstantBuffer {
	winrt::com_ptr<ID3D12Resource> p_buffer;

public:
	ConstantBuffer(winrt::com_ptr<ID3D12Resource> p_buffer, D3D12_GPU_VIRTUAL_ADDRESS gpu_address, T* data)
		:p_buffer(p_buffer), gpu_address(gpu_address), data(data)
	{}

	D3D12_GPU_VIRTUAL_ADDRESS gpu_address;
	T* data;//cpu address
};
}
}
}