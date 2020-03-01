#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
namespace PancystarEngine 
{
	enum PancyBufferType 
	{
		Buffer_ShaderResource_static = 0,
		Buffer_ShaderResource_dynamic,
		Buffer_Constant,
		Buffer_Vertex,
		Buffer_Index,
		Buffer_UnorderedAccess_static
	};
	struct PancyCommonBufferDesc 
	{
		PancyBufferType buffer_type;
		D3D12_RESOURCE_DESC buffer_res_desc = {};
		std::string buffer_data_file;
	};
	class CommonBufferJsonReflect :public PancyJsonReflectTemplate<PancyCommonBufferDesc>
	{ 
	public:
		CommonBufferJsonReflect();
	private:
		PancystarEngine::EngineFailReason InitChildReflectClass() override;
		void InitBasicVariable() override;
	};
	CommonBufferJsonReflect::CommonBufferJsonReflect() 
	{
		
	}
	PancystarEngine::EngineFailReason CommonBufferJsonReflect::InitChildReflectClass()
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
	//��������Դ
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		pancy_resource_size subresources_size = 0;
		UINT8* map_pointer = NULL;
		ResourceBlockGpu *buffer_data = nullptr;     //buffer����ָ��
	public:
		PancyBasicBuffer(const bool &if_could_reload);
		~PancyBasicBuffer();
		inline const pancy_resource_size GetBufferSize() const
		{
			return subresources_size;
		}
		inline UINT8* GetBufferCPUPointer() const
		{
			return map_pointer;
		}
		inline ResourceBlockGpu *GetGpuResourceData() const
		{
			return buffer_data;
		}
		//��⵱ǰ����Դ�Ƿ��Ѿ�������GPU
		bool CheckIfResourceLoadFinish() override;
	private:
		void BuildJsonReflect(PancyJsonReflect **pointer_data) override;
		PancystarEngine::EngineFailReason InitResource() override;
	};

	struct SkinAnimationBlock 
	{
		pancy_resource_size start_pos;
		pancy_resource_size block_size;
	};
	//���㶯������
	struct mesh_animation_data
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 tangent;
		//float delta_time;
		mesh_animation_data()
		{
			position = DirectX::XMFLOAT3(0, 0, 0);
			normal = DirectX::XMFLOAT3(0, 0, 0);
			tangent = DirectX::XMFLOAT3(0, 0, 0);
		}
	};
	//��������������
	class PancySkinAnimationBuffer
	{
		//�������Ĵ�С
		pancy_resource_size animation_buffer_size;//�洢��Ƥ����Ļ������Ĵ�С
		pancy_resource_size bone_buffer_size;//�洢��������Ļ������Ĵ�С

		//��ǰ�Ѿ���ռ�õ�ָ��λ��
		pancy_resource_size now_used_position_animation;//��ǰ���������������ʹ�����ָ��
		pancy_resource_size now_used_position_bone;//��ǰ�������󻺳�����ʹ�����ָ��

		//�洢ÿһ������������Compute Shader����λ��
		std::unordered_map<pancy_object_id, SkinAnimationBlock> animation_block_map;
		//�洢ÿһ�����������������ʼλ��
		std::unordered_map<pancy_object_id, SkinAnimationBlock> bone_block_map;
		//��������������������
		VirtualResourcePointer buffer_animation;//�������������
		VirtualResourcePointer buffer_bone;     //�������󻺳���

		//�������ݵ�CPUָ��
		UINT8* bone_data_pointer;
	public:
		PancySkinAnimationBuffer(const pancy_resource_size &animation_buffer_size_in, const pancy_resource_size &bone_buffer_size_in);
		~PancySkinAnimationBuffer();
		PancystarEngine::EngineFailReason Create();
		//��յ�ǰ����ʹ�õĹ�����������(���ڶ���������֡���ã�����Ҫ�������Ѱַ���͵���ɾ���)
		void ClearUsedBuffer();
		//�ӵ�ǰ��Ƥ���������������һ��������(��Ƥ�����������GPU������ݣ����ֻ��Ҫ����)
		PancystarEngine::EngineFailReason BuildAnimationBlock(
			const pancy_resource_size &vertex_num,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_animation_block
		);
		//�ӵ�ǰ�������󻺳���������һ��������(����������������CPU������ݣ������Ҫ���������һ������)
		PancystarEngine::EngineFailReason BuildBoneBlock(
			const pancy_resource_size &matrix_num, 
			const DirectX::XMFLOAT4X4 *matrix_data,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_bone_block
		);
		//��ȡ����洢������
		inline VirtualResourcePointer& GetBoneMatrixResource() 
		{
			return buffer_bone;
		}
		//��ȡ��Ƥ���������
		inline VirtualResourcePointer& GetSkinVertexResource()
		{
			return buffer_animation;
		}
	};
	
}