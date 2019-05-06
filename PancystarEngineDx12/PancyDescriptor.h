#pragma once

#include"PancyModelBasic.h"
namespace PancystarEngine 
{
	//����������
	class DescriptorObject
	{
		//�����ύ��Ⱦ��ֱ����Ϣ
		ID3D12PipelineState  *PSO_pointer;
		ID3D12RootSignature  *rootsignature;
		ID3D12DescriptorHeap *descriptor_heap_use;
		std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> descriptor_offset;
		//��Դ������Ϣ
		std::string PSO_name_descriptor;
		ResourceViewPack descriptor_block_id;
		pancy_object_id resource_view_num;
		std::unordered_map<std::string, PancystarEngine::PancyConstantBuffer*> per_object_cbuffer;//ÿ�������������cbuffer����Ҫ����������Ƭ�洢����
		std::unordered_map<std::string, pancy_object_id> per_object_cbuffer_index;
	public:
		DescriptorObject();
		~DescriptorObject();
		ID3D12PipelineState  *GetPSO()
		{
			return PSO_pointer;
		}
		ID3D12RootSignature *GetRootSignature()
		{
			return rootsignature;
		}
		ID3D12DescriptorHeap *GetDescriptoHeap()
		{
			return descriptor_heap_use;
		}
		std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> GetDescriptorOffset()
		{
			return descriptor_offset;
		}
		PancystarEngine::EngineFailReason Create(
			const std::string &PSO_name,
			const std::string &descriptor_name,
			const std::vector<std::string> &cbuffer_name_per_object,
			const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame,
			const std::vector<SubMemoryPointer> &resource_data_per_frame,
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
			const std::vector<SubMemoryPointer> &output_data_per_frame,
			const std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> &output_desc_per_frame_in,
			const std::vector<SubMemoryPointer> &resource_data_per_object,
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
		);
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
	};
	//������������
	class DescriptorControl 
	{
		pancy_object_id now_object_id_top;
		std::queue<pancy_object_id> empty_object_id;
		std::vector<std::unordered_map<pancy_object_id, DescriptorObject *>> descriptor_data_map;
		DescriptorControl();
	public:
		~DescriptorControl();
		static DescriptorControl* GetInstance()
		{
			static DescriptorControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new DescriptorControl();
			}
			return this_instance;
		}
		//����������ɫ������(����Ҫ����ģ������)
		PancystarEngine::EngineFailReason BuildDescriptorCompute(
			const pancy_object_id &PSO_id,
			const std::vector<std::string> &cbuffer_name_per_object_in,
			//���ܻ������������������Դ
			const std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> &cbuffer_per_frame_in,
			const std::vector<std::vector<SubMemoryPointer>> &SRV_per_frame_in,
			const std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> &SRV_desc_per_frame_in,
			const std::vector<std::vector<SubMemoryPointer>> &UAV_per_frame_in,
			const std::vector<std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC>> &UAV_desc_per_frame_in,
			pancy_object_id &descriptor_ID
		);
		//����3D��ɫ������(��Ҫ����ģ������)
		PancystarEngine::EngineFailReason BuildDescriptorGraph(
			const pancy_object_id &model_id,
			const pancy_object_id &PSO_id,
			const std::vector<std::string> &cbuffer_name_per_object_in,
			//���ܻ������������������Դ
			const std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> &cbuffer_per_frame_in,
			const std::vector<std::vector<SubMemoryPointer>> &resource_data_per_frame_in,
			const std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> &resource_desc_per_frame_in,
			pancy_object_id &descriptor_ID
		);
		//��ȡ������
		PancystarEngine::EngineFailReason GetDescriptor(const pancy_object_id &descriptor_id, DescriptorObject **descriptor_data);
		//ɾ��������
		PancystarEngine::EngineFailReason DeleteDescriptor(const pancy_object_id &descriptor_id);
	};
	//ȫ�ֹ�������������
	class PancySkinAnimationControl
	{
		pancy_resource_size animation_buffer_size;                    //�洢��������Ļ�������С
		pancy_resource_size bone_buffer_size;                         //�洢��������Ļ�������С
		pancy_object_id PSO_skinmesh;                                 //������������Ⱦ״̬��
		std::vector<PancySkinAnimationBuffer*> skin_naimation_buffer; //���������Ļ�������Ϣ
		pancy_object_id compute_descriptor;                           //���ڼ�����ƤЧ������������Ϣ
	public:
		PancySkinAnimationControl(
			const pancy_resource_size &animation_buffer_size_in,
			const pancy_resource_size &bone_buffer_size_in
		);
		PancystarEngine::EngineFailReason Create();
		PancystarEngine::EngineFailReason BuildDescriptor(
			const pancy_object_id &mesh_buffer,
			const UINT &vertex_num,
			const UINT &per_vertex_size,
			pancy_object_id &descriptor_id
		);
		~PancySkinAnimationControl();
	};
	
	PancySkinAnimationControl::PancySkinAnimationControl(
		const pancy_resource_size &animation_buffer_size_in,
		const pancy_resource_size &bone_buffer_size_in
	)
	{
		animation_buffer_size = animation_buffer_size_in;
		bone_buffer_size = bone_buffer_size_in;
		pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
		skin_naimation_buffer.resize(Frame_num);
	}

	PancystarEngine::EngineFailReason PancySkinAnimationControl::Create()
	{
		PancystarEngine::EngineFailReason check_error;
		//����PSO
		check_error = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json", PSO_skinmesh);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//������������������
		pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
		for (int i = 0; i < Frame_num; ++i) 
		{
			skin_naimation_buffer[i] = new PancySkinAnimationBuffer(animation_buffer_size, bone_buffer_size);
			check_error = skin_naimation_buffer[i]->Create();
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason PancySkinAnimationControl::BuildDescriptor(
		const pancy_object_id &mesh_buffer,
		const UINT &vertex_num,
		const UINT &per_vertex_size,
		pancy_object_id &descriptor_id
	)
	{
		PancystarEngine::EngineFailReason check_error;
		//���ݻ�������ID�Ż�ȡ���Ӧ���Դ���Դ
		SubMemoryPointer now_buffer_data;
		check_error = PancystarEngine::PancyBasicBufferControl::GetInstance()->GetBufferSubResource(mesh_buffer, now_buffer_data);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//Ϊ���뻺��������������
		std::vector<std::string> Cbuffer_name_per_object;
		std::vector<std::vector<PancystarEngine::PancyConstantBuffer *>> Cbuffer_per_frame;
		Cbuffer_name_per_object.push_back("per_object");
		//ȫ�ֵĻ�����/��������
		std::vector<std::vector<SubMemoryPointer>> SRV_per_frame;
		std::vector<std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC>> SRV_desc_per_frame;
		//ȫ�����������/����(UAV)
		std::vector<std::vector<SubMemoryPointer>> UAV_per_frame;
		std::vector<std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC>> UAV_desc_per_frame;
		pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
		for (int i = 0; i < Frame_num; ++i) 
		{
			//��ǰ������ģ�Ͷ�����Ϣ
			D3D12_SHADER_RESOURCE_VIEW_DESC  now_buffer_desc = {};
			now_buffer_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			now_buffer_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
			now_buffer_desc.Buffer.StructureByteStride = per_vertex_size;
			now_buffer_desc.Buffer.NumElements = vertex_num;
			now_buffer_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			now_buffer_desc.Buffer.FirstElement = 0;

			SRV_per_frame[i].push_back(now_buffer_data);
			SRV_desc_per_frame[i].push_back(now_buffer_desc);
			//�������󻺳���
			SubMemoryPointer bone_matrix_buffer_res;
			D3D12_SHADER_RESOURCE_VIEW_DESC  bone_matrix_buffer_desc = {};
			skin_naimation_buffer[i]->GetBoneMatrixResource(bone_matrix_buffer_res);
			SRV_per_frame[i].push_back(bone_matrix_buffer_res);

			bone_matrix_buffer_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			bone_matrix_buffer_desc.ViewDimension = D3D12_SRV_DIMENSION::D3D12_SRV_DIMENSION_BUFFER;
			bone_matrix_buffer_desc.Buffer.StructureByteStride = sizeof(DirectX::XMFLOAT4X4);
			pancy_resource_size matrix_num = bone_buffer_size / sizeof(DirectX::XMFLOAT4X4);
			bone_matrix_buffer_desc.Buffer.NumElements = matrix_num;
			bone_matrix_buffer_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			bone_matrix_buffer_desc.Buffer.FirstElement = 0;

			SRV_desc_per_frame[i].push_back(bone_matrix_buffer_desc);
			//��Ƥ���������
			SubMemoryPointer skin_vertex_buffer_res;
			skin_naimation_buffer[i]->GetSkinVertexResource(skin_vertex_buffer_res);
			UAV_per_frame[i].push_back(skin_vertex_buffer_res);
			D3D12_UNORDERED_ACCESS_VIEW_DESC skin_vertex_buffer_desc = {};
			skin_vertex_buffer_desc.ViewDimension = D3D12_UAV_DIMENSION::D3D12_UAV_DIMENSION_BUFFER;
			skin_vertex_buffer_desc.Buffer.StructureByteStride = sizeof(mesh_animation_data);
			pancy_resource_size vertex_num = animation_buffer_size / sizeof(mesh_animation_data);
			skin_vertex_buffer_desc.Buffer.NumElements = vertex_num;
			skin_vertex_buffer_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
			skin_vertex_buffer_desc.Buffer.FirstElement = 0;
			UAV_desc_per_frame[i].push_back(skin_vertex_buffer_desc);
		}
		check_error = PancystarEngine::DescriptorControl::GetInstance()->BuildDescriptorCompute(
			PSO_skinmesh,
			Cbuffer_name_per_object,
			Cbuffer_per_frame,
			SRV_per_frame,
			SRV_desc_per_frame,
			UAV_per_frame,
			UAV_desc_per_frame,
			descriptor_id
			);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	
}
