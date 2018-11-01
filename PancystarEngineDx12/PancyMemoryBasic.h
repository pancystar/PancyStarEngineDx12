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
public:
	MemoryBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in
	);
	inline ComPtr<ID3D12Resource> GetResource() 
	{
		return resource_data;
	}
	inline uint64_t GetSize()
	{
		return memory_size;
	}
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
		const CD3DX12_RESOURCE_DESC &resource_desc,
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
	int32_t size_per_block;
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
		const CD3DX12_RESOURCE_DESC &resource_desc,
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
		const CD3DX12_RESOURCE_DESC &resource_desc,
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
	std::unordered_map<pancy_object_id, SubMemoryPointer> sub_memory_data;
public:
	SubMemoryData();
	PancystarEngine::EngineFailReason Create(
		const std::string &buffer_desc_file,
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		const pancy_object_id &per_memory_size_in
	);
	PancystarEngine::EngineFailReason BuildSubMemory(pancy_object_id &offset);
	PancystarEngine::EngineFailReason FreeSubMemory(const pancy_object_id &offset);
	inline pancy_object_id GetEmptySize() 
	{
		return static_cast<pancy_object_id>(empty_sub_memory.size());
	}
};
SubMemoryData::SubMemoryData()
{
}
PancystarEngine::EngineFailReason SubMemoryData::Create(
	const std::string &buffer_desc_file,
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	const pancy_object_id &per_memory_size_in
)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = MemoryHeapGpuControl::GetInstance()->BuildResourceFromHeap(buffer_desc_file, resource_desc, resource_state, buffer_data);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	per_memory_size = per_memory_size_in;
	auto memory_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(buffer_data);
	auto check_size = memory_data->GetSize() % static_cast<uint64_t>(per_memory_size_in);
	if (check_size != 0) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"the memory size:"+std::to_string(memory_data->GetSize())+" could not mod the submemory size: "+std::to_string(per_memory_size_in));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build subresource from memory block", error_message);
		return error_message;
	}
	pancy_object_id all_empty_num = static_cast<pancy_object_id>(memory_data->GetSize() / static_cast<uint64_t>(per_memory_size_in));
	for (pancy_object_id i = 0; i < all_empty_num; ++i)
	{
		empty_sub_memory.insert(i);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubMemoryData::BuildSubMemory(pancy_object_id &offset) 
{
	if (empty_sub_memory.size() == 0) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the memory block is full, could not build new memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build subresource from memory block", error_message);
		return error_message;
	}
	auto new_sub_memory = *empty_sub_memory.begin();
	offset = new_sub_memory;
	empty_sub_memory.erase(new_sub_memory);
	SubMemoryPointer new_submemory;
	new_submemory.buffer_data = buffer_data;
	new_submemory.offset = offset;
	sub_memory_data.insert(std::pair<pancy_object_id, SubMemoryPointer>(offset, new_submemory));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubMemoryData::FreeSubMemory(const pancy_object_id &offset)
{
	auto check_data = sub_memory_data.find(offset);
	if (check_data == sub_memory_data.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the sub memory"+ std::to_string(offset)+" from memory_block",PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free subresource from memory block", error_message);
		return error_message;
	}
	sub_memory_data.erase(offset);
	empty_sub_memory.insert(offset);
	return PancystarEngine::succeed;
}
class SubresourceLiner 
{
	std::string heap_name;
	CD3DX12_RESOURCE_DESC resource_desc;
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
		const CD3DX12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in
	);
	PancystarEngine::EngineFailReason BuildSubresource(
		pancy_object_id &new_memory_block_id, 
		pancy_object_id &sub_memory_offset
	);
	PancystarEngine::EngineFailReason ReleaseSubResource(
		const pancy_object_id &new_memory_block_id,
		const pancy_object_id &sub_memory_offset
	);
};
SubresourceLiner::SubresourceLiner(
	const std::string &heap_name_in,
	const CD3DX12_RESOURCE_DESC &resource_desc_in,
	const D3D12_RESOURCE_STATES &resource_state_in,
	const pancy_object_id &per_memory_size_in
) 
{
	max_id = 0;
	heap_name = heap_name_in;
	resource_desc = resource_desc_in;
	resource_state = resource_state_in;
	per_memory_size = per_memory_size_in;
}
PancystarEngine::EngineFailReason SubresourceLiner::BuildSubresource(
	pancy_object_id &new_memory_block_id, 
	pancy_object_id &sub_memory_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	if (empty_memory_heap.size() == 0) 
	{
		SubMemoryData *new_data = new SubMemoryData();
		check_error = new_data->Create(heap_name, resource_desc, resource_state, per_memory_size);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		pancy_object_id id_now;
		//��ȡ��ǰ���õ�id��
		if (free_id.size() == 0) 
		{
			id_now = max_id;
			max_id += 1;
		}
		else 
		{
			id_now = *free_id.begin();
			free_id.erase(id_now);
		}
		//����һ���յ���Դ
		submemory_list.insert(std::pair<pancy_object_id, SubMemoryData*>(id_now, new_data));
		empty_memory_heap.insert(id_now);
	}
	//��ȡһ�����пռ���ڴ��
	new_memory_block_id = *empty_memory_heap.begin();
	auto new_memory_block = submemory_list.find(new_memory_block_id);
	//�ڸ��ڴ���п���һ��subresource
	check_error = new_memory_block->second->BuildSubMemory(sub_memory_offset);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	//��鿪�ٹ�����ڴ���Ƿ�����
	if (new_memory_block->second->GetEmptySize() == 0) 
	{
		empty_memory_heap.erase(new_memory_block_id);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceLiner::ReleaseSubResource
(
	const pancy_object_id &new_memory_block_id,
	const pancy_object_id &sub_memory_offset
) 
{
	auto memory_check = submemory_list.find(new_memory_block_id);
	if (memory_check == submemory_list.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find memory_block id:"+std::to_string(new_memory_block_id)+" from submemory list: "+ heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Release sub memory from submemory list",error_message);
		return error_message;
	}
	memory_check->second->FreeSubMemory(sub_memory_offset);
	//����Ƿ�֮ǰ����
	if (empty_memory_heap.find(new_memory_block_id) == empty_memory_heap.end()) 
	{
		empty_memory_heap.insert(new_memory_block_id);
	}
	return PancystarEngine::succeed;
}
class SubresourceControl 
{
	pancy_object_id subresource_id_self_add;
	std::unordered_map<pancy_resource_id, SubresourceLiner*> subresource_list_map;
	std::unordered_map<std::string, pancy_resource_id> subresource_init_list;
	std::unordered_set<pancy_object_id> subresource_free_id;
public:
	SubresourceControl();
	~SubresourceControl();
	void InitSubResourceType(
		const std::string &heap_name_in,
		const CD3DX12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		pancy_resource_id &subresource_type_id
	);
	PancystarEngine::EngineFailReason BuildSubresource(
		const std::string &heap_name_in,
		const CD3DX12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		SubMemoryPointer &submemory_pointer
	);
	PancystarEngine::EngineFailReason FreeSubResource(const SubMemoryPointer &submemory_pointer);
	PancystarEngine::EngineFailReason GetDx12ResourceData(const SubMemoryPointer &submemory_pointer);
};
SubresourceControl::SubresourceControl()
{
	subresource_id_self_add = 0;
}
void SubresourceControl::InitSubResourceType(
	const std::string &heap_name_in,
	const CD3DX12_RESOURCE_DESC &resource_desc_in,
	const D3D12_RESOURCE_STATES &resource_state_in,
	const pancy_object_id &per_memory_size_in,
	pancy_resource_id &subresource_type_id
) 
{
	std::string hash_name = heap_name_in + "::" + std::to_string(resource_state_in)+"::"+std::to_string(per_memory_size_in);
	SubresourceLiner *new_subresource = new SubresourceLiner(heap_name_in, resource_desc_in, resource_state_in, per_memory_size_in);
	if (subresource_free_id.size() == 0) 
	{
		subresource_type_id = *subresource_free_id.begin();
		subresource_free_id.erase(subresource_type_id);
	}
	else 
	{
		subresource_type_id = subresource_id_self_add;
		subresource_id_self_add += 1;
	}
	//����һ���µ���Դ�洢������������
	subresource_list_map.insert(std::pair<pancy_resource_id, SubresourceLiner*>(subresource_type_id, new_subresource));
	subresource_init_list.insert(std::pair<std::string, pancy_resource_id>(hash_name, subresource_type_id));
}
PancystarEngine::EngineFailReason SubresourceControl::BuildSubresource(
	const std::string &heap_name_in,
	const CD3DX12_RESOURCE_DESC &resource_desc_in,
	const D3D12_RESOURCE_STATES &resource_state_in,
	const pancy_object_id &per_memory_size_in,
	SubMemoryPointer &submemory_pointer
) 
{
	PancystarEngine::EngineFailReason check_error;
	std::string hash_name = heap_name_in + "::" + std::to_string(resource_state_in) + "::" + std::to_string(per_memory_size_in);
	//��ȡ��Դ���ƶ�Ӧ��id��,���û�������´���һ��
	auto check_data = subresource_init_list.find(hash_name);
	if (check_data == subresource_init_list.end())
	{
		InitSubResourceType(heap_name_in, resource_desc_in, resource_state_in, per_memory_size_in, submemory_pointer.type_id);
	}
	else 
	{
		submemory_pointer.type_id = check_data->second;
	}
	auto now_subresource_type = subresource_list_map.find(submemory_pointer.type_id);
	check_error = now_subresource_type->second->BuildSubresource(submemory_pointer.list_id, submemory_pointer.offset);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubresourceControl::FreeSubResource(const SubMemoryPointer &submemory_pointer)
{
	PancystarEngine::EngineFailReason check_error;
	auto now_subresource_type = subresource_list_map.find(submemory_pointer.type_id);
	if (now_subresource_type == subresource_list_map.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_type id:" + std::to_string(submemory_pointer.type_id) + " from subresource control: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Release sub memory from submemory control", error_message);
		return error_message;
	}
	check_error = now_subresource_type->second->ReleaseSubResource(submemory_pointer.list_id, submemory_pointer.offset);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//todo�����ģ�ͷźʹ�����Դ�Ĳ���
//��Դ������ͼ
class PancyResourceView
{
	ComPtr<ID3D12DescriptorHeap> heap_data;
	D3D12_DESCRIPTOR_HEAP_TYPE resource_type;
	//��Դ��ͼ��λ��
	int32_t heap_offset;
	//��Դ��ͼ��������Դ
	int32_t resource_view_number;
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
		const VirtualMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const pancy_object_id &self_offset, 
		const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const pancy_object_id &self_offset, 
		const VirtualMemoryPointer &resource_in, 
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const pancy_object_id &self_offset, 
		const VirtualMemoryPointer &resource_in, 
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
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
	//������Դ��ͼ
	PancystarEngine::EngineFailReason BuildSRV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset,
		const VirtualMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const VirtualMemoryPointer &resource_in, 
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const VirtualMemoryPointer &resource_in, 
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
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
	PancystarEngine::EngineFailReason BuildDescriptorHeap(
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &heap_block_size_in,
		const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in,
		pancy_resource_id &descriptor_heap_id
	);
	PancystarEngine::EngineFailReason FreeDescriptorHeap(
		pancy_resource_id &descriptor_heap_id
	);
	//������Դ��ͼ
	PancystarEngine::EngineFailReason BuildSRV(
		const pancy_resource_id &descriptor_heap_id,
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset, 
		const VirtualMemoryPointer &resource_in,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildCBV(
		const pancy_resource_id &descriptor_heap_id, 
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc
	);
	PancystarEngine::EngineFailReason BuildUAV(
		const pancy_resource_id &descriptor_heap_id, 
		const pancy_object_id &descriptor_block_id,
		const pancy_object_id &self_offset, 
		const VirtualMemoryPointer &resource_in, 
		const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildRTV(
		const pancy_resource_id &descriptor_heap_id, 
		const pancy_object_id &descriptor_block_id, 
		const pancy_object_id &self_offset, 
		const VirtualMemoryPointer &resource_in, 
		const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
	);
	~PancyDescriptorHeapControl();
};
