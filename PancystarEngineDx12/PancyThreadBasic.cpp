#include"PancyThreadBasic.h"
ThreadPoolGPUControl* ThreadPoolGPUControl::threadpool_control_instance = NULL;
PancyRenderCommandList::PancyRenderCommandList(PancyThreadIdGPU command_list_ID_in)
{
	command_list_ID = command_list_ID_in;
	if_preparing.store(false);
	if_finish.store(true);
}
PancystarEngine::EngineFailReason PancyRenderCommandList::Create(ID3D12CommandAllocator *allocator_use_in, ID3D12PipelineState *pso_use_in, D3D12_COMMAND_LIST_TYPE command_list_type_in)
{
	command_list_type = command_list_type_in;
	//����commondlist
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandList(0, command_list_type_in, allocator_use_in, pso_use_in, IID_PPV_ARGS(&command_list_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create CommandList Error When build commandlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
		return error_message;
	}
	if_preparing.store(true);
	if_finish.store(false);
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason CommandListEngine::Create()
{
	HRESULT hr;
	//����������alloctor
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(engine_type_id, IID_PPV_ARGS(&allocator_use));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	if_alloctor_locked = false;
	//�������ڽ���gpuͬ����Ϣ��fence
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&GPU_thread_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	//����һ��ͬ��fence��Ϣ���¼�
	wait_thread_ID = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (wait_thread_ID == nullptr)
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create fence event Error When build commandlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
		return error_message;
	}
	now_prepare_commandlist = 0;
	return PancystarEngine::succeed;
}
CommandListEngine::~CommandListEngine()
{
	ReleaseList(command_list_work);
	ReleaseList(command_list_finish);
	ReleaseList(command_list_empty);
}
CommandListEngine::CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need)
{
	//Ϊid������
	command_list_ID_selfadd = 0;
	engine_type_id = type_need;
	fence_value_self_add = 1;
	now_prepare_commandlist = 0;
	wait_thread_ID = NULL;
}
void CommandListEngine::UpdateLastRenderList()
{
	if (if_alloctor_locked)
	{
		//�����һ�������commandlist�Ƿ��Ѿ��������
		auto now_use_commandlist = command_list_empty.find(now_prepare_commandlist);
		if (now_use_commandlist != command_list_empty.end())
		{
			if (!now_use_commandlist->second->CheckIfPrepare())
			{
				//��һ��commandlist�Ѿ�������ϣ������ƶ���������б�
				command_list_finish.insert(*now_use_commandlist);
				command_list_empty.erase(now_use_commandlist);
				if_alloctor_locked = false;
			}
		}
	}
}
PancystarEngine::EngineFailReason CommandListEngine::GetEmptyRenderlist
(
	ID3D12PipelineState *pso_use_in,
	PancyRenderCommandList** command_list_data,
	PancyThreadIdGPU &command_list_ID
)
{
	//����һ�µ�ǰ����ʹ����Դ��������GPU��������Ƿ��Ѿ��������
	UpdateLastRenderList();
	//��Դ����������ʹ��
	if (if_alloctor_locked)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource alloctor been using now,could not alloct new resource:");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU", error_message);
		return error_message;
	}
	//��鵱ǰ�Ƿ���ʣ��Ŀ���commandlist
	if (!command_list_empty.empty())
	{
		//�ӵ�ǰ���е�commanlist���ȡһ��commandlist
		command_list_ID = command_list_empty.begin()->first;
		*command_list_data = command_list_empty.begin()->second;
		(*command_list_data)->LockPrepare(allocator_use.Get(), pso_use_in);
		now_prepare_commandlist = command_list_ID;
	}
	else
	{
		//�½�һ�����е�commandlist
		PancyRenderCommandList *new_render_command_list = new PancyRenderCommandList(command_list_ID_selfadd);
		PancystarEngine::EngineFailReason check_error;
		check_error = new_render_command_list->Create(allocator_use.Get(), pso_use_in, engine_type_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		command_list_empty.insert(std::pair<uint32_t, PancyRenderCommandList*>(command_list_ID_selfadd, new_render_command_list));
		command_list_ID = command_list_empty.begin()->first;
		*command_list_data = command_list_empty.begin()->second;
		now_prepare_commandlist = command_list_ID;
		command_list_ID_selfadd += 1;
	}
	//��ס��Դ������
	if_alloctor_locked = true;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason CommandListEngine::FreeAlloctor()
{
	if (command_list_work.size() != 0)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "not all of the command list work finished engine ID: " + std::to_string(engine_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free command allocter int Thread Pool GPU", error_message);
		return error_message;
	}
	HRESULT hr = allocator_use->Reset();
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "release command allocator error: " + std::to_string(engine_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Free command allocter int Thread Pool GPU", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason CommandListEngine::SubmitRenderlist
(
	const uint32_t command_list_num,
	const PancyThreadIdGPU *command_list_ID
)
{
	if (command_list_num > MaxSubmitCommandList)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not submit too much commandlist which num > " + std::to_string(MaxSubmitCommandList));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
		return error_message;
	}
	//����commandlist��
	UpdateLastRenderList();
	//���������ύ��commandlist����
	std::vector<PancyRenderCommandList*> now_render_list_array;
	//��֯���ύ������commandlist
	auto now_working_list = GPU_broken_point.find(fence_value_self_add);
	if (now_working_list == GPU_broken_point.end())
	{
		std::vector<PancyThreadIdGPU> new_thread;
		GPU_broken_point.insert(std::pair<PancyFenceIdGPU, std::vector<PancyThreadIdGPU>>(fence_value_self_add, new_thread));
		now_working_list = GPU_broken_point.find(fence_value_self_add);
	}
	for (uint32_t i = 0; i < command_list_num; ++i)
	{
		//�����ύ��commandlist���뵱ǰ��GPU�ϵ�����
		now_working_list->second.push_back(command_list_ID[i]);
		//��װcommandlist
		auto finish_command_list_use = command_list_finish.find(command_list_ID[i]);
		if (finish_command_list_use != command_list_finish.end())
		{
			now_render_list_array.push_back(finish_command_list_use->second);
			commandlist_array[i] = finish_command_list_use->second->GetCommandList().Get();
			//ת��commandlist
			command_list_work.insert(*finish_command_list_use);
			command_list_finish.erase(finish_command_list_use);
		}
		else
		{
			//�޷��ҵ�commandlist
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find finish thread ID" + std::to_string(command_list_ID[i]), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
			return error_message;
		}
	}
	if (command_list_num != 0)
	{
		ComPtr<ID3D12CommandQueue> now_command_queue;
		//�ҵ���Ӧ��commandqueue
		if (engine_type_id == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect();
		}
		if (engine_type_id == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCopy();
		}
		if (engine_type_id == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCompute();
		}
		//�ύcommandlist
		now_command_queue->ExecuteCommandLists(command_list_num, commandlist_array);
		return PancystarEngine::succeed;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason CommandListEngine::SetGpuBrokenFence(PancyFenceIdGPU &broken_point_id)
{
	ComPtr<ID3D12CommandQueue> now_command_queue;
	//�ҵ���Ӧ��commandqueue
	if (engine_type_id == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)
	{
		now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect();
	}
	if (engine_type_id == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)
	{
		now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCopy();
	}
	if (engine_type_id == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE)
	{
		now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCompute();
	}
	//ΪGPU����һ���ص��ź�
	HRESULT hr = now_command_queue->Signal(GPU_thread_fence.Get(), fence_value_self_add);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "signel waiting value error in engine ID: " + std::to_string(engine_type_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
		return error_message;
	}
	broken_point_id = fence_value_self_add;
	fence_value_self_add += 1;
	return PancystarEngine::succeed;
}
bool CommandListEngine::CheckGpuBrokenFence(const PancyFenceIdGPU &broken_point_id)
{
	PancyFenceIdGPU now_wait_value = GPU_thread_fence->GetCompletedValue();
	if (now_wait_value < broken_point_id)
	{
		return false;
	}
	//��С�ڵ��ڸ���λ��������λ�����ӵ��߳�����Ϊ�ѹ������
	auto now_working_list = GPU_broken_point.begin();
	while (now_working_list != GPU_broken_point.end() && now_working_list->first <= broken_point_id) 
	{
		for (auto working_thread = now_working_list->second.begin(); working_thread != now_working_list->second.end(); ++working_thread)
		{
			auto work_command_list_use = command_list_work.find(*working_thread);
			if (work_command_list_use == command_list_work.end())
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the working thread id :" + std::to_string(*working_thread));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait fence value in render command list class", error_message);
			}
			//�����̱߳��Ϊ���н����Ĵ������߳�
			work_command_list_use->second->EndProcessing();
			command_list_empty.insert(*work_command_list_use);
			command_list_work.erase(work_command_list_use);
		}
		GPU_broken_point.erase(now_working_list);
		now_working_list = GPU_broken_point.begin();
	}
	return true;
}
PancystarEngine::EngineFailReason CommandListEngine::WaitGpuBrokenFence(const PancyFenceIdGPU &broken_point_id)
{
	if (!CheckGpuBrokenFence(broken_point_id))
	{
		HRESULT hr;
		
		hr = GPU_thread_fence->SetEventOnCompletion(broken_point_id, wait_thread_ID);
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(hr, "reflect GPU thread fence to CPU thread handle failed");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait fence value in render command list class", error_message);
			return error_message;
		}
		WaitForSingleObject(wait_thread_ID, INFINITE);
		//��С�ڵ��ڸ���λ��������λ�����ӵ��߳�����Ϊ�ѹ������
		auto now_working_list = GPU_broken_point.begin();
		while (now_working_list != GPU_broken_point.end() && now_working_list->first <= broken_point_id)
		{
			for (auto working_thread = now_working_list->second.begin(); working_thread != now_working_list->second.end(); ++working_thread)
			{
				auto work_command_list_use = command_list_work.find(*working_thread);
				if (work_command_list_use == command_list_work.end())
				{
					PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the working thread id :" + std::to_string(*working_thread));
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait fence value in render command list class", error_message);
					return error_message;
				}
				//�����̱߳��Ϊ���н����Ĵ������߳�
				work_command_list_use->second->EndProcessing();
				command_list_empty.insert(*work_command_list_use);
				command_list_work.erase(work_command_list_use);
			}
			GPU_broken_point.erase(now_working_list);
			now_working_list = GPU_broken_point.begin();
		}
	}
	return PancystarEngine::succeed;
}

ThreadPoolGPU::ThreadPoolGPU(uint32_t GPUThreadPoolID_in, bool if_rename_in)
{
	GPUThreadPoolID = GPUThreadPoolID_in;
	if_rename = if_rename_in;
	if (if_rename) 
	{
		int32_t frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
		multi_engine_list.resize(frame_num);
	}
	else 
	{
		multi_engine_list.resize(1);
	}
}
ThreadPoolGPU::~ThreadPoolGPU()
{
	for (int32_t i = 0; i < multi_engine_list.size(); ++i) 
	{
		ReleaseList(multi_engine_list[i]);
	}
}
PancystarEngine::EngineFailReason ThreadPoolGPU::BuildNewEngine(D3D12_COMMAND_LIST_TYPE engine_type)
{
	int32_t frame_num;
	//��ȡ������ʹ�õ����rename����
	if (if_rename) 
	{
		frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	}
	else 
	{
		frame_num = 1;
	}
	//������Ӧ���͵�commandlist����
	for (int i = 0; i < frame_num; ++i) 
	{
		PancystarEngine::EngineFailReason check_error;
		CommandListEngine *new_engine_list = new CommandListEngine(engine_type);
		check_error = new_engine_list->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		multi_engine_list[i].insert(std::pair<PancyEngineIdGPU, CommandListEngine*>(static_cast<PancyEngineIdGPU>(engine_type), new_engine_list));
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::Create()
{
	return PancystarEngine::succeed;
}
CommandListEngine* ThreadPoolGPU::GetThreadPool(D3D12_COMMAND_LIST_TYPE engine_type)
{
	int32_t now_frame;
	if (if_rename) 
	{
		now_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	}
	else 
	{
		now_frame = 0;
	}
	PancyEngineIdGPU engine_id = static_cast<PancyEngineIdGPU>(engine_type);
	auto engine_data = multi_engine_list[now_frame].find(engine_id);
	if (engine_data == multi_engine_list[now_frame].end())
	{
		//��δ������Ӧ���͵Ĵ洢����װ��
		PancystarEngine::EngineFailReason check_error = BuildNewEngine(engine_type);
		if (check_error.CheckIfSucceed())
		{
			engine_data = multi_engine_list[now_frame].find(engine_id);
			return engine_data->second;
		}
		else
		{
			return NULL;
		}
	}
	return engine_data->second;
}
CommandListEngine* ThreadPoolGPU::GetLastThreadPool(D3D12_COMMAND_LIST_TYPE engine_type) 
{
	int32_t now_frame;
	if (if_rename)
	{
		now_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	}
	else
	{
		now_frame = 0;
	}
	PancyEngineIdGPU engine_id = static_cast<PancyEngineIdGPU>(engine_type);
	auto engine_data = multi_engine_list[now_frame].find(engine_id);
	if (engine_data == multi_engine_list[now_frame].end())
	{
		//��δ������Ӧ���͵Ĵ洢����װ��
		PancystarEngine::EngineFailReason check_error = BuildNewEngine(engine_type);
		if (check_error.CheckIfSucceed())
		{
			engine_data = multi_engine_list[now_frame].find(engine_id);
			return engine_data->second;
		}
		else
		{
			return NULL;
		}
	}
	return engine_data->second;
}
//�̳߳ع�����
ThreadPoolGPUControl::ThreadPoolGPUControl()
{
	main_contex = NULL;
	resource_load_contex = NULL;
	main_contex = new ThreadPoolGPU(0);
	resource_load_contex = new ThreadPoolGPU(1,false);
	GPU_thread_pool_self_add = 2;
}
PancystarEngine::EngineFailReason ThreadPoolGPUControl::Create()
{
	PancystarEngine::EngineFailReason check_error;
	check_error = main_contex->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = resource_load_contex->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
ThreadPoolGPUControl::~ThreadPoolGPUControl()
{
	if (main_contex != NULL)
	{
		delete main_contex;
	}
	if (resource_load_contex != NULL)
	{
		delete resource_load_contex;
	}
	for (auto data_working = GPU_thread_pool_working.begin(); data_working != GPU_thread_pool_working.end(); ++data_working)
	{
		delete data_working->second;
	}
	GPU_thread_pool_working.clear();
	for (auto data_working = GPU_thread_pool_empty.begin(); data_working != GPU_thread_pool_empty.end(); ++data_working)
	{
		delete data_working->second;
	}
	GPU_thread_pool_empty.clear();
}
