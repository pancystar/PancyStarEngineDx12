#include"PancyResourceBasic.h"
using namespace PancystarEngine;
//������Դ������ָ��
VirtualResourcePointer::VirtualResourcePointer()
{
	//��ԴID��
	resource_id = 0;
	if_NULL = true;
}
VirtualResourcePointer::VirtualResourcePointer(const pancy_object_id &resource_id_in)
{
	auto check_error = PancyGlobelResourceControl::GetInstance()->AddResurceReference(resource_id_in);
	if (check_error.CheckIfSucceed())
	{
		check_error = PancyGlobelResourceControl::GetInstance()->GetResourceById(resource_id_in, &data_pointer);
		if (check_error.CheckIfSucceed())
		{
			resource_id = resource_id_in;
			if_NULL = false;
		}
	}
}
VirtualResourcePointer::VirtualResourcePointer(const VirtualResourcePointer & copy_data)
{
	auto check_error = PancyGlobelResourceControl::GetInstance()->AddResurceReference(copy_data.resource_id);
	if (check_error.CheckIfSucceed())
	{
		check_error = PancyGlobelResourceControl::GetInstance()->GetResourceById(copy_data.resource_id, &data_pointer);
		if (check_error.CheckIfSucceed())
		{
			resource_id = copy_data.resource_id;
			if_NULL = false;
		}
	}
}
VirtualResourcePointer::~VirtualResourcePointer()
{
	PancyGlobelResourceControl::GetInstance()->DeleteResurceReference(resource_id);
}
VirtualResourcePointer& VirtualResourcePointer::operator=(const VirtualResourcePointer& b)
{
	MakeShared(b.resource_id);
	return *this;
}
PancystarEngine::EngineFailReason VirtualResourcePointer::MakeShared(const pancy_object_id &resource_id_in)
{
	PancystarEngine::EngineFailReason check_error;
	if (!if_NULL)
	{
		check_error = PancyGlobelResourceControl::GetInstance()->DeleteResurceReference(resource_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		resource_id = 0;
	}
	check_error = PancyGlobelResourceControl::GetInstance()->AddResurceReference(resource_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyGlobelResourceControl::GetInstance()->GetResourceById(resource_id_in, &data_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	resource_id = resource_id_in;
	if_NULL = false;
	return PancystarEngine::succeed;
}
//�м䶯̬��Դ�ķ����
PancyDynamicRingBuffer::PancyDynamicRingBuffer()
{
	LoadInitData();
}
PancystarEngine::EngineFailReason PancyDynamicRingBuffer::LoadInitData()
{
	buffer_size = 256 * 1024 * 1024;
	pointer_head_offset = 0;
	pointer_tail_offset = 0;
	CD3DX12_HEAP_DESC heapDesc(buffer_size, D3D12_HEAP_TYPE_UPLOAD, 0, D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS);
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(&ringbuffer_heap_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Create DynamicRingBuffer Heap error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDynamicRingBuffer::LoadInitData", check_error);
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancyDynamicRingBuffer::~PancyDynamicRingBuffer()
{
	while (!ResourceUploadingMap.empty())
	{
		auto top_data = ResourceUploadingMap.front();
		delete top_data;
		ResourceUploadingMap.pop();
	}
}
PancystarEngine::EngineFailReason PancyDynamicRingBuffer::AllocNewDynamicData(
	pancy_resource_size data_size,
	ResourceBlockGpu &gpu_resource_pointer,
	UploadResourceBlock **new_block
)
{
	PancystarEngine::EngineFailReason check_error;
	//���ring-buffer�л����ڴ��������Ϣ��������һ����Դ������
	if (!ResourceUploadingMap.empty())
	{
		RefreshOldDynamicData();
	}
	pancy_resource_size alloc_start_position;
	//�鿴��ǰ�������Ƿ����㹻�Ŀռ�
	if (pointer_tail_offset > pointer_head_offset)
	{
		//ͷָ��С��βָ�룬��ʱ�Ŀ��ÿռ����ͷβָ��֮��Ŀռ�
		pancy_resource_size head_could_use_size = pointer_tail_offset - pointer_head_offset;
		if (head_could_use_size < data_size)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "dynamic ring-buffer is full, could not alloc new data");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDynamicRingBuffer::AllocNewDynamicData", error_message);
			return error_message;
		}
		alloc_start_position = pointer_head_offset;
	}
	else
	{
		//ͷָ�����βָ�룬��ʱ�Ŀ��ÿռ�����Ƭ����һ��ͷָ�뵽bufferβ���Ŀռ䣬�����bufferͷ��βָ��Ŀռ�
		pancy_resource_size head_could_use_size_1 = buffer_size - pointer_head_offset;
		if (head_could_use_size_1 >= data_size)
		{
			alloc_start_position = pointer_head_offset;
		}
		else
		{
			pancy_resource_size head_could_use_size_2 = pointer_tail_offset;
			if (head_could_use_size_2 >= data_size)
			{
				alloc_start_position = 0;
			}
			else
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "dynamic ring-buffer is full, could not alloc new data");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDynamicRingBuffer::AllocNewDynamicData", error_message);
				return error_message;
			}
		}
	}
	//�����µ�ͷ��ָ����Ϣ������buffer����
	ComPtr<ID3D12Resource> resource_data;
	D3D12_RESOURCE_DESC resource_desc;
	resource_desc.Alignment = 0;
	resource_desc.DepthOrArraySize = 1;
	resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resource_desc.Format = DXGI_FORMAT_UNKNOWN;
	resource_desc.Height = 1;
	resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resource_desc.MipLevels = 1;
	resource_desc.SampleDesc.Count = 1;
	resource_desc.SampleDesc.Quality = 0;
	resource_desc.Width = data_size;
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = resource_desc.Format;
	clearValue.Color[0] = 0.0f;
	clearValue.Color[1] = 0.0f;
	clearValue.Color[2] = 0.0f;
	clearValue.Color[3] = 0.0f;
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreatePlacedResource(
		ringbuffer_heap_data.Get(),
		alloc_start_position,
		&resource_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&clearValue,
		IID_PPV_ARGS(&resource_data)
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason check_error(hr, "Allocate Memory From Dynamic ring-buffer Heap error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDynamicRingBuffer::AllocNewDynamicData", check_error);
		return check_error;
	}
	//�������ɹ�����Դ�󶨵�gpu��Դ
	*new_block = new UploadResourceBlock(
		pointer_head_offset,
		alloc_start_position + data_size,
		data_size,
		resource_data,
		D3D12_HEAP_TYPE_UPLOAD,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&gpu_resource_pointer
	);
	pointer_head_offset = alloc_start_position + data_size;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDynamicRingBuffer::CopyDataToGpu(
	PancyRenderCommandList *commandlist,
	void* data_pointer,
	const pancy_resource_size &data_size,
	ResourceBlockGpu &gpu_resource_pointer
)
{
	PancystarEngine::EngineFailReason check_error;
	UploadResourceBlock* new_dynamic_block;
	check_error = AllocNewDynamicData(data_size, gpu_resource_pointer, &new_dynamic_block);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����ݴ�CPU������dynamic-buffer
	check_error = new_dynamic_block->dynamic_buffer_resource.WriteFromCpuToBuffer(0, data_pointer, data_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����ݴ�dynamic-buffer�������Դ�
	check_error = new_dynamic_block->static_gpu_resource->CopyFromDynamicBufferToGpu(
		commandlist,
		new_dynamic_block->dynamic_buffer_resource,
		0,
		0,
		data_size
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����Դ��¼������
	ResourceUploadingMap.push(new_dynamic_block);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDynamicRingBuffer::CopyDataToGpu(
	PancyRenderCommandList *commandlist,
	std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	UINT64* pRowSizesInBytes,
	UINT* pNumRows,
	const pancy_resource_size &data_size,
	ResourceBlockGpu &gpu_resource_pointer
)
{
	PancystarEngine::EngineFailReason check_error;
	UploadResourceBlock* new_dynamic_block;
	check_error = AllocNewDynamicData(data_size, gpu_resource_pointer, &new_dynamic_block);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����ݴ�CPU������dynamic-buffer
	check_error = new_dynamic_block->dynamic_buffer_resource.WriteFromCpuToBuffer(
		0,
		subresources,
		pLayouts,
		pRowSizesInBytes,
		pNumRows
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����ݴ�dynamic-buffer�������Դ�
	check_error = new_dynamic_block->static_gpu_resource->CopyFromDynamicBufferToGpu(
		commandlist,
		new_dynamic_block->dynamic_buffer_resource,
		pLayouts,
		subresources.size()
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����Դ��¼������
	ResourceUploadingMap.push(new_dynamic_block);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDynamicRingBuffer::RefreshOldDynamicData()
{
	while (!ResourceUploadingMap.empty())
	{
		auto top_data = ResourceUploadingMap.front();
		if (top_data->static_gpu_resource->GetResourceLoadingState() == PancyResourceLoadState::RESOURCE_LOAD_GPU_FINISH)
		{
			ResourceUploadingMap.pop();
			//��⵱ǰ��Դ�ͷ�֮ǰ��ָ���Ƿ����ڴ���һ��
			if (top_data->pointer_before_alloc != pointer_tail_offset)
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "dynamic buffer pointer dismatch");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::RefreshOldDynamicData", error_message);
				return error_message;
			}
			//�ͷ���Ϻ��޸�βָ��
			pointer_tail_offset = top_data->pointer_after_alloc;
			delete top_data;
		}
		else
		{
			break;
		}
	}
	return PancystarEngine::succeed;
}
//������Դ
PancyBasicVirtualResource::PancyBasicVirtualResource(const bool &if_could_reload_in)
{
	if_could_reload = if_could_reload_in;
	reference_count.store(0);
}
PancyBasicVirtualResource::~PancyBasicVirtualResource()
{
	delete resource_desc_value;
}
void PancyBasicVirtualResource::AddReference()
{
	reference_count.fetch_add(1);
}
void PancyBasicVirtualResource::DeleteReference()
{
	if (reference_count > 0)
	{
		reference_count.fetch_sub(1);
	}
	else
	{
		reference_count.store(0);
	}
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create(const std::string &resource_name_in)
{
	//�����ļ����;����Ժ��ַ�ʽ������Դ
	if (resource_name_in.find(".json") == resource_name_in.size()-5)
	{
		Json::Value jsonRoot;
		auto check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_name_in, jsonRoot);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = Create(resource_name_in, jsonRoot);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else 
	{
		auto check_error = LoadResourceDirect(resource_name_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create(const std::string &resource_name_in, const Json::Value &root_value_in)
{
	resource_type_name = typeid(*this).name();
	BuildJsonReflect(&resource_desc_value);
	if (resource_desc_value == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse json type: " + resource_name_in);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicVirtualResource::Create", error_message);
		return error_message;
	}
	auto check_error = resource_desc_value->LoadFromJsonMemory(resource_name_in, root_value_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = InitResource();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create(const std::string &resource_name_in, void *resource_data,const std::string &resource_type,const pancy_resource_size &resource_size)
{
	resource_type_name = typeid(*this).name();
	BuildJsonReflect(&resource_desc_value);
	if (resource_desc_value == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse json type: " + resource_name_in);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicVirtualResource::Create", error_message);
		return error_message;
	}
	auto check_error = resource_desc_value->ResetMemoryByMemberData(resource_data, resource_type, resource_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = InitResource();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::LoadResourceDirect(const std::string &file_name)
{
	//Ĭ������£��������κη�json�ļ��ļ���
	PancystarEngine::EngineFailReason error_message(E_FAIL,"could not parse file: " + file_name);
	PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicVirtualResource::LoadResourceDirect", error_message);
	return error_message;
}
//������Դ������
PancyGlobelResourceControl::PancyGlobelResourceControl()
{
}
PancyGlobelResourceControl::~PancyGlobelResourceControl()
{
	for (auto data_resource = basic_resource_array.begin(); data_resource != basic_resource_array.end(); ++data_resource)
	{
		delete data_resource->second;
	}
	basic_resource_array.clear();
	resource_name_list.clear();
	free_id_list.clear();
}
PancystarEngine::EngineFailReason PancyGlobelResourceControl::GetResourceById(
	const pancy_object_id &resource_id,
	PancyBasicVirtualResource **data_pointer
)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyGlobelResourceControl::GetResourceById", error_message);
		return error_message;
	}
	*data_pointer = data_now->second;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyGlobelResourceControl::AddResourceToControl(
	const std::string &name_resource_in,
	PancyBasicVirtualResource *new_data,
	VirtualResourcePointer &res_pointer,
	const bool &if_allow_repeat
)
{
	PancystarEngine::EngineFailReason check_error;
	int id_now;
	//�ж��Ƿ��п��е�id���
	if (free_id_list.size() > 0)
	{
		id_now = *free_id_list.begin();
		free_id_list.erase(id_now);
	}
	else
	{
		id_now = basic_resource_array.size();
	}
	if (!if_allow_repeat)
	{
		//�������-id����������
		resource_name_list.insert(std::pair<std::string, pancy_object_id>(name_resource_in, id_now));
	}
	//���뵽��Դ�б�
	basic_resource_array.insert(std::pair<pancy_object_id, PancyBasicVirtualResource*>(id_now, new_data));
	check_error = res_pointer.MakeShared(id_now);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyGlobelResourceControl::AddResurceReference(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyGlobelResourceControl::AddResurceReference", error_message);
		return error_message;
	}
	data_now->second->AddReference();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyGlobelResourceControl::DeleteResurceReference(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyGlobelResourceControl::DeleteResurceReference", error_message);
		return error_message;
	}
	data_now->second->DeleteReference();
	//���ü���Ϊ0,ɾ������Դ
	if (data_now->second->GetReferenceCount() == 0)
	{
		//ɾ����Դ��Ӧ����
		resource_name_list.erase(data_now->second->GetResourceName());
		//��ӵ�������Դ
		free_id_list.insert(data_now->first);
		//ɾ����Դ
		delete data_now->second;
		basic_resource_array.erase(data_now);
	}
	return PancystarEngine::succeed;
}