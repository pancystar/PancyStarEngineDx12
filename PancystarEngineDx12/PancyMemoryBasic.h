#pragma once
#include"PancyDx12Basic.h"
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
	uint64_t memory_size;//�洢��Ĵ�С
	ComPtr<ID3D12Resource> resource_data;//�洢�������
	D3D12_HEAP_TYPE resource_usage;
	UINT8* map_pointer;
public:
	MemoryBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in,
		D3D12_HEAP_TYPE resource_usage_in
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
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(const int32_t &pointer_offset, const void* copy_data, const int32_t data_size);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const int32_t &pointer_offset, 
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason ReadFromBufferToCpu(const int32_t &pointer_offset, void* copy_data, const int32_t data_size);
};
//�����Դ��
class MemoryHeapGpu
{
	std::string heap_type_name;
	uint64_t size_per_block;
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
	inline uint64_t GetMemorySizePerBlock()
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
	int64_t size_per_block;
	pancy_resource_id max_block_num;
	//�Դ�ѵ�����
	std::string heap_type_name;

	//�����Դ�ѵ�����
	std::unordered_map<pancy_resource_id, MemoryHeapGpu*> memory_heap_data;
	//�ճ����Դ��
	std::unordered_set<pancy_resource_id> empty_memory_heap;
public:
	MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
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
		const pancy_resource_id &commit_block_num,
		const uint64_t &per_block_size,
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
	pancy_object_id per_memory_size;//ÿ�������������Ĵ�С
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
	inline int64_t GetBlockSize() 
	{
		return static_cast<int64_t>(per_memory_size);
	}
};
class SubresourceLiner 
{
	std::string heap_name;
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
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in
	);
	~SubresourceLiner();
	PancystarEngine::EngineFailReason BuildSubresource(
		pancy_object_id &new_memory_block_id, 
		pancy_object_id &sub_memory_offset
	);
	PancystarEngine::EngineFailReason ReleaseSubResource(
		const pancy_object_id &new_memory_block_id,
		const pancy_object_id &sub_memory_offset
	);
	MemoryBlockGpu* GetSubResource(pancy_object_id sub_memory_id, int64_t &per_memory_size);
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
	MemoryBlockGpu*  GetResourceData(const SubMemoryPointer &submemory_pointer,int64_t &per_memory_size);
private:
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
	inline ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap() 
	{
		return heap_data;
	}
	inline pancy_object_id GetOffsetNum(pancy_resource_id heap_offset, pancy_object_id self_offset, CD3DX12_GPU_DESCRIPTOR_HANDLE &descriptor_table)
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap_data->GetGPUDescriptorHandleForHeapStart());
		pancy_object_id id_offset = static_cast<pancy_object_id>(heap_offset) * heap_block_size * per_offset_size + self_offset * per_offset_size;
		srvHandle.Offset(id_offset);
		descriptor_table = srvHandle;
		return id_offset;
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
		ResourceViewPack &RSV_pack_id
	);
	PancystarEngine::EngineFailReason FreeDescriptorHeap(
		pancy_resource_id &descriptor_heap_id
	);
	inline ComPtr<ID3D12DescriptorHeap> GetDescriptorHeap(pancy_resource_id heap_id)
	{
		auto heap_data = resource_heap_list.find(heap_id);
		if (heap_data == resource_heap_list.end()) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find the descriptor heap ID: "+ std::to_string(heap_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor heap",error_message);
			return NULL;
		}
		return heap_data->second->GetDescriptorHeap();
	}
	inline pancy_object_id GetOffsetNum(ResourceViewPointer heap_pointer, CD3DX12_GPU_DESCRIPTOR_HANDLE &descriptor_table)
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

