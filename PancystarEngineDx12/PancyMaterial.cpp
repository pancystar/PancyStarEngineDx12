#include"PancyMaterial.h"
using namespace PancystarEngine;
PancyJsonMaterialBufferDescriptor::PancyJsonMaterialBufferDescriptor() : PancyJsonReflectTemplate<PancyBufferDescriptorDesc>()
{
}
void PancyJsonMaterialBufferDescriptor::InitBasicVariable()
{
	Init_Json_Data_Vatriable(reflect_data.Format);
	Init_Json_Data_Vatriable(reflect_data.Shader4ComponentMapping);
	Init_Json_Data_Vatriable(reflect_data.FirstElement);
	Init_Json_Data_Vatriable(reflect_data.NumElements);
	Init_Json_Data_Vatriable(reflect_data.StructureByteStride);
	Init_Json_Data_Vatriable(reflect_data.Flags);
}
PancyJsonMaterialBufferShaderResource::PancyJsonMaterialBufferShaderResource() :PancyJsonReflectTemplate<PancyMaterialShaderResourceDataBuffer>()
{
}
void PancyJsonMaterialBufferShaderResource::InitBasicVariable()
{
	Init_Json_Data_Vatriable(reflect_data.shader_resource_type);
	Init_Json_Data_Vatriable(reflect_data.shader_resource_slot_name);
	Init_Json_Data_Vatriable(reflect_data.shader_resource_path);
	Init_Json_Data_Vatriable(reflect_data.buffer_descriptor_desc);
}
PancyJsonMaterialTexureDescriptor::PancyJsonMaterialTexureDescriptor() : PancyJsonReflectTemplate<PancyTextureDescriptorDesc>()
{

}
void PancyJsonMaterialTexureDescriptor::InitBasicVariable()
{
	//��������
	Init_Json_Data_Vatriable(reflect_data.Format);
	Init_Json_Data_Vatriable(reflect_data.ViewDimension);
	Init_Json_Data_Vatriable(reflect_data.Shader4ComponentMapping);
	//��������
	Init_Json_Data_Vatriable(reflect_data.MostDetailedMip);
	Init_Json_Data_Vatriable(reflect_data.MipLevels);
	Init_Json_Data_Vatriable(reflect_data.PlaneSlice);
	Init_Json_Data_Vatriable(reflect_data.First2DArrayFace);
	Init_Json_Data_Vatriable(reflect_data.NumCubes);
	Init_Json_Data_Vatriable(reflect_data.FirstArraySlice);
	Init_Json_Data_Vatriable(reflect_data.ArraySize);
	Init_Json_Data_Vatriable(reflect_data.UnusedField_NothingToDefine);
	Init_Json_Data_Vatriable(reflect_data.ResourceMinLODClamp);
}
PancyJsonMaterialTexureShaderResource::PancyJsonMaterialTexureShaderResource() : PancyJsonReflectTemplate<PancyMaterialShaderResourceDataTexture>()
{
}
void PancyJsonMaterialTexureShaderResource::InitBasicVariable()
{
	Init_Json_Data_Vatriable(reflect_data.shader_resource_type);
	Init_Json_Data_Vatriable(reflect_data.shader_resource_slot_name);
	Init_Json_Data_Vatriable(reflect_data.shader_resource_path);
	Init_Json_Data_Vatriable(reflect_data.texture_descriptor_desc);
}
CommonMaterialDescJsonReflect::CommonMaterialDescJsonReflect()
{
}
void CommonMaterialDescJsonReflect::InitBasicVariable()
{
	Init_Json_Data_Vatriable(reflect_data.pipeline_state_name);
	Init_Json_Data_Vatriable(reflect_data.marterial_buffer_slot_data);
	Init_Json_Data_Vatriable(reflect_data.marterial_texture_slot_data);
}
PancyMaterialBasic::PancyMaterialBasic(const bool &if_could_reload_in) :PancyCommonVirtualResource<PancyCommonMaterialDesc>(if_could_reload_in)
{
}
PancystarEngine::EngineFailReason PancyMaterialBasic::BuildBufferDescriptorByDesc(
	const std::string &resource_file_name,
	const PancyBufferDescriptorDesc &descriptor_desc,
	std::vector<VirtualResourcePointer> &resource_pointer_array,
	std::vector<BasicDescriptorDesc> &resource_descriptor_descr_array
)
{
	PancystarEngine::EngineFailReason check_error;
	auto& new_res_data = ShaderResourceData.emplace_back();
	check_error = LoadBufferResourceFromFile(resource_file_name, new_res_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	resource_pointer_array.push_back(new_res_data);
	//��ȡbuffer��shader�󶨸�ʽ
	auto &now_buffer_shader_resource_desc = descriptor_desc;
	auto &descriptor_build_desc = resource_descriptor_descr_array.emplace_back();
	descriptor_build_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
	descriptor_build_desc.shader_resource_view_desc.Format = now_buffer_shader_resource_desc.Format;
	descriptor_build_desc.shader_resource_view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	descriptor_build_desc.shader_resource_view_desc.Shader4ComponentMapping = now_buffer_shader_resource_desc.Shader4ComponentMapping;
	descriptor_build_desc.shader_resource_view_desc.Buffer.FirstElement = now_buffer_shader_resource_desc.FirstElement;
	descriptor_build_desc.shader_resource_view_desc.Buffer.Flags = now_buffer_shader_resource_desc.Flags;
	descriptor_build_desc.shader_resource_view_desc.Buffer.NumElements = now_buffer_shader_resource_desc.NumElements;
	descriptor_build_desc.shader_resource_view_desc.Buffer.StructureByteStride = now_buffer_shader_resource_desc.StructureByteStride;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyMaterialBasic::BuildTextureDescriptorByDesc(
	const std::string &resource_file_name,
	const PancyTextureDescriptorDesc &descriptor_desc,
	std::vector<VirtualResourcePointer> &resource_pointer_array,
	std::vector<BasicDescriptorDesc> &resource_descriptor_descr_array
)
{
	PancystarEngine::EngineFailReason check_error;
	auto& new_res_data = ShaderResourceData.emplace_back();
	check_error = LoadDDSTextureResource(resource_file_name, new_res_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	resource_pointer_array.push_back(new_res_data);
	//��ȡbuffer��shader�󶨸�ʽ
	auto &now_texture_shader_resource_desc = descriptor_desc;
	auto &descriptor_build_desc = resource_descriptor_descr_array.emplace_back();
	descriptor_build_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
	descriptor_build_desc.shader_resource_view_desc.Format = now_texture_shader_resource_desc.Format;
	descriptor_build_desc.shader_resource_view_desc.ViewDimension = now_texture_shader_resource_desc.ViewDimension;
	descriptor_build_desc.shader_resource_view_desc.Shader4ComponentMapping = now_texture_shader_resource_desc.Shader4ComponentMapping;
	switch (descriptor_build_desc.shader_resource_view_desc.ViewDimension)
	{
	case D3D12_SRV_DIMENSION_TEXTURE1D:
	{
		descriptor_build_desc.shader_resource_view_desc.Texture1D.MipLevels = now_texture_shader_resource_desc.MipLevels;
		descriptor_build_desc.shader_resource_view_desc.Texture1D.MostDetailedMip = now_texture_shader_resource_desc.MostDetailedMip;
		descriptor_build_desc.shader_resource_view_desc.Texture1D.ResourceMinLODClamp = now_texture_shader_resource_desc.ResourceMinLODClamp;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
	{
		descriptor_build_desc.shader_resource_view_desc.Texture1DArray.ArraySize = now_texture_shader_resource_desc.ArraySize;
		descriptor_build_desc.shader_resource_view_desc.Texture1DArray.FirstArraySlice = now_texture_shader_resource_desc.FirstArraySlice;
		descriptor_build_desc.shader_resource_view_desc.Texture1DArray.MipLevels = now_texture_shader_resource_desc.MipLevels;
		descriptor_build_desc.shader_resource_view_desc.Texture1DArray.MostDetailedMip = now_texture_shader_resource_desc.MostDetailedMip;
		descriptor_build_desc.shader_resource_view_desc.Texture1DArray.ResourceMinLODClamp = now_texture_shader_resource_desc.ResourceMinLODClamp;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURE2D:
	{
		descriptor_build_desc.shader_resource_view_desc.Texture2D.MipLevels = now_texture_shader_resource_desc.MipLevels;
		descriptor_build_desc.shader_resource_view_desc.Texture2D.MostDetailedMip = now_texture_shader_resource_desc.MostDetailedMip;
		descriptor_build_desc.shader_resource_view_desc.Texture2D.PlaneSlice = now_texture_shader_resource_desc.PlaneSlice;
		descriptor_build_desc.shader_resource_view_desc.Texture2D.ResourceMinLODClamp = now_texture_shader_resource_desc.ResourceMinLODClamp;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
	{
		descriptor_build_desc.shader_resource_view_desc.Texture2DArray.ArraySize = now_texture_shader_resource_desc.ArraySize;
		descriptor_build_desc.shader_resource_view_desc.Texture2DArray.FirstArraySlice = now_texture_shader_resource_desc.FirstArraySlice;
		descriptor_build_desc.shader_resource_view_desc.Texture2DArray.MipLevels = now_texture_shader_resource_desc.MipLevels;
		descriptor_build_desc.shader_resource_view_desc.Texture2DArray.MostDetailedMip = now_texture_shader_resource_desc.MostDetailedMip;
		descriptor_build_desc.shader_resource_view_desc.Texture2DArray.PlaneSlice = now_texture_shader_resource_desc.PlaneSlice;
		descriptor_build_desc.shader_resource_view_desc.Texture2DArray.ResourceMinLODClamp = now_texture_shader_resource_desc.ResourceMinLODClamp;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURE2DMS:
	{
		descriptor_build_desc.shader_resource_view_desc.Texture2DMS.UnusedField_NothingToDefine = now_texture_shader_resource_desc.UnusedField_NothingToDefine;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
	{
		descriptor_build_desc.shader_resource_view_desc.Texture2DMSArray.ArraySize = now_texture_shader_resource_desc.ArraySize;
		descriptor_build_desc.shader_resource_view_desc.Texture2DMSArray.FirstArraySlice = now_texture_shader_resource_desc.FirstArraySlice;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURE3D:
	{
		descriptor_build_desc.shader_resource_view_desc.Texture3D.MipLevels = now_texture_shader_resource_desc.MipLevels;
		descriptor_build_desc.shader_resource_view_desc.Texture3D.MostDetailedMip = now_texture_shader_resource_desc.MostDetailedMip;
		descriptor_build_desc.shader_resource_view_desc.Texture3D.ResourceMinLODClamp = now_texture_shader_resource_desc.ResourceMinLODClamp;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURECUBE:
	{
		descriptor_build_desc.shader_resource_view_desc.TextureCube.MipLevels = now_texture_shader_resource_desc.MipLevels;
		descriptor_build_desc.shader_resource_view_desc.TextureCube.MostDetailedMip = now_texture_shader_resource_desc.MostDetailedMip;
		descriptor_build_desc.shader_resource_view_desc.TextureCube.ResourceMinLODClamp = now_texture_shader_resource_desc.ResourceMinLODClamp;
		break;
	}
	case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
	{
		descriptor_build_desc.shader_resource_view_desc.TextureCubeArray.First2DArrayFace = now_texture_shader_resource_desc.First2DArrayFace;
		descriptor_build_desc.shader_resource_view_desc.TextureCubeArray.MipLevels = now_texture_shader_resource_desc.MipLevels;
		descriptor_build_desc.shader_resource_view_desc.TextureCubeArray.MostDetailedMip = now_texture_shader_resource_desc.MostDetailedMip;
		descriptor_build_desc.shader_resource_view_desc.TextureCubeArray.NumCubes = now_texture_shader_resource_desc.NumCubes;
		descriptor_build_desc.shader_resource_view_desc.TextureCubeArray.ResourceMinLODClamp = now_texture_shader_resource_desc.ResourceMinLODClamp;
		break;
	}
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "texture type SRV Desc not support");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyMaterialBasic::BuildTexturerDescriptorByDesc", error_message);
		return error_message;
	}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyMaterialBasic::LoadResoureDataByDesc(const PancyCommonMaterialDesc &ResourceDescStruct)
{
	PancystarEngine::EngineFailReason check_error;
	//�������е�˽��buffer���ݣ�Ϊbuffer���ݿ��ٿռ�
	for (size_t now_res_index = 0; now_res_index < ResourceDescStruct.marterial_buffer_slot_data.size(); ++now_res_index)
	{
		std::vector<VirtualResourcePointer> all_resource_array;
		std::vector<BasicDescriptorDesc> all_descriptor_array;
		auto &now_buffer_desc = ResourceDescStruct.marterial_buffer_slot_data[now_res_index];
		//��ȡ��buffer����Դ
		if (ResourceDescStruct.marterial_buffer_slot_data[now_res_index].shader_resource_type == MaterialShaderResourceType::MaterialShaderResourceBufferBind)
		{
			//bind���͵���Դ��ֻȡ��һ����������������
			check_error = BuildBufferDescriptorByDesc(
				now_buffer_desc.shader_resource_path[0],
				now_buffer_desc.buffer_descriptor_desc[0],
				all_resource_array,
				all_descriptor_array
			);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//����������
			BindDescriptorPointer new_res_descriptor;
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonDescriptor(all_descriptor_array, all_resource_array, false, new_res_descriptor);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//�����������뵽map��
			bind_shader_resource.insert(std::pair<std::string, BindDescriptorPointer>(now_buffer_desc.shader_resource_slot_name, new_res_descriptor));
		}
		else if (ResourceDescStruct.marterial_buffer_slot_data[now_res_index].shader_resource_type == MaterialShaderResourceType::MaterialShaderResourceBufferBindLess)
		{
			std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> bindless_srv_array;
			//bindless���͵���Դ����Ҫÿһ�����ݾ���������������
			for (size_t buffer_bindless_id = 0; buffer_bindless_id < now_buffer_desc.shader_resource_path.size(); ++buffer_bindless_id)
			{
				check_error = BuildBufferDescriptorByDesc(
					now_buffer_desc.shader_resource_path[buffer_bindless_id],
					now_buffer_desc.buffer_descriptor_desc[buffer_bindless_id],
					all_resource_array,
					all_descriptor_array
				);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
			}
			//��srv������֯��������vector
			for (size_t bindless_descriptor_id = 0; bindless_descriptor_id < all_descriptor_array.size(); ++bindless_descriptor_id)
			{
				bindless_srv_array.push_back(all_descriptor_array[bindless_descriptor_id].shader_resource_view_desc);
			}
			//����������
			BindlessDescriptorPointer new_res_descriptor;
			pancy_object_id resource_size = static_cast<pancy_object_id>(all_resource_array.size());
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonBindlessShaderResourceView(bindless_srv_array, all_resource_array, resource_size, new_res_descriptor);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//�����������뵽map��
			bindless_shader_resource.insert(std::pair<std::string, BindlessDescriptorPointer>(now_buffer_desc.shader_resource_slot_name, new_res_descriptor));
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "buffer type SRV Desc not support");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyMaterialBasic::LoadResoureDataByDesc", error_message);
			return error_message;
		}
	}
	//�������е�˽��texture���ݣ�Ϊtexture���ݿ��ٿռ�
	for (size_t now_res_index = 0; now_res_index < ResourceDescStruct.marterial_texture_slot_data.size(); ++now_res_index)
	{
		std::vector<VirtualResourcePointer> all_resource_array;
		std::vector<BasicDescriptorDesc> all_descriptor_array;
		auto &now_buffer_desc = ResourceDescStruct.marterial_texture_slot_data[now_res_index];
		//��ȡ��buffer����Դ
		if (now_buffer_desc.shader_resource_type == MaterialShaderResourceType::MaterialShaderResourceTextureBind)
		{
			std::string resource_real_path;
			std::string resource_real_name;
			std::string resource_real_type;
			PancystarEngine::DivideFilePath(resource_name, resource_real_path, resource_real_name, resource_real_type);
			//bind���͵���Դ��ֻȡ��һ����������������
			check_error = BuildTextureDescriptorByDesc(
				resource_real_path + now_buffer_desc.shader_resource_path[0],
				now_buffer_desc.texture_descriptor_desc[0],
				all_resource_array,
				all_descriptor_array
			);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//����������
			BindDescriptorPointer new_res_descriptor;
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonDescriptor(all_descriptor_array, all_resource_array, false, new_res_descriptor);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//�����������뵽map��
			bind_shader_resource.insert(std::pair<std::string, BindDescriptorPointer>(now_buffer_desc.shader_resource_slot_name, new_res_descriptor));
		}
		else if (now_buffer_desc.shader_resource_type == MaterialShaderResourceType::MaterialShaderResourceTextureBindLess)
		{
			if (now_buffer_desc.shader_resource_path.size() != now_buffer_desc.texture_descriptor_desc.size()) 
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "texture number dismatch texture desc member, load material error");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyMaterialBasic::BuildTexturerDescriptorByDesc", error_message);
				return error_message;
			}
			std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> bindless_srv_array;
			//bindless���͵���Դ����Ҫÿһ�����ݾ���������������
			for (size_t buffer_bindless_id = 0; buffer_bindless_id < now_buffer_desc.shader_resource_path.size(); ++buffer_bindless_id)
			{
				std::string resource_real_path;
				std::string resource_real_name;
				std::string resource_real_type;
				PancystarEngine::DivideFilePath(resource_name, resource_real_path, resource_real_name, resource_real_type);
				//������ݲ����ļ����ڵ�λ���޸������·����Ϣ
				check_error = BuildTextureDescriptorByDesc(
					resource_real_path + now_buffer_desc.shader_resource_path[buffer_bindless_id],
					now_buffer_desc.texture_descriptor_desc[buffer_bindless_id],
					all_resource_array,
					all_descriptor_array
				);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
			}
			//��srv������֯��������vector
			for (size_t bindless_descriptor_id = 0; bindless_descriptor_id < all_descriptor_array.size(); ++bindless_descriptor_id)
			{
				bindless_srv_array.push_back(all_descriptor_array[bindless_descriptor_id].shader_resource_view_desc);
			}
			//����������
			BindlessDescriptorPointer new_res_descriptor;
			pancy_object_id resource_size = static_cast<pancy_object_id>(all_resource_array.size());
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonBindlessShaderResourceView(bindless_srv_array, all_resource_array, resource_size, new_res_descriptor);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//�����������뵽map��
			bindless_shader_resource.insert(std::pair<std::string, BindlessDescriptorPointer>(now_buffer_desc.shader_resource_slot_name, new_res_descriptor));
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "texture type SRV Desc not support");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyMaterialBasic::LoadResoureDataByDesc", error_message);
			return error_message;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyMaterialBasic::BuildRenderParam(PancyRenderParamID &render_param_id)
{
	auto &res_desc = GetResourceDesc();
	auto check_error = RenderParamSystem::GetInstance()->GetCommonRenderParam(res_desc.pipeline_state_name, bind_shader_resource, bindless_shader_resource, render_param_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
bool PancyMaterialBasic::CheckIfResourceLoadFinish()
{
	for (int index_shader_resource = 0; index_shader_resource < ShaderResourceData.size(); ++index_shader_resource) 
	{
		auto res_data = ShaderResourceData[index_shader_resource].GetResourceData();
		if (res_data == NULL || !res_data->CheckIfResourceLoadFinish()) 
		{
			return false;
		}
	}
	return true;
}
PancystarEngine::EngineFailReason PancystarEngine::LoadMaterialFromFile(
	const std::string &name_resource_in,
	VirtualResourcePointer &id_need
)
{
	auto check_error = PancyGlobelResourceControl::GetInstance()->LoadResource<PancyMaterialBasic>(
		name_resource_in,
		id_need,
		false
		);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void PancystarEngine::InitMaterialJsonReflect()
{
	JSON_REFLECT_INIT_ENUM(MaterialShaderResourceBufferBind);
	JSON_REFLECT_INIT_ENUM(MaterialShaderResourceTextureBind);
	JSON_REFLECT_INIT_ENUM(MaterialShaderResourceBufferBindLess);
	JSON_REFLECT_INIT_ENUM(MaterialShaderResourceTextureBindLess);
	JSON_REFLECT_INIT_ENUM(D3D12_BUFFER_SRV_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_BUFFER_SRV_FLAG_RAW);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_UNKNOWN);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_BUFFER);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURE1D);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURE1DARRAY);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURE2D);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURE2DARRAY);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURE2DMS);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURE3D);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURECUBE);
	JSON_REFLECT_INIT_ENUM(D3D12_SRV_DIMENSION_TEXTURECUBEARRAY);

	InitJsonReflectParseClass(PancyBufferDescriptorDesc, PancyJsonMaterialBufferDescriptor);
	InitJsonReflectParseClass(PancyTextureDescriptorDesc, PancyJsonMaterialTexureDescriptor);
	InitJsonReflectParseClass(PancyMaterialShaderResourceDataBuffer, PancyJsonMaterialBufferShaderResource);
	InitJsonReflectParseClass(PancyMaterialShaderResourceDataTexture, PancyJsonMaterialTexureShaderResource);
	InitJsonReflectParseClass(PancyCommonMaterialDesc, CommonMaterialDescJsonReflect);
	/*
	PancyCommonMaterialDesc new_desc;
	PancyMaterialShaderResourceDataBuffer new_buffer;
	PancyMaterialShaderResourceDataTexture new_texture;
	PancyBufferDescriptorDesc buffer_desc;
	PancyTextureDescriptorDesc texture_desc;
	
	new_buffer.buffer_descriptor_desc.push_back(buffer_desc);
	new_texture.texture_descriptor_desc.push_back(texture_desc);
	new_texture.shader_resource_path.push_back("");
	new_desc.marterial_buffer_slot_data.push_back(new_buffer);
	new_desc.marterial_texture_slot_data.push_back(new_texture);
	PancyJsonReflectControl::GetInstance()->GetJsonReflect(typeid(PancyCommonMaterialDesc).name())->ResetMemoryByMemberData(&new_desc,typeid(&new_desc).name(),sizeof(new_desc));
	PancyJsonReflectControl::GetInstance()->GetJsonReflect(typeid(PancyCommonMaterialDesc).name())->SaveToJsonFile("222.json");
	*/
}