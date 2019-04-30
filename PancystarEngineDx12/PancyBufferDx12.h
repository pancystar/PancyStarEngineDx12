#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
#define MaxWasteSpace 16777216
//Cbuffer资源堆(开辟范围16K-64k,区间精度1K)
#define ConstantBufferHeapAliaze256K 262144
#define ConstantBufferSubResourceAliaze256K 1024
//Cbuffer资源堆(开辟范围0-16k,区间精度256B)
#define ConstantBufferHeapAliaze64K 65536
#define ConstantBufferSubResourceAliaze64K 256

//64M的资源堆(开辟空间范围为4M-16M，区间精度4M)
#define BufferHeapAliaze64M 67108864
#define BufferSubResourceAliaze64M 4194304
//16M的资源堆(开辟空间范围为1M-4M，区间精度512k,整体最大空闲浪费量80-90M)
#define BufferHeapAliaze16M 16777216
#define BufferSubResourceAliaze16M 524288
//4M的资源堆(开辟空间范围为256k-1M,区间精度128k)
#define BufferHeapAliaze4M 4194304
#define BufferSubResourceAliaze4M 131072
//1M的资源堆(开辟空间范围为0-256k,区间精度32k)
#define BufferHeapAliaze1M 1048576
#define BufferSubResourceAliaze1M 65536
namespace PancystarEngine 
{
	enum PancyBufferType 
	{
		Buffer_ShaderResource_static = 0,
		Buffer_ShaderResource_dynamic,
		Buffer_Constant,
		Buffer_Vertex,
		Buffer_Index
	};
	//缓冲区资源
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		PancyBufferType  buffer_type;
		SubMemoryPointer buffer_data;     //buffer数据指针
		pancy_object_id  upload_buffer_id;
		PancyFenceIdGPU  WaitFence;       //需要等待的GPU眼位号
	public:
		PancyBasicBuffer(const std::string &resource_name_in, Json::Value root_value_in);
		~PancyBasicBuffer();
		inline SubMemoryPointer GetBufferSubResource()
		{
			return buffer_data;
		}
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state);
		//重写更新GPU资源到CPU的函数
		PancystarEngine::EngineFailReason UpdateResourceToGPU(
			ResourceStateType &now_res_state,
			void* resource,
			const pancy_resource_size &resource_size_in,
			const pancy_resource_size &resource_offset_in
		);
		//检测当前的资源是否已经被载入GPU
		void CheckIfResourceLoadToGpu(ResourceStateType &now_res_state);
	};
	
	//缓冲区管理器
	class PancyBasicBufferControl :public PancyBasicResourceControl
	{
		PancyBasicBufferControl(const std::string &resource_type_name_in);
	public:
		static PancyBasicBufferControl* GetInstance()
		{
			static PancyBasicBufferControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyBasicBufferControl("Buffer Resource Control");
			}
			return this_instance;
		}
		//根据指定的纹理资源类型，创建一套确定buffer资源格式的json文件(heap,subresource)
		PancystarEngine::EngineFailReason BuildBufferTypeJson(
			const PancyBufferType &buffer_type,
			const pancy_resource_size &data_size,
			std::string &subresource_desc_name
		);
		//使用标准的DirectX资源更新buffer，与void*数据不同，要求必须是动态buffer，因此可以不需要更新缓冲区状态
		PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
			const pancy_object_id  &resource_id,
			const pancy_resource_size &pointer_offset,
			std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			UINT64* pRowSizesInBytes,
			UINT* pNumRows
		);
		//获取buffer数据的submemory指针
		PancystarEngine::EngineFailReason GetBufferSubResource(const pancy_object_id  &resource_id, SubMemoryPointer &submemory);
		//获取buffer数据的CPU端真实指针
		PancystarEngine::EngineFailReason GetBufferCPUPointer(const pancy_object_id  &resource_id, UINT8** map_pointer_out);
	private:
		PancystarEngine::EngineFailReason BuildResource(
			const Json::Value &root_value,
			const std::string &name_resource_in,
			PancyBasicVirtualResource** resource_out
		);
	};
	//常量缓冲区
	struct CbufferVariable 
	{
		pancy_resource_size variable_size;
		pancy_resource_size start_offset;
	};
	class PancyConstantBuffer
	{
		pancy_resource_size cbuffer_size;
		std::string cbuffer_name;       //常量缓冲区的名称
		std::string cbuffer_effect_name; //创建常量缓冲区的渲染管线名称
		//常量缓冲区的数据
		pancy_object_id buffer_ID;
		//所有成员变量的起始位置
		std::unordered_map<std::string, CbufferVariable> member_variable;
		//常量缓冲区在CPU端的指针
		UINT8* map_pointer_out;
	public:
		PancyConstantBuffer(const std::string &cbuffer_name_in, const std::string &cbuffer_effect_name_in);
		~PancyConstantBuffer();
		PancystarEngine::EngineFailReason Create();
		//PancystarEngine::EngineFailReason Create(const std::string &hash_name,const Json::Value &root_value);
		PancystarEngine::EngineFailReason SetMatrix(const std::string &variable,const DirectX::XMFLOAT4X4 &mat_data,const pancy_resource_size &offset);
		PancystarEngine::EngineFailReason SetFloat4(const std::string &variable, const DirectX::XMFLOAT4 &vector_data, const pancy_resource_size &offset);
		PancystarEngine::EngineFailReason SetUint4(const std::string &variable, const DirectX::XMUINT4 &vector_data, const pancy_resource_size &offset);
		PancystarEngine::EngineFailReason SetStruct(const std::string &variable, const void* struct_data, const pancy_resource_size &data_size, const pancy_resource_size &offset);
		PancystarEngine::EngineFailReason GetBufferSubResource(SubMemoryPointer &submemory);
	private:
		PancystarEngine::EngineFailReason ErrorVariableNotFind(const std::string &variable_name);
		PancystarEngine::EngineFailReason GetCbufferDesc(const std::string &file_name,const Json::Value &root_value);
	};
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
		pancy_object_id buffer_ID_animation;//动画结果缓冲区
		pancy_object_id buffer_ID_bone;     //骨骼矩阵缓冲区
		//骨骼数据的CPU指针
		UINT8* bone_data_pointer;
	public:
		PancySkinAnimationBuffer(const pancy_resource_size &animation_buffer_size_in, const pancy_resource_size &bone_buffer_size_in);
		~PancySkinAnimationBuffer();
		PancystarEngine::EngineFailReason Create();
		//清空当前所有使用的骨骼动画数据(由于动画数据逐帧重置，不需要考虑随机寻址类型的增删查改)
		void ClearUsedBuffer();
		//从当前蒙皮结果缓冲区中请求一块数据区(蒙皮结果数据区由GPU填充数据，因而只需要开辟)
		PancystarEngine::EngineFailReason BuildAnimationBlock(const pancy_resource_size &vertex_num, pancy_object_id &block_id);
		//从当前骨骼矩阵缓冲区中请求一块数据区(骨骼矩阵数据区由CPU填充数据，因而需要将填充数据一并传入)
		PancystarEngine::EngineFailReason BuildBoneBlock(
			const pancy_resource_size &matrix_num, 
			pancy_object_id &block_id, 
			const DirectX::XMFLOAT4X4 *matrix_data
		);
	};
	
}