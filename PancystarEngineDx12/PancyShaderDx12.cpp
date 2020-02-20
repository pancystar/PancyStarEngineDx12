#include"PancyShaderDx12.h"
using namespace PancystarEngine;
//���붥���ʽ
InputLayoutDesc::InputLayoutDesc()
{
}
InputLayoutDesc::~InputLayoutDesc()
{
	for (auto now_vertex_desc = vertex_buffer_desc_map.begin(); now_vertex_desc != vertex_buffer_desc_map.end(); ++now_vertex_desc)
	{
		delete now_vertex_desc->second.inputElementDescs;
		now_vertex_desc->second.inputElementDescs = NULL;
	}
	vertex_buffer_desc_map.clear();
}
void InputLayoutDesc::AddVertexDesc(std::string vertex_desc_name_in, std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc_list)
{
	PancyVertexBufferDesc new_vertex_desc;
	new_vertex_desc.vertex_desc_name = vertex_desc_name_in;
	new_vertex_desc.input_element_num = input_element_desc_list.size();
	new_vertex_desc.inputElementDescs = new D3D12_INPUT_ELEMENT_DESC[input_element_desc_list.size()];
	uint32_t count_element = 0;
	for (auto now_element = input_element_desc_list.begin(); now_element != input_element_desc_list.end(); ++now_element)
	{
		new_vertex_desc.inputElementDescs[count_element] = *now_element;
		count_element += 1;
	}
	vertex_buffer_desc_map.insert(std::pair<std::string, PancyVertexBufferDesc>(vertex_desc_name_in, new_vertex_desc));
}
//shader������
PancyShaderBasic::PancyShaderBasic(
	const std::string &shader_file_in,
	const std::string &main_func_name,
	const std::string &shader_type
)
{
	shader_file_name = shader_file_in;
	shader_entry_point_name = main_func_name;
	shader_type_name = shader_type;
}
PancystarEngine::EngineFailReason PancyShaderBasic::Create()
{
	//| D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES
#if defined(_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#else
	UINT compileFlags = 0 | D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES;
#endif
	//����shader�ļ�
	ID3D10Blob *error_message = NULL;
	HRESULT hr = D3DCompileFromFile(shader_file_name.GetUnicodeString().c_str(), nullptr, nullptr, shader_entry_point_name.GetAsciiString().c_str(), shader_type_name.GetAsciiString().c_str(), compileFlags, 0, &shader_memory_pointer, &error_message);
	if (FAILED(hr))
	{

		char data[1000];
		if (error_message != NULL)
		{
			memcpy(data, error_message->GetBufferPointer(), error_message->GetBufferSize());
		}
		PancystarEngine::EngineFailReason error_message(hr, "Compile shader : " + shader_file_name.GetAsciiString() + ":: " + shader_entry_point_name.GetAsciiString() + " error:" + data);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("load shader from file", error_message);
		return error_message;
	}
	//��ȡshader����
	hr = D3DReflect(shader_memory_pointer->GetBufferPointer(), shader_memory_pointer->GetBufferSize(), IID_ID3D12ShaderReflection, &shader_reflection);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "get shader reflect message : " + shader_file_name.GetAsciiString() + ":: " + shader_entry_point_name.GetAsciiString() + " error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("load shader from file", error_message);
		return error_message;
	}
	//��ȡ�����ʽ
	D3D12_SHADER_DESC shader_desc;
	shader_reflection->GetDesc(&shader_desc);
	for (UINT i = 0; i < shader_desc.InputParameters; ++i)
	{
		D3D12_SIGNATURE_PARAMETER_DESC now_param_desc;
		shader_reflection->GetInputParameterDesc(i, &now_param_desc);
	}
	//��ȡ����������
	/*
	for (UINT i = 0; i < shader_desc.ConstantBuffers; ++i)
	{
		auto now_constant_buffer = shader_reflection->GetConstantBufferByIndex(i);
		D3D12_SHADER_BUFFER_DESC buffer_shader;
		now_constant_buffer->GetDesc(&buffer_shader);
		int a = 0;
	}

	for (UINT i = 0; i < shader_desc.BoundResources; ++i)
	{
		D3D12_SHADER_INPUT_BIND_DESC now_bind;
		shader_reflection->GetResourceBindingDesc(i, &now_bind);
		int a = 0;
	}
	*/
	return PancystarEngine::succeed;
}
//shader������
PancyShaderControl::PancyShaderControl()
{

}
PancystarEngine::EngineFailReason PancyShaderControl::LoadShader(const std::string & shader_file, const std::string & shader_main_func, const std::string & shader_type)
{
	std::string shader_final_name = shader_file + "::" + shader_main_func;
	auto check_data = shader_list.find(shader_final_name);
	if (check_data != shader_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the shader " + shader_final_name + " had been insert to map before", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build new shader", error_message);
		return error_message;
	}
	PancyShaderBasic *new_shader = new PancyShaderBasic(shader_file, shader_main_func, shader_type);
	auto check_error = new_shader->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	shader_list.insert(std::pair<std::string, PancyShaderBasic*>(shader_final_name, new_shader));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyShaderControl::GetShaderReflection(const std::string & shader_file, const std::string & shader_main_func, const std::string & shader_type, ComPtr<ID3D12ShaderReflection> *res_data)
{
	PancystarEngine::EngineFailReason check_error;
	std::string shader_final_name = shader_file + "::" + shader_main_func;
	auto shader_data = shader_list.find(shader_final_name);
	if (shader_data == shader_list.end())
	{
		//shader��δ���أ����´��ļ��м���shader
		check_error = LoadShader(shader_file, shader_main_func, shader_type);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	HRESULT hr = shader_data->second->GetShaderReflect().As(res_data);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not copy shader reflection com_ptr of shader " + shader_final_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get shader reflection", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyShaderControl::GetShaderData(const std::string & shader_file, const std::string & shader_main_func, const std::string & shader_type, ComPtr<ID3DBlob> *res_data)
{
	PancystarEngine::EngineFailReason check_error;
	std::string shader_final_name = shader_file + "::" + shader_main_func;
	auto shader_data = shader_list.find(shader_final_name);
	if (shader_data == shader_list.end())
	{
		//shader��δ���أ����´��ļ��м���shader
		check_error = LoadShader(shader_file, shader_main_func, shader_type);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		shader_data = shader_list.find(shader_final_name);
	}
	HRESULT hr = shader_data->second->GetShader().As(res_data);
	if (FAILED(hr)) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not copy shader data com_ptr of shader " + shader_final_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get shader data", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancyShaderControl::~PancyShaderControl()
{
	for (auto data = shader_list.begin(); data != shader_list.end(); ++data)
	{
		delete data->second;
	}
	shader_list.clear();
}
//rootsignature
PancyRootSignature::PancyRootSignature(const std::string &file_name)
{
	root_signature_name = file_name;
}
PancystarEngine::EngineFailReason PancyRootSignature::Create()
{
	std::string file_name = root_signature_name.GetAsciiString();
	D3D12_STATIC_SAMPLER_DESC *data_sampledesc;
	CD3DX12_DESCRIPTOR_RANGE1 *ranges;
	CD3DX12_ROOT_PARAMETER1 *rootParameters;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC desc_out;
	Json::Value jsonRoot;
	auto check_error = PancyJsonTool::GetInstance()->LoadJsonFile(file_name, jsonRoot);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_json_value now_value;
	//��ȡÿ��shader�ⲿ����
	Json::Value value_parameters = jsonRoot.get("D3D12_ROOT_PARAMETER", Json::Value::null);
	if (value_parameters == Json::Value::null)
	{
		//�޷��ҵ��洢parameter������
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable D3D12_ROOT_PARAMETER");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	int num_parameter = value_parameters.size();
	if (num_parameter > 0)
	{
		ranges = new CD3DX12_DESCRIPTOR_RANGE1[num_parameter];
		rootParameters = new CD3DX12_ROOT_PARAMETER1[num_parameter];
	}
	else
	{
		ranges = NULL;
		rootParameters = NULL;
	}
	int32_t all_descriptor_num = 0;
	for (int i = 0; i < num_parameter; ++i)
	{
		pancy_json_value data_param_value[7];
		std::string find_name[] =
		{
			"RangeType",
			"NumDescriptors",
			"BaseShaderRegister",
			"RegisterSpace",
			"flags",
			"numDescriptorRanges",
			"ShaderVisibility"
		};
		pancy_json_data_type out_data_type[] =
		{
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum
		};
		for (int j = 0; j < 7; ++j)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_parameters[i], find_name[j], out_data_type[j], data_param_value[j]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//��ȡ��Ҫrange_type(cbv/srv/......)
		D3D12_DESCRIPTOR_RANGE_TYPE range_type = static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(data_param_value[0].int_value);
		//��ȡ��Ҫ������descriptor(cbv/srv/......)������
		int32_t descriptor_num = data_param_value[1].int_value;
		//��ȡ����descriptor������Ӧ���׸��Ĵ����ĵ�ַ
		int32_t base_registor = data_param_value[2].int_value;
		//��ȡ����descriptor������Ӧ�ļĴ�����ĵ�ַ
		int32_t base_registor_space = data_param_value[3].int_value;
		//��ȡ����descriptor������Ӧ�ĸ�ʽ
		D3D12_DESCRIPTOR_RANGE_FLAGS flag_type = static_cast<D3D12_DESCRIPTOR_RANGE_FLAGS>(data_param_value[4].int_value);
		//ע��descriptor range
		ranges[i].Init(range_type, descriptor_num, base_registor, base_registor_space, flag_type);
		//��ȡ��descriptor range��Ҫ�������Ĵ���
		int32_t descriptor_range_num = data_param_value[5].int_value;
		//��ȡshader�ķ���Ȩ��
		D3D12_SHADER_VISIBILITY shader_visibility_type = static_cast<D3D12_SHADER_VISIBILITY>(data_param_value[6].int_value);
		//ע��rootsignature��ʽ
		rootParameters[i].InitAsDescriptorTable(descriptor_range_num, &ranges[i], shader_visibility_type);
		all_descriptor_num += descriptor_num;
	}

	int num_static_sampler;
	//��ȡÿ����̬������
	Json::Value value_staticsamplers = jsonRoot.get("D3D12_STATIC_SAMPLER_DESC", Json::Value::null);
	if (value_staticsamplers == Json::Value::null)
	{
		//�޷��ҵ��洢static sampler������
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable D3D12_STATIC_SAMPLER_DESC");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	num_static_sampler = value_staticsamplers.size();
	if (num_static_sampler > 0)
	{
		data_sampledesc = new D3D12_STATIC_SAMPLER_DESC[num_static_sampler];
	}
	else
	{
		data_sampledesc = NULL;
	}
	for (int i = 0; i < num_static_sampler; ++i)
	{
		pancy_json_value data_num[13];
		std::string find_name[] =
		{
			"Filter",
			"AddressU",
			"AddressV",
			"AddressW",
			"MipLODBias",
			"MaxAnisotropy",
			"ComparisonFunc",
			"BorderColor",
			"MinLOD",
			"MaxLOD",
			"ShaderRegister",
			"RegisterSpace",
			"ShaderVisibility"
		};
		pancy_json_data_type out_data_type[] =
		{
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_float,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_float,
			pancy_json_data_type::json_data_float,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_enum
		};
		for (int j = 0; j < 13; ++j)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_staticsamplers[i], find_name[j], out_data_type[j], data_num[j]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}

		data_sampledesc[i].Filter = static_cast<D3D12_FILTER>(data_num[0].int_value);
		data_sampledesc[i].AddressU = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(data_num[1].int_value);
		data_sampledesc[i].AddressV = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(data_num[2].int_value);
		data_sampledesc[i].AddressW = static_cast<D3D12_TEXTURE_ADDRESS_MODE>(data_num[3].int_value);
		data_sampledesc[i].MipLODBias = data_num[4].float_value;
		data_sampledesc[i].MaxAnisotropy = data_num[5].int_value;
		data_sampledesc[i].ComparisonFunc = static_cast<D3D12_COMPARISON_FUNC>(data_num[6].int_value);
		data_sampledesc[i].BorderColor = static_cast<D3D12_STATIC_BORDER_COLOR>(data_num[7].int_value);
		data_sampledesc[i].MinLOD = data_num[8].float_value;
		data_sampledesc[i].MaxLOD = data_num[9].float_value;
		data_sampledesc[i].ShaderRegister = data_num[10].int_value;
		data_sampledesc[i].RegisterSpace = data_num[11].int_value;
		data_sampledesc[i].ShaderVisibility = static_cast<D3D12_SHADER_VISIBILITY>(data_num[12].int_value);
	}
	//��ȡrootsignature��ʽ
	Json::Value value_root_signature_flag = jsonRoot.get("RootSignatureFlags", Json::Value::null);
	if (value_staticsamplers == Json::Value::null)
	{
		//�޷��ҵ��洢root signature flag������
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable RootSignatureFlags");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Root Signature json file " + file_name + " error", error_mesage);
		return error_mesage;
	}
	D3D12_ROOT_SIGNATURE_FLAGS root_signature = static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(0);
	for (uint32_t i = 0; i < value_root_signature_flag.size(); ++i)
	{
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_root_signature_flag, i, pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		root_signature = root_signature | static_cast<D3D12_ROOT_SIGNATURE_FLAGS>(now_value.int_value);
	}
	desc_out.Init_1_1(num_parameter, rootParameters, num_static_sampler, data_sampledesc, root_signature);

	//����rootsignature
	check_error = BuildResource(desc_out);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//ɾ����ʱ��Դ
	if (ranges != NULL)
	{
		delete[] ranges;
		ranges = NULL;
	}
	if (rootParameters != NULL)
	{
		delete[] rootParameters;
		rootParameters = NULL;
	}
	if (data_sampledesc != NULL)
	{
		delete[] data_sampledesc;
		data_sampledesc = NULL;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRootSignature::BuildResource(
	const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootSignatureDesc
)
{
	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	HRESULT hr;
	hr = D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "serial rootsignature " + root_signature_name.GetAsciiString() + " error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create Root Signature", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root_signature_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create root signature " + root_signature_name.GetAsciiString() + " failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create Root Signature", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
std::string PancyRootSignature::GetJsonFileRealName(const std::string &file_name_in)
{
	std::string new_str;
	int st = 0;
	int length = -4;
	for (int i = file_name_in.size() - 2; i >= 0; --i)
	{
		if (file_name_in[i - 1] == '\\')
		{
			st = i;
			break;
		}
		else
		{
			length += 1;
		}
	}
	return file_name_in.substr(st, length);
}
//rootsignature������
PancyRootSignatureControl::PancyRootSignatureControl()
{
	RootSig_id_self_add = 0;
	AddRootSignatureGlobelVariable();
}
void PancyRootSignatureControl::AddRootSignatureGlobelVariable()
{
	//descriptor range��ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER);
	//descriptor flag��ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC);
	//shader����Ȩ��
	JSON_REFLECT_INIT_ENUM(D3D12_SHADER_VISIBILITY_ALL);
	JSON_REFLECT_INIT_ENUM(D3D12_SHADER_VISIBILITY_VERTEX);
	JSON_REFLECT_INIT_ENUM(D3D12_SHADER_VISIBILITY_HULL);
	JSON_REFLECT_INIT_ENUM(D3D12_SHADER_VISIBILITY_DOMAIN);
	JSON_REFLECT_INIT_ENUM(D3D12_SHADER_VISIBILITY_GEOMETRY);
	JSON_REFLECT_INIT_ENUM(D3D12_SHADER_VISIBILITY_PIXEL);
	//������ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_POINT_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_ANISOTROPIC);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_POINT_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_POINT_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_COMPARISON_ANISOTROPIC);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_POINT_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_MIN_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MINIMUM_ANISOTROPIC);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_POINT_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_LINEAR_MAG_POINT_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_MAG_LINEAR_MIP_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_LINEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_FILTER_MAXIMUM_ANISOTROPIC);
	//Ѱַ��ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_ADDRESS_MODE_MIRROR);
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_ADDRESS_MODE_BORDER);
	JSON_REFLECT_INIT_ENUM(D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE);
	//�ȽϺ�����ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_NEVER);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_LESS);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_LESS_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_GREATER);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_NOT_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_GREATER_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_ALWAYS);
	//�����߽�Ĳ�����ɫ
	JSON_REFLECT_INIT_ENUM(D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK);
	JSON_REFLECT_INIT_ENUM(D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);
	JSON_REFLECT_INIT_ENUM(D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);
	//root signature�ķ���Ȩ��
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS);
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS);
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS);
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS);
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
	JSON_REFLECT_INIT_ENUM(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT);
}
PancystarEngine::EngineFailReason PancyRootSignatureControl::GetResource(const pancy_object_id &root_signature_id, ID3D12RootSignature** root_signature_res)
{
	PancystarEngine::EngineFailReason check_error;
	auto root_signature_find = root_signature_array.find(root_signature_id);
	if (root_signature_find == root_signature_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find rootsignature ID: " + std::to_string(root_signature_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find rootsignature by id", error_message);
		return error_message;
	}
	root_signature_find->second->GetResource(root_signature_res);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRootSignatureControl::BuildRootSignature(const std::string &rootsig_config_file)
{
	PancystarEngine::EngineFailReason check_error;
	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc = {};
	//�ж��ظ�
	auto check_data = RootSig_name.find(rootsig_config_file);
	if (check_data != RootSig_name.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the root signature " + rootsig_config_file + " had been insert to map before", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build new root signature", error_message);
		return error_message;
	}
	//����RootSignature
	PancyRootSignature *data_root_signature = new PancyRootSignature(rootsig_config_file);
	check_error = data_root_signature->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����ID���Ƿ�Ϊ���������¿��ٵ���ԴID��
	pancy_object_id now_next_id;
	if (empty_RootSig_id.size() > 0)
	{
		now_next_id = empty_RootSig_id.front();
		empty_RootSig_id.pop();
	}
	else
	{
		now_next_id = RootSig_id_self_add;
		RootSig_id_self_add += 1;
	}
	//����Դ���뵽��Դ��
	RootSig_name.insert(std::pair<std::string, pancy_object_id >(rootsig_config_file, now_next_id));
	RootSig_name_reflect.insert(std::pair<pancy_object_id, std::string>(now_next_id, rootsig_config_file));
	root_signature_array.insert(std::pair<pancy_object_id, PancyRootSignature*>(now_next_id, data_root_signature));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRootSignatureControl::GetRootSignature(const std::string &name_in, pancy_object_id &root_signature_id)
{
	PancystarEngine::EngineFailReason check_error;
	auto root_signature_find = RootSig_name.find(name_in);
	if (root_signature_find == RootSig_name.end())
	{
		//δ�ҵ�root signature�����Լ���
		check_error = BuildRootSignature(name_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		root_signature_find = RootSig_name.find(name_in);
	}
	root_signature_id = root_signature_find->second;
	return PancystarEngine::succeed;
}
PancyRootSignatureControl::~PancyRootSignatureControl()
{
	for (auto data = root_signature_array.begin(); data != root_signature_array.end(); ++data)
	{
		delete data->second;
	}
	root_signature_array.clear();
}
//pipline state object graph
PancyPiplineStateObjectGraph::PancyPiplineStateObjectGraph(const std::string &pso_name_in)
{
	cbuffer_desc_parse = new CommonBufferJsonReflect();
	cbuffer_desc_parse->Create();
	pipline_type = PSO_TYPE_GRAPHIC;
	pso_name = pso_name_in;
}
/*
PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::GetDescriptorHeapUse(std::string &descriptor_heap_name)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyRootSignatureControl::GetInstance()->GetDescriptorHeapUse(root_signature_ID, descriptor_heap_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::GetDescriptorDistribute(std::vector<pancy_object_id> &descriptor_distribute)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyRootSignatureControl::GetInstance()->GetDescriptorDistribute(root_signature_ID, descriptor_distribute);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::CheckCbuffer(const std::string &cbuffer_name)
{
	auto cbuffer_data = Cbuffer_map.find(cbuffer_name);
	if (cbuffer_data == Cbuffer_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in PSO: " + pso_name.GetAsciiString());
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Check Pso cbuffer", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::GetCbuffer(const std::string &cbuffer_name, const Json::Value *& CbufferData) 
{
	auto cbuffer_data = Cbuffer_map.find(cbuffer_name);
	if (cbuffer_data == Cbuffer_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in PSO: " + pso_name.GetAsciiString());
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Pso cbuffer desc", error_message);
		return error_message;
	}
	CbufferData = &cbuffer_data->second;
	return PancystarEngine::succeed;
}
*/
PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::BuildCbufferByName(
	const std::string &cbuffer_name,
	PancyConstantBuffer &cbuffer_data_out
) 
{
	auto cbuffer_allocator = Cbuffer_map.find(cbuffer_name);
	if (cbuffer_allocator == Cbuffer_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in PSO: " + pso_name.GetAsciiString());
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyPiplineStateObjectGraph::BuildCbufferByName", error_message);
		return error_message;
	}
	auto check_error = cbuffer_allocator->second->BuildNewCbuffer(cbuffer_data_out);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::ReleaseCbufferByID(
	const std::string &cbuffer_name,
	const pancy_object_id &buffer_resource_id,
	const pancy_object_id &buffer_offset_id
)
{
	auto cbuffer_allocator = Cbuffer_map.find(cbuffer_name);
	if (cbuffer_allocator == Cbuffer_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in PSO: " + pso_name.GetAsciiString());
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyPiplineStateObjectGraph::ReleaseCbufferByID", error_message);
		return error_message;
	}
	auto check_error = cbuffer_allocator->second->ReleaseCbuffer(buffer_resource_id, buffer_offset_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::Create()
{
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_out = {};
	PancystarEngine::PancyString root_sig_name;
	std::string file_name = pso_name.GetAsciiString();
	//��ȡ��ʽ
	Json::Value jsonRoot;
	auto check_error = PancyJsonTool::GetInstance()->LoadJsonFile(file_name, jsonRoot);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_json_value now_value;
	//��ȡrootsignature�ļ���
	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, jsonRoot, "RootSignature", pancy_json_data_type::json_data_string, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	root_sig_name = now_value.string_value;
	check_error = PancyRootSignatureControl::GetInstance()->GetRootSignature(now_value.string_value, root_signature_ID);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	ID3D12RootSignature* root_signature_use;
	PancyRootSignatureControl::GetInstance()->GetResource(root_signature_ID, &root_signature_use);
	//��ȡ��ǰpipeline�ĸ�ʽ
	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, jsonRoot, "PSOType", pancy_json_data_type::json_data_enum, now_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pipline_type = static_cast<PSOType>(now_value.int_value);
	if (pipline_type == PSOType::PSO_TYPE_GRAPHIC)
	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_out = {};
		desc_out.pRootSignature = root_signature_use;
		//��ȡ��ɫ��
		std::string shader_version[] =
		{
			"vs_5_1",
			"ps_5_1",
			"gs_5_1",
			"hs_5_1",
			"ds_5_1",
		};
		std::string vertex_shader_name, vertex_shader_mainfunc;
		for (int i = 0; i < 5; ++i)
		{
			std::string shader_file_name, shader_func_name;
			Pancy_json_shader_type now_shader_read = static_cast<Pancy_json_shader_type>(static_cast<int32_t>(Pancy_json_shader_type::json_shader_vertex) + i);
			check_error = PancyJsonTool::GetInstance()->GetJsonShader(file_name, jsonRoot, now_shader_read, shader_file_name, shader_func_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			if (now_shader_read == json_shader_vertex)
			{
				vertex_shader_name = shader_file_name;
				vertex_shader_mainfunc = shader_func_name;
			}
			if (shader_file_name != "0" && shader_func_name != "0")
			{
				ComPtr<ID3DBlob> shader_data;
				check_error = PancyShaderControl::GetInstance()->GetShaderData(shader_file_name, shader_func_name, shader_version[static_cast<int32_t>(now_shader_read)],&shader_data);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				//��ȡshader reflect���洢cbuffer����Ϣ
				ComPtr<ID3D12ShaderReflection> now_shader_reflect;
				check_error = PancyShaderControl::GetInstance()->GetShaderReflection(shader_file_name, shader_func_name, shader_version[static_cast<int32_t>(now_shader_read)],&now_shader_reflect);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				D3D12_SHADER_DESC shader_desc;
				now_shader_reflect->GetDesc(&shader_desc);
				for (UINT j = 0; j < shader_desc.ConstantBuffers; ++j)
				{
					auto now_constant_buffer = now_shader_reflect->GetConstantBufferByIndex(j);
					D3D12_SHADER_BUFFER_DESC buffer_shader;
					now_constant_buffer->GetDesc(&buffer_shader);
					if (Cbuffer_map.find(buffer_shader.Name) == Cbuffer_map.end())
					{
						//��cbuffer�Ĵ�С����256λ����
						pancy_resource_size alize_cbuffer_size = buffer_shader.Size;
						pancy_resource_size alize_buffer_resource_size = 65536;
						if (alize_cbuffer_size % 256 != 0)
						{
							alize_cbuffer_size = (1 + (alize_cbuffer_size / 256)) * 256;
						}
						//����cbuffer�Ĵ�С���������ٵ�buffer��Դ�Ĵ�С
						if (alize_cbuffer_size > 256 * 6) 
						{
							//����Cbuffer(������instance����),����256k��buffer��Ϊ����buffer
							alize_buffer_resource_size = 256 * 1024;
						}
						else 
						{
							//С��cbuffer������64k��buffer��Ϊ����buffer
							alize_buffer_resource_size = 64 * 1024;
						}
						PancyCommonBufferDesc cbuffer_resource_desc;
						cbuffer_resource_desc.buffer_type = Buffer_Constant;
						cbuffer_resource_desc.heap_flag_in = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;
						cbuffer_resource_desc.heap_type = D3D12_HEAP_TYPE_UPLOAD;
						cbuffer_resource_desc.buffer_res_desc.Alignment = 0;
						cbuffer_resource_desc.buffer_res_desc.DepthOrArraySize = 1;
						cbuffer_resource_desc.buffer_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
						cbuffer_resource_desc.buffer_res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
						cbuffer_resource_desc.buffer_res_desc.Height = 1;
						cbuffer_resource_desc.buffer_res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
						cbuffer_resource_desc.buffer_res_desc.MipLevels = 1;
						cbuffer_resource_desc.buffer_res_desc.SampleDesc.Count = 1;
						cbuffer_resource_desc.buffer_res_desc.SampleDesc.Quality = 0;
						cbuffer_resource_desc.buffer_res_desc.Width = alize_buffer_resource_size;
						//��cbuffer�Ĵ洢buffer��ʽ���л�Ϊjson�ڴ�
						check_error = cbuffer_desc_parse->ResetMemoryByMemberData(&cbuffer_resource_desc,typeid(PancyCommonBufferDesc).name(),sizeof(cbuffer_resource_desc));
						if (!check_error.CheckIfSucceed()) 
						{
							return check_error;
						}
						Json::Value new_cbuffer_resource_value;
						check_error = cbuffer_desc_parse->SaveToJsonMemory(new_cbuffer_resource_value);
						if (!check_error.CheckIfSucceed())
						{
							return check_error;
						}
						//todo����pso����cbuffer�Ĵ���������
						//׼������һ���µĳ���������
						Json::Value new_cbuffer_desc_value;
						//������Դ��ʽ
						//std::string subresource_name;
						//check_error = PancystarEngine::PancyBasicBufferControl::GetInstance()->BuildBufferTypeJson(PancystarEngine::Buffer_Constant, buffer_shader.Size, subresource_name);
						PancyJsonTool::GetInstance()->SetJsonValue(new_cbuffer_desc_value, "BufferType", "Buffer_Constant");
						//PancyJsonTool::GetInstance()->SetJsonValue(new_cbuffer_desc_value, "SubResourceFile", subresource_name);
						//if (!check_error.CheckIfSucceed())
						//{
						//	return check_error;
						//}
						//��䳣��������
						Json::Value cbuffer_value;
						PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_value, "BufferSize", alize_cbuffer_size);
						for (int i = 0; i < buffer_shader.Variables; ++i)
						{
							Json::Value cbuffer_variable_value;
							auto shader_variable = now_constant_buffer->GetVariableByIndex(i);
							D3D12_SHADER_VARIABLE_DESC shader_var_desc;
							shader_variable->GetDesc(&shader_var_desc);
							PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_variable_value, "StartOffset", shader_var_desc.StartOffset);
							PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_variable_value, "Size", shader_var_desc.Size);
							PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_variable_value, "Name", shader_var_desc.Name);
							PancyJsonTool::GetInstance()->AddJsonArrayValue(cbuffer_value, "VariableMember", cbuffer_variable_value);
						}
						PancyJsonTool::GetInstance()->SetJsonValue(new_cbuffer_desc_value, "CbufferDesc", cbuffer_value);
						//����ǰcbuffer�ĸ�ʽ������map��
						ConstantBufferAlloctor *new_cbuffer_alloctar = new ConstantBufferAlloctor(
							alize_buffer_resource_size,
							buffer_shader.Name,
							pso_name.GetAsciiString(),
							new_cbuffer_desc_value,
							cbuffer_value
						);
						Cbuffer_map.insert(std::pair<std::string, ConstantBufferAlloctor*>(buffer_shader.Name, new_cbuffer_alloctar));
					}
				}
				//���shader��Ϣ
				switch (now_shader_read)
				{
				case json_shader_vertex:
					desc_out.VS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
					break;
				case json_shader_pixel:
					desc_out.PS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
					break;
				case json_shader_geometry:
					desc_out.GS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
					break;
				case json_shader_hull:
					desc_out.HS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
					break;
				case json_shader_domin:
					desc_out.DS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
					break;
				default:
					break;
				}
			}
		}
		//���ݶ�����ɫ����ȡ���������ʽ
		ComPtr<ID3D12ShaderReflection> vertex_shader_reflect;
		check_error = PancyShaderControl::GetInstance()->GetShaderReflection(vertex_shader_name, vertex_shader_mainfunc, "vs_5_1", &vertex_shader_reflect);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (InputLayoutDesc::GetInstance()->GetVertexDesc(vertex_shader_name + "::" + vertex_shader_mainfunc) == NULL)
		{
			//δ�ҵ����뻺����������µĻ�����
			std::vector<D3D12_INPUT_ELEMENT_DESC> input_desc_array;
			check_error = GetInputDesc(vertex_shader_reflect, input_desc_array);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			InputLayoutDesc::GetInstance()->AddVertexDesc(vertex_shader_name + "::" + vertex_shader_mainfunc, input_desc_array);
		}
		auto vertex_desc = InputLayoutDesc::GetInstance()->GetVertexDesc(vertex_shader_name + "::" + vertex_shader_mainfunc);
		desc_out.InputLayout.pInputElementDescs = vertex_desc->inputElementDescs;
		desc_out.InputLayout.NumElements = static_cast<UINT>(vertex_desc->input_element_num);
		//��ȡ��դ����ʽ
		Json::Value value_rasterize_state = jsonRoot.get("RasterizerState", Json::Value::null);
		if (value_rasterize_state == Json::Value::null)
		{
			//�޷��ҵ���դ����ʽ
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable RasterizerState");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_rasterize_state, "FILL_MODE", pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.RasterizerState.FillMode = static_cast<D3D12_FILL_MODE>(now_value.int_value);
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_rasterize_state, "CULL_MODE", pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(now_value.int_value);
		//��ȡalpha��ϸ�ʽ
		desc_out.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		Json::Value value_blend_state = jsonRoot.get("BlendState", Json::Value::null);
		if (value_blend_state == Json::Value::null)
		{
			//�޷��ҵ�alpha��ϸ�ʽ
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable BlendState");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_blend_state, "AlphaToCoverageEnable", pancy_json_data_type::json_data_bool, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.BlendState.AlphaToCoverageEnable = now_value.bool_value;
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_blend_state, "IndependentBlendEnable", pancy_json_data_type::json_data_bool, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.BlendState.IndependentBlendEnable = now_value.bool_value;
		Json::Value value_blend_target = value_blend_state.get("RenderTarget", Json::Value::null);
		if (value_blend_target == Json::Value::null)
		{
			//�޷��ҵ�alpha���Ŀ���ʽ
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable BlendState::RenderTarget");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		if (value_blend_target.size() > 0)
		{
			for (uint32_t i = 0; i < value_blend_target.size(); ++i)
			{
				pancy_json_value data_num[10];
				pancy_json_data_type out_data_type[] =
				{
					pancy_json_data_type::json_data_bool,
					pancy_json_data_type::json_data_bool,
					pancy_json_data_type::json_data_enum,
					pancy_json_data_type::json_data_enum,
					pancy_json_data_type::json_data_enum,
					pancy_json_data_type::json_data_enum,
					pancy_json_data_type::json_data_enum,
					pancy_json_data_type::json_data_enum,
					pancy_json_data_type::json_data_enum,
					pancy_json_data_type::json_data_enum
				};
				std::string find_name[] =
				{
					"BlendEnable",
					"LogicOpEnable",
					"SrcBlend",
					"DestBlend",
					"BlendOp",
					"SrcBlendAlpha",
					"DestBlendAlpha",
					"BlendOpAlpha",
					"LogicOp",
					"RenderTargetWriteMask"
				};
				for (int j = 0; j < 10; ++j)
				{
					check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_blend_target[i], find_name[j], out_data_type[j], data_num[j]);
					if (!check_error.CheckIfSucceed())
					{
						return check_error;
					}
				}
				desc_out.BlendState.RenderTarget[i].BlendEnable = data_num[0].bool_value;
				desc_out.BlendState.RenderTarget[i].LogicOpEnable = data_num[1].bool_value;
				desc_out.BlendState.RenderTarget[i].SrcBlend = static_cast<D3D12_BLEND>(data_num[2].int_value);
				desc_out.BlendState.RenderTarget[i].DestBlend = static_cast<D3D12_BLEND>(data_num[3].int_value);
				desc_out.BlendState.RenderTarget[i].BlendOp = static_cast<D3D12_BLEND_OP>(data_num[4].int_value);
				desc_out.BlendState.RenderTarget[i].SrcBlendAlpha = static_cast<D3D12_BLEND>(data_num[5].int_value);
				desc_out.BlendState.RenderTarget[i].DestBlendAlpha = static_cast<D3D12_BLEND>(data_num[6].int_value);
				desc_out.BlendState.RenderTarget[i].BlendOpAlpha = static_cast<D3D12_BLEND_OP>(data_num[7].int_value);
				desc_out.BlendState.RenderTarget[i].LogicOp = static_cast<D3D12_LOGIC_OP>(data_num[8].int_value);
				desc_out.BlendState.RenderTarget[i].RenderTargetWriteMask = static_cast<UINT8>(data_num[9].int_value);
			}
		}
		//��ȡ���ģ�建������ʽ
		Json::Value value_depthstencil_state = jsonRoot.get("DepthStencilState", Json::Value::null);
		if (value_depthstencil_state == Json::Value::null)
		{
			//�޷��ҵ����ģ�建������ʽ
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable DepthStencilState");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		std::string find_depthstencil_name[] =
		{
			"DepthEnable",
			"DepthWriteMask",
			"DepthFunc",
			"StencilEnable",
			"StencilReadMask",
			"StencilWriteMask",
		};
		pancy_json_value data_depthstencil_num[6];
		pancy_json_data_type out_data_depthstencil_type[] =
		{
			pancy_json_data_type::json_data_bool,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_bool,
			pancy_json_data_type::json_data_int,
			pancy_json_data_type::json_data_int
		};
		for (int j = 0; j < 6; ++j)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_depthstencil_state, find_depthstencil_name[j], out_data_depthstencil_type[j], data_depthstencil_num[j]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		desc_out.DepthStencilState.DepthEnable = data_depthstencil_num[0].bool_value;
		desc_out.DepthStencilState.DepthWriteMask = static_cast<D3D12_DEPTH_WRITE_MASK>(data_depthstencil_num[1].int_value);
		desc_out.DepthStencilState.DepthFunc = static_cast<D3D12_COMPARISON_FUNC>(data_depthstencil_num[2].int_value);
		desc_out.DepthStencilState.StencilEnable = data_depthstencil_num[3].bool_value;
		desc_out.DepthStencilState.StencilReadMask = static_cast<UINT8>(data_depthstencil_num[4].int_value);
		desc_out.DepthStencilState.StencilWriteMask = static_cast<UINT8>(data_depthstencil_num[5].int_value);
		Json::Value value_depthstencil_frontface_state = value_depthstencil_state.get("FrontFaceDesc", Json::Value::null);
		if (value_depthstencil_frontface_state == Json::Value::null)
		{
			//�޷��ҵ����ģ�建������ʽ
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable DepthStencilState::FrontFaceDesc");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		std::string find_depstencil_face_name[] =
		{
			"StencilFunc",
			"StencilDepthFailOp",
			"StencilPassOp",
			"StencilFailOp"
		};
		pancy_json_value data_depstencil_face_num[4];
		pancy_json_data_type out_data_depstencil_face_type[] =
		{
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum,
			pancy_json_data_type::json_data_enum
		};
		for (int j = 0; j < 4; ++j)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_depthstencil_frontface_state, find_depstencil_face_name[j], out_data_depstencil_face_type[j], data_depstencil_face_num[j]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		desc_out.DepthStencilState.FrontFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(data_depstencil_face_num[0].int_value);
		desc_out.DepthStencilState.FrontFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[1].int_value);
		desc_out.DepthStencilState.FrontFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[2].int_value);
		desc_out.DepthStencilState.FrontFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[3].int_value);
		Json::Value value_depthstencil_backface_state = value_depthstencil_state.get("BackFaceDesc", Json::Value::null);
		if (value_depthstencil_backface_state == Json::Value::null)
		{
			//�޷��ҵ����ģ�建������ʽ
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable DepthStencilState::BackFaceDesc");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		for (int j = 0; j < 4; ++j)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_depthstencil_backface_state, find_depstencil_face_name[j], out_data_depstencil_face_type[j], data_depstencil_face_num[j]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		desc_out.DepthStencilState.BackFace.StencilFunc = static_cast<D3D12_COMPARISON_FUNC>(data_depstencil_face_num[0].int_value);
		desc_out.DepthStencilState.BackFace.StencilDepthFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[1].int_value);
		desc_out.DepthStencilState.BackFace.StencilPassOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[2].int_value);
		desc_out.DepthStencilState.BackFace.StencilFailOp = static_cast<D3D12_STENCIL_OP>(data_depstencil_face_num[3].int_value);
		//��ȡ��������
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, jsonRoot, "SampleMask", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.SampleMask = static_cast<UINT>(now_value.int_value);
		//��ȡͼԪ��֯��ʽ
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, jsonRoot, "PrimitiveTopologyType", pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.PrimitiveTopologyType = static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(now_value.int_value);
		//��ȡ��ȾĿ�������
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, jsonRoot, "NumRenderTargets", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.NumRenderTargets = now_value.int_value;
		//��ȡ��ȾĿ���ʽ
		Json::Value data_render_target = jsonRoot.get("RTVFormats", Json::Value::null);
		if (data_render_target == Json::Value::null)
		{
			//�޷��ҵ���ȾĿ������
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable data_render_target");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		if (desc_out.NumRenderTargets != data_render_target.size())
		{
			//��ȾĿ����������
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "the render_target need " + std::to_string(desc_out.NumRenderTargets) + " but find " + std::to_string(data_render_target.size()));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		for (uint32_t i = 0; i < desc_out.NumRenderTargets; ++i)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, data_render_target, i, pancy_json_data_type::json_data_enum, now_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			desc_out.RTVFormats[i] = static_cast<DXGI_FORMAT>(now_value.int_value);
		}
		//��ȡ���ģ�建������ʽ
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, jsonRoot, "DSVFormat", pancy_json_data_type::json_data_enum, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.DSVFormat = static_cast<DXGI_FORMAT>(now_value.int_value);
		//��ȡ����ݸ�ʽ
		Json::Value data_sample_desc = jsonRoot.get("SampleDesc", Json::Value::null);
		if (data_sample_desc == Json::Value::null)
		{
			//�޷��ҵ���ȾĿ������
			PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value of variable SampleDesc");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("combile Graphic PSO json file " + file_name + " error", error_mesage);
			return error_mesage;
		}
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, data_sample_desc, "Count", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.SampleDesc.Count = now_value.int_value;
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, data_sample_desc, "Quality", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		desc_out.SampleDesc.Quality = now_value.int_value;
		//��ȡ��ǰpipeline����Դ�󶨸�ʽ
		Json::Value list_descriptor_desc = jsonRoot.get("DescriptorType", Json::Value::null);
		for (int i = 0; i < list_descriptor_desc.size(); ++i)
		{
			std::string descriptor_name;
			PancyShaderDescriptorType descriptor_type;
			Json::Value data_descriptor_desc = list_descriptor_desc.get(i, Json::Value::null);
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, data_descriptor_desc,"name", pancy_json_data_type::json_data_string, now_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			descriptor_name = now_value.string_value;
			check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, data_descriptor_desc, "type", pancy_json_data_type::json_data_enum, now_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			descriptor_type = static_cast<PancyShaderDescriptorType>(now_value.int_value);
			PancyDescriptorPSODescription new_descriptor_root_signature_bind;
			new_descriptor_root_signature_bind.descriptor_name = descriptor_name;
			new_descriptor_root_signature_bind.rootsignature_slot = i;
			switch (descriptor_type)
			{
			case CbufferPrivate:
				private_cbuffer.push_back(new_descriptor_root_signature_bind);
				break;
			case CbufferGlobel:
				globel_cbuffer.push_back(new_descriptor_root_signature_bind);
				break;
			case SRVGlobel:
				globel_shader_res.push_back(new_descriptor_root_signature_bind);
				break;
			case SRVPrivate:
				private_shader_res.push_back(new_descriptor_root_signature_bind);
				break;
			case SRVBindless:
				bindless_shader_res.push_back(new_descriptor_root_signature_bind);
				break;
			default:
				break;
			}
		}
		//������Դ
		HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateGraphicsPipelineState(&desc_out, IID_PPV_ARGS(&pso_data));
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(hr, "Create PSO error name " + pso_name.GetAsciiString());
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build PSO", error_message);
			return error_message;
		}
	}
	else if (pipline_type == PSOType::PSO_TYPE_COMPUTE)
	{
		D3D12_COMPUTE_PIPELINE_STATE_DESC  desc_out = {};
		desc_out.pRootSignature = root_signature_use;
		//������ɫ��pipeline
		std::string shader_file_name, shader_func_name;
		check_error = PancyJsonTool::GetInstance()->GetJsonShader(file_name, jsonRoot, json_shader_compute, shader_file_name, shader_func_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (shader_file_name != "0" && shader_func_name != "0")
		{
			//��ȡshader����
			ComPtr<ID3DBlob> shader_data;
			check_error = PancyShaderControl::GetInstance()->GetShaderData(shader_file_name, shader_func_name, "cs_5_1", &shader_data);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//��ȡshader reflect���洢cbuffer����Ϣ
			ComPtr<ID3D12ShaderReflection> now_shader_reflect;
			check_error = PancyShaderControl::GetInstance()->GetShaderReflection(shader_file_name, shader_func_name, "cs_5_1", &now_shader_reflect);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			D3D12_SHADER_DESC shader_desc;
			now_shader_reflect->GetDesc(&shader_desc);
			for (UINT j = 0; j < shader_desc.ConstantBuffers; ++j)
			{
				auto now_constant_buffer = now_shader_reflect->GetConstantBufferByIndex(j);
				D3D12_SHADER_BUFFER_DESC buffer_shader;
				now_constant_buffer->GetDesc(&buffer_shader);
				if (Cbuffer_map.find(buffer_shader.Name) == Cbuffer_map.end())
				{
					//׼������һ���µĳ���������
					Json::Value root_value;
					//������Դ��ʽ
					std::string subresource_name;
					check_error = PancystarEngine::PancyBasicBufferControl::GetInstance()->BuildBufferTypeJson(PancystarEngine::Buffer_Constant, buffer_shader.Size, subresource_name);
					PancyJsonTool::GetInstance()->SetJsonValue(root_value, "BufferType", "Buffer_Constant");
					PancyJsonTool::GetInstance()->SetJsonValue(root_value, "SubResourceFile", subresource_name);
					if (!check_error.CheckIfSucceed())
					{
						return check_error;
					}
					//��䳣��������
					Json::Value cbuffer_value;
					PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_value, "BufferSize", buffer_shader.Size);
					for (int i = 0; i < buffer_shader.Variables; ++i)
					{
						Json::Value cbuffer_variable_value;
						auto shader_variable = now_constant_buffer->GetVariableByIndex(i);
						D3D12_SHADER_VARIABLE_DESC shader_var_desc;
						shader_variable->GetDesc(&shader_var_desc);
						PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_variable_value, "StartOffset", shader_var_desc.StartOffset);
						PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_variable_value, "Size", shader_var_desc.Size);
						PancyJsonTool::GetInstance()->SetJsonValue(cbuffer_variable_value, "Name", shader_var_desc.Name);
						PancyJsonTool::GetInstance()->AddJsonArrayValue(cbuffer_value, "VariableMember", cbuffer_variable_value);
					}
					PancyJsonTool::GetInstance()->SetJsonValue(root_value, "CbufferDesc", cbuffer_value);
					//���ﲻ�ٽ�cbuffer��Ϣ�洢���ļ��У���Ϊֱ�ӱ������ڴ���
					Cbuffer_map.insert(std::pair<std::string, Json::Value>(buffer_shader.Name, root_value));

				}
			}
			//���shader��Ϣ
			desc_out.CS = CD3DX12_SHADER_BYTECODE(shader_data.Get());
			//��ȡ��ǰpipeline����Դ�󶨸�ʽ
			Json::Value list_descriptor_desc = jsonRoot.get("DescriptorType", Json::Value::null);
			for (int i = 0; i < list_descriptor_desc.size(); ++i)
			{
				std::string descriptor_name;
				PancyShaderDescriptorType descriptor_type;
				Json::Value data_descriptor_desc = list_descriptor_desc.get(i, Json::Value::null);
				check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, data_descriptor_desc, "name", pancy_json_data_type::json_data_string, now_value);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				descriptor_name = now_value.string_value;
				check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, data_descriptor_desc, "type", pancy_json_data_type::json_data_enum, now_value);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				descriptor_type = static_cast<PancyShaderDescriptorType>(now_value.int_value);
				PancyDescriptorPSODescription new_descriptor_root_signature_bind;
				new_descriptor_root_signature_bind.descriptor_name = descriptor_name;
				new_descriptor_root_signature_bind.rootsignature_slot = i;
				switch (descriptor_type)
				{
				case CbufferPrivate:
					private_cbuffer.push_back(new_descriptor_root_signature_bind);
					break;
				case CbufferGlobel:
					globel_cbuffer.push_back(new_descriptor_root_signature_bind);
					break;
				case SRVGlobel:
					globel_shader_res.push_back(new_descriptor_root_signature_bind);
					break;
				case SRVPrivate:
					private_shader_res.push_back(new_descriptor_root_signature_bind);
					break;
				case SRVBindless:
					bindless_shader_res.push_back(new_descriptor_root_signature_bind);
					break;
				default:
					break;
				}
			}
			//������Դ
			HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateComputePipelineState(&desc_out, IID_PPV_ARGS(&pso_data));
			if (FAILED(hr))
			{
				PancystarEngine::EngineFailReason error_message(hr, "Create PSO error name " + pso_name.GetAsciiString());
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build PSO", error_message);
				return error_message;
			}
		}
	}
	return PancystarEngine::succeed;
}
const std::vector<PancyDescriptorPSODescription> &PancyPiplineStateObjectGraph::GetDescriptor(const PancyShaderDescriptorType &descriptor_type)
{
	switch (descriptor_type)
	{
	case PancyShaderDescriptorType::CbufferGlobel:
		return globel_cbuffer;
		break;
	case PancyShaderDescriptorType::CbufferPrivate:
		return private_cbuffer;
		break;
	case PancyShaderDescriptorType::SRVGlobel:
		return globel_shader_res;
		break;
	case PancyShaderDescriptorType::SRVPrivate:
		return private_shader_res;
		break;
	case PancyShaderDescriptorType::SRVBindless:
		return bindless_shader_res;
		break;
	default:
		break;
	}
	PancystarEngine::EngineFailReason error_message(E_FAIL, "unrecognized PSO descriptor type");
	PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO Descriptor", error_message);
	return globel_cbuffer;
}
PancystarEngine::EngineFailReason PancyPiplineStateObjectGraph::GetInputDesc(ComPtr<ID3D12ShaderReflection> t_ShaderReflection, std::vector<D3D12_INPUT_ELEMENT_DESC> &t_InputElementDescVec)
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;
	/*
	http://www.cnblogs.com/macom/archive/2013/10/30/3396419.html
	*/
	D3D12_SHADER_DESC t_ShaderDesc;
	hr = t_ShaderReflection->GetDesc(&t_ShaderDesc);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "get desc of shader reflect error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get input desc from shader reflect", error_message);
		return error_message;
	}
	unsigned int t_ByteOffset = 0;
	for (int i = 0; i != t_ShaderDesc.InputParameters; ++i)
	{
		D3D12_SIGNATURE_PARAMETER_DESC t_SP_DESC;
		t_ShaderReflection->GetInputParameterDesc(i, &t_SP_DESC);

		D3D12_INPUT_ELEMENT_DESC t_InputElementDesc;
		t_InputElementDesc.SemanticName = t_SP_DESC.SemanticName;
		t_InputElementDesc.SemanticIndex = t_SP_DESC.SemanticIndex;
		t_InputElementDesc.InputSlot = 0;
		t_InputElementDesc.AlignedByteOffset = t_ByteOffset;
		t_InputElementDesc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		t_InputElementDesc.InstanceDataStepRate = 0;
		if (t_SP_DESC.Mask == 1)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32_FLOAT;
			}
			t_ByteOffset += 4;
		}
		else if (t_SP_DESC.Mask <= 3)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32_FLOAT;
			}
			t_ByteOffset += 8;
		}
		else if (t_SP_DESC.Mask <= 7)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			}
			t_ByteOffset += 12;
		}
		else if (t_SP_DESC.Mask <= 15)
		{
			if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_UINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_SINT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_SINT;
			}
			else if (t_SP_DESC.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32)
			{
				t_InputElementDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			t_ByteOffset += 16;
		}
		t_InputElementDescVec.push_back(t_InputElementDesc);
	}
	return PancystarEngine::succeed;
}
PancyPiplineStateObjectGraph::~PancyPiplineStateObjectGraph()
{
	delete cbuffer_desc_parse;
	for (auto cbuffer_alloctor_value = Cbuffer_map.begin(); cbuffer_alloctor_value != Cbuffer_map.end(); ++cbuffer_alloctor_value) 
	{
		delete cbuffer_alloctor_value->second;
	}
	Cbuffer_map.clear();
}
//effect
PancyEffectGraphic::PancyEffectGraphic()
{
	PSO_id_self_add = 0;
	AddPSOGlobelVariable();
}
void PancyEffectGraphic::AddPSOGlobelVariable()
{
	//PSO��ʽ
	JSON_REFLECT_INIT_ENUM(PSO_TYPE_GRAPHIC);
	JSON_REFLECT_INIT_ENUM(PSO_TYPE_COMPUTE);
	//����������ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_FILL_MODE_WIREFRAME);
	JSON_REFLECT_INIT_ENUM(D3D12_FILL_MODE_SOLID);
	//������������ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_CULL_MODE_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_CULL_MODE_FRONT);
	JSON_REFLECT_INIT_ENUM(D3D12_CULL_MODE_BACK);
	//alpha���ϵ��
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_ZERO);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_ONE);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_SRC_COLOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_INV_SRC_COLOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_SRC_ALPHA);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_INV_SRC_ALPHA);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_DEST_ALPHA);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_INV_DEST_ALPHA);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_DEST_COLOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_INV_DEST_COLOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_SRC_ALPHA_SAT);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_BLEND_FACTOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_INV_BLEND_FACTOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_SRC1_COLOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_INV_SRC1_COLOR);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_SRC1_ALPHA);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_INV_SRC1_ALPHA);
	//alpha��ϲ���
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_OP_ADD);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_OP_SUBTRACT);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_OP_REV_SUBTRACT);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_OP_MIN);
	JSON_REFLECT_INIT_ENUM(D3D12_BLEND_OP_MAX);
	//alpha���logic����
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_CLEAR);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_SET);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_COPY);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_COPY_INVERTED);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_NOOP);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_INVERT);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_AND);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_NAND);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_OR);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_NOR);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_XOR);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_EQUIV);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_AND_REVERSE);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_AND_INVERTED);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_OR_REVERSE);
	JSON_REFLECT_INIT_ENUM(D3D12_LOGIC_OP_OR_INVERTED);
	//alpha���Ŀ������
	JSON_REFLECT_INIT_ENUM(D3D12_COLOR_WRITE_ENABLE_RED);
	JSON_REFLECT_INIT_ENUM(D3D12_COLOR_WRITE_ENABLE_GREEN);
	JSON_REFLECT_INIT_ENUM(D3D12_COLOR_WRITE_ENABLE_BLUE);
	JSON_REFLECT_INIT_ENUM(D3D12_COLOR_WRITE_ENABLE_ALPHA);
	JSON_REFLECT_INIT_ENUM(D3D12_COLOR_WRITE_ENABLE_ALL);
	//��Ȼ�����д����
	JSON_REFLECT_INIT_ENUM(D3D12_DEPTH_WRITE_MASK_ZERO);
	JSON_REFLECT_INIT_ENUM(D3D12_DEPTH_WRITE_MASK_ALL);
	//��Ȼ������ȽϺ���
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_NEVER);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_LESS);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_LESS_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_GREATER);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_NOT_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_GREATER_EQUAL);
	JSON_REFLECT_INIT_ENUM(D3D12_COMPARISON_FUNC_ALWAYS);
	//ģ�建��������
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_KEEP);
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_ZERO);
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_REPLACE);
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_INCR_SAT);
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_DECR_SAT);
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_INVERT);
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_INCR);
	JSON_REFLECT_INIT_ENUM(D3D12_STENCIL_OP_DECR);
	//��ȾͼԪ��ʽ
	JSON_REFLECT_INIT_ENUM(D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED);
	JSON_REFLECT_INIT_ENUM(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
	JSON_REFLECT_INIT_ENUM(D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE);
	JSON_REFLECT_INIT_ENUM(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	JSON_REFLECT_INIT_ENUM(D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
	//������ʽ
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
	//���Բ��Բ���
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_SHARED);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_DENY_BUFFERS);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_DISPLAY);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_HARDWARE_PROTECTED);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES);
	JSON_REFLECT_INIT_ENUM(D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES);
	//Descriptor��ʽ
	JSON_REFLECT_INIT_ENUM(CbufferPrivate);
	JSON_REFLECT_INIT_ENUM(CbufferGlobel);
	JSON_REFLECT_INIT_ENUM(SRVGlobel);
	JSON_REFLECT_INIT_ENUM(SRVPrivate);
	JSON_REFLECT_INIT_ENUM(SRVBindless);
}
PancystarEngine::EngineFailReason PancyEffectGraphic::BuildPso(const std::string &pso_config_file)
{
	PancystarEngine::EngineFailReason check_error;
	//����һ���µ�PSO�ṹ
	PancyPiplineStateObjectGraph *new_pancy = new PancyPiplineStateObjectGraph(pso_config_file);
	check_error = new_pancy->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����ID���Ƿ�Ϊ���������¿��ٵ���ԴID��
	pancy_object_id now_next_id;
	if (empty_PSO_id.size() > 0)
	{
		now_next_id = empty_PSO_id.front();
		empty_PSO_id.pop();
	}
	else
	{
		now_next_id = PSO_id_self_add;
		PSO_id_self_add += 1;
	}
	//����Դ���뵽��Դ��
	PSO_name.insert(std::pair<std::string, pancy_object_id>(pso_config_file, now_next_id));
	PSO_name_reflect.insert(std::pair<pancy_object_id, std::string>(now_next_id, pso_config_file));
	PSO_array.insert(std::pair<pancy_object_id, PancyPiplineStateObjectGraph*>(now_next_id, new_pancy));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::GetPSOName(const pancy_object_id &PSO_id, std::string &pso_name_out)
{
	auto PSO_array_find = PSO_name_reflect.find(PSO_id);
	if (PSO_array_find == PSO_name_reflect.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	pso_name_out = PSO_array_find->second;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::GetPSOResource(const pancy_object_id &PSO_id, ID3D12PipelineState** PSO_res)
{
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	PSO_array_find->second->GetResource(PSO_res);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::GetRootSignatureResource(const pancy_object_id &PSO_id, ID3D12RootSignature** RootSig_res)
{
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	PancyRootSignatureControl::GetInstance()->GetResource(PSO_array_find->second->GetRootSignature(), RootSig_res);
	return PancystarEngine::succeed;
}
/*
PancystarEngine::EngineFailReason PancyEffectGraphic::GetPSODescriptorName(const pancy_object_id &PSO_id, std::string &descriptor_heap_name)
{
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	PSO_array_find->second->GetDescriptorHeapUse(descriptor_heap_name);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::GetDescriptorDistribute(const pancy_object_id &PSO_id, std::vector<pancy_object_id> &descriptor_distribute)
{
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	PSO_array_find->second->GetDescriptorDistribute(descriptor_distribute);
	return PancystarEngine::succeed;
}
*/
PancystarEngine::EngineFailReason PancyEffectGraphic::GetPSO(const std::string &name_in, pancy_object_id &PSO_id)
{
	PancystarEngine::EngineFailReason check_error;
	auto PSO_array_find = PSO_name.find(name_in);
	if (PSO_array_find == PSO_name.end())
	{
		//δ�ҵ�PSO�����Լ���
		check_error = BuildPso(name_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		PSO_array_find = PSO_name.find(name_in);
	}
	PSO_id = PSO_array_find->second;
	return PancystarEngine::succeed;
}
/*
PancystarEngine::EngineFailReason PancyEffectGraphic::CheckCbuffer(const pancy_object_id &PSO_id, const std::string &name_in)
{
	PancystarEngine::EngineFailReason check_error;
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	check_error = PSO_array_find->second->CheckCbuffer(name_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::GetCbuffer(const pancy_object_id &PSO_id, const std::string &cbuffer_name, const Json::Value *& CbufferData)
{
	PancystarEngine::EngineFailReason check_error;
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	check_error = PSO_array_find->second->GetCbuffer(cbuffer_name, CbufferData);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
*/
PancystarEngine::EngineFailReason PancyEffectGraphic::BuildCbufferByName(
	const pancy_object_id &PSO_id,
	const std::string &cbuffer_name,
	PancyConstantBuffer &cbuffer_data_out
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyEffectGraphic::BuildCbufferByName", error_message);
		return error_message;
	}
	check_error = PSO_array_find->second->BuildCbufferByName(cbuffer_name, cbuffer_data_out);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyEffectGraphic::ReleaseCbufferByID(
	const pancy_object_id &PSO_id,
	const std::string &cbuffer_name,
	const pancy_object_id &buffer_resource_id,
	const pancy_object_id &buffer_offset_id
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyEffectGraphic::BuildCbufferByName", error_message);
		return error_message;
	}
	check_error = PSO_array_find->second->ReleaseCbufferByID(cbuffer_name, buffer_resource_id, buffer_offset_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason PancyEffectGraphic::GetDescriptorDesc(
	const pancy_object_id &PSO_id,
	const PancyShaderDescriptorType &descriptor_type,
	const std::vector<PancyDescriptorPSODescription> *&descriptor_param_data
)
{
	auto PSO_array_find = PSO_array.find(PSO_id);
	if (PSO_array_find == PSO_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the PSO ID:" + std::to_string(PSO_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get PSO name", error_message);
		return error_message;
	}
	const std::vector<PancyDescriptorPSODescription> &pre_data = PSO_array_find->second->GetDescriptor(descriptor_type);
	descriptor_param_data = &pre_data;
	return PancystarEngine::succeed;
}
PancyEffectGraphic::~PancyEffectGraphic()
{
	for (auto data = PSO_array.begin(); data != PSO_array.end(); ++data)
	{
		delete data->second;
	}
	PSO_array.clear();
}


ConstantBufferAlloctor::ConstantBufferAlloctor(
	const pancy_resource_size &cbuffer_size_in,
	const std::string &cbuffer_name_in,
	const std::string &cbuffer_effect_name_in,
	Json::Value &buffer_resource_desc_value_in,
	Json::Value &cbuffer_desc_value_in
)
{
	cbuffer_size = cbuffer_size_in;
	cbuffer_name = cbuffer_name_in;
	cbuffer_effect_name = cbuffer_effect_name_in;
	buffer_resource_desc_value = buffer_resource_desc_value_in;
	cbuffer_desc_value = cbuffer_desc_value_in;
}
PancystarEngine::EngineFailReason ConstantBufferAlloctor::BuildNewCbuffer(PancyConstantBuffer &cbuffer_data)
{
	CbufferPackList *now_used_buffer_resource = NULL;
	//�ȴ����е�buffer��Ѱ����û�п��еĴ洢��
	for (auto buffer_resource_data = all_cbuffer_list.begin(); buffer_resource_data != all_cbuffer_list.end(); ++buffer_resource_data)
	{
		if (buffer_resource_data->second->now_empty_offset.size() > 0)
		{
			now_used_buffer_resource = buffer_resource_data->second;
			break;
		}
	}
	//������е�buffer�洢�鶼�Ѿ���ʹ���ˣ����¿�һ���յ�buffer
	if (now_used_buffer_resource == NULL)
	{
		VirtualResourcePointer new_buffer_resource;
		auto check_error = PancyGlobelResourceControl::GetInstance()->LoadResource<PancyBasicBuffer>(
			cbuffer_effect_name + "::" + cbuffer_name,
			buffer_resource_desc_value,
			new_buffer_resource,
			true
			);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_used_buffer_resource = new CbufferPackList(new_buffer_resource, cbuffer_size);
		all_cbuffer_list.insert(std::pair<pancy_object_id, CbufferPackList*>(new_buffer_resource.GetResourceId(), now_used_buffer_resource));
	}
	//���µĴ洢���ָ��ƫ�Ƶ㴴��һ���µ�cbuffer
	auto now_alloc_data = *now_used_buffer_resource->now_empty_offset.begin();
	auto check_error = cbuffer_data.Create(
		cbuffer_name,
		cbuffer_effect_name,
		now_used_buffer_resource->buffer_pointer.GetResourceId(),
		now_alloc_data,
		cbuffer_size,
		cbuffer_desc_value
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//���ָ���Ĵ洢���Ѿ���ʹ��
	now_used_buffer_resource->now_empty_offset.erase(now_alloc_data);
	now_used_buffer_resource->now_use_offset.insert(now_alloc_data);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ConstantBufferAlloctor::ReleaseCbuffer(const pancy_object_id &buffer_resource_id, const pancy_object_id &buffer_offset_id)
{
	//�ȼ���������Դ�Ƿ����Ѿ����ٵ���Դ
	auto buffer_resource_pointer = all_cbuffer_list.find(buffer_resource_id);
	if (buffer_resource_pointer == all_cbuffer_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find buffer resource: " + std::to_string(buffer_resource_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ConstantBufferAlloctor::ReleaseCbuffer", error_message);
		return error_message;
	}
	if (buffer_resource_pointer->second->now_use_offset.find(buffer_offset_id) == buffer_resource_pointer->second->now_use_offset.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find cbuffer offset: " + std::to_string(buffer_offset_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ConstantBufferAlloctor::ReleaseCbuffer", error_message);
		return error_message;
	}
	//����Դ��ƫ�����ͷ�
	buffer_resource_pointer->second->now_use_offset.erase(buffer_offset_id);
	buffer_resource_pointer->second->now_empty_offset.insert(buffer_offset_id);
	//����ͷ���Ϻ���Դû���κ�������ɾ��������Դ
	if (buffer_resource_pointer->second->now_use_offset.size() == 0)
	{
		delete buffer_resource_pointer->second;
		all_cbuffer_list.erase(buffer_resource_pointer);
	}
	return PancystarEngine::succeed;
}
//����������
PancystarEngine::EngineFailReason PancyConstantBuffer::SetMatrix(const std::string &variable, const DirectX::XMFLOAT4X4 &mat_data, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	//����Cbuffer�еľ�����Ҫ��ת�ò���
	DirectX::XMFLOAT4X4 transpose_mat;
	DirectX::XMStoreFloat4x4(&transpose_mat, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mat_data)));
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * sizeof(DirectX::XMFLOAT4X4), &transpose_mat, sizeof(transpose_mat));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::SetFloat4(const std::string &variable, const DirectX::XMFLOAT4 &vector_data, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * sizeof(DirectX::XMFLOAT4), &vector_data, sizeof(vector_data));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::SetUint4(const std::string &variable, const DirectX::XMUINT4 &vector_data, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * sizeof(DirectX::XMUINT4), &vector_data, sizeof(vector_data));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::SetStruct(const std::string &variable, const void* struct_data, const pancy_resource_size &data_size, const pancy_resource_size &offset)
{
	auto start_pos = member_variable.find(variable);
	if (start_pos == member_variable.end())
	{
		return ErrorVariableNotFind(variable);
	}
	memcpy(map_pointer_out + start_pos->second.start_offset + offset * data_size, struct_data, data_size);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::ErrorVariableNotFind(const std::string &variable_name)
{
	PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find shader variable:" + variable_name + " in Cbuffer: " + cbuffer_name + " PSO:" + cbuffer_effect_name);
	PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer Variable", error_message);
	return error_message;
}
PancyConstantBuffer::PancyConstantBuffer()
{
}
PancystarEngine::EngineFailReason PancyConstantBuffer::Create(
	const std::string &cbuffer_name_in,
	const std::string &cbuffer_effect_name_in,
	const pancy_object_id &buffer_id_in,
	const pancy_resource_size &buffer_offset_id_in,
	const pancy_resource_size &cbuffer_size_in,
	const Json::Value &root_value
)
{
	//��������Ϣ
	cbuffer_name = cbuffer_name_in;
	cbuffer_effect_name = cbuffer_effect_name_in;
	buffer_id = buffer_id_in;
	buffer_offset_id = buffer_offset_id_in;
	PancyBasicBuffer *pointer = dynamic_cast<PancyBasicBuffer*>(buffer_ID.GetResourceData());
	cbuffer_size = cbuffer_size_in;
	map_pointer_out = pointer->GetBufferCPUPointer() + buffer_offset_id * cbuffer_size;
	//��json�ļ��ж�ȡcbuffer�Ĳ��ַ���
	PancystarEngine::EngineFailReason check_error;
	std::string cbuffer_final_name = cbuffer_effect_name + "_" + cbuffer_name;
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = GetCbufferDesc(cbuffer_final_name, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if_loaded = true;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyConstantBuffer::GetCbufferDesc(const std::string &file_name, const Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_json_value now_value;
	Json::Value value_cbuffer_desc = root_value.get("CbufferDesc", Json::Value::null);
	if (value_cbuffer_desc == Json::Value::null)
	{
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value: CbufferDesc of variable: " + file_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyConstantBuffer::GetCbufferDesc", error_mesage);
		return error_mesage;
	}
	//��ȡ��������С
	check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_desc, "BufferSize", pancy_json_data_type::json_data_int, now_value);
	cbuffer_size = static_cast<pancy_resource_size>(now_value.int_value);
	//��ȡ���������������б���
	Json::Value value_cbuffer_member = value_cbuffer_desc.get("VariableMember", Json::Value::null);
	if (value_cbuffer_member == Json::Value::null)
	{
		PancystarEngine::EngineFailReason error_mesage(E_FAIL, "could not find value: VariableMember of variable: " + file_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyConstantBuffer::GetCbufferDesc", error_mesage);
		return error_mesage;
	}
	for (int32_t i = 0; i < value_cbuffer_member.size(); ++i)
	{
		CbufferVariable new_variable_data;
		std::string variable_name;
		Json::Value value_cbuffer_variable;
		value_cbuffer_variable = value_cbuffer_member.get(i, Json::nullValue);
		//��ȡ��������
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_variable, "Name", pancy_json_data_type::json_data_string, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		variable_name = now_value.string_value;
		//��ȡ������С
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_variable, "Size", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_variable_data.variable_size = now_value.int_value;
		//��ȡ����ƫ��
		check_error = PancyJsonTool::GetInstance()->GetJsonData(file_name, value_cbuffer_variable, "StartOffset", pancy_json_data_type::json_data_int, now_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_variable_data.start_offset = now_value.int_value;
		//��¼��ǰ���±���
		member_variable.insert(std::pair<std::string, CbufferVariable>(variable_name, new_variable_data));
	}
	return PancystarEngine::succeed;
}
PancyConstantBuffer::~PancyConstantBuffer()
{
	if (if_loaded)
	{
		pancy_object_id effect_pso_id;
		auto check_error = PancyEffectGraphic::GetInstance()->GetPSO(cbuffer_effect_name, effect_pso_id);
		if (!check_error.CheckIfSucceed()) 
		{
			return;
		}
		check_error = PancyEffectGraphic::GetInstance()->ReleaseCbufferByID(effect_pso_id, cbuffer_name, buffer_id, buffer_offset_id);
		if (!check_error.CheckIfSucceed())
		{
			return;
		}
		member_variable.clear();
	}
}
