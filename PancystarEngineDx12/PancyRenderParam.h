#pragma once
#include"PancyDescriptor.h"
#include"PancyShaderDx12.h"
namespace PancystarEngine
{
	class BasicRenderParam
	{
		std::string render_param_name;
		//��Ⱦ����
		std::string          PSO_name;
		ID3D12PipelineState  *PSO_pointer = NULL;
		ID3D12RootSignature  *rootsignature = NULL;
		//��������
		ID3D12DescriptorHeap *descriptor_heap_use = NULL;
		//��Ⱦ����������������������жϵ�ǰ����Ⱦ��Ԫ�Ƿ��Ѿ�ע�����
		bool if_render_param_inited = false;
		pancy_object_id globel_cbuffer_num = 99999;
		pancy_object_id private_cbuffer_num = 99999;
		pancy_object_id globel_shader_resource_num = 99999;
		pancy_object_id bind_shader_resource_num = 99999;
		pancy_object_id bindless_shader_resource_num = 99999;
		//��Ⱦ���������������
		std::unordered_map<std::string, BindDescriptorPointer> globel_constant_buffer;       //ȫ�ֳ���������
		std::unordered_map<std::string, BindDescriptorPointer> private_constant_buffer;      //˽�г���������
		std::unordered_map<std::string, BindDescriptorPointer> globel_shader_resource;       //ȫ��������
		std::unordered_map<std::string, BindDescriptorPointer> bind_shader_resource;         //˽��������
		std::unordered_map<std::string, BindlessDescriptorPointer> bindless_shader_resource; //���������
		//��Ⱦ��Ҫ�󶨵�rootsignature slot
		std::unordered_map<std::string, pancy_object_id> globel_constant_buffer_root_signature_offset;    //ȫ�ֳ���������slot
		std::unordered_map<std::string, pancy_object_id> private_constant_buffer_root_signature_offset;   //˽�г���������slot
		std::unordered_map<std::string, pancy_object_id> globel_shader_resource_root_signature_offset;    //ȫ��������slot
		std::unordered_map<std::string, pancy_object_id> bind_shader_resource_root_signature_offset;      //˽��������slot
		std::unordered_map<std::string, pancy_object_id> bindless_shader_resource_root_signature_offset;  //���������slot
		//˽�д洢��Դ
		std::unordered_map<std::string, std::vector<PancyConstantBuffer*>> per_object_cbuffer;//ÿ�������������cbuffer����Ҫ����������Ƭ�洢����
	public:
		BasicRenderParam(const std::string &render_param_name_in);
		~BasicRenderParam();
		PancystarEngine::EngineFailReason GetPsoData(ID3D12PipelineState  **pso_data);
		PancystarEngine::EngineFailReason SetCbufferMatrix(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4X4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferFloat4(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferUint4(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMUINT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferStructData(
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const void* data_in,
			const pancy_resource_size &data_size,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason CommonCreate(
			const std::string &PSO_name,
			const std::unordered_map<std::string, BindDescriptorPointer> &bind_shader_resource_in,
			const std::unordered_map<std::string, BindlessDescriptorPointer> &bindless_shader_resource_in
		);
		PancystarEngine::EngineFailReason AddToCommandList(PancyRenderCommandList *m_commandList, const D3D12_COMMAND_LIST_TYPE &render_param_type);
	private:
		//������������Ⱦ����
		PancystarEngine::EngineFailReason BindDescriptorToRootsignature(
			const PancyDescriptorType &bind_descriptor_type,
			const std::unordered_map<std::string, BindDescriptorPointer> &descriptor_data,
			const std::unordered_map<std::string, pancy_object_id> &root_signature_slot_data,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList
		);
		//�󶨽������������Ⱦ����
		PancystarEngine::EngineFailReason BindBindlessDescriptorToRootsignature(
			const PancyDescriptorType &bind_descriptor_type,
			const std::unordered_map<std::string, BindlessDescriptorPointer> &descriptor_data,
			const std::unordered_map<std::string, pancy_object_id> &root_signature_slot_data,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList
		);
		//��鵱ǰ����Ⱦ��Ԫ�Ƿ��Ѿ�ע�����
		bool CheckIfInitFinished();
	};

	struct PancyRenderParamID
	{
		//��Ⱦ״̬ID��
		pancy_object_id PSO_id;
		pancy_object_id render_param_id;
	};
	class RenderParamSystem
	{
		//�洢ÿһ��psoΪ��ͬ������������������������
		std::unordered_map<pancy_object_id, pancy_object_id> render_param_id_self_add;
		std::unordered_map<pancy_object_id, std::queue<pancy_object_id>> render_param_id_reuse_table;
		std::unordered_map<pancy_object_id, std::unordered_map<std::string, pancy_object_id>> render_param_name_table;
		std::unordered_map<pancy_object_id, std::unordered_map<pancy_object_id, BasicRenderParam*>> render_param_table;
	private:
		RenderParamSystem();
	public:
		static RenderParamSystem* GetInstance()
		{
			static RenderParamSystem* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new RenderParamSystem();
			}
			return this_instance;
		}
		~RenderParamSystem();
		PancystarEngine::EngineFailReason GetCommonRenderParam(
			const std::string &PSO_name,
			const std::string &render_param_name,
			const std::unordered_map<std::string, BindDescriptorPointer> &bind_shader_resource_in,
			const std::unordered_map<std::string, BindlessDescriptorPointer> &bindless_shader_resource_in,
			PancyRenderParamID &render_param_id
		);
		PancystarEngine::EngineFailReason AddRenderParamToCommandList(
			const PancyRenderParamID &renderparam_id,
			PancyRenderCommandList *m_commandList,
			const D3D12_COMMAND_LIST_TYPE &render_param_type
		);
		PancystarEngine::EngineFailReason GetPsoData(
			const PancyRenderParamID &renderparam_id,
			ID3D12PipelineState  **pso_data
		);
		PancystarEngine::EngineFailReason SetCbufferMatrix(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4X4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferFloat4(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMFLOAT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferUint4(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const DirectX::XMUINT4 &data_in,
			const pancy_resource_size &offset
		);
		PancystarEngine::EngineFailReason SetCbufferStructData(
			const PancyRenderParamID &renderparam_id,
			const std::string &cbuffer_name,
			const std::string &variable_name,
			const void* data_in,
			const pancy_resource_size &data_size,
			const pancy_resource_size &offset
		);
		//todo:ʹ�����ü���ɾ������Ҫ����Ⱦ��Ԫ
		PancystarEngine::EngineFailReason DeleteCommonRenderParam(PancyRenderParamID &render_param_id);
	private:
		PancystarEngine::EngineFailReason GetResource(const PancyRenderParamID &renderparam_id, BasicRenderParam** data_pointer);
	};
	//ȫ�ֹ�������������
	class PancySkinAnimationControl
	{
		pancy_resource_size animation_buffer_size;                    //�洢��������Ļ�������С
		pancy_resource_size bone_buffer_size;                         //�洢��������Ļ�������С
		pancy_object_id PSO_skinmesh;                                 //������������Ⱦ״̬��
		std::vector<PancySkinAnimationBuffer*> skin_naimation_buffer; //���������Ļ�������Ϣ
		PancySkinAnimationControl(
			const pancy_resource_size &animation_buffer_size_in,
			const pancy_resource_size &bone_buffer_size_in
		);
		PancystarEngine::EngineFailReason Create();
	public:
		static PancySkinAnimationControl *this_instance;
		static PancystarEngine::EngineFailReason SingleCreate(
			const pancy_resource_size &animation_buffer_size_in,
			const pancy_resource_size &bone_buffer_size_in
		)
		{
			if (this_instance != NULL)
			{

				PancystarEngine::EngineFailReason error_message(E_FAIL, "the d3d input instance have been created before");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create directx input object", error_message);
				return error_message;
			}
			else
			{
				this_instance = new PancySkinAnimationControl(animation_buffer_size_in, bone_buffer_size_in);
				PancystarEngine::EngineFailReason check_error = this_instance->Create();
				return check_error;
			}
		}
		static PancySkinAnimationControl * GetInstance()
		{
			return this_instance;
		}
		/*
		PancystarEngine::EngineFailReason BuildDescriptor(
			const pancy_object_id &mesh_buffer,
			const UINT &vertex_num,
			const UINT &per_vertex_size,
			pancy_object_id &descriptor_id
		);

		//��ȡ��Ƥ���������
		PancystarEngine::EngineFailReason GetSkinAnimationBuffer(std::vector<SubMemoryPointer> &skin_animation_data, pancy_resource_size &animation_buffer_size_in);
		*/
		//��յ�ǰ֡�Ļ�����ʹ����Ϣ
		void ClearUsedBuffer();
		//�����Ⱦcommandlist
		PancystarEngine::EngineFailReason BuildCommandList(
			const pancy_object_id &mesh_buffer,
			const pancy_object_id &vertex_num,
			const PancyRenderParamID &render_param_id,
			const pancy_resource_size &matrix_num,
			const DirectX::XMFLOAT4X4 *matrix_data,
			SkinAnimationBlock &new_animation_block,
			PancyRenderCommandList *m_commandList_skin
		);
		~PancySkinAnimationControl();
	private:
		//�ӵ�ǰ��Ƥ���������������һ��������(��Ƥ�����������GPU������ݣ����ֻ��Ҫ����)
		PancystarEngine::EngineFailReason BuildAnimationBlock(
			const pancy_resource_size &vertex_num,
			pancy_object_id &block_id,
			SkinAnimationBlock &animation_block_pos
		);
		//�ӵ�ǰ�������󻺳���������һ��������(����������������CPU������ݣ������Ҫ���������һ������)
		PancystarEngine::EngineFailReason BuildBoneBlock(
			const pancy_resource_size &matrix_num,
			const DirectX::XMFLOAT4X4 *matrix_data,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_bone_block
		);
	};
}