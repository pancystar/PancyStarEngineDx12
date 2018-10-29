#pragma once
#include"PancyDx12Basic.h"
//显存指针
struct VirtualMemoryPointer
{
	bool if_heap;//资源是否是由堆上分配而来
	pancy_resource_id heap_type;      //堆类型
	pancy_resource_id heap_list_id;   //堆id号
	pancy_resource_id memory_block_id;//内存块id号
	pancy_object_id memory_resource_id;//直接指向内存的id号
	VirtualMemoryPointer()
	{
		if_heap = false;
		heap_type = 0;
		heap_list_id = 0;
		memory_block_id = 0;
		memory_resource_id = 0;
	}
};
//显存块
class MemoryBlockGpu
{
	uint64_t memory_size;//存储块的大小
	ComPtr<ID3D12Resource> resource_data;//存储块的数据
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
//保留显存堆
class MemoryHeapGpu
{
	std::string heap_type_name;
	uint64_t size_per_block;
	pancy_resource_id max_block_num;
	ComPtr<ID3D12Heap> heap_data;
	//std::unordered_set<pancy_resource_id> free_list;

	//所有显存的数据
	std::unordered_map<pancy_resource_id, MemoryBlockGpu*> memory_heap_block;
	//空出的显存
	std::unordered_set<pancy_resource_id> empty_memory_block;
public:
	MemoryHeapGpu(const std::string &heap_type_name_in);
	//每个显存块的大小
	inline uint64_t GetMemorySizePerBlock()
	{
		return size_per_block;
	}
	//显存堆的总显存块数量
	inline pancy_resource_id GetMaxMemoryBlockNum()
	{
		return max_block_num;
	}
	//显存堆尚未使用的显存大小
	inline size_t GetFreeMemoryBlockNum()
	{
		return empty_memory_block.size();
	}
	PancystarEngine::EngineFailReason Create(const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID
	);
	//获取显存资源
	MemoryBlockGpu* GetMemoryResource(const pancy_resource_id &memory_block_ID);
	//检验对应id的资源是否已经被分配
	bool CheckIfFree(pancy_resource_id memory_block_ID);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(const pancy_resource_id &memory_block_ID);
	~MemoryHeapGpu();
};
//线性增长的显存堆
class MemoryHeapLinear
{
	//显存堆的格式
	CD3DX12_HEAP_DESC heap_desc;
	int32_t size_per_block;
	pancy_resource_id max_block_num;
	//显存堆的名称
	std::string heap_type_name;

	//所有显存堆的数据
	std::unordered_map<pancy_resource_id, MemoryHeapGpu*> memory_heap_data;
	//空出的显存堆
	std::unordered_set<pancy_resource_id> empty_memory_heap;
public:
	MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID,//显存块地址指针
		pancy_resource_id &memory_heap_ID//显存段地址指针
	);
	MemoryBlockGpu* GetMemoryResource(
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
		
	);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(
		const pancy_resource_id &memory_heap_ID,
		const pancy_resource_id &memory_block_ID
	);
	~MemoryHeapLinear();
};
//资源管理器
class MemoryHeapGpuControl
{
	pancy_object_id resource_memory_id_self_add;
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, MemoryHeapLinear*> resource_heap_list;//显存堆表
	std::unordered_map<pancy_object_id, MemoryBlockGpu*>resource_memory_list;//离散的显存块表
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
	//不存放在指定堆上的资源
	MemoryBlockGpu* GetMemoryFromList(const pancy_object_id &memory_block_ID);
	PancystarEngine::EngineFailReason FreeResourceCommit(const pancy_object_id &memory_block_ID);
	//存放在指定堆上的资源
	MemoryBlockGpu * GetMemoryResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
	);
	
	PancystarEngine::EngineFailReason LoadHeapFromFile(
		const std::string &HeapFileName,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
	PancystarEngine::EngineFailReason FreeResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
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
//二级资源
struct SubMemoryPointer 
{
	VirtualMemoryPointer buffer_data;
	pancy_object_id offset;
};
class SubMemoryData 
{
	VirtualMemoryPointer buffer_data;//显存资源指针
	pancy_object_id per_memory_size;//每个常量缓冲区的大小
	std::unordered_set<pancy_object_id> empty_sub_memory;
	std::unordered_map<pancy_object_id, SubMemoryPointer*> sub_memory_data;
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

}
//资源描述视图
class PancyResourceView
{
	ComPtr<ID3D12DescriptorHeap> heap_data;
	D3D12_DESCRIPTOR_HEAP_TYPE resource_type;
	//资源视图的位置
	int32_t heap_offset;
	//资源视图包含的资源
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
//资源描述符管理堆
class PancyDescriptorHeap 
{
	std::string descriptor_heap_name;
	pancy_object_id heap_block_size;    //描述符每个存储块的大小(拥有的视图数量)
	pancy_object_id heap_block_num;     //描述符的存储块数量
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc;
	ComPtr<ID3D12DescriptorHeap> heap_data;
	//所有描述符的数据
	std::unordered_map<pancy_object_id, PancyResourceView*> resource_view_heap_block;
	//空出的描述符
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
	//创建资源视图
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
//资源描述符堆管理器
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
	//创建资源视图
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
