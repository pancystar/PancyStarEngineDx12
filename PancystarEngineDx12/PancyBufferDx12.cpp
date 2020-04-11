#include"PancyBufferDx12.h"
using namespace PancystarEngine;
CommonBufferJsonReflect::CommonBufferJsonReflect()
{

}
void CommonBufferJsonReflect::InitBasicVariable()
{
	Init_Json_Data_Vatriable(reflect_data.buffer_type);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Dimension);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Alignment);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Width);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Height);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.DepthOrArraySize);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.MipLevels);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Format);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.SampleDesc.Count);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.SampleDesc.Quality);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Layout);
	Init_Json_Data_Vatriable(reflect_data.buffer_res_desc.Flags);
	Init_Json_Data_Vatriable(reflect_data.buffer_data_file);
}
//����������
PancyBasicBuffer::PancyBasicBuffer(const bool &if_could_reload) :PancyCommonVirtualResource<PancyCommonBufferDesc>(if_could_reload)
{
}
PancystarEngine::EngineFailReason PancyBasicBuffer::WriteDataToBuffer(void* cpu_data_pointer, const pancy_resource_size &data_size)
{
	auto check_error = CopyCpuDataToBufferGpu(cpu_data_pointer, data_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBuffer::CopyCpuDataToBufferGpu(void* cpu_data_pointer, const pancy_resource_size &data_size)
{
	if (data_size > subresources_size) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not copy data to GPU, size too large than buffer");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicBuffer::CopyCpuDataToBufferGpu", error_message);
		return error_message;
	}
	//��ȡ���ڿ�����commond list
	PancyRenderCommandList *copy_render_list;
	PancyThreadIdGPU copy_render_list_ID;
	auto check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//������Դ����
	check_error = PancyDynamicRingBuffer::GetInstance()->CopyDataToGpu(copy_render_list, cpu_data_pointer, data_size, *buffer_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	copy_render_list->UnlockPrepare();
	//�ύ��Ⱦ����
	ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SubmitRenderlist(1, &copy_render_list_ID);
	//����ȴ���λ
	PancyFenceIdGPU WaitFence;
	ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(WaitFence);
	check_error = buffer_data->SetResourceCopyBrokenFence(WaitFence);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicBuffer::LoadResoureDataByDesc(const PancyCommonBufferDesc &resource_desc)
{
	PancystarEngine::EngineFailReason check_error;
	ComPtr<ID3D12Resource> resource_data;
	//��d3d�㼶�ϴ���һ�������ѵ�buffer��Դ
	D3D12_HEAP_TYPE heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES resource_build_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON;
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
		resource_build_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;
		break;
	}
	case Buffer_Constant:
	{
		heap_type = D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD;
		resource_build_state = D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_GENERIC_READ;
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
		D3D12_HEAP_FLAG_NONE,
		&resource_desc.buffer_res_desc,
		resource_build_state,
		nullptr,
		IID_PPV_ARGS(&resource_data)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Create commit resource error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicBuffer::InitResource", error_message);
		return error_message;
	}
	//���㻺�����Ĵ�С��������Դ��
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetCopyableFootprints(&resource_desc.buffer_res_desc, 0, 1, 0, nullptr, nullptr, nullptr, &subresources_size);
	if (resource_desc.buffer_res_desc.Width != subresources_size)
	{
		PancystarEngine::EngineFailReason error_message(0, "buffer resource size dismatch, maybe it's not a buffer resource");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicBuffer::InitResource", error_message);
		return error_message;
	}
	buffer_data = new ResourceBlockGpu(subresources_size, resource_data, heap_type, resource_build_state);
	if (heap_type == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		check_error = buffer_data->GetCpuMapPointer(&map_pointer);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}

	//�����Ҫ�������ݣ������ݿ�����buffer��
	if (resource_desc.buffer_type == Buffer_Index || resource_desc.buffer_type == Buffer_Vertex || resource_desc.buffer_type == Buffer_ShaderResource_static)
	{
		char* buffer_memory = NULL;
		//todo:���ļ��ж�ȡbuffer
		if (resource_desc.buffer_data_file != "")
		{
			/*
			check_error = CopyCpuDataToBufferGpu(cpu_data_pointer, data_size);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			*/
		}

	}
	return PancystarEngine::succeed;
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
	if (now_buffer_resource_value->GetResourceTypeName() != typeid(PancyBasicBuffer).name())
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
		typeid(PancyCommonBufferDesc*).name(),
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
void PancystarEngine::InitBufferJsonReflect()
{
	JSON_REFLECT_INIT_ENUM(Buffer_ShaderResource_static);
	JSON_REFLECT_INIT_ENUM(Buffer_ShaderResource_dynamic);
	JSON_REFLECT_INIT_ENUM(Buffer_Constant);
	JSON_REFLECT_INIT_ENUM(Buffer_Vertex);
	JSON_REFLECT_INIT_ENUM(Buffer_Index);
	JSON_REFLECT_INIT_ENUM(Buffer_UnorderedAccess_static);

	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_DIMENSION_UNKNOWN);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_DIMENSION_BUFFER);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_DIMENSION_TEXTURE1D);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_DIMENSION_TEXTURE2D);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_DIMENSION_TEXTURE3D);

	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_UNKNOWN);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32A32_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32A32_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32A32_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32A32_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32B32_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16B16A16_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16B16A16_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16B16A16_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16B16A16_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16B16A16_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16B16A16_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G32_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32G8X24_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_X32_TYPELESS_G8X24_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R10G10B10A2_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R10G10B10A2_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R10G10B10A2_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R11G11B10_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8B8A8_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8B8A8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8B8A8_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8B8A8_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8B8A8_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16G16_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_D32_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R32_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R24G8_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_D24_UNORM_S8_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_X24_TYPELESS_G8_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16_FLOAT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_D16_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R16_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8_UINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8_SINT);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_A8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R1_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R9G9B9E5_SHAREDEXP);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R8G8_B8G8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_G8R8_G8B8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC1_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC1_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC1_UNORM_SRGB);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC2_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC2_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC2_UNORM_SRGB);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC3_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC3_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC3_UNORM_SRGB);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC4_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC4_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC4_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC5_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC5_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC5_SNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B5G6R5_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B5G5R5A1_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B8G8R8A8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B8G8R8X8_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B8G8R8A8_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B8G8R8X8_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B8G8R8X8_UNORM_SRGB);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC6H_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC6H_UF16);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC6H_SF16);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC7_TYPELESS);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC7_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_BC7_UNORM_SRGB);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_AYUV);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_Y410);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_Y416);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_NV12);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_P010);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_P016);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_420_OPAQUE);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_YUY2);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_Y210);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_Y216);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_NV11);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_AI44);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_IA44);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_P8);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_A8P8);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_B4G4R4A4_UNORM);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_P208);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_V208);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_V408);
	JSON_REFLECT_INIT_ENUM(DXGI_FORMAT_FORCE_UINT);

	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_LAYOUT_UNKNOWN);
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_LAYOUT_ROW_MAJOR);
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE);
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_LAYOUT_64KB_STANDARD_SWIZZLE);

	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS);
	JSON_REFLECT_INIT_ENUM(D3D12_RESOURCE_FLAG_VIDEO_DECODE_REFERENCE_ONLY);

	InitJsonReflectParseClass(PancyCommonBufferDesc, CommonBufferJsonReflect);
}

