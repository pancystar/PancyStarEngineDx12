#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"
#define MaxWasteSpace 16777216
//Cbuffer��Դ��(���ٷ�Χ16K-64k,���侫��1K)
#define ConstantBufferHeapAliaze256K 262144
#define ConstantBufferSubResourceAliaze256K 1024
//Cbuffer��Դ��(���ٷ�Χ0-16k,���侫��256B)
#define ConstantBufferHeapAliaze64K 65536
#define ConstantBufferSubResourceAliaze64K 256

//64M����Դ��(���ٿռ䷶ΧΪ4M-16M�����侫��4M)
#define BufferHeapAliaze64M 67108864
#define BufferSubResourceAliaze64M 4194304
//16M����Դ��(���ٿռ䷶ΧΪ1M-4M�����侫��512k,�����������˷���80-90M)
#define BufferHeapAliaze16M 16777216
#define BufferSubResourceAliaze16M 524288
//4M����Դ��(���ٿռ䷶ΧΪ256k-1M,���侫��128k)
#define BufferHeapAliaze4M 4194304
#define BufferSubResourceAliaze4M 131072
//1M����Դ��(���ٿռ䷶ΧΪ0-256k,���侫��32k)
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
	//��������Դ
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		PancyBufferType  buffer_type;
		SubMemoryPointer buffer_data;     //buffer����ָ��
		pancy_object_id  upload_buffer_id;
		PancyFenceIdGPU  WaitFence;       //��Ҫ�ȴ���GPU��λ��
	public:
		PancyBasicBuffer(const std::string &resource_name_in, Json::Value root_value_in);
		~PancyBasicBuffer();
		inline SubMemoryPointer GetBufferSubResource()
		{
			return buffer_data;
		}
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state);
		//��д����GPU��Դ��CPU�ĺ���
		PancystarEngine::EngineFailReason UpdateResourceToGPU(
			ResourceStateType &now_res_state,
			void* resource,
			const pancy_resource_size &resource_size_in,
			const pancy_resource_size &resource_offset_in
		);
		//��⵱ǰ����Դ�Ƿ��Ѿ�������GPU
		void CheckIfResourceLoadToGpu(ResourceStateType &now_res_state);
	};
	
	//������������
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
		//����ָ����������Դ���ͣ�����һ��ȷ��buffer��Դ��ʽ��json�ļ�(heap,subresource)
		PancystarEngine::EngineFailReason BuildBufferTypeJson(
			const PancyBufferType &buffer_type,
			const pancy_resource_size &data_size,
			std::string &subresource_desc_name
		);
		//ʹ�ñ�׼��DirectX��Դ����buffer����void*���ݲ�ͬ��Ҫ������Ƕ�̬buffer����˿��Բ���Ҫ���»�����״̬
		PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
			const pancy_object_id  &resource_id,
			const pancy_resource_size &pointer_offset,
			std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			UINT64* pRowSizesInBytes,
			UINT* pNumRows
		);
		//��ȡbuffer���ݵ�submemoryָ��
		PancystarEngine::EngineFailReason GetBufferSubResource(const pancy_object_id  &resource_id, SubMemoryPointer &submemory);
		//��ȡbuffer���ݵ�CPU����ʵָ��
		PancystarEngine::EngineFailReason GetBufferCPUPointer(const pancy_object_id  &resource_id, UINT8** map_pointer_out);
	private:
		PancystarEngine::EngineFailReason BuildResource(
			const Json::Value &root_value,
			const std::string &name_resource_in,
			PancyBasicVirtualResource** resource_out
		);
	};
	//����������
	struct CbufferVariable 
	{
		pancy_resource_size variable_size;
		pancy_resource_size start_offset;
	};
	class PancyConstantBuffer
	{
		pancy_resource_size cbuffer_size;
		std::string cbuffer_name;       //����������������
		std::string cbuffer_effect_name; //������������������Ⱦ��������
		//����������������
		pancy_object_id buffer_ID;
		//���г�Ա��������ʼλ��
		std::unordered_map<std::string, CbufferVariable> member_variable;
		//������������CPU�˵�ָ��
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
	
	
}