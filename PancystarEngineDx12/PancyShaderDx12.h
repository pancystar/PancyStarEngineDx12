#pragma once
#include"PancyDx12Basic.h"
#include"PancyJsonTool.h"
#include"PancyMemoryBasic.h"
//几何体的格式对接类型
struct PancyVertexBufferDesc
{
	std::string vertex_desc_name;
	size_t input_element_num;
	D3D12_INPUT_ELEMENT_DESC *inputElementDescs = NULL;
};
//几何体格式管理器(用于注册顶点)
class InputLayoutDesc
{
	std::unordered_map<std::string, PancyVertexBufferDesc> vertex_buffer_desc_map;
private:
	InputLayoutDesc();
public:
	static InputLayoutDesc* GetInstance()
	{
		static InputLayoutDesc* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new InputLayoutDesc();
		}
		return this_instance;
	}
	~InputLayoutDesc();
	void AddVertexDesc(std::string vertex_desc_name_in, std::vector<D3D12_INPUT_ELEMENT_DESC> input_element_desc_list);
	inline const PancyVertexBufferDesc* GetVertexDesc(std::string vertex_desc_name_in)
	{
		auto new_vertex_desc = vertex_buffer_desc_map.find(vertex_desc_name_in);
		if (new_vertex_desc != vertex_buffer_desc_map.end())
		{
			return &new_vertex_desc->second;
		}
		return NULL;
	}
};
class PancyShaderBasic
{
	PancystarEngine::PancyString shader_file_name;
	PancystarEngine::PancyString shader_entry_point_name;
	PancystarEngine::PancyString shader_type_name;
	ComPtr<ID3DBlob> shader_memory_pointer;
	ComPtr<ID3D12ShaderReflection> shader_reflection;
public:
	PancyShaderBasic
	(
		const std::string &shader_file_in,
		const std::string &main_func_name,
		const std::string &shader_type
	);
	ComPtr<ID3DBlob> GetShader()
	{
		return shader_memory_pointer;
	}
	ComPtr<ID3D12ShaderReflection> GetShaderReflect()
	{
		return shader_reflection;
	}
	PancystarEngine::EngineFailReason Create();
};
class PancyShaderControl
{
	std::unordered_map<std::string, PancyShaderBasic*> shader_list;
	PancyShaderControl();
public:
	static PancyShaderControl* GetInstance()
	{
		static PancyShaderControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyShaderControl();
		}
		return this_instance;
	}
	~PancyShaderControl();
	PancystarEngine::EngineFailReason LoadShader(std::string shader_file, std::string shader_main_func, std::string shader_type);
	ComPtr<ID3D12ShaderReflection> GetShaderReflection(std::string shader_file, std::string shader_main_func);
	ComPtr<ID3DBlob> GetShaderData(std::string shader_file, std::string shader_main_func);
};

//RootSignature
class PancyRootSignature
{
	pancy_resource_id descriptor_heap_id;
	PancystarEngine::PancyString root_signature_name;
	ComPtr<ID3D12RootSignature> root_signature_data;
public:
	PancyRootSignature(const std::string &file_name);
	inline ComPtr<ID3D12RootSignature> GetRootSignature() 
	{
		return root_signature_data;
	};
	PancystarEngine::EngineFailReason Create(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootSignatureDesc,const pancy_resource_id &descriptor_heap_id_in);
};
//RootSignature管理器
class PancyRootSignatureControl
{
	D3D12_STATIC_SAMPLER_DESC *data_sampledesc;
	CD3DX12_DESCRIPTOR_RANGE1 *ranges;
	CD3DX12_ROOT_PARAMETER1 *rootParameters;
	std::unordered_map<std::string, PancyRootSignature*> root_signature_array;
private:
	PancyRootSignatureControl();
public:
	static PancyRootSignatureControl* GetInstance()
	{
		static PancyRootSignatureControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyRootSignatureControl();
		}
		return this_instance;
	}
	PancyRootSignature* GetRootSignature(std::string name_in);
	PancystarEngine::EngineFailReason BuildRootSignature(std::string rootsig_config_file);
	PancystarEngine::EngineFailReason GetDesc(
		const std::string &file_name,
		CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &desc_out,
		pancy_resource_id &descriptor_heap_id
	);
	~PancyRootSignatureControl();
private:
	void AddRootSignatureGlobelVariable();
};
//PSO object
class PancyPiplineStateObjectGraph
{
	PancystarEngine::PancyString root_signature_name;
	PancystarEngine::PancyString pso_name;
	ComPtr<ID3D12PipelineState> pso_data;
public:
	PancyPiplineStateObjectGraph(std::string pso_name_in);
	PancystarEngine::EngineFailReason Create(const D3D12_GRAPHICS_PIPELINE_STATE_DESC &pso_desc_in);
	inline ComPtr<ID3D12PipelineState> GetData() 
	{
		return pso_data;
	}
};
//pso管理器
class PancyEffectGraphic
{
	std::unordered_map<std::string, PancyPiplineStateObjectGraph*> PSO_array;
private:
	PancyEffectGraphic();
public:
	static PancyEffectGraphic* GetInstance()
	{
		static PancyEffectGraphic* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyEffectGraphic();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildPso(std::string pso_config_file);
	PancystarEngine::EngineFailReason GetDesc(const std::string &file_name, D3D12_GRAPHICS_PIPELINE_STATE_DESC &desc_out);
	PancystarEngine::EngineFailReason GetInputDesc(ComPtr<ID3D12ShaderReflection> t_ShaderReflection, std::vector<D3D12_INPUT_ELEMENT_DESC> &t_InputElementDescVec);
	PancyPiplineStateObjectGraph* GetPSO(std::string name_in);
	~PancyEffectGraphic();
private:
	void AddPSOGlobelVariable();
};



