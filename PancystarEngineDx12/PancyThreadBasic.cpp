#include"PancyThreadBasic.h"
ThreadPoolGPUControl* ThreadPoolGPUControl::threadpool_control_instance = NULL;
PancyRenderCommandList::PancyRenderCommandList(PancyThreadIdGPU command_list_ID_in)
{
	command_list_ID = command_list_ID_in;
	if_preparing.store(false);
	if_finish.store(true);
}
PancystarEngine::EngineFailReason PancyRenderCommandList::Create(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in, D3D12_COMMAND_LIST_TYPE command_list_type_in)
{
	command_list_type = command_list_type_in;
	//创建commondlist
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandList(0, command_list_type_in, allocator_use_in.Get(), nullptr, IID_PPV_ARGS(&command_list_data));
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
	//创建并解锁alloctor
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(engine_type_id, IID_PPV_ARGS(&allocator_use));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	if_alloctor_locked = false;
	//创建用于接收gpu同步信息的fence
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&GPU_thread_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	//创建一个同步fence消息的事件
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
	//为id号清零
	command_list_ID_selfadd = 0;
	engine_type_id = type_need;
	fence_value_self_add = 0;
	now_prepare_commandlist = 0;
	wait_thread_ID = NULL;
}
void CommandListEngine::UpdateLastRenderList()
{
	if (if_alloctor_locked)
	{
		//检查上一个处理的commandlist是否已经处理完毕
		auto now_use_commandlist = command_list_empty.find(now_prepare_commandlist);
		if (now_use_commandlist != command_list_empty.end())
		{
			if (!now_use_commandlist->second->CheckIfPrepare())
			{
				//上一个commandlist已经处理完毕，将其移动到已完成列表
				command_list_finish.insert(*now_use_commandlist);
				command_list_empty.erase(now_use_commandlist);
				if_alloctor_locked = false;
			}
		}
	}
}
PancystarEngine::EngineFailReason CommandListEngine::GetEmptyRenderlist
(
	ComPtr<ID3D12PipelineState> pso_use_in,
	PancyRenderCommandList** command_list_data,
	PancyThreadIdGPU &command_list_ID
)
{
	//更新一下当前正在使用资源分配器的GPU命令，看其是否已经分配完毕
	UpdateLastRenderList();
	//资源分配器正在使用
	if (if_alloctor_locked)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource alloctor been using now,could not alloct new resource:");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU", error_message);
		return error_message;
	}
	//检查当前是否还有剩余的空闲commandlist
	if (!command_list_empty.empty())
	{
		//从当前空闲的commanlist里获取一个commandlist
		command_list_ID = command_list_empty.begin()->first;
		*command_list_data = command_list_empty.begin()->second;
		(*command_list_data)->LockPrepare(allocator_use, pso_use_in);
		now_prepare_commandlist = command_list_ID;
	}
	else
	{
		//新建一个空闲的commandlist
		PancyRenderCommandList *new_render_command_list = new PancyRenderCommandList(command_list_ID_selfadd);
		PancystarEngine::EngineFailReason check_error;
		check_error = new_render_command_list->Create(allocator_use, pso_use_in, engine_type_id);
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
	//锁住资源分配器
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
	//更新commandlist池
	UpdateLastRenderList();
	//开辟用于提交的commandlist数组
	std::vector<PancyRenderCommandList*> now_render_list_array;
	//组织待提交的所有commandlist
	auto now_working_list = GPU_broken_point.find(fence_value_self_add);
	if (now_working_list == GPU_broken_point.end())
	{
		std::vector<PancyThreadIdGPU> new_thread;
		GPU_broken_point.insert(std::pair<PancyFenceIdGPU, std::vector<PancyThreadIdGPU>>(fence_value_self_add, new_thread));
		now_working_list = GPU_broken_point.find(fence_value_self_add);
	}
	for (uint32_t i = 0; i < command_list_num; ++i)
	{
		//将待提交的commandlist加入当前的GPU断点区域
		now_working_list->second.push_back(command_list_ID[i]);
		//组装commandlist
		auto finish_command_list_use = command_list_finish.find(command_list_ID[i]);
		if (finish_command_list_use != command_list_finish.end())
		{
			now_render_list_array.push_back(finish_command_list_use->second);
			commandlist_array[i] = finish_command_list_use->second->GetCommandList().Get();
			//转换commandlist
			command_list_work.insert(*finish_command_list_use);
			command_list_finish.erase(finish_command_list_use);
		}
		else
		{
			//无法找到commandlist
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find finish thread ID" + std::to_string(command_list_ID[i]), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
			return error_message;
		}
	}
	if (command_list_num != 0)
	{
		ComPtr<ID3D12CommandQueue> now_command_queue;
		//找到对应的commandqueue
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
		//提交commandlist
		now_command_queue->ExecuteCommandLists(command_list_num, commandlist_array);
		return PancystarEngine::succeed;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason CommandListEngine::SetGpuBrokenFence(PancyFenceIdGPU &broken_point_id)
{
	ComPtr<ID3D12CommandQueue> now_command_queue;
	//找到对应的commandqueue
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
	//为GPU创建一个回调信号
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
	//将该GPU断点所控制的所有GPU线程回收
	auto now_working_list = GPU_broken_point.find(broken_point_id);
	if (now_working_list != GPU_broken_point.end())
	{
		for (auto working_thread = now_working_list->second.begin(); working_thread != now_working_list->second.end(); ++working_thread)
		{
			auto work_command_list_use = command_list_work.find(*working_thread);
			if (work_command_list_use == command_list_work.end())
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the working thread id :" + std::to_string(*working_thread));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait fence value in render command list class", error_message);
			}
			//将该线程标记为运行结束的待回收线程
			work_command_list_use->second->EndProcessing();
			command_list_empty.insert(*work_command_list_use);
			command_list_work.erase(work_command_list_use);
		}
		GPU_broken_point.erase(now_working_list);
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
		//将该GPU断点所控制的所有GPU线程回收
		auto now_working_list = GPU_broken_point.find(broken_point_id);
		if (now_working_list != GPU_broken_point.end())
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
				//将该线程标记为运行结束的待回收线程
				work_command_list_use->second->EndProcessing();
				command_list_empty.insert(*work_command_list_use);
				command_list_work.erase(work_command_list_use);
			}
			GPU_broken_point.erase(now_working_list);
		}
	}
	return PancystarEngine::succeed;
}

ThreadPoolGPU::ThreadPoolGPU(uint32_t GPUThreadPoolID_in)
{
	GPUThreadPoolID = GPUThreadPoolID_in;
}
ThreadPoolGPU::~ThreadPoolGPU()
{
	ReleaseList(multi_engine_list);
}
PancystarEngine::EngineFailReason ThreadPoolGPU::Create()
{
	//创建direct类型的commandlist引擎
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_direct = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_DIRECT);
	check_error = new_engine_list_direct->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	multi_engine_list.insert(std::pair<PancyEngineIdGPU, CommandListEngine*>(static_cast<PancyEngineIdGPU>(D3D12_COMMAND_LIST_TYPE_DIRECT), new_engine_list_direct));

	//创建copy类型的commandlist引擎
	CommandListEngine *new_engine_list_copy = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_COPY);
	check_error = new_engine_list_copy->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	multi_engine_list.insert(std::pair<PancyEngineIdGPU, CommandListEngine*>(static_cast<PancyEngineIdGPU>(D3D12_COMMAND_LIST_TYPE_COPY), new_engine_list_copy));
	//创建compute类型的commandlist引擎
	CommandListEngine *new_engine_list_compute = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	check_error = new_engine_list_compute->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	multi_engine_list.insert(std::pair<PancyEngineIdGPU, CommandListEngine*>(static_cast<PancyEngineIdGPU>(D3D12_COMMAND_LIST_TYPE_COMPUTE), new_engine_list_compute));
	return PancystarEngine::succeed;
}

ThreadPoolGPUControl::ThreadPoolGPUControl()
{
	main_contex = NULL;
	main_contex = new ThreadPoolGPU(0);
	GPU_thread_pool_self_add = 1;
}
PancystarEngine::EngineFailReason ThreadPoolGPUControl::Create()
{
	auto check_error = main_contex->Create();
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
