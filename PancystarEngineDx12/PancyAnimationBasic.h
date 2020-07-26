#pragma once
#include"PancyBufferDx12.h"
#include"PancyRenderParam.h"
namespace PancystarEngine 
{
#define MaxSkinAnimationComputeNum 4096
	//骨骼动画缓冲区块
	struct SkinAnimationBlock
	{
		pancy_resource_size start_pos;
		pancy_resource_size block_size;
	};
	//顶点动画数据
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
	//骨骼动画缓冲区
	class PancySkinAnimationBuffer
	{
		//缓冲区的大小
		pancy_resource_size animation_buffer_size;//存储蒙皮结果的缓冲区的大小
		pancy_resource_size bone_buffer_size;//存储骨骼矩阵的缓冲区的大小

		//当前已经被占用的指针位置
		pancy_resource_size now_used_position_animation;//当前动画结果缓冲区的使用情况指针
		pancy_resource_size now_used_position_bone;//当前骨骼矩阵缓冲区的使用情况指针

		//存储每一个骨骼动画的Compute Shader计算位置
		std::unordered_map<pancy_object_id, SkinAnimationBlock> animation_block_map;
		//存储每一个骨骼矩阵区域的起始位置
		std::unordered_map<pancy_object_id, SkinAnimationBlock> bone_block_map;
		//骨骼动画缓冲区的数据
		VirtualResourcePointer buffer_animation;         //动画结果缓冲区
		std::vector<VirtualResourcePointer> buffer_bone; //骨骼矩阵缓冲区
		std::vector<VirtualResourcePointer> buffer_globel_index;      //节点全局序号缓冲区

		PancystarEngine::BindDescriptorPointer bone_matrix_descriptor_uav;
		PancystarEngine::BindDescriptorPointer globel_id_descriptor_uav;

		PancystarEngine::BindDescriptorPointer bone_matrix_descriptor_srv;
		PancystarEngine::BindDescriptorPointer globel_id_descriptor_srv;
	public:
		PancySkinAnimationBuffer(const pancy_resource_size &animation_buffer_size_in, const pancy_resource_size &bone_buffer_size_in);
		~PancySkinAnimationBuffer();
		PancystarEngine::EngineFailReason Create();
		//清空当前所有使用的骨骼动画数据(由于动画数据逐帧重置，不需要考虑随机寻址类型的增删查改)
		void ClearUsedBuffer();
		//从当前蒙皮结果缓冲区中请求一块数据区(蒙皮结果数据区由GPU填充数据，因而只需要开辟)
		PancystarEngine::EngineFailReason BuildAnimationBlock(
			const pancy_resource_size &vertex_num,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_animation_block
		);
		//从当前骨骼矩阵缓冲区中请求一块数据区(骨骼矩阵数据区由CPU填充数据，因而需要将填充数据一并传入)
		PancystarEngine::EngineFailReason BuildBoneBlock(
			const pancy_resource_size &matrix_num,
			pancy_object_id &block_id,
			SkinAnimationBlock &new_bone_block
		);
		//获取矩阵存储缓冲区
		inline VirtualResourcePointer& GetBoneMatrixResource(const pancy_object_id &frame_id)
		{
			return buffer_bone[frame_id];
		}
		//获取蒙皮结果缓冲区
		inline VirtualResourcePointer& GetSkinVertexResource()
		{
			return buffer_animation;
		}
		//获取骨骼id号缓冲区
		inline VirtualResourcePointer& GetBoneIDResource()
		{
			return buffer_globel_index[0];
		}
	};
	struct AnimationStartMessage
	{
		//当前的骨骼信息
		pancy_object_id bone_matrix_offset_from_begin;
		pancy_object_id bone_matrix_num;
		//当前的父骨骼id信息
		pancy_object_id bone_parent_id_offset_from_begin;
		pancy_object_id bone_parent_id_num;
		//当前的动画信息
		pancy_object_id drawcall_id;
		pancy_object_id animation_resample_num;
		pancy_object_id animation_start_pos;//动画数据的起始点(即选择播放的是哪个动画)
		//当前的骨骼偏移信息
		pancy_object_id offset_matrix_offset_from_begin;
		pancy_object_id offset_matrix_num;
		//当前的骨骼矩阵申请情况
		pancy_object_id bone_buffer_block_id;

	};
	//全局骨骼动画控制器
	class PancySkinAnimationControl
	{
		PancyDescriptorHeapDynamic skin_mesh_animation_buffer_descriptor;
		PancyDescriptorHeapDynamic skin_mesh_offset_matrix_descriptor;
		PancyDescriptorHeapDynamic skin_mesh_parent_id_descriptor;

		pancy_resource_size animation_buffer_size;                    //存储动画结果的缓冲区大小
		pancy_resource_size bone_buffer_size;                         //存储骨骼矩阵的缓冲区大小

		pancy_object_id PSO_skinmesh_animation_interpolation;         //骨骼动画的插值阶段
		pancy_object_id PSO_skinmesh_skintree_desample;               //骨骼动画的骨骼树下采样阶段
		pancy_object_id PSO_skinmesh_cluster_combine;                 //骨骼动画的矩阵合并阶段
		//计算着色器的渲染单元id号
		PancystarEngine::PancyRenderParamID render_param_id_animation_sample;
		PancystarEngine::PancyRenderParamID render_param_id_bone_compute[10];
		PancystarEngine::PancyRenderParamID render_param_id_combine_matrix;

		pancy_object_id PSO_skinmesh;                                 //骨骼动画的渲染状态表
		std::vector<PancySkinAnimationBuffer*> skin_naimation_buffer; //骨骼动画的缓冲区信息
		//todo:这里暂时先使用shared_ptr之后需要改成weak
		pancy_object_id bone_count_time_num = 0;
		std::vector<VirtualResourcePointer> skin_animation_matrix_buffer; //骨骼的动画矩阵缓冲区信息
		std::vector<VirtualResourcePointer> skin_offset_matrix_buffer;    //骨骼的偏移矩阵缓冲区信息
		std::vector<VirtualResourcePointer> skin_parent_id_buffer;        //骨骼的父骨骼缓冲区信息

		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> skin_animation_matrix_descriptor;     //骨骼动画的骨骼矩阵缓冲区信息
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> skin_offset_matrix_descriptor;   //骨骼动画的偏移矩阵缓冲区信息
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> skin_parent_id_descriptor;       //骨骼动画的父骨骼id缓冲区信息

		std::vector<AnimationStartMessage> skin_animation_message;              //骨骼动画的位置信息
		pancy_object_id globel_bone_matrix_size = 0;
		pancy_object_id globel_offset_matrix_size = 0;
		pancy_object_id globel_parent_id_size = 0;
		//初始化
		PancySkinAnimationControl(
			const pancy_resource_size &animation_buffer_size_in,
			const pancy_resource_size &bone_buffer_size_in
		);
		PancystarEngine::EngineFailReason Create();
		//临时存储交换缓冲区描述符
		BindDescriptorPointer buffer_descriptor_srv_id1;
		BindDescriptorPointer buffer_descriptor_srv_id2;
		BindDescriptorPointer buffer_descriptor_uav_id1;
		BindDescriptorPointer buffer_descriptor_uav_id2;
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
		//清空当前帧的缓冲区使用信息
		void ClearUsedBuffer();
		//计算骨骼矩阵
		PancystarEngine::EngineFailReason ComputeBoneMatrix(const float &play_time);
		//填充渲染commandlist
		PancystarEngine::EngineFailReason BuildCommandList(
			const pancy_object_id& bone_block_id,
			VirtualResourcePointer &mesh_buffer,
			const pancy_object_id &vertex_num,
			const PancyRenderParamID &render_param_id,
			const pancy_resource_size &matrix_num,
			SkinAnimationBlock &new_animation_block,
			PancyRenderCommandList *m_commandList_skin
		);
		//从当前骨骼矩阵缓冲区中请求一块数据区(骨骼矩阵数据区由CPU填充数据，因而需要将填充数据一并传入)
		PancystarEngine::EngineFailReason BuildBoneBlock(
			VirtualResourcePointer& animation_matrix_buffer,
			VirtualResourcePointer& offset_matrix_buffer,
			VirtualResourcePointer& parent_id_buffer,
			const D3D12_SHADER_RESOURCE_VIEW_DESC& animation_buffer_descriptor,
			const D3D12_SHADER_RESOURCE_VIEW_DESC& offset_matrix_descriptor,
			const D3D12_SHADER_RESOURCE_VIEW_DESC& parent_id_descriptor,
			const pancy_resource_size& animation_resample_num,
			const pancy_resource_size& animation_start_pos,
			const pancy_resource_size& offset_matrix_num,
			const pancy_resource_size& bone_parent_id_num,
			const pancy_resource_size& bone_matrix_num,
			pancy_object_id& block_id,
			SkinAnimationBlock& new_bone_block
		);
		~PancySkinAnimationControl();
	private:
		//从当前蒙皮结果缓冲区中请求一块数据区(蒙皮结果数据区由GPU填充数据，因而只需要开辟)
		PancystarEngine::EngineFailReason BuildAnimationBlock(
			const pancy_resource_size &vertex_num,
			pancy_object_id &block_id,
			SkinAnimationBlock &animation_block_pos
		);
		//todo:移植到描述符管理
		PancystarEngine::EngineFailReason BuildGlobelBufferDescriptorUAV(
			const std::string& descriptor_name,
			std::vector<VirtualResourcePointer>& buffer_data,
			const pancy_resource_size &buffer_size,
			const pancy_resource_size& per_element_size
		);
		PancystarEngine::EngineFailReason BuildGlobelBufferDescriptorSRV(
			const std::string& descriptor_name,
			std::vector<VirtualResourcePointer>& buffer_data,
			const pancy_resource_size& buffer_size,
			const pancy_resource_size& per_element_size
			);
		PancystarEngine::EngineFailReason BuildPrivateBufferDescriptorUAV(
			std::vector<VirtualResourcePointer>& buffer_data,
			const pancy_resource_size& buffer_size,
			const pancy_resource_size& per_element_size,
			BindDescriptorPointer& descriptor_id
		);
		PancystarEngine::EngineFailReason BuildPrivateBufferDescriptorSRV(
			std::vector<VirtualResourcePointer>& buffer_data,
			const pancy_resource_size& buffer_size,
			const pancy_resource_size& per_element_size,
			BindDescriptorPointer& descriptor_id
		);

	};
}