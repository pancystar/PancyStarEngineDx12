#pragma once
#include"PancyBufferDx12.h"
#include"PancyRenderParam.h"
namespace PancystarEngine 
{
	//����������������
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
		//��յ�ǰ֡�Ļ�����ʹ����Ϣ
		void ClearUsedBuffer();
		//�����Ⱦcommandlist
		PancystarEngine::EngineFailReason BuildCommandList(
			const VirtualResourcePointer &mesh_buffer,
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