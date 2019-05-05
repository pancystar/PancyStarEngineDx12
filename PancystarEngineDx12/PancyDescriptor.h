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
	//��������
	/*
	class DescriptorObjectList
	{
		std::string PSO_name;
		std::string descriptor_name;
		//���е�����������
		std::queue<DescriptorObject*> empty_list;
		//����ʹ�õ�����������
		std::queue<DescriptorObject*> used_list;
		//���������󶨵���Դ(�ⲿ��Դ,����Ҫ�ͷ�)
		std::vector<std::string> cbuffer_name_per_object;
		std::vector<PancystarEngine::PancyConstantBuffer *> cbuffer_per_frame;
		std::vector<SubMemoryPointer> resource_data_per_frame;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> resource_desc_per_frame;
		std::vector<SubMemoryPointer> resource_data_per_object;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> resource_desc_per_per_object;
	public:
		DescriptorObjectList(
			const std::string &PSO_name_in,
			const std::string &descriptor_name_in
		);
		~DescriptorObjectList();
		PancystarEngine::EngineFailReason Create(
			const std::vector<std::string> &cbuffer_name_per_object,
			const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame,
			const std::vector<SubMemoryPointer> &resource_data_per_frame,
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
			const std::vector<SubMemoryPointer> &resource_data_per_object_in,
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
		);
		PancystarEngine::EngineFailReason GetEmptyList(DescriptorObject** descripto_res);
		void Reset();
	};
	*/
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
			const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
			const std::vector<SubMemoryPointer> &SRV_per_frame_in,
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc_per_frame_in,
			const std::vector<SubMemoryPointer> &UAV_per_frame_in,
			const std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> &UAV_desc_per_frame_in,
			pancy_object_id &descriptor_ID
		);
		//����3D��ɫ������(��Ҫ����ģ������)
		PancystarEngine::EngineFailReason BuildDescriptorGraph(
			const pancy_object_id &model_id,
			const pancy_object_id &PSO_id,
			const std::vector<std::string> &cbuffer_name_per_object_in,
			const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
			const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
			pancy_object_id &descriptor_ID
		);
		//��ȡ������
		PancystarEngine::EngineFailReason GetDescriptor(const pancy_object_id &descriptor_id, DescriptorObject **descriptor_data);
		//ɾ��������
		PancystarEngine::EngineFailReason DeleteDescriptor(const pancy_object_id &descriptor_id);
	};
	
}
