#pragma once
#include"PancyDx12Basic.h"
#include"PancyThreadBasic.h"
#include<DirectXTex.h>
#include<LoaderHelpers.h>
#include<DDSTextureLoader.h>
#include<WICTextureLoader.h>
//����������
enum PancyDescriptorType
{
	DescriptorTypeShaderResourceView = 0,
	DescriptorTypeConstantBufferView,
	DescriptorTypeUnorderedAccessView,
	DescriptorTypeRenderTargetView,
	DescriptorTypeDepthStencilView,
};
struct BasicDescriptorDesc
{
	//������������
	PancyDescriptorType basic_descriptor_type;
	//�洢���е���������ʽ
	pancy_resource_size offset = 0;
	pancy_resource_size block_size = 0;
	D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc = {};
	D3D12_RENDER_TARGET_VIEW_DESC render_target_view_desc = {};
	D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
};
//��Դ����״̬
enum PancyResourceLoadState
{
	RESOURCE_LOAD_FAILED = 0,
	RESOURCE_LOAD_CPU_FINISH,
	RESOURCE_LOAD_GPU_LOADING,
	RESOURCE_LOAD_GPU_FINISH
};
//GPU��Դ��
class ResourceBlockGpu
{
	bool if_start_copying_gpu;
	PancyFenceIdGPU  wait_fence;               //��ǰ��Դ�ļ��صȴ��ź���(�������Դ���Դ)
	PancyResourceLoadState now_res_load_state; //��ǰ��Դ�ļ���״̬
	pancy_resource_size memory_size;           //�洢��Ĵ�С
	ComPtr<ID3D12Resource> resource_data;      //�洢�������
	D3D12_HEAP_TYPE resource_usage;
	UINT8* map_pointer;
	D3D12_RESOURCE_STATES now_subresource_state;//��ǰ��Դ��ʹ�ø�ʽ
public:
	ResourceBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in,
		const D3D12_HEAP_TYPE &resource_usage_in,
		const D3D12_RESOURCE_STATES &resource_state
	);
	~ResourceBlockGpu();
	inline ID3D12Resource* GetResource() const
	{
		return resource_data.Get();
	}
	inline uint64_t GetSize() const
	{
		return memory_size;
	}
	//������Դ���ʸ�ʽ
	PancystarEngine::EngineFailReason BuildCommonDescriptorView(
		const BasicDescriptorDesc &descriptor_desc,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor
	);
	PancystarEngine::EngineFailReason BuildVertexBufferView(
		const pancy_resource_size &offset,
		const pancy_resource_size &block_size,
		UINT StrideInBytes,
		D3D12_VERTEX_BUFFER_VIEW &VBV_out
	);
	PancystarEngine::EngineFailReason BuildIndexBufferView(
		const pancy_resource_size &offset,
		const pancy_resource_size &block_size,
		DXGI_FORMAT StrideInBytes,
		D3D12_INDEX_BUFFER_VIEW &IBV_out
	);
	//�鿴��ǰ��Դ�ļ���״̬
	PancyResourceLoadState GetResourceLoadingState();
	//�鿴��ǰ��Դ��ʹ�ø�ʽ
	D3D12_RESOURCE_STATES GetResourceState()
	{
		return now_subresource_state;
	}
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const pancy_resource_size &pointer_offset,
		const void* copy_data,
		const pancy_resource_size &data_size
	);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const pancy_resource_size &pointer_offset,
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason CopyFromDynamicBufferToGpu(
		PancyRenderCommandList *commandlist,
		ResourceBlockGpu &dynamic_buffer,
		const pancy_resource_size &src_offset,
		const pancy_resource_size &dst_offset,
		const pancy_resource_size &data_size
	);
	PancystarEngine::EngineFailReason CopyFromDynamicBufferToGpu(
		PancyRenderCommandList *commandlist,
		ResourceBlockGpu &dynamic_buffer,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		const pancy_object_id &Layout_num
	);
	PancystarEngine::EngineFailReason SetResourceCopyBrokenFence(const PancyFenceIdGPU &broken_fence_id);
	PancystarEngine::EngineFailReason GetCpuMapPointer(UINT8** map_pointer_out)
	{
		if (now_res_load_state == RESOURCE_LOAD_FAILED)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "resource load failed, could not copy data to memory");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
			return error_message;
		}
		if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get CPU Pointer of memory block gpu", error_message);
			map_pointer_out = NULL;
			return error_message;
		}
		*map_pointer_out = map_pointer;
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason ReadFromBufferToCpu(
		const pancy_resource_size &pointer_offset,
		void* copy_data,
		const pancy_resource_size data_size
	);
private:
	void BuildConstantBufferView(
		const pancy_resource_size &offset,
		const pancy_resource_size &block_size,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor
	);
	void BuildShaderResourceView(
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	void BuildRenderTargetView(
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_RENDER_TARGET_VIEW_DESC  &RTV_desc
	);
	void BuildUnorderedAccessView(
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC  &UAV_desc
	);
	void BuildDepthStencilView(
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_DEPTH_STENCIL_VIEW_DESC  &DSV_desc
	);
	PancystarEngine::EngineFailReason ResourceBarrier(
		PancyRenderCommandList *commandlist,
		ID3D12Resource *src_submemory,
		const D3D12_RESOURCE_STATES &last_state,
		const D3D12_RESOURCE_STATES &now_state
	);
};


