#include"PancyResourceBasic.h"
using namespace PancystarEngine;
//GPU�ɷ��ʵ���Դ��
ResourceBlockGpu::ResourceBlockGpu(
	const uint64_t &memory_size_in,
	ComPtr<ID3D12Resource> resource_data_in,
	const D3D12_HEAP_TYPE &resource_usage_in,
	const D3D12_RESOURCE_STATES &resource_state
)
{
	if_start_copying_gpu = false;
	memory_size = memory_size_in;
	resource_data = resource_data_in;
	resource_usage = resource_usage_in;
	now_res_load_state = RESOURCE_LOAD_FAILED;
	if (resource_usage == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		CD3DX12_RANGE readRange(0, 0);
		HRESULT hr = resource_data->Map(0, &readRange, reinterpret_cast<void**>(&map_pointer));
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(hr, "map dynamic buffer to cpu error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::ResourceBlockGpu", error_message);
		}
		now_res_load_state = RESOURCE_LOAD_CPU_FINISH;
	}
	else
	{
		map_pointer = NULL;
		now_res_load_state = RESOURCE_LOAD_GPU_FINISH;
	}
	now_subresource_state = resource_state;
}
PancyResourceLoadState ResourceBlockGpu::GetResourceLoadingState()
{
	if (if_start_copying_gpu && now_res_load_state == PancyResourceLoadState::RESOURCE_LOAD_GPU_LOADING)
	{
		bool if_GPU_finished = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->CheckGpuBrokenFence(wait_fence);
		if (if_GPU_finished)
		{
			//��Դ�������
			now_res_load_state = PancyResourceLoadState::RESOURCE_LOAD_GPU_FINISH;
			if_start_copying_gpu = false;
		}
	}
	return now_res_load_state;
}
PancystarEngine::EngineFailReason ResourceBlockGpu::ResourceBarrier(
	PancyRenderCommandList *commandlist,
	ID3D12Resource *src_memory,
	const D3D12_RESOURCE_STATES &last_state,
	const D3D12_RESOURCE_STATES &now_state
)
{
	if (now_subresource_state != last_state)
	{
		if (now_subresource_state == now_state)
		{
			return PancystarEngine::succeed;
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "resource state dismatch, could not change resource using state");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::ResourceBarrier", error_message);
			return error_message;
		}
	}
	commandlist->GetCommandList()->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			src_memory,
			last_state,
			now_state
		)
	);
	//�޸���Դ�ķ��ʸ�ʽ
	now_subresource_state = now_state;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ResourceBlockGpu::CopyFromDynamicBufferToGpu(
	PancyRenderCommandList *commandlist,
	ResourceBlockGpu &dynamic_buffer,
	const pancy_resource_size &src_offset,
	const pancy_resource_size &dst_offset,
	const pancy_resource_size &data_size
)
{
	if (now_res_load_state == RESOURCE_LOAD_FAILED)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource load failed, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::CopyFromDynamicBufferToGpu", error_message);
		return error_message;
	}
	if (dynamic_buffer.GetResourceLoadingState() != RESOURCE_LOAD_CPU_FINISH)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource haven't load to cpu, could not copy data to GPU");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::CopyFromDynamicBufferToGpu", error_message);
		return error_message;
	}
	if (now_res_load_state != RESOURCE_LOAD_GPU_FINISH)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource is not a GPU resource, could not copy data to GPU");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::CopyFromDynamicBufferToGpu", error_message);
		return error_message;
	}
	auto src_size_check = src_offset + data_size;
	auto dst_size_check = dst_offset + data_size;
	if (src_size_check > dynamic_buffer.GetSize() || dst_size_check > memory_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource size overflow, could not copy resource from cpu to GPU");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::CopyFromDynamicBufferToGpu", error_message);
		return error_message;
	}
	auto check_error = ResourceBarrier(commandlist, resource_data.Get(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	commandlist->GetCommandList()->CopyBufferRegion(
		resource_data.Get(),
		dst_offset,
		dynamic_buffer.GetResource(),
		src_offset,
		data_size);
	check_error = ResourceBarrier(commandlist, resource_data.Get(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	now_res_load_state = RESOURCE_LOAD_GPU_LOADING;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ResourceBlockGpu::CopyFromDynamicBufferToGpu(
	PancyRenderCommandList *commandlist,
	ResourceBlockGpu &dynamic_buffer,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	const pancy_object_id &Layout_num
)
{
	if (now_res_load_state == RESOURCE_LOAD_FAILED)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource load failed, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::CopyFromDynamicBufferToGpu", error_message);
		return error_message;
	}
	if (dynamic_buffer.GetResourceLoadingState() != RESOURCE_LOAD_CPU_FINISH)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource haven't laod to cpu, could not copy data to GPU");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::CopyFromDynamicBufferToGpu", error_message);
		return error_message;
	}
	if (now_res_load_state != RESOURCE_LOAD_GPU_FINISH)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource is not a GPU resource, could not copy data to GPU");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::CopyFromDynamicBufferToGpu", error_message);
		return error_message;
	}
	auto check_error = ResourceBarrier(commandlist, resource_data.Get(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	for (UINT i = 0; i < Layout_num; ++i)
	{
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT real_layout;
		real_layout.Footprint = pLayouts[i].Footprint;
		real_layout.Offset = pLayouts[i].Offset;

		CD3DX12_TEXTURE_COPY_LOCATION Dst(resource_data.Get(), i + 0);
		CD3DX12_TEXTURE_COPY_LOCATION Src(dynamic_buffer.GetResource(), real_layout);
		commandlist->GetCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
	}
	check_error = ResourceBarrier(commandlist, resource_data.Get(), D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	now_res_load_state = RESOURCE_LOAD_GPU_LOADING;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ResourceBlockGpu::SetResourceCopyBrokenFence(const PancyFenceIdGPU &broken_fence_id)
{
	if (now_res_load_state != PancyResourceLoadState::RESOURCE_LOAD_GPU_LOADING)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "haven't copy any data to cpu, could not set broken fence");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::SetResourceCopyBrokenFence", error_message);
		return error_message;
	}
	wait_fence = broken_fence_id;
	if_start_copying_gpu = true;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ResourceBlockGpu::WriteFromCpuToBuffer(
	const pancy_resource_size &pointer_offset,
	const void* copy_data,
	const pancy_resource_size &data_size
)
{
	if (now_res_load_state == RESOURCE_LOAD_FAILED)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource load failed, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
		return error_message;
	}
	if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
		return error_message;
	}
	if (now_res_load_state != RESOURCE_LOAD_CPU_FINISH)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource is not a CPU copy resource, could not copy data to CPU");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
		return error_message;
	}
	memcpy(map_pointer + pointer_offset, copy_data, data_size);
	now_res_load_state = RESOURCE_LOAD_CPU_FINISH;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ResourceBlockGpu::WriteFromCpuToBuffer(
	const pancy_resource_size &pointer_offset,
	std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
	UINT64* pRowSizesInBytes,
	UINT* pNumRows
)
{
	if (now_res_load_state == RESOURCE_LOAD_FAILED)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource load failed, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
		return error_message;
	}
	if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
		return error_message;
	}
	if (now_res_load_state != RESOURCE_LOAD_CPU_FINISH)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource is not a CPU copy resource, could not copy data to CPU");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::WriteFromCpuToBuffer", error_message);
		return error_message;
	}
	//��ȡ������ָ��
	UINT8* pData = map_pointer + pointer_offset;
	//��ȡsubresource
	D3D12_SUBRESOURCE_DATA *pSrcData = &subresources[0];
	UINT subres_size = static_cast<UINT>(subresources.size());
	for (UINT i = 0; i < subres_size; ++i)
	{
		D3D12_MEMCPY_DEST DestData = { pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch, pLayouts[i].Footprint.RowPitch * pNumRows[i] };
		MemcpySubresource(&DestData, &pSrcData[i], (SIZE_T)pRowSizesInBytes[i], pNumRows[i], pLayouts[i].Footprint.Depth);
	}
	now_res_load_state = RESOURCE_LOAD_CPU_FINISH;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ResourceBlockGpu::ReadFromBufferToCpu(
	const pancy_resource_size &pointer_offset,
	void* copy_data,
	const pancy_resource_size data_size
)
{
	if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_READBACK)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not readback, could not read data back");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("ResourceBlockGpu::ReadFromBufferToCpu", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
	//todo: �ض�GPU����
}
ResourceBlockGpu::~ResourceBlockGpu()
{
	if (resource_usage == D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
	{
		resource_data->Unmap(0, NULL);
	}
}
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
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create(const std::string &resource_name_in, const Json::Value &root_value_in)
{
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