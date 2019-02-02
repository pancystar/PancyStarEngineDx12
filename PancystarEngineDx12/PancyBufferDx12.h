#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
#define MaxWasteSpace 67108864
//Cbuffer资源堆(开辟范围16K-64k,区间精度1K)
#define ConstantBufferHeapAliaze256K 262144
#define ConstantBufferSubResourceAliaze256K 1024
//Cbuffer资源堆(开辟范围0-16k,区间精度256B)
#define ConstantBufferHeapAliaze64K 65536
#define ConstantBufferSubResourceAliaze64K 256

//64M的资源堆(开辟空间范围为4M-16M，区间精度4M)
#define BufferHeapAliaze64M 67108864
#define BufferSubResourceAliaze64M 4194304
//16M的资源堆(开辟空间范围为1M-4M，区间精度1M)
#define BufferHeapAliaze16M 16777216
#define BufferSubResourceAliaze16M 1048576
//4M的资源堆(开辟空间范围为256k-1M,区间精度128k)
#define BufferHeapAliaze4M 4194304
#define BufferSubResourceAliaze4M 131072
//1M的资源堆(开辟空间范围为0-256k,区间精度32k)
#define BufferHeapAliaze1M 1048576
#define BufferSubResourceAliaze1M 32768
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
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		PancyBufferType  buffer_type;
		SubMemoryPointer buffer_data;     //buffer数据指针
		SubMemoryPointer update_tex_data; //buffer上传数据指针
		PancyFenceIdGPU  WaitFence;       //需要等待的GPU眼位号
	public:
		PancyBasicBuffer(const std::string &resource_name_in, Json::Value root_value_in);
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state);
		//重写更新GPU资源到CPU的函数
		PancystarEngine::EngineFailReason UpdateResourceToGPU(
			ResourceStateType &now_res_state,
			void* resource,
			const pancy_resource_size &resource_size_in
		);
		//检测当前的资源是否已经被载入GPU
		void CheckIfResourceLoadToGpu(ResourceStateType &now_res_state);
	};
	class PancyBasicBufferControl :public PancyBasicResourceControl
	{
	public:
		PancyBasicBufferControl(const std::string &resource_type_name_in);
		//根据指定的纹理资源类型，创建一套确定buffer资源格式的json文件(heap,subresource)
		PancystarEngine::EngineFailReason BuildBufferTypeJson(
			const PancyBufferType &buffer_type,
			const pancy_resource_size &data_size,
			std::string &subresource_desc_name
		);
	private:
		PancystarEngine::EngineFailReason BuildResource(
			const Json::Value &root_value,
			const std::string &name_resource_in,
			PancyBasicVirtualResource** resource_out
		);
	};
	
	
}