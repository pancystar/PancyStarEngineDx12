#pragma once
#include"PancyDx12Basic.h"
#include"PancyThreadBasic.h"
#include<DirectXTex.h>
#include<LoaderHelpers.h>
#include<DDSTextureLoader.h>
#include<WICTextureLoader.h>
#define MaxHeapDivide 32
//�Դ�ָ��
struct VirtualMemoryPointer
{
	bool if_heap;//��Դ�Ƿ����ɶ��Ϸ������
	pancy_resource_id heap_type;      //������
	pancy_resource_id heap_list_id;   //��id��
	pancy_resource_id memory_block_id;//�ڴ��id��
	pancy_object_id memory_resource_id;//ֱ��ָ���ڴ��id��
	VirtualMemoryPointer()
	{
		if_heap = false;
		heap_type = 0;
		heap_list_id = 0;
		memory_block_id = 0;
		memory_resource_id = 0;
	}
};
//�Դ��
class MemoryBlockGpu
{
	pancy_resource_size memory_size;//�洢��Ĵ�С
	ComPtr<ID3D12Resource> resource_data;//�洢�������
	D3D12_HEAP_TYPE resource_usage;
	UINT8* map_pointer;
	D3D12_RESOURCE_STATES now_subresource_state;//��ǰ��Դ��ʹ�ø�ʽ
public:
	MemoryBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in,
		const D3D12_HEAP_TYPE &resource_usage_in,
		const D3D12_RESOURCE_STATES &resource_state
	);
	~MemoryBlockGpu();
	inline ComPtr<ID3D12Resource> GetResource()
	{
		return resource_data;
	}
	inline uint64_t GetSize()
	{
		return memory_size;
	}
	//�鿴��ǰ��Դ��ʹ�ø�ʽ
	inline D3D12_RESOURCE_STATES GetResourceState()
	{
		return now_subresource_state;
	}
	//�޸ĵ�ǰ��Դ��ʹ�ø�ʽ
	inline void SetResourceState(const D3D12_RESOURCE_STATES &state)
	{
		now_subresource_state = state;
	}
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(const pancy_resource_size &pointer_offset, const void* copy_data, const pancy_resource_size data_size);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const pancy_resource_size &pointer_offset,
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason GetCpuMapPointer(UINT8** map_pointer_out)
	{
		if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get CPU Pointer of memory block gpu", error_message);
			map_pointer_out = NULL;
			return error_message;
		}
		*map_pointer_out = map_pointer;
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason ReadFromBufferToCpu(const pancy_resource_size &pointer_offset, void* copy_data, const pancy_resource_size data_size);
};
//�����Դ��
class MemoryHeapGpu
{
	std::string heap_type_name;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	ComPtr<ID3D12Heap> heap_data;
	//std::unordered_set<pancy_resource_id> free_list;

	//�����Դ������
	std::unordered_map<pancy_resource_id, MemoryBlockGpu*> memory_heap_block;
	//�ճ����Դ�
	std::unordered_set<pancy_resource_id> empty_memory_block;
public:
	MemoryHeapGpu(const std::string &heap_type_name_in);
	//ÿ���Դ��Ĵ�С
	inline pancy_resource_size GetMemorySizePerBlock()
	{
		return size_per_block;
	}
	//�Դ�ѵ����Դ������
	inline pancy_resource_id GetMaxMemoryBlockNum()
	{
		return max_block_num;
	}
	//�Դ����δʹ�õ��Դ��С
	inline size_t GetFreeMemoryBlockNum()
	{
		return empty_memory_block.size();
	}
	PancystarEngine::EngineFailReason Create(const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//���Դ�ѿ�����Դ
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID
	);
	//��ȡ�Դ���Դ
	MemoryBlockGpu* GetMemoryResource(const pancy_resource_id &memory_block_ID);
	//�����Ӧid����Դ�Ƿ��Ѿ�������
	bool CheckIfFree(pancy_resource_id memory_block_ID);
	//�ͷ�һ����Ӧid����Դ
	PancystarEngine::EngineFailReason FreeMemoryReference(const pancy_resource_id &memory_block_ID);
	~MemoryHeapGpu();
};
//�����������Դ��
class MemoryHeapLinear
{
	//�Դ�ѵĸ�ʽ
	CD3DX12_HEAP_DESC heap_desc;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	//�Դ�ѵ�����
	std::string heap_type_name;

	//�����Դ�ѵ�����
	std::unordered_map<pancy_resource_id, MemoryHeapGpu*> memory_heap_data;
	//�ճ����Դ��
	std::unordered_set<pancy_resource_id> empty_memory_heap;
public:
	MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const pancy_resource_size &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//���Դ�ѿ�����Դ
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID,//�Դ���ַָ��
		pancy_resource_id &memory_heap_ID//�Դ�ε�ַָ��
	);
	MemoryBlockGpu* GetMemoryResource(
		const pancy_resource_id &memory_heap_ID,//�Դ�ε�ַָ��
		const pancy_resource_id &memory_block_ID//�Դ���ַָ��

	);
	//�ͷ�һ����Ӧid����Դ
	PancystarEngine::EngineFailReason FreeMemoryReference(
		const pancy_resource_id &memory_heap_ID,
		const pancy_resource_id &memory_block_ID
	);
	//��ȡ��ǰ�����Ϳ����Ķ�����
	inline pancy_object_id GetHeapNum()
	{
		return memory_heap_data.size();
	}
	//��ȡ��ǰ�����͵�ÿ���ѵĴ�С
	inline pancy_resource_size GetPerHeapSize()
	{
		return heap_desc.SizeInBytes;
	}
	~MemoryHeapLinear();
};
//��Դ������
class MemoryHeapGpuControl
{
	pancy_object_id resource_memory_id_self_add;
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, MemoryHeapLinear*> resource_heap_list;//�Դ�ѱ�
	std::unordered_map<pancy_object_id, MemoryBlockGpu*>resource_memory_list;//��ɢ���Դ���
	std::unordered_set<pancy_object_id> resource_memory_free_id;
	MemoryHeapGpuControl();
public:
	static MemoryHeapGpuControl* GetInstance()
	{
		static MemoryHeapGpuControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new MemoryHeapGpuControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildResourceCommit(
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		VirtualMemoryPointer &virtual_pointer
	);
	PancystarEngine::EngineFailReason BuildResourceFromHeap(
		const std::string &HeapFileName,
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		VirtualMemoryPointer &virtual_pointer
	);
	MemoryBlockGpu *GetMemoryResource(const VirtualMemoryPointer &virtual_pointer);
	PancystarEngine::EngineFailReason FreeResource(const VirtualMemoryPointer &virtual_pointer);
	void GetHeapDesc(const pancy_resource_id &heap_id, pancy_object_id &heap_num, pancy_resource_size &per_heap_size);
	void GetHeapDesc(const std::string &heap_name, pancy_object_id &heap_num, pancy_resource_size &per_heap_size);
	~MemoryHeapGpuControl();
private:
	//�������ָ�����ϵ���Դ
	MemoryBlockGpu* GetMemoryFromList(const pancy_object_id &memory_block_ID);
	PancystarEngine::EngineFailReason FreeResourceCommit(const pancy_object_id &memory_block_ID);
	//�����ָ�����ϵ���Դ
	MemoryBlockGpu * GetMemoryResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//�Դ����ַָ��
		const pancy_resource_id &memory_heap_ID,//�Դ�ε�ַָ��
		const pancy_resource_id &memory_block_ID//�Դ���ַָ��
	);

	PancystarEngine::EngineFailReason LoadHeapFromFile(
		const std::string &HeapFileName,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
	PancystarEngine::EngineFailReason FreeResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//�Դ����ַָ��
		const pancy_resource_id &memory_heap_ID,//�Դ�ε�ַָ��
		const pancy_resource_id &memory_block_ID//�Դ���ַָ��
	);
	PancystarEngine::EngineFailReason BuildHeap(
		const std::string &HeapFileName,
		const pancy_resource_size &heap_size,
		const pancy_resource_size &per_block_size,
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
};
//������Դ
struct SubMemoryPointer
{
	pancy_resource_id type_id;
	pancy_object_id list_id;
	pancy_object_id offset;
};
class SubMemoryData
{
	VirtualMemoryPointer buffer_data;//�Դ���Դָ��
	pancy_resource_size per_memory_size;//ÿ���������Ĵ�С
	std::unordered_set<pancy_object_id> empty_sub_memory;
	std::unordered_set<pancy_object_id> sub_memory_data;
public:
	SubMemoryData();
	PancystarEngine::EngineFailReason Create(
		const std::string &buffer_desc_file,
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		const pancy_object_id &per_memory_size_in
	);
	PancystarEngine::EngineFailReason BuildSubMemory(pancy_object_id &offset);
	PancystarEngine::EngineFailReason FreeSubMemory(const pancy_object_id &offset);
	//�鿴��ǰ������Դ�Ĵ�С
	inline pancy_object_id GetEmptySize()
	{
		return static_cast<pancy_object_id>(empty_sub_memory.size());
	}

	//��ȡ��Դ
	inline MemoryBlockGpu* GetResource()
	{
		return MemoryHeapGpuControl::GetInstance()->GetMemoryResource(buffer_data);
	}
	void GetLogMessage(std::vector<std::string> &log_message);
	void GetLogMessage(Json::Value &root_value, bool &if_empty);
	inline pancy_resource_size GetBlockSize()
	{
		return static_cast<pancy_resource_size>(per_memory_size);
	}
};
class SubresourceLiner
{
	std::string heap_name;
	std::string hash_name;
	D3D12_RESOURCE_DESC resource_desc;
	D3D12_RESOURCE_STATES resource_state;
	pancy_object_id per_memory_size;
	std::unordered_map<pancy_object_id, SubMemoryData*> submemory_list;
	//�ճ����Դ�
	std::unordered_set<pancy_object_id> empty_memory_heap;
	//��ǰ����id���Լ��Ѿ��ͷŵ�id��
	pancy_object_id max_id;
	std::unordered_set<pancy_object_id> free_id;
public:
	SubresourceLiner(
		const std::string &heap_name_in,
		const std::string &hash_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in
	);
	~SubresourceLiner();
	inline std::string GetHeapName()
	{
		return heap_name;
	}
	inline std::string GetResourceName()
	{
		return hash_name;
	}
	void GetLogMessage(std::vector<std::string> &log_message);
	void GetLogMessage(Json::Value &root_value);
	PancystarEngine::EngineFailReason BuildSubresource(
		pancy_object_id &new_memory_block_id,
		pancy_object_id &sub_memory_offset
	);
	PancystarEngine::EngineFailReason ReleaseSubResource(
		const pancy_object_id &new_memory_block_id,
		const pancy_object_id &sub_memory_offset
	);
	MemoryBlockGpu* GetSubResource(pancy_object_id sub_memory_id, pancy_resource_size &per_memory_size);
};
class SubresourceControl
{
	pancy_object_id subresource_id_self_add;
	std::unordered_map<pancy_resource_id, SubresourceLiner*> subresource_list_map;
	std::unordered_map<std::string, pancy_resource_id> subresource_init_list;
	std::unordered_set<pancy_object_id> subresource_free_id;
private:
	SubresourceControl();
public:
	~SubresourceControl();
	static SubresourceControl* GetInstance()
	{
		static SubresourceControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new SubresourceControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildSubresourceFromFile(
		const std::string &resource_name_in,
		SubMemoryPointer &submemory_pointer
	);
	PancystarEngine::EngineFailReason FreeSubResource(const SubMemoryPointer &submemory_pointer);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const SubMemoryPointer &submemory_pointer,
		const pancy_resource_size &pointer_offset,
		const void* copy_data,
		const pancy_resource_size data_size
	);
	PancystarEngine::EngineFailReason GetBufferCpuPointer(
		const SubMemoryPointer &submemory_pointer,
		UINT8** map_pointer_out
	);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const SubMemoryPointer &submemory_pointer,
		const pancy_resource_size &pointer_offset,
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason CopyResource(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const SubMemoryPointer &dst_submemory,
		const pancy_resource_size &src_offset,
		const pancy_resource_size &dst_offset,
		const pancy_resource_size &data_size
	);
	PancystarEngine::EngineFailReason CopyResource(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const SubMemoryPointer &dst_submemory,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		const pancy_object_id &Layout_num
	);
	PancystarEngine::EngineFailReason ResourceBarrier(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const D3D12_RESOURCE_STATES &last_state,
		const D3D12_RESOURCE_STATES &now_state
	);
	PancystarEngine::EngineFailReason GetResourceState(const SubMemoryPointer &src_submemory, D3D12_RESOURCE_STATES &res_state);
	PancystarEngine::EngineFailReason CaptureTextureDataToWindows(
		const SubMemoryPointer &tex_data,
		const bool &if_cube_map,
		DirectX::ScratchImage *new_image
	);
	PancystarEngine::EngineFailReason GetSubResourceDesc(
		const SubMemoryPointer & tex_data,
		D3D12_RESOURCE_DESC &resource_desc
	);
	PancystarEngine::EngineFailReason BuildConstantBufferView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor
	);
	PancystarEngine::EngineFailReason BuildShaderResourceView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildRenderTargetView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_RENDER_TARGET_VIEW_DESC  &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildUnorderedAccessView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC  &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildDepthStencilView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_DEPTH_STENCIL_VIEW_DESC  &DSV_desc
	);
	PancystarEngine::EngineFailReason BuildVertexBufferView(
		const SubMemoryPointer &src_submemory,
		UINT StrideInBytes,
		D3D12_VERTEX_BUFFER_VIEW &VBV_out
	);
	PancystarEngine::EngineFailReason BuildIndexBufferView(
		const SubMemoryPointer &src_submemory,
		DXGI_FORMAT StrideInBytes,
		D3D12_INDEX_BUFFER_VIEW &IBV_out
	);
	void WriteSubMemoryMessageToFile(const std::string &log_file_name);
	//void WriteSubMemoryMessageToFile(Json::Value &root_value);
private:
	MemoryBlockGpu*  GetResourceData(const SubMemoryPointer &submemory_pointer, pancy_resource_size &per_memory_size);
	void InitSubResourceType(
		const std::string &hash_name,
		const std::string &heap_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		pancy_resource_id &subresource_type_id
	);
	PancystarEngine::EngineFailReason BuildSubresource(
		const std::string &hash_name,
		const std::string &heap_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		SubMemoryPointer &submemory_pointer
	);
};

//��ͨ�İ�������
struct CommonDescriptorPointer
{
	//�Ƿ�ʹ��˫����
	bool if_multi_buffer;
	//������ҳ����ʼ��ַ
	std::vector<pancy_object_id> descriptor_offset;
	//����������
	D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type;
	CommonDescriptorPointer() 
	{
		//��ʼ����ͨ������ָ��
		if_multi_buffer = false;
		descriptor_offset.resize(2);
		descriptor_type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	}
};
//��󶨵�������ҳ
struct BindlessResourceViewPointer
{
	//������ҳ����ʼ��ַ
	pancy_object_id resource_view_offset;
	//������ҳ������������������
	pancy_object_id resource_view_num;
	//ÿһ���������ĸ�ʽ����
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> SRV_desc;
	//ÿһ����������ָ�����Դ
	std::vector<SubMemoryPointer> describe_memory_data;
	BindlessResourceViewPointer()
	{
		resource_view_offset = 0;
		resource_view_num = 0;
	}
};
//��������
class BindlessResourceViewSegmental
{
	ID3D12DescriptorHeap *descriptor_heap_data;//�������ѵ���ʵ��ַ
	pancy_object_id per_descriptor_size;       //ÿ���������Ĵ�С
	pancy_object_id segmental_offset_position; //��ǰ����������ȫ�ֵ�ƫ��
	pancy_object_id max_descriptor_num;        //��ǰ��������������ɵ�����������
	pancy_object_id now_pointer_offset;        //��ǰ����������ʹ�����ݵ�ƫ��
	pancy_object_id now_pointer_refresh;       //��ǰ�������������Ҫһ�������������������ʼ����λ��
	pancy_object_id now_descriptor_pack_id_self_add;//��ǰ��������Ϊÿ��������ҳ�����ID������������
	std::queue<pancy_object_id> now_descriptor_pack_id_reuse;//֮ǰ���ͷŵ������ڿ�������ʹ�õ�������ID

	std::unordered_map<pancy_resource_id, BindlessResourceViewPointer> descriptor_data;//ÿ�������������������ҳ
public:
	BindlessResourceViewSegmental(
		const pancy_object_id &max_descriptor_num,
		const pancy_object_id &segmental_offset_position_in,
		const pancy_object_id &per_descriptor_size_in,
		const ComPtr<ID3D12DescriptorHeap> descriptor_heap_data_in
	);
	inline pancy_object_id GetEmptyDescriptorNum()
	{
		return max_descriptor_num - now_pointer_offset;
	}
	//�����������￪��һ��bindless������ҳ
	PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPack(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer> &describe_memory_data,
		const pancy_object_id &SRV_pack_size,
		pancy_object_id &SRV_pack_id
	);
	//������������ɾ��һ��bindless������ҳ
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPack(const pancy_object_id &SRV_pack_id);
	//Ϊ�������ν���һ����������Ƭ���������
	PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewPack();
	//������������ɾ��һ��bindless������ҳ��ִ��һ���������
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPackAndRefresh(const pancy_object_id &SRV_pack_id);
private:
	//����������ҳ��ָ����Ϣ�����������ѿ���������
	PancystarEngine::EngineFailReason BuildShaderResourceView(const BindlessResourceViewPointer &resource_view_pointer);
};

//���������ҳ��id��
struct BindlessDescriptorID
{
	//ȫ��ID
	pancy_object_id bindless_id;
	//������Դ������
	pancy_object_id empty_resource_size;
	//����С�������
	bool operator<(const BindlessDescriptorID& other)  const;
};
struct BindlessResourceViewID
{
	pancy_object_id segmental_id;
	pancy_object_id page_id;
};
//���ڴ�����������
class PancyDescriptorHeap
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc;                                        //�������ѵĸ�ʽ
	std::string descriptor_heap_name;                                                  //�������ѵ�����
	pancy_object_id per_descriptor_size;                                               //ÿ���������Ĵ�С
	ComPtr<ID3D12DescriptorHeap> descriptor_heap_data;                                 //�������ѵ���ʵ����

	//�������ֲ�ͬ��������(ȫ����������bindless�������Լ���ͨ�İ�������)
	std::unordered_map<std::string, pancy_object_id> descriptor_globel_map;//�������ѵ�ȫ�������������ݼ���

	pancy_object_id bind_descriptor_num;                                               //�����������֧�ֵİ�����������
	std::queue<pancy_object_id> bind_descriptor_offset_reuse;                          //���������Ļ������õ�ID��
	std::unordered_map<pancy_object_id, CommonDescriptorPointer> descriptor_bind_map;  //�������ѵİ������������ݼ���

	pancy_object_id bindless_descriptor_num;                                                 //�����������֧�ֵ�bindless����������
	pancy_object_id per_segmental_size;                                                      //ÿһ���������εĴ�С
	std::unordered_map<pancy_object_id, BindlessDescriptorID> bindless_descriptor_id_map;    //�������ѵ�����bindless�������ε�id����
	std::map<BindlessDescriptorID, BindlessResourceViewSegmental*> descriptor_segmental_map; //�������ѵ�����bindless�������ε����ݼ���

public:
	PancyDescriptorHeap();
	~PancyDescriptorHeap();
	PancystarEngine::EngineFailReason Create(
		const D3D12_DESCRIPTOR_HEAP_DESC &descriptor_heap_desc,
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &globel_descriptor_num_in,
		const pancy_object_id &bind_descriptor_num_in,
		const pancy_object_id &bindless_descriptor_num_in,
		const pancy_object_id &per_segmental_size_in
	);
	//����ȫ��������
	PancystarEngine::EngineFailReason BuildGlobelShaderResourceView(
		const std::string &globel_name,
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector <SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer
	);
	PancystarEngine::EngineFailReason BuildGlobelRenderTargetView(
		const std::string &globel_name,
		const std::vector<D3D12_RENDER_TARGET_VIEW_DESC> &RTV_desc,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer
	);
	PancystarEngine::EngineFailReason BuildGlobelUnorderedAccessView(
		const std::string &globel_name,
		const std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> &UAV_desc,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer
	);
	PancystarEngine::EngineFailReason BuildGlobelDepthStencilView(
		const std::string &globel_name,
		const std::vector < D3D12_DEPTH_STENCIL_VIEW_DESC> &DSV_desc,
		const std::vector <SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer
	);
	PancystarEngine::EngineFailReason BuildGlobelConstantBufferView(
		const std::string &globel_name,
		const std::vector <SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer
	);
	PancystarEngine::EngineFailReason DeleteGlobelDescriptor(const std::string &globel_name);
	//����˽�еİ�������
	PancystarEngine::EngineFailReason BuildBindShaderResourceView(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer,
		pancy_object_id &descriptor_id
	);
	PancystarEngine::EngineFailReason BuildBindRenderTargetView(
		const std::vector<D3D12_RENDER_TARGET_VIEW_DESC> &RTV_desc,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer,
		pancy_object_id &descriptor_id
	);
	PancystarEngine::EngineFailReason BuildBindUnorderedAccessView(
		const std::vector<D3D12_UNORDERED_ACCESS_VIEW_DESC> &UAV_desc,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer,
		pancy_object_id &descriptor_id
	);
	PancystarEngine::EngineFailReason BuildBindDepthStencilView(
		const std::vector<D3D12_DEPTH_STENCIL_VIEW_DESC> &DSV_desc,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer,
		pancy_object_id &descriptor_id
	);
	PancystarEngine::EngineFailReason BuildBindConstantBufferView(
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer,
		pancy_object_id &descriptor_id
	);
	PancystarEngine::EngineFailReason DeleteGlobelDescriptor(const pancy_object_id &descriptor_id);
	//����˽�е�bindless������ҳ
	PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPage(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer> &describe_memory_data,
		const pancy_object_id &SRV_pack_size,
		BindlessResourceViewID &descriptor_id
	);
	//ɾ��˽�е�bindless������ҳ(����ָ���Ƿ�ɾ����Ϻ��ҳ��Ƭ��������)
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPage(
		const BindlessResourceViewID &descriptor_id,
		bool is_refresh_segmental = true
	);
	//�������е��������Σ����Ľϴ����л���ͼ��Դ��ʱ����ϲ�������Ƭ��ɾ��ʹ��
	PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewSegmental();
private:
	//����ˢ�½����Դ�������εĴ�С��������������ɾ��ĵ�ʱ�򱻵���
	PancystarEngine::EngineFailReason RefreshBindlessResourcesegmentalSize(const pancy_object_id &resourc_id);
	//Ԥ��������������
	PancystarEngine::EngineFailReason PreBuildBindDescriptor(
		const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_type,
		const bool if_build_multi_buffer,
		std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> &descriptor_cpu_handle,
		CommonDescriptorPointer &new_descriptor_data
	);
};
struct BindDescriptor 
{
	//��������ID
	pancy_resource_id descriptor_heap_id;
	//������ID
	pancy_object_id descriptor_id;
};
class PancyDescriptorHeapControl
{
	pancy_resource_id common_descriptor_heap_shader_resource;
	pancy_resource_id common_descriptor_heap_render_target;
	pancy_resource_id common_descriptor_heap_depth_stencil;
	std::unordered_map<pancy_resource_id, PancyDescriptorHeap*> descriptor_heap_map;
	PancyDescriptorHeapControl();
public:
	static PancyDescriptorHeapControl* GetInstance()
	{
		static PancyDescriptorHeapControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyDescriptorHeapControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildCommonShaderResourceView();
	PancystarEngine::EngineFailReason BuildCommonUnorderedAccessView();
	PancystarEngine::EngineFailReason BuildCommonConstantBufferView();
	PancystarEngine::EngineFailReason BuildCommonRenderTargetView();
	PancystarEngine::EngineFailReason BuildCommonDepthStencilView();
};
/*
//��Դ������ͼ
struct ResourceViewPack
{
	pancy_resource_id descriptor_heap_type_id;
	pancy_resource_id descriptor_heap_offset;
};
struct ResourceViewPointer
{
	ResourceViewPack resource_view_pack_id;
	pancy_resource_id resource_view_offset_id;
};
class PancyResourceView
{
	ComPtr<ID3D12DescriptorHeap> heap_data;
	D3D12_DESCRIPTOR_HEAP_TYPE resource_type;
	//��Դ��ͼ��λ��
	int32_t heap_offset;
	//��Դ��ͼ��������Դ
	uint32_t resource_view_number;
	pancy_object_id resource_block_size;
public:
	PancyResourceView(
		ComPtr<ID3D12DescriptorHeap> heap_data_in,
		const int32_t &heap_offset_in,
		D3D12_DESCRIPTOR_HEAP_TYPE &resource_type_in,
		const int32_t &view_number_in
	);
	PancystarEngine::EngineFailReason BuildSRV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildDSV(
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
	);
};


struct ResourceViewPointer
{
	pancy_resource_id resource_view_offset;
	pancy_resource_id resource_view_num ;
};
//���������������Ͻ��п��ٴ洢���򿪱ٵ�ָ������

struct DescriptorHeapPointer
{
	//��Դ�����ʼָ��λ��
	pancy_resource_size resource_pointer_start;
	//��Դ��ĵ�ǰ����ָ��λ��(Ĭ�ϴӺ���ǰ����)
	pancy_resource_size resource_pointer_now;
	//��Դ��Ľ���ָ��λ��
	pancy_resource_size resource_pointer_end;
	//��Դ��Ĵ��ͷ�λ��
	pancy_resource_size resource_pointer_free;
	DescriptorHeapPointer()
	{
		resource_pointer_start = 0;
		resource_pointer_now = 0;
		resource_pointer_end = 0;
		resource_pointer_free = 0;
	}
};
class PancyDescriptorHeap
{
	string descriptor_heap_name;  // ����������ѵ�����
	int globel_resource_view_num; //��������������֧�ֵ�ȫ������������
	int constant_buffer_view_num; // ��������������֧�ֵĳ�������������������
	DescriptorHeapPointer private_resource_view_num_8;  //8��һ���������������
	DescriptorHeapPointer private_resource_view_num_16; //16��һ���������������
	DescriptorHeapPointer private_resource_view_num_32; //32��һ���������������
	DescriptorHeapPointer private_resource_view_num_64; //64��һ���������������
	//����������������
	std::unordered_map<pancy_object_id, ResourceViewPointer> resource_view_heap_block;
public:
	PancyDescriptorHeap();
	PancystarEngine::EngineFailReason BuildDescriptorGlobel();

};
//��Դ�����������
class PancyDescriptorHeap
{
	std::string descriptor_heap_name;
	pancy_object_id heap_block_size;    //������ÿ���洢��Ĵ�С(ӵ�е���ͼ����)
	pancy_object_id heap_block_num;     //�������Ĵ洢������
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	ComPtr<ID3D12DescriptorHeap> heap_data;
	UINT per_offset_size;
	//����������������
	std::unordered_map<pancy_object_id, PancyResourceView*> resource_view_heap_block;
	//�ճ���������
	std::unordered_set<pancy_object_id> empty_view_block;
public:
	PancyDescriptorHeap(
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &heap_block_size_in,
		const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in
	);
	~PancyDescriptorHeap();
	inline std::string GetDescriptorName()
	{
		return descriptor_heap_name;
	};
	PancystarEngine::EngineFailReason Create();
	PancystarEngine::EngineFailReason BuildHeapBlock(pancy_resource_id &resource_view_ID);
	PancystarEngine::EngineFailReason FreeHeapBlock(const pancy_resource_id &resource_view_ID);
	PancystarEngine::EngineFailReason GetDescriptorHeap(ID3D12DescriptorHeap **descriptor_heap_use);
	inline PancystarEngine::EngineFailReason GetOffsetNum(pancy_resource_id heap_offset, pancy_object_id self_offset, CD3DX12_GPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap_data->GetGPUDescriptorHandleForHeapStart());
		pancy_object_id id_offset = static_cast<pancy_object_id>(heap_offset) * heap_block_size * per_offset_size + self_offset * per_offset_size;
		srvHandle.Offset(id_offset);
		descriptor_table = srvHandle;
		return PancystarEngine::succeed;
	}
	inline pancy_object_id GetOffsetNum(pancy_resource_id heap_offset, pancy_object_id self_offset, CD3DX12_CPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
		pancy_object_id id_offset = static_cast<pancy_object_id>(heap_offset) * heap_block_size * per_offset_size + self_offset * per_offset_size;
		srvHandle.Offset(id_offset);
		descriptor_table = srvHandle;
		return id_offset;
	}
	//������Դ��ͼ
	PancystarEngine::EngineFailReason BuildSRV(
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildDSV(
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset,
		const SubMemoryPointer &resource_in,
		const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
	);
private:
	PancyResourceView* GetHeapBlock(const pancy_resource_id &resource_view_ID,PancystarEngine::EngineFailReason &check_error);
};
*/
/*
//��Դ�������ѹ�����
class PancyDescriptorHeapControl
{
	pancy_resource_id descriptor_heap_id_selfadd;
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, PancyDescriptorHeap*> resource_heap_list;
	std::unordered_set<pancy_resource_id> resource_memory_free_id;
	PancyDescriptorHeapControl();
public:
	static PancyDescriptorHeapControl* GetInstance()
	{
		static PancyDescriptorHeapControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyDescriptorHeapControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildResourceViewFromFile(
		const std::string &file_name,
		ResourceViewPack &RSV_pack_id,
		pancy_object_id &per_resource_view_pack_size
	);
	inline PancystarEngine::EngineFailReason FreeResourceView(const ResourceViewPack &RSV_pack_id)
	{
		auto heap_data = resource_heap_list.find(RSV_pack_id.descriptor_heap_type_id);
		if (heap_data == resource_heap_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find resource view type:"+std::to_string(RSV_pack_id.descriptor_heap_type_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Resource View from descriptor heap",error_message);
			return error_message;
		}
		PancystarEngine::EngineFailReason check_error = heap_data->second->FreeHeapBlock(RSV_pack_id.descriptor_heap_offset);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason FreeDescriptorHeap(
		pancy_resource_id &descriptor_heap_id
	);
	PancystarEngine::EngineFailReason GetDescriptorHeap(const ResourceViewPack &heap_id, ID3D12DescriptorHeap **descriptor_heap_use);
	inline PancystarEngine::EngineFailReason GetOffsetNum(ResourceViewPointer heap_pointer, CD3DX12_GPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		PancystarEngine::EngineFailReason check_error;
		auto heap_data = resource_heap_list.find(heap_pointer.resource_view_pack_id.descriptor_heap_type_id);
		if (heap_data == resource_heap_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap ID: " + std::to_string(heap_pointer.resource_view_pack_id.descriptor_heap_type_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor heap offset", error_message);
			return error_message;
		}
		check_error = heap_data->second->GetOffsetNum(heap_pointer.resource_view_pack_id.descriptor_heap_offset, heap_pointer.resource_view_offset_id, descriptor_table);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	inline pancy_object_id GetOffsetNum(ResourceViewPointer heap_pointer, CD3DX12_CPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		auto heap_data = resource_heap_list.find(heap_pointer.resource_view_pack_id.descriptor_heap_type_id);
		if (heap_data == resource_heap_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap ID: " + std::to_string(heap_pointer.resource_view_pack_id.descriptor_heap_type_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor heap offset", error_message);
			return NULL;
		}
		return heap_data->second->GetOffsetNum(heap_pointer.resource_view_pack_id.descriptor_heap_offset, heap_pointer.resource_view_offset_id, descriptor_table);
	}
	//������Դ��ͼ
	PancystarEngine::EngineFailReason BuildSRV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildDSV(
		const ResourceViewPointer &RSV_point,
		const SubMemoryPointer &resource_in,
		const D3D12_DEPTH_STENCIL_VIEW_DESC    &DSV_desc
	);
	~PancyDescriptorHeapControl();
private:
PancystarEngine::EngineFailReason BuildDescriptorHeap(
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &heap_block_size_in,
		const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in,
		ResourceViewPack &RSV_pack_id
	);
};
*/
