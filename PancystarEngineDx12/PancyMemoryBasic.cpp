#include"PancyMemoryBasic.h"
//GPU资源块
MemoryBlockGpu::MemoryBlockGpu(
	const uint64_t &memory_size_in,
	ComPtr<ID3D12Resource> resource_data_in
)
{
	memory_size = memory_size_in;
	resource_data = resource_data_in;
}
//GPU资源堆
MemoryHeapGpu::MemoryHeapGpu(const std::string &heap_type_name_in)
{
	heap_type_name = heap_type_name_in;
	size_per_block = 0;
	max_block_num = 0;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::Create(const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in)
{
	size_per_block = size_per_block_in;
	max_block_num = max_block_num_in;
	//检查堆缓存的大小
	if (heap_desc_in.SizeInBytes != size_per_block * max_block_num)
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "Memory Heap Size In" + heap_type_name + " need " + std::to_string(size_per_block * max_block_num) + " But Find " + std::to_string(heap_desc_in.SizeInBytes));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty Heap", check_error);
		return check_error;
	}
	//创建资源堆
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateHeap(&heap_desc_in, IID_PPV_ARGS(&heap_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Create Memory Heap " + heap_type_name + "error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty Heap", check_error);
		return check_error;
	}
	//初始化堆内空闲的资源块
	for (pancy_resource_id i = 0; i < max_block_num; ++i)
	{
		empty_memory_block.insert(i);
	}
	return PancystarEngine::succeed;
}
bool MemoryHeapGpu::CheckIfFree(pancy_resource_id memory_block_ID)
{
	if (memory_block_ID >= max_block_num)
	{
		return false;
	}
	auto check_data = empty_memory_block.find(memory_block_ID);
	if (check_data != empty_memory_block.end())
	{
		return true;
	}
	return false;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::BuildMemoryResource(
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	//Microsoft::WRL::Details::ComPtrRef<ComPtr<ID3D12Resource>> ppvResourc,
	pancy_resource_id &memory_block_ID
)
{
	ComPtr<ID3D12Resource> ppvResourc;
	//检查是否还有空余的存储空间
	auto rand_free_memory = empty_memory_block.begin();
	if (rand_free_memory == empty_memory_block.end())
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The Heap " + heap_type_name + " Is empty, can't alloc new memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	auto check_desc = heap_data->GetDesc();
	//在显存堆上创建显存资源
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreatePlacedResource(
		heap_data.Get(),
		(*rand_free_memory) * size_per_block,
		&resource_desc,
		resource_state,
		nullptr,
		IID_PPV_ARGS(&ppvResourc)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Allocate Memory From Heap " + heap_type_name + "error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Allocated Memorty From Heap", check_error);
		return check_error;
	}
	memory_block_ID = *rand_free_memory;
	empty_memory_block.erase(rand_free_memory);
	MemoryBlockGpu *new_memory_block_data = new MemoryBlockGpu(size_per_block, ppvResourc);
	memory_heap_block.insert(std::pair<pancy_resource_id, MemoryBlockGpu*>(memory_block_ID, new_memory_block_data));
	return PancystarEngine::succeed;
}
MemoryBlockGpu* MemoryHeapGpu::GetMemoryResource(const pancy_resource_id &memory_block_ID) 
{
	auto check_data = memory_heap_block.find(memory_block_ID);
	if (check_data == memory_heap_block.end()) 
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The memory block ID " + std::to_string(memory_block_ID) + " Haven't been allocated or illegal memory id");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap"+ heap_type_name, check_error);
		return NULL;
	}
	return check_data->second;
}
PancystarEngine::EngineFailReason MemoryHeapGpu::FreeMemoryReference(const pancy_resource_id &memory_block_ID)
{
	if (memory_block_ID >= max_block_num)
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The heap " + heap_type_name + " Only have " + std::to_string(max_block_num) + " Memory block,ID " + std::to_string(memory_block_ID) + " Out of range");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	auto check_data = empty_memory_block.find(memory_block_ID);
	if (check_data != empty_memory_block.end())
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The memory block ID " + std::to_string(memory_block_ID) + " Haven't been allocated");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	auto memory_data = memory_heap_block.find(memory_block_ID);
	if (memory_data == memory_heap_block.end()) 
	{
		PancystarEngine::EngineFailReason check_error(E_FAIL, "The memory block ID " + std::to_string(memory_block_ID) + " Haven't been allocated or illegal memory id");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free Memorty From Heap", check_error);
		return check_error;
	}
	empty_memory_block.insert(memory_block_ID);
	delete memory_data->second;
	memory_heap_block.erase(memory_data);
	return PancystarEngine::succeed;
}
MemoryHeapGpu::~MemoryHeapGpu()
{
	for (auto data_heap = memory_heap_block.begin(); data_heap != memory_heap_block.end(); ++data_heap)
	{
		delete data_heap->second;
	}
	memory_heap_block.clear();
}
//GPU线性增长的资源堆
MemoryHeapLinear::MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in)
{
	heap_desc = heap_desc_in;
	size_per_block = size_per_block_in;
	max_block_num = max_block_num_in;
	heap_type_name = heap_type_name_in;
}
PancystarEngine::EngineFailReason MemoryHeapLinear::BuildMemoryResource(
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	pancy_resource_id &memory_block_ID,//显存块地址指针
	pancy_resource_id &memory_heap_ID//显存段地址指针
)
{
	if (empty_memory_heap.size() == 0)
	{
		pancy_resource_id new_id = static_cast<pancy_resource_id>(memory_heap_data.size());
		if (new_id + static_cast<pancy_resource_id>(1) < new_id)
		{
			//内存已经满了
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap: " + heap_type_name + " was full so could not get empty memory");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build resource heap list", error_message);
			return error_message;
		}
		//创建一个空的存储段
		MemoryHeapGpu *new_heap = new MemoryHeapGpu(heap_type_name);
		auto check_error = new_heap->Create(heap_desc, size_per_block, max_block_num);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		memory_heap_data.insert(std::pair<pancy_resource_id, MemoryHeapGpu*>(new_id + static_cast<pancy_resource_id>(1), new_heap));
		empty_memory_heap.insert(new_id + static_cast<pancy_resource_id>(1));
	}
	//挑选一个空的存储段
	pancy_resource_id new_empty_id = *empty_memory_heap.begin();
	auto new_empty_heap = memory_heap_data.find(new_empty_id);
	//开辟存储空间
	auto check_error = new_empty_heap->second->BuildMemoryResource(resource_desc, resource_state, memory_block_ID);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	memory_heap_ID = new_empty_id;
	//如果开辟空间后，该堆的存储空间满，将其从待分配队列移除
	if (new_empty_heap->second->GetFreeMemoryBlockNum() == 0)
	{
		empty_memory_heap.erase(new_empty_id);
	}
	return PancystarEngine::succeed;
}
MemoryBlockGpu* MemoryHeapLinear::GetMemoryResource(
	const pancy_resource_id &memory_heap_ID,//显存段地址指针
	const pancy_resource_id &memory_block_ID//显存块地址指针
	
)
{
	//根据段指针找到显存段
	auto memory_heap_now = memory_heap_data.find(memory_heap_ID);
	if (memory_heap_now == memory_heap_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap: " + std::to_string(memory_heap_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free resource heap data", error_message);
		return NULL;
	}
	//根据块指针找到显存块
	return memory_heap_now->second->GetMemoryResource(memory_block_ID);
}
PancystarEngine::EngineFailReason MemoryHeapLinear::FreeMemoryReference(
	const pancy_resource_id &memory_heap_ID,
	const pancy_resource_id &memory_block_ID
)
{
	//根据段指针找到显存段
	auto memory_heap_now = memory_heap_data.find(memory_heap_ID);
	if (memory_heap_now == memory_heap_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap: " + std::to_string(memory_heap_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free resource heap data", error_message);
		return error_message;
	}
	//根据块指针释放显存块
	memory_heap_now->second->FreeMemoryReference(memory_block_ID);
	//如果显存资源堆之前已满，释放资源后重新放入待分配池
	if (empty_memory_heap.find(memory_heap_ID) == empty_memory_heap.end())
	{
		empty_memory_heap.insert(memory_heap_ID);
	}
	return PancystarEngine::succeed;
}
MemoryHeapLinear::~MemoryHeapLinear()
{
	for (auto data_heap = memory_heap_data.begin(); data_heap != memory_heap_data.end(); ++data_heap)
	{
		delete data_heap->second;
	}
	memory_heap_data.clear();
	empty_memory_heap.clear();
}
//GPU资源堆管理器
MemoryHeapGpuControl::MemoryHeapGpuControl()
{
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_DEFAULT", static_cast<int32_t>(D3D12_HEAP_TYPE_DEFAULT));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_UPLOAD", static_cast<int32_t>(D3D12_HEAP_TYPE_UPLOAD));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_READBACK", static_cast<int32_t>(D3D12_HEAP_TYPE_READBACK));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_TYPE_CUSTOM", static_cast<int32_t>(D3D12_HEAP_TYPE_CUSTOM));

	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_NONE", static_cast<int32_t>(D3D12_HEAP_FLAG_NONE));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_SHARED", static_cast<int32_t>(D3D12_HEAP_FLAG_SHARED));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_BUFFERS", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_BUFFERS));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_DISPLAY", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_DISPLAY));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER", static_cast<int32_t>(D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_HARDWARE_PROTECTED", static_cast<int32_t>(D3D12_HEAP_FLAG_HARDWARE_PROTECTED));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_WRITE_WATCH));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_SHADER_ATOMICS));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES));
	PancyJsonTool::GetInstance()->SetGlobelVraiable("D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES", static_cast<int32_t>(D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES));
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildResourceCommit(
	const D3D12_HEAP_TYPE &heap_type_in,
	const D3D12_HEAP_FLAGS &heap_flag_in,
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	VirtualMemoryPointer &virtual_pointer
)
{
	//创建资源
	ComPtr<ID3D12Resource> ppvResourc;
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(heap_type_in),
		heap_flag_in,
		&resource_desc,
		resource_state,
		nullptr,
		IID_PPV_ARGS(&ppvResourc));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Build commit memory resource error ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty From List", check_error);
		return check_error;
	}
	if (resource_memory_free_id.size() > 0)
	{
		virtual_pointer.memory_resource_id = *resource_memory_free_id.begin();
		resource_memory_free_id.erase(virtual_pointer.memory_resource_id);
	}
	else
	{
		if (resource_memory_list.size() + static_cast<pancy_resource_id>(1) > resource_memory_list.size())
		{
			virtual_pointer.memory_resource_id = static_cast<pancy_resource_id>(resource_memory_list.size());
		}
		else
		{
			PancystarEngine::EngineFailReason check_error(hr, "commit resource memory list if full,use big id to recombile project");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Memorty From List", check_error);
			return check_error;
		}
	}
	MemoryBlockGpu *new_block = new MemoryBlockGpu(virtual_pointer.memory_resource_id, ppvResourc);
	resource_memory_list.insert(std::pair<pancy_object_id, MemoryBlockGpu *>(virtual_pointer.memory_resource_id, new_block));
	return PancystarEngine::succeed;
}
MemoryBlockGpu* MemoryHeapGpuControl::GetMemoryResource(const VirtualMemoryPointer &virtual_pointer)
{
	if (virtual_pointer.if_heap) 
	{
		return GetMemoryResourceFromHeap(virtual_pointer.heap_type, virtual_pointer.heap_list_id, virtual_pointer.memory_block_id);
	}
	return GetMemoryFromList(virtual_pointer.memory_resource_id);
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::FreeResource(const VirtualMemoryPointer &virtual_pointer)
{
	if (virtual_pointer.if_heap)
	{
		return FreeResourceFromHeap(virtual_pointer.heap_type, virtual_pointer.heap_list_id, virtual_pointer.memory_block_id);
	}
	return FreeResourceCommit(virtual_pointer.memory_resource_id);
}
MemoryBlockGpu* MemoryHeapGpuControl::GetMemoryFromList(const pancy_object_id &memory_block_ID)
{
	auto check_data = resource_memory_list.find(memory_block_ID);
	if (check_data == resource_memory_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the commit resource id:" + std::to_string(memory_block_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get commit resource  data", error_message);
		return NULL;
	}
	return check_data->second;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::FreeResourceCommit(const pancy_object_id &memory_block_ID)
{
	auto check_data = resource_memory_list.find(memory_block_ID);
	if (check_data == resource_memory_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the commit resource id:" + std::to_string(memory_block_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free commit resource  data", error_message);
		return error_message;
	}
	delete check_data->second;
	resource_memory_free_id.insert(check_data->first);
	resource_memory_list.erase(check_data);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::LoadHeapFromFile(
	const std::string &HeapFileName,
	pancy_resource_id &resource_id,
	uint64_t heap_alignment_size
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_resource_id commit_block_num;
	uint64_t per_block_size;
	D3D12_HEAP_TYPE heap_type_in;
	D3D12_HEAP_FLAGS heap_flag_in;
	Json::Value root_value;
	pancy_json_value rec_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(HeapFileName, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, root_value, "commit_block_num", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	commit_block_num = static_cast<pancy_resource_id>(rec_value.int_value);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, root_value, "per_block_size", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_block_size = static_cast<uint64_t>(rec_value.int_value);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, root_value, "heap_type_in", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	heap_type_in = static_cast<D3D12_HEAP_TYPE>(rec_value.int_value);
	Json::Value value_heap_flags = root_value.get("heap_flag_in", Json::Value::null);
	int32_t rec_data = 0;
	for (uint32_t i = 0; i < value_heap_flags.size(); ++i)
	{
		PancyJsonTool::GetInstance()->GetJsonData(HeapFileName, value_heap_flags, i, pancy_json_data_type::json_data_enum, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		rec_data = rec_data | rec_value.int_value;
	}
	heap_flag_in = static_cast<D3D12_HEAP_FLAGS>(rec_data);
	check_error = BuildHeap(HeapFileName, commit_block_num, per_block_size, heap_type_in, heap_flag_in, resource_id, heap_alignment_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildHeap(
	const std::string &heap_desc_name,
	const pancy_resource_id &commit_block_num,
	const uint64_t &per_block_size,
	const D3D12_HEAP_TYPE &heap_type_in,
	const D3D12_HEAP_FLAGS &heap_flag_in,
	pancy_resource_id &resource_id,
	uint64_t heap_alignment_size
)
{
	auto check_resource_id = resource_init_list.find(heap_desc_name);
	if (check_resource_id != resource_init_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource: " + check_resource_id->first + "jave been build", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build resource heap from file", error_message);
		return error_message;
	}
	CD3DX12_HEAP_DESC heapDesc(commit_block_num * per_block_size, heap_type_in, heap_alignment_size, heap_flag_in);
	MemoryHeapLinear *new_heap = new MemoryHeapLinear(heap_desc_name, heapDesc, per_block_size, commit_block_num);
	resource_id = static_cast<pancy_resource_id>(resource_init_list.size());
	resource_init_list.insert(std::pair<std::string, pancy_resource_id>(heap_desc_name, resource_id));
	resource_heap_list.insert(std::pair<pancy_resource_id, MemoryHeapLinear*>(resource_id, new_heap));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::BuildResourceFromHeap(
	const std::string &HeapFileName,
	const CD3DX12_RESOURCE_DESC &resource_desc,
	const D3D12_RESOURCE_STATES &resource_state,
	VirtualMemoryPointer &virtual_pointer
)
{
	virtual_pointer.if_heap = true;
	auto heap_list_id = resource_init_list.find(HeapFileName);
	if (heap_list_id == resource_init_list.end())
	{
		auto check_error = LoadHeapFromFile(HeapFileName, virtual_pointer.heap_type);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else 
	{
		virtual_pointer.heap_type = heap_list_id->second;
	}
	auto heap_list_data = resource_heap_list.find(virtual_pointer.heap_type);
	auto check_error = heap_list_data->second->BuildMemoryResource(resource_desc, resource_state, virtual_pointer.memory_block_id, virtual_pointer.heap_list_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
MemoryBlockGpu* MemoryHeapGpuControl::GetMemoryResourceFromHeap(
	const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
	const pancy_resource_id &memory_heap_ID,//显存段地址指针
	const pancy_resource_id &memory_block_ID//显存块地址指针
) 
{
	auto heap_list_data = resource_heap_list.find(memory_heap_list_ID);
	if (heap_list_data == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap list: " + std::to_string(memory_heap_list_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get resource heap data", error_message);
		return NULL;
	}
	return heap_list_data->second->GetMemoryResource(memory_heap_ID, memory_block_ID);
}
PancystarEngine::EngineFailReason MemoryHeapGpuControl::FreeResourceFromHeap(
	const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
	const pancy_resource_id &memory_heap_ID,//显存段地址指针
	const pancy_resource_id &memory_block_ID//显存块地址指针
)
{
	auto heap_list_data = resource_heap_list.find(memory_heap_list_ID);
	if (heap_list_data == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource heap list: " + std::to_string(memory_heap_list_ID) + " could not be find");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free resource heap data", error_message);
		return error_message;
	}
	auto check_error = heap_list_data->second->FreeMemoryReference(memory_heap_ID, memory_block_ID);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
MemoryHeapGpuControl::~MemoryHeapGpuControl()
{
	for (auto data_heap = resource_heap_list.begin(); data_heap != resource_heap_list.end(); ++data_heap)
	{
		delete data_heap->second;
	}
	for (auto data_res = resource_memory_list.begin(); data_res != resource_memory_list.end(); ++data_res)
	{
		delete data_res->second;
	}
	resource_heap_list.clear();
	resource_init_list.clear();
	resource_memory_list.clear();
	resource_memory_free_id.clear();
}
//资源描述视图
PancyResourceView::PancyResourceView(
	ComPtr<ID3D12DescriptorHeap> heap_data_in,
	const int32_t &heap_offset_in,
	D3D12_DESCRIPTOR_HEAP_TYPE &resource_type_in,
	const int32_t &view_number_in
)
{
	heap_data = heap_data_in;
	heap_offset = heap_offset_in;
	resource_view_number = view_number_in;
	resource_type = resource_type_in;
	resource_block_size = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetDescriptorHandleIncrementSize(resource_type);
}
PancystarEngine::EngineFailReason PancyResourceView::BuildSRV(
	const pancy_object_id &self_offset,
	const VirtualMemoryPointer &resource_in, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the srv id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add SRV to descriptor heap block", error_message);
		return error_message;
	}
	//检验资源是否存在
	auto resource_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(resource_in);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource from pointer: " + std::to_string(resource_in.heap_type) + "::" + std::to_string(resource_in.heap_list_id) + "::" + std::to_string(resource_in.memory_block_id) + "//" + std::to_string(resource_in.memory_resource_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add SRV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateShaderResourceView(resource_data->GetResource().Get(), &SRV_desc, cpuHandle);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildCBV(
	const pancy_object_id &self_offset, 
	const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc
)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the CBV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add CBV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateConstantBufferView(&CBV_desc, cpuHandle);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildUAV(
	const pancy_object_id &self_offset, 
	const VirtualMemoryPointer &resource_in, 
	const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the UAV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add UAV to descriptor heap block", error_message);
		return error_message;
	}
	//检验资源是否存在
	auto resource_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(resource_in);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource from pointer: " + std::to_string(resource_in.heap_type) + "::" + std::to_string(resource_in.heap_list_id) + "::" + std::to_string(resource_in.memory_block_id) + "//" + std::to_string(resource_in.memory_resource_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add UAV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	/*
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateUnorderedAccessView(resource_data->GetResource().Get(), &UAV_desc, cpuHandle);
	*/
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyResourceView::BuildRTV(
	const pancy_object_id &self_offset,
	const VirtualMemoryPointer &resource_in, 
	const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
)
{
	//检验偏移是否合法
	if (self_offset >= resource_view_number)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the RTV id: " + std::to_string(self_offset) + " is bigger than the max id of descriptor heap block");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add RTV to descriptor heap block", error_message);
		return error_message;
	}
	//检验资源是否存在
	auto resource_data = MemoryHeapGpuControl::GetInstance()->GetMemoryResource(resource_in);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource from pointer: " + std::to_string(resource_in.heap_type) + "::" + std::to_string(resource_in.heap_list_id) + "::" + std::to_string(resource_in.memory_block_id) + "//" + std::to_string(resource_in.memory_resource_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add RTV to descriptor heap block", error_message);
		return error_message;
	}
	//创建描述符
	int32_t heap_start_pos = heap_offset * resource_view_number * static_cast<int32_t>(resource_block_size) + static_cast<int32_t>(self_offset) * static_cast<int32_t>(resource_block_size);
	CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(heap_data->GetCPUDescriptorHandleForHeapStart());
	cpuHandle.Offset(heap_start_pos);
	PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRenderTargetView(resource_data->GetResource().Get(), &RTV_desc, cpuHandle);
	return PancystarEngine::succeed;
}
//资源描述符管理堆
PancyDescriptorHeap::PancyDescriptorHeap(
	const std::string &descriptor_heap_name_in,
	const pancy_object_id &heap_block_size_in,
	const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in
)
{
	descriptor_heap_name = descriptor_heap_name_in;
	heap_block_size = heap_block_size_in;
	heap_block_num = heap_desc_in.NumDescriptors / heap_block_size_in;
	heap_desc = heap_desc_in;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::Create()
{
	//创建描述符堆
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(&heap_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "create descriptor heap error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap", error_message);
		return error_message;
	}
	//将描述符堆内的数据全部赋为空闲状态
	for (pancy_object_id i = 0; i < heap_block_num; ++i)
	{
		empty_view_block.insert(i);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildHeapBlock(pancy_resource_id &resource_view_ID)
{
	if (resource_view_ID > heap_block_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap resource id: " + std::to_string(resource_view_ID) + " is bigger than the descriptor heap" + descriptor_heap_name + " size: " + std::to_string(heap_block_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	if (resource_view_heap_block.find(resource_view_ID) != resource_view_heap_block.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap resource id: " + std::to_string(resource_view_ID) + " is already build do not rebuild resource in heap: " + descriptor_heap_name, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	if (empty_view_block.size() == 0)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap" + descriptor_heap_name + " do not have enough space to build a new resource view");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource view from desciptor heap", error_message);
		return error_message;
	}
	pancy_object_id empty_id = *empty_view_block.begin();
	PancyResourceView *new_data = new PancyResourceView(heap_data, empty_id, heap_desc.Type, heap_block_size);
	resource_view_heap_block.insert(std::pair<pancy_object_id, PancyResourceView*>(empty_id, new_data));
	empty_view_block.erase(empty_id);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::FreeHeapBlock(const pancy_resource_id &resource_view_ID)
{
	auto now_resource_remove = resource_view_heap_block.find(resource_view_ID);
	if (now_resource_remove == resource_view_heap_block.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find heap resource id: " + std::to_string(resource_view_ID) + " in descriptor heap: " + descriptor_heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("remove resource view from desciptor heap", error_message);
		return error_message;
	}
	delete now_resource_remove->second;
	empty_view_block.insert(now_resource_remove->first);
	resource_view_heap_block.erase(now_resource_remove);
	return PancystarEngine::succeed;
}
PancyResourceView* PancyDescriptorHeap::GetHeapBlock(const pancy_resource_id &resource_view_ID, PancystarEngine::EngineFailReason &check_error)
{
	auto now_resource_remove = resource_view_heap_block.find(resource_view_ID);
	if (now_resource_remove == resource_view_heap_block.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find heap resource id: " + std::to_string(resource_view_ID) + " in descriptor heap: " + descriptor_heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("remove resource view from desciptor heap", error_message);
		check_error = error_message;
		return NULL;
	}
	check_error = PancystarEngine::succeed;
	return now_resource_remove->second;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildSRV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const VirtualMemoryPointer &resource_in,
	const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildSRV(self_offset, resource_in, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildCBV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildCBV(self_offset, CBV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildUAV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const VirtualMemoryPointer &resource_in,
	const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildUAV(self_offset, resource_in, UAV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildRTV(
	const pancy_object_id &descriptor_block_id,
	const pancy_object_id &self_offset,
	const VirtualMemoryPointer &resource_in,
	const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
)
{
	PancystarEngine::EngineFailReason check_error;
	PancyResourceView *descriptor_heap_use = GetHeapBlock(descriptor_block_id, check_error);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = descriptor_heap_use->BuildRTV(self_offset, resource_in, RTV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancyDescriptorHeap::~PancyDescriptorHeap()
{
	for (auto free_data = resource_view_heap_block.begin(); free_data != resource_view_heap_block.end(); ++free_data)
	{
		delete free_data->second;
	}
	resource_view_heap_block.clear();
}
//资源描述符堆管理器
PancyDescriptorHeapControl::PancyDescriptorHeapControl()
{
	descriptor_heap_id_selfadd = 0;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildDescriptorHeap(
	const std::string &descriptor_heap_name_in,
	const pancy_object_id &heap_block_size_in,
	const D3D12_DESCRIPTOR_HEAP_DESC &heap_desc_in,
	pancy_resource_id &descriptor_heap_id
)
{
	if (resource_init_list.find(descriptor_heap_name_in) != resource_init_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap: " + descriptor_heap_name_in + " already been build before", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap", error_message);
		return error_message;
	}
	if (resource_memory_free_id.size() != 0)
	{
		descriptor_heap_id = *resource_memory_free_id.begin();
		resource_memory_free_id.erase(descriptor_heap_id);
	}
	else
	{
		descriptor_heap_id = descriptor_heap_id_selfadd;
		descriptor_heap_id_selfadd += 1;
	}
	PancyDescriptorHeap *new_heap = new PancyDescriptorHeap(descriptor_heap_name_in, heap_block_size_in, heap_desc_in);
	PancystarEngine::EngineFailReason check_error = new_heap->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	resource_init_list.insert(std::pair<std::string, pancy_resource_id>(descriptor_heap_name_in, descriptor_heap_id));
	resource_heap_list.insert(std::pair<pancy_resource_id, PancyDescriptorHeap*>(descriptor_heap_id, new_heap));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::FreeDescriptorHeap(pancy_resource_id &descriptor_heap_id)
{
	if (resource_heap_list.find(descriptor_heap_id) == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free descriptor heap", error_message);
		return error_message;
	}
	auto now_free_data = resource_heap_list.find(descriptor_heap_id);
	//删除资源的名称存档
	resource_init_list.erase(now_free_data->second->GetDescriptorName());
	//释放资源的id到空闲队列
	resource_memory_free_id.insert(descriptor_heap_id);
	//删除资源
	delete now_free_data->second;
	//删除资源的记录
	resource_heap_list.erase(now_free_data);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildSRV(
	const pancy_resource_id &descriptor_heap_id, 
	const pancy_object_id &descriptor_block_id, 
	const pancy_object_id &self_offset, 
	const VirtualMemoryPointer &resource_in, 
	const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
)
{
	auto descriptor_heap_use = resource_heap_list.find(descriptor_heap_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildSRV(descriptor_block_id, self_offset, resource_in, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCBV(
	const pancy_resource_id &descriptor_heap_id, 
	const pancy_object_id &descriptor_block_id, 
	const pancy_object_id &self_offset, 
	const D3D12_CONSTANT_BUFFER_VIEW_DESC  &CBV_desc
)
{
	auto descriptor_heap_use = resource_heap_list.find(descriptor_heap_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildCBV(descriptor_block_id, self_offset, CBV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildUAV(
	const pancy_resource_id &descriptor_heap_id, 
	const pancy_object_id &descriptor_block_id, 
	const pancy_object_id &self_offset, 
	const VirtualMemoryPointer &resource_in, 
	const D3D12_UNORDERED_ACCESS_VIEW_DESC &UAV_desc
)
{
	auto descriptor_heap_use = resource_heap_list.find(descriptor_heap_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildUAV(descriptor_block_id, self_offset, resource_in, UAV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildRTV(
	const pancy_resource_id &descriptor_heap_id, 
	const pancy_object_id &descriptor_block_id, 
	const pancy_object_id &self_offset,
	const VirtualMemoryPointer &resource_in, 
	const D3D12_RENDER_TARGET_VIEW_DESC    &RTV_desc
)
{
	auto descriptor_heap_use = resource_heap_list.find(descriptor_heap_id);
	if (descriptor_heap_use == resource_heap_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the descriptor heap id: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Resource view from descriptor heap", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = descriptor_heap_use->second->BuildRTV(descriptor_block_id, self_offset, resource_in, RTV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancyDescriptorHeapControl::~PancyDescriptorHeapControl()
{
	std::vector<pancy_resource_id> free_id_list;
	for (auto data_free = resource_heap_list.begin(); data_free != resource_heap_list.end(); ++data_free)
	{
		free_id_list.push_back(data_free->first);
	}
	for (int i = 0; i < free_id_list.size(); ++i)
	{
		FreeDescriptorHeap(free_id_list[i]);
	}
	resource_memory_free_id.clear();
}
//二级资源
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the memory size:" + std::to_string(memory_data->GetSize()) + " could not mod the submemory size: " + std::to_string(per_memory_size_in));
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
	sub_memory_data.insert(offset);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SubMemoryData::FreeSubMemory(const pancy_object_id &offset)
{
	auto check_data = sub_memory_data.find(offset);
	if (check_data == sub_memory_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the sub memory" + std::to_string(offset) + " from memory_block", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free subresource from memory block", error_message);
		return error_message;
	}
	sub_memory_data.erase(offset);
	empty_sub_memory.insert(offset);
	return PancystarEngine::succeed;
}
//二级资源链
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
		//获取当前闲置的id号
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
		//插入一个空的资源
		submemory_list.insert(std::pair<pancy_object_id, SubMemoryData*>(id_now, new_data));
		empty_memory_heap.insert(id_now);
	}
	//获取一个尚有空间的内存块
	new_memory_block_id = *empty_memory_heap.begin();
	auto new_memory_block = submemory_list.find(new_memory_block_id);
	//在该内存块中开辟一个subresource
	check_error = new_memory_block->second->BuildSubMemory(sub_memory_offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//检查开辟过后该内存块是否已满
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_block id:" + std::to_string(new_memory_block_id) + " from submemory list: " + heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Release sub memory from submemory list", error_message);
		return error_message;
	}
	memory_check->second->FreeSubMemory(sub_memory_offset);
	//检查是否之前已满
	if (empty_memory_heap.find(new_memory_block_id) == empty_memory_heap.end())
	{
		empty_memory_heap.insert(new_memory_block_id);
	}
	return PancystarEngine::succeed;
}
MemoryBlockGpu* SubresourceLiner::GetSubResource(pancy_object_id sub_memory_id)
{
	auto subresource_block = submemory_list.find(sub_memory_id);
	if (subresource_block == submemory_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_block id:" + std::to_string(sub_memory_id) + " from submemory list: " + heap_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Release sub memory from submemory list", error_message);
		return NULL;
	}
	return subresource_block->second->GetResource();
}
//二级资源管理
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
	std::string hash_name = heap_name_in + "::" + std::to_string(resource_state_in) + "::" + std::to_string(per_memory_size_in);
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
	//创建一个新的资源存储链并保存名称
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
	//获取资源名称对应的id号,如果没有则重新创建一个
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
MemoryBlockGpu*  SubresourceControl::GetResourceData(const SubMemoryPointer &submemory_pointer)
{
	auto submemory_list = subresource_list_map.find(submemory_pointer.type_id);
	if (submemory_list == subresource_list_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find memory_type id:" + std::to_string(submemory_pointer.type_id) + " from subresource control: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get sub memory from submemory control", error_message);
		return NULL;
	}
	return submemory_list->second->GetSubResource(submemory_pointer.list_id);
}