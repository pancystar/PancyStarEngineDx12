#pragma once
#include"PancystarEngineBasicDx12.h"
#include <atomic>
class PancyDx12DeviceBasic
{
	UINT FrameCount;
	//������Ϣ
	HWND hwnd_window;
	uint32_t width;
	uint32_t height;
	//dxgi�豸(���ڸ��½�����)
	ComPtr<IDXGIFactory4> dxgi_factory;
	//d3d�豸
	ComPtr<ID3D12Device> m_device;
	//������
	ComPtr<IDXGISwapChain3> dx12_swapchain;
	//��Ⱦ����(direct����,copy���ͣ�compute����)
	ComPtr<ID3D12CommandQueue> command_queue_direct;
	ComPtr<ID3D12CommandQueue> command_queue_copy;
	ComPtr<ID3D12CommandQueue> command_queue_compute;
	//ͬ���ź�
	ComPtr<ID3D12Fence> Direct_queue_fence;
	std::atomic<UINT64> Direct_queue_fence_value;
	HANDLE Direct_queue_fence_event;
private:
	PancyDx12DeviceBasic(HWND hwnd_window, uint32_t width_in, uint32_t height_in);
public:
	static PancyDx12DeviceBasic* d3dbasic_instance;
	static PancystarEngine::EngineFailReason SingleCreate(HWND hwnd_window_in, UINT wind_width_in, UINT wind_hight_in)
	{
		if (d3dbasic_instance != NULL)
		{
			return PancystarEngine::succeed;
		}
		else
		{
			d3dbasic_instance = new PancyDx12DeviceBasic(hwnd_window_in, wind_width_in, wind_hight_in);
			PancystarEngine::EngineFailReason check_failed = d3dbasic_instance->Init();
			return check_failed;
		}
	}
	static PancyDx12DeviceBasic* GetInstance()
	{
		return d3dbasic_instance;
	}
	PancystarEngine::EngineFailReason Init();
	PancystarEngine::EngineFailReason ResetScreen(uint32_t window_width_in, uint32_t window_height_in);
	//��ȡcom��Դ��Ϣ
	inline ComPtr<ID3D12Device> GetD3dDevice()
	{
		return m_device;
	};
	inline ComPtr<ID3D12CommandQueue> GetCommandQueueDirect()
	{
		return command_queue_direct;
	}
	inline ComPtr<ID3D12CommandQueue> GetCommandQueueCopy()
	{
		return command_queue_copy;
	}
	inline ComPtr<ID3D12CommandQueue> GetCommandQueueCompute()
	{
		return command_queue_compute;
	}
	inline ComPtr<IDXGISwapChain3> GetSwapchain()
	{
		return dx12_swapchain;
	}
	inline ComPtr<ID3D12Fence> GetDirectQueueFence()
	{
		return Direct_queue_fence;
	}
	inline UINT64 GetDirectQueueFenceValue()
	{
		return Direct_queue_fence_value;
	}
	inline HANDLE GetDirectQueueFenceEvent()
	{
		return Direct_queue_fence_event;
	}
	inline void AddDirectQueueFenceValue()
	{
		Direct_queue_fence_value += 1;
	}
	//��ȡ֡����
	inline UINT GetFrameNum()
	{
		return FrameCount;
	}

private:
	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
};
class PancyRenderCommandList
{
	std::atomic<bool> if_preparing;                       //�Ƿ�����׼��
	std::atomic<bool> if_finish;                          //�Ƿ��Ѿ���GPU�������
	uint32_t command_list_ID;                             //commandlist�ı��
	HANDLE thread_ID;                                     //GPU�̵߳ı��
	UINT64 fence_value;                                   //������դ�����
	ComPtr<ID3D12GraphicsCommandList> command_list_data;  //commandlistָ��
	D3D12_COMMAND_LIST_TYPE command_list_type;            //commandlist����
public:
	PancyRenderCommandList(uint32_t command_list_ID_in);
	PancystarEngine::EngineFailReason Create
	(
		ComPtr<ID3D12CommandAllocator> allocator_use_in,
		ComPtr<ID3D12PipelineState> pso_use_in,
		D3D12_COMMAND_LIST_TYPE command_list_type
	);
	PancystarEngine::EngineFailReason BindFenceToThread(ComPtr<ID3D12Fence> fence_use, UINT64 fence_value_use);
	inline uint32_t GetFenceValue()
	{
		return fence_value;
	}
	inline D3D12_COMMAND_LIST_TYPE GetCommandListType()
	{
		return command_list_type;
	}
	inline HANDLE GetThreadID()
	{
		return thread_ID;
	}
	inline ComPtr<ID3D12GraphicsCommandList> GetCommandList()
	{
		return command_list_data;
	}
	inline bool CheckIfPrepare()
	{
		return if_preparing.load();
	};
	inline bool CheckIfFinish()
	{
		return if_finish.load();
	}
	inline void LockPrepare(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in)
	{
		if (if_preparing.load() == false)
		{
			if_preparing.store(true);
			command_list_data->Reset(allocator_use_in.Get(), pso_use_in.Get());
		}
	}
	inline void UnlockPrepare()
	{
		if (if_preparing.load() == true)
		{
			if_preparing.store(false);
			command_list_data->Close();
		}
	}
	inline void StartProcessing()
	{
		if_finish.store(false);
	}
	inline void EndProcessing()
	{
		if_finish.store(true);
	}

};
PancyRenderCommandList::PancyRenderCommandList(uint32_t command_list_ID_in)
{
	thread_ID = NULL;
	command_list_ID = command_list_ID_in;
	if_preparing.store(false);
	if_finish.store(true);
}
PancystarEngine::EngineFailReason PancyRenderCommandList::Create(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in, D3D12_COMMAND_LIST_TYPE command_list_type)
{
	//����commondlist
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandList(0, command_list_type, allocator_use_in.Get(), nullptr, IID_PPV_ARGS(&command_list_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create CommandList Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
		return error_message;
	}
	if_preparing.store(true);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyRenderCommandList::BindFenceToThread(ComPtr<ID3D12Fence> fence_use, UINT64 fence_value_use)
{
	HRESULT hr;
	fence_value = fence_value_use;
	hr = fence_use->SetEventOnCompletion(fence_value, thread_ID);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "reflect GPU thread fence to CPU thread handle failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("build fence value in render command list class", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}



struct CommandListEngine
{
	uint32_t engine_type_id;
	uint32_t now_prepare_commandlist;
	//multi engine ����
	uint32_t command_list_ID_selfadd;
	ComPtr<ID3D12CommandAllocator> allocator_use;//commandlist������
	ComPtr<ID3D12Fence> GPU_thread_fence;//�����ڵ�ǰ�̴߳���GPUͬ����fence(direct)
	UINT64 fence_value_self_add;
	CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need, PancystarEngine::EngineFailReason &error_message_out);
};
CommandListEngine::CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need, PancystarEngine::EngineFailReason &error_message_out)
{
	HRESULT hr;
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(type_need, IID_PPV_ARGS(&allocator_use));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		error_message_out = error_message;
		return;
	}
	command_list_ID_selfadd = 0;
	engine_type_id = static_cast<uint32_t>(type_need);
	fence_value_self_add = 0;
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&GPU_thread_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		error_message_out = error_message;
		return;
	}
	now_prepare_commandlist = -1;
	error_message_out = PancystarEngine::succeed;
}

class ThreadPoolGPU
{
	uint32_t GPUThreadPoolID;
	uint32_t max_command_list;
	//multi engine ����
	std::unordered_map<uint32_t, CommandListEngine*> multi_engine_list;
	//���ڹ�����commandlist
	std::unordered_map<uint32_t, PancyRenderCommandList*> command_list_work;
	//������ϵ�commandlist
	std::unordered_map<uint32_t, PancyRenderCommandList*> command_list_finish;
	//������ϵĿ���commandlist
	std::unordered_map<uint32_t, PancyRenderCommandList*> command_list_empty;
public:
	ThreadPoolGPU(uint32_t GPUThreadPoolID_in);
	PancystarEngine::EngineFailReason Create();
	//��ȡһ�����е�commandlist
	PancystarEngine::EngineFailReason GetEmptyRenderlist
	(
		ComPtr<ID3D12PipelineState> pso_use_in,
		const D3D12_COMMAND_LIST_TYPE &command_list_type,
		PancyRenderCommandList* command_list_data,
		uint32_t &command_list_ID
	);
	//�ȴ�һ��������ϵ�commandlist
	PancystarEngine::EngineFailReason WaitWorkRenderlist(const uint32_t &command_list_ID);
	//�ύһ��׼����ϵ�commandlist
	PancystarEngine::EngineFailReason SubmitRenderlist
	(
		const D3D12_COMMAND_LIST_TYPE &command_list_type,
		const uint32_t command_list_num,
		const uint32_t *command_list_ID
	);
};
ThreadPoolGPU::ThreadPoolGPU(uint32_t GPUThreadPoolID_in)
{
	GPUThreadPoolID = GPUThreadPoolID_in;
	max_command_list = 1024 * 1024;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::Create()
{
	HRESULT hr;
	//����direct���͵�commandlist����
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_direct = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_DIRECT, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_direct->engine_type_id, new_engine_list_direct));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����copy���͵�commandlist����
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_copy = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_COPY, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_copy->engine_type_id, new_engine_list_copy));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����compute���͵�commandlist����
	PancystarEngine::EngineFailReason check_error;
	CommandListEngine *new_engine_list_compute = new CommandListEngine(D3D12_COMMAND_LIST_TYPE_COMPUTE, check_error);
	multi_engine_list.insert(std::pair<uint32_t, CommandListEngine*>(new_engine_list_compute->engine_type_id, new_engine_list_compute));
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	/*
	//����commandallocator
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator_use_direct));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Direct CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&allocator_use_copy));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Copy CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COMPUTE, IID_PPV_ARGS(&allocator_use_compute));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Compute CommandAllocator Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	//����fence
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render ThreadPoolGPU", error_message);
		return error_message;
	}
	*/
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::GetEmptyRenderlist
(
	ComPtr<ID3D12PipelineState> pso_use_in,
	const D3D12_COMMAND_LIST_TYPE &command_list_type,
	PancyRenderCommandList* command_list_data,
	uint32_t &command_list_ID
)
{
	auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
	if (commandlist_engine == multi_engine_list.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"havent init the commandlist engine ID: " + std::to_string(command_list_type));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU",error_message);
		return error_message;
	}
	if (commandlist_engine->second->now_prepare_commandlist != -1)
	{
		command_list_ID = -1;
		//�����һ�������commandlist�Ƿ��Ѿ�closed
		auto now_use_commandlist = command_list_empty.find(commandlist_engine->second->now_prepare_commandlist);
		if (now_use_commandlist != command_list_empty.end())
		{
			if (now_use_commandlist->second->CheckIfPrepare())
			{
				//��Դ�������Ѿ��������޷�����µ�commandlist
				PancystarEngine::EngineFailReason error_message(E_FAIL, "The Direct command allocator is locked by command list ID: " + std::to_string(commandlist_engine->second->now_prepare_commandlist));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add command list int Thread Pool GPU" + std::to_string(GPUThreadPoolID), error_message);
				return error_message;
			}
			else
			{
				//��һ��commandlist�Ѿ�������ϣ������ƶ���������б�
				command_list_finish.insert(*now_use_commandlist);
				command_list_empty.erase(now_use_commandlist);
				commandlist_engine->second->now_prepare_commandlist = -1;
			}
		}
	}
	//��鵱ǰ�Ƿ���ʣ��Ŀ���commandlist
	if (!command_list_empty.empty())
	{
		//�ӵ�ǰ���е�commanlist���ȡһ��commandlist
		command_list_ID = command_list_empty.begin()->first;
		command_list_data = command_list_empty.begin()->second;
		command_list_data->LockPrepare(commandlist_engine->second->allocator_use, pso_use_in);
		commandlist_engine->second->now_prepare_commandlist = command_list_ID;
	}
	else
	{
		//�½�һ�����е�commandlist
		int ID_offset;
		uint32_t comman_list_type = static_cast<uint32_t>(command_list_type);
		ID_offset = comman_list_type * max_command_list;
		int ID_final = commandlist_engine->second->command_list_ID_selfadd + ID_offset;

		PancyRenderCommandList *new_render_command_list = new PancyRenderCommandList(ID_final);
		PancystarEngine::EngineFailReason check_error;
		check_error = new_render_command_list->Create(commandlist_engine->second->allocator_use, pso_use_in, command_list_type);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		command_list_empty.insert(std::pair<uint32_t, PancyRenderCommandList*>(ID_final, new_render_command_list));
		command_list_ID = command_list_empty.begin()->first;
		command_list_data = command_list_empty.begin()->second;
		commandlist_engine->second->now_prepare_commandlist = command_list_ID;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::WaitWorkRenderlist(const uint32_t &command_list_ID)
{
	auto work_command_list_use = command_list_work.find(command_list_ID);
	if (work_command_list_use != command_list_work.end())
	{
		auto command_list_type = work_command_list_use->second->GetCommandListType();
		auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
		if (commandlist_engine == multi_engine_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "havent init the commandlist engine ID: " + std::to_string(command_list_type));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait command list int Thread Pool GPU", error_message);
			return error_message;
		}
		//�ҵ��˹���commandlist���鿴�Ƿ���Ҫ�ȴ�
		uint32_t fence_value = work_command_list_use->second->GetFenceValue();
		if (commandlist_engine->second->GPU_thread_fence->GetCompletedValue() < fence_value)
		{
			//դ����δ��ײ�ƣ���ʼ�ȴ�
			WaitForSingleObject(work_command_list_use->second->GetThreadID(), INFINITE);
		}
		//��������ϵ�commandlist����
		command_list_empty.insert(*work_command_list_use);
		command_list_work.erase(work_command_list_use);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find working thread ID" + std::to_string(command_list_ID), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("wait command list int Thread Pool GPU", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason ThreadPoolGPU::SubmitRenderlist
(
	const D3D12_COMMAND_LIST_TYPE &command_list_type,
	const uint32_t command_list_num,
	const uint32_t *command_list_ID
)
{
	int real_command_list_num = 0;
	ID3D12CommandList** commandlist_array = new ID3D12CommandList*[command_list_num];
	std::vector<PancyRenderCommandList*> now_render_list_array;
	for (uint32_t i = 0; i < command_list_num; ++i)
	{
		//��װcommandlist
		auto finish_command_list_use = command_list_finish.find(command_list_ID[i]);
		if (finish_command_list_use == command_list_finish.end())
		{
			if (finish_command_list_use->second->GetCommandListType() == command_list_type)
			{
				now_render_list_array.push_back(finish_command_list_use->second);
				commandlist_array[real_command_list_num] = finish_command_list_use->second->GetCommandList().Get();
				real_command_list_num += 1;
			}
			else
			{
				//commandlist��ʽ����ȷ
				PancystarEngine::EngineFailReason error_message(E_FAIL, "the command list ID:" + std::to_string(command_list_ID[i]) + "have a wrong command list type", PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
				return error_message;
			}
		}
		else
		{
			//�޷��ҵ�commandlist
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find finish thread ID" + std::to_string(command_list_ID[i]), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
			return error_message;
		}
	}
	if (real_command_list_num != 0)
	{
		ComPtr<ID3D12CommandQueue> now_command_queue;
		//�ҵ���Ӧ��commandqueue
		if (command_list_type == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect();
		}
		if (command_list_type == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCopy();
		}
		if (command_list_type == D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE)
		{
			now_command_queue = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueCompute();
		}
		auto commandlist_engine = multi_engine_list.find(static_cast<uint32_t>(command_list_type));
		if (commandlist_engine == multi_engine_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "havent init the commandlist engine ID: " + std::to_string(command_list_type));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("submit command list int Thread Pool GPU", error_message);
			return error_message;
		}
		//�ύcommandlist
		now_command_queue->ExecuteCommandLists(real_command_list_num, commandlist_array);
		//���õȴ�դ��
		now_command_queue->Signal(commandlist_engine->second->GPU_thread_fence.Get(), commandlist_engine->second->fence_value_self_add);
		for (auto render_command_list = now_render_list_array.begin(); render_command_list != now_render_list_array.end(); ++render_command_list)
		{
			auto check_error = (*render_command_list)->BindFenceToThread(commandlist_engine->second->GPU_thread_fence, commandlist_engine->second->fence_value_self_add);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		commandlist_engine->second->fence_value_self_add += 1;
	}
}