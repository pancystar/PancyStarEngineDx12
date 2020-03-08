#include"PancyBufferDx12.h"
using namespace PancystarEngine;
//基础缓冲区
PancyBasicBuffer::PancyBasicBuffer(const bool &if_could_reload) :PancyBasicVirtualResource(if_could_reload)
{
}
void PancyBasicBuffer::BuildJsonReflect(PancyJsonReflect **pointer_data)
{
	*pointer_data = new CommonBufferJsonReflect();
}
PancystarEngine::EngineFailReason PancyBasicBuffer::InitResource()
{
	PancystarEngine::EngineFailReason check_error;
	ComPtr<ID3D12Resource> resource_data;
	PancyCommonBufferDesc resource_desc;
	//将资源的格式信息从反射类内拷贝出来
	check_error = resource_desc_value->CopyMemberData(&resource_desc, typeid(resource_desc).name(), sizeof(resource_desc));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//在d3d层级上创建一个单独堆的buffer资源
	D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	switch (resource_desc.buffer_type)
	{
	case Buffer_ShaderResource_static:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	case Buffer_ShaderResource_dynamic:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		break;
	}
	case Buffer_Constant:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		break;
	}
	case Buffer_Vertex:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	case Buffer_Index:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	case Buffer_UnorderedAccess_static:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
		break;
	}
	default:
		break;
	}
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(heap_type),
		D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS,
		&resource_desc.buffer_res_desc,
		D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&resource_data)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Create commit resource error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicBuffer::InitResource", error_message);
		return error_message;
	}
	//计算缓冲区的大小，创建资源块
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&resource_desc.buffer_res_desc, 0, 1, 0, nullptr, nullptr, nullptr, &subresources_size);
	if (resource_desc.buffer_res_desc.Width != subresources_size)
	{
		PancystarEngine::EngineFailReason error_message(0, "buffer resource size dismatch, maybe it's not a buffer resource");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicBuffer::InitResource", error_message);
		return error_message;
	}
	buffer_data = new ResourceBlockGpu(subresources_size, resource_data, heap_type, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	if (heap_type == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		check_error = buffer_data->GetCpuMapPointer(&map_pointer);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}

	//如果需要拷贝数据，将数据拷贝到buffer中
	if (resource_desc.buffer_type == Buffer_Index || resource_desc.buffer_type == Buffer_Vertex || resource_desc.buffer_type == Buffer_ShaderResource_static)
	{
		char* buffer_memory = NULL;
		//todo:从文件中读取buffer
		//获取用于拷贝的commond list
		PancyRenderCommandList *copy_render_list;
		PancyThreadIdGPU copy_render_list_ID;
		check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//拷贝资源数据
		check_error = PancyDynamicRingBuffer::GetInstance()->CopyDataToGpu(copy_render_list, buffer_memory, subresources_size, *buffer_data);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		copy_render_list->UnlockPrepare();
		//提交渲染命令
		ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SubmitRenderlist(1, &copy_render_list_ID);
		//分配等待眼位
		PancyFenceIdGPU WaitFence;
		ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(WaitFence);
		check_error = buffer_data->SetResourceCopyBrokenFence(WaitFence);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
}
bool PancyBasicBuffer::CheckIfResourceLoadFinish()
{
	PancyResourceLoadState now_load_state = buffer_data->GetResourceLoadingState();
	if (now_load_state == PancyResourceLoadState::RESOURCE_LOAD_CPU_FINISH || now_load_state == PancyResourceLoadState::RESOURCE_LOAD_GPU_FINISH)
	{
		return true;
	}
	return false;
}
PancyBasicBuffer::~PancyBasicBuffer() 
{
	if (buffer_data != NULL) 
	{
		delete buffer_data;
	}
}
ResourceBlockGpu * PancystarEngine::GetBufferResourceData(VirtualResourcePointer & virtual_pointer, PancystarEngine::EngineFailReason & check_error)
{
	check_error = PancystarEngine::succeed;
	auto now_buffer_resource_value = virtual_pointer.GetResourceData();
	if (now_buffer_resource_value->GetResourceTypeName != typeid(PancyBasicBuffer).name())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the vertex resource is not a buffer");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("GetBufferResourceData", error_message);
		check_error = error_message;
	}
	const PancyBasicBuffer* buffer_real_pointer = dynamic_cast<const PancyBasicBuffer*>(now_buffer_resource_value);
	auto gpu_buffer_data = buffer_real_pointer->GetGpuResourceData();
	if (gpu_buffer_data != NULL)
	{
		return gpu_buffer_data;
	}
	return NULL;
}
PancystarEngine::EngineFailReason PancystarEngine::BuildBufferResource(
	const std::string &name_resource_in,
	PancyCommonBufferDesc &resource_data,
	VirtualResourcePointer &id_need,
	bool if_allow_repeat
)
{
	auto check_error = PancyGlobelResourceControl::GetInstance()->LoadResource<PancyBasicBuffer>(
		name_resource_in,
		&resource_data,
		typeid(PancyCommonBufferDesc).name(),
		sizeof(PancyCommonBufferDesc),
		id_need,
		if_allow_repeat
		);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}


