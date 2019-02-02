#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyResourceBasic.h"
#include"PancyJsonTool.h"
#include"PancyDx12Basic.h"
#define PancyFenceIdGPU  uint64_t
#define PancyThreadIdGPU uint32_t
#define PancyThreadPoolIdGPU uint16_t
#define PancyEngineIdGPU uint8_t
#define MaxSubmitCommandList 256
#define PancyNowGraphicsCommandList ID3D12GraphicsCommandList1
//commandlist����(GPU�߳�(fence)����)
class PancyRenderCommandList
{
	std::atomic<bool> if_preparing;                       //�Ƿ�����׼��
	std::atomic<bool> if_finish;                          //�Ƿ��Ѿ���GPU�������
	PancyThreadIdGPU command_list_ID;                     //commandlist�ı��
	ComPtr<PancyNowGraphicsCommandList> command_list_data;  //commandlistָ��
	D3D12_COMMAND_LIST_TYPE command_list_type;            //commandlist����
public:
	PancyRenderCommandList(PancyThreadIdGPU command_list_ID_in);
	PancystarEngine::EngineFailReason Create
	(
		ComPtr<ID3D12CommandAllocator> allocator_use_in,
		ComPtr<ID3D12PipelineState> pso_use_in,
		D3D12_COMMAND_LIST_TYPE command_list_type
	);
	inline D3D12_COMMAND_LIST_TYPE GetCommandListType()
	{
		return command_list_type;
	}
	inline ComPtr<PancyNowGraphicsCommandList> GetCommandList()
	{
		return command_list_data;
	}
	//��⵱ǰ��commandlist�Ƿ�������׼��״̬(�Ƿ�����ռ��alloctor������Դ����)
	inline bool CheckIfPrepare()
	{
		return if_preparing.load();
	};
	//��⵱ǰ��commandlist�Ƿ��Ѿ��������(�Ƿ�����ͷŻ�����ʹ��)
	inline bool CheckIfFinish()
	{
		return if_finish.load();
	}
	//��ס��Դ������������commandlist����Դ����(�����������״̬,ǰ��״̬Ϊ->ִ�����״̬)
	inline void LockPrepare(ComPtr<ID3D12CommandAllocator> allocator_use_in, ComPtr<ID3D12PipelineState> pso_use_in)
	{
		if (if_preparing.load() == false && if_finish.load() == true)
		{
			if_finish.store(false);
			if_preparing.store(true);
			command_list_data->Reset(allocator_use_in.Get(), pso_use_in.Get());
		}
	}
	//������Դ���������ر�commandlist����Դ����(���빤��/������״̬,ǰ��״̬Ϊ->�������״̬)
	inline void UnlockPrepare()
	{
		if (if_preparing.load() == true)
		{
			command_list_data->Close();
			if_preparing.store(false);
		}
	}
	//��־��Դ�������(����ִ�����״̬,ǰ��״̬Ϊ->����/������״̬)
	inline void EndProcessing() 
	{
		if (if_preparing.load() == false && if_finish.load() == false) 
		{
			if_finish.store(true);
		}
	}
};
class CommandListEngine
{
	bool if_alloctor_locked;//��Դ�������Ƿ�����ʹ��
	D3D12_COMMAND_LIST_TYPE engine_type_id;//GPU����Ԫ�ı��(ͼ�δ���/ͨ�ü���/��Դ�ϴ�/��Ƶ����)
	PancyThreadIdGPU now_prepare_commandlist;//��ǰ���ڱ������commandlist(�����Դ������δ��ʹ���������Ч)
	PancyThreadIdGPU command_list_ID_selfadd;//��������commandlist��id��
	ComPtr<ID3D12CommandAllocator> allocator_use;//commandlist������
	HANDLE wait_thread_ID;
	ComPtr<ID3D12Fence> GPU_thread_fence;//�����ڽ��յ�ǰ�̴߳���GPUͬ����Ϣ��fence
	PancyFenceIdGPU fence_value_self_add;//��������fencevalue
	std::map<PancyFenceIdGPU, std::vector<PancyThreadIdGPU>> GPU_broken_point;//��¼ÿ�������GPU�ϵ���Ӱ�쵽��commandlist
	//���ڹ�����commandlist
	std::unordered_map<PancyThreadIdGPU, PancyRenderCommandList*> command_list_work;
	//������ϵ�commandlist
	std::unordered_map<PancyThreadIdGPU, PancyRenderCommandList*> command_list_finish;
	//������ϵĿ���commandlist
	std::unordered_map<PancyThreadIdGPU, PancyRenderCommandList*> command_list_empty;
	ID3D12CommandList* commandlist_array[MaxSubmitCommandList];
public:
	CommandListEngine(const D3D12_COMMAND_LIST_TYPE &type_need);
	~CommandListEngine();
	PancystarEngine::EngineFailReason Create();
	//��ȡһ�����е�commandlist
	PancystarEngine::EngineFailReason GetEmptyRenderlist
	(
		ComPtr<ID3D12PipelineState> pso_use_in,
		PancyRenderCommandList** command_list_data,
		PancyThreadIdGPU &command_list_ID
	);
	//����һ��GPU�ϵ�
	PancystarEngine::EngineFailReason SetGpuBrokenFence(PancyFenceIdGPU &broken_point_id);
	//��ѯһ��GPU�ϵ�
	bool CheckGpuBrokenFence(const PancyFenceIdGPU &broken_point_id);
	//�ȴ�һ��GPU�ϵ�
	PancystarEngine::EngineFailReason WaitGpuBrokenFence(const PancyFenceIdGPU &broken_point_id);
	//��ȡ��һ����Ҫ���е�GPU�ϵ�
	inline PancyFenceIdGPU GetNextBrokenFence() 
	{
		return fence_value_self_add;
	}
	//�ύһ��׼����ϵ�commandlist
	PancystarEngine::EngineFailReason SubmitRenderlist
	(
		const uint32_t command_list_num,
		const PancyThreadIdGPU *command_list_ID
	);
	//�ͷ�commandalloctor���ڴ�
	PancystarEngine::EngineFailReason FreeAlloctor();
private:
	//���µ�ǰ���ڱ������commandlist��������Ƿ��Ѿ��������
	void UpdateLastRenderList();
	template<class T>
	void ReleaseList(T &list_in)
	{
		for (auto data_release = list_in.begin(); data_release != list_in.end(); ++data_release)
		{
			delete data_release->second;
		}
		list_in.clear();
	}

};

class ThreadPoolGPU
{
	PancyThreadPoolIdGPU GPUThreadPoolID;
	int32_t pre_thread_id;
	//multi engine ����(����backbuffer������������ʹ�ü���rename������)
	std::vector<std::unordered_map<PancyEngineIdGPU, CommandListEngine*>> multi_engine_list;
	//�̳߳��Ƿ���潻��������rename����
	bool if_rename;
public:
	ThreadPoolGPU(uint32_t GPUThreadPoolID_in,bool if_rename_in = true);
	~ThreadPoolGPU();
	//��ȡ��Ӧ���͵��̳߳�
	CommandListEngine* GetThreadPool(D3D12_COMMAND_LIST_TYPE engine_type);
	//��ȡ��һ֡���̳߳�
	CommandListEngine* GetLastThreadPool(D3D12_COMMAND_LIST_TYPE engine_type);
	PancystarEngine::EngineFailReason Create();
private:
	template<class T>
	void ReleaseList(T &list_in)
	{
		for (auto data_release = list_in.begin(); data_release != list_in.end(); ++data_release)
		{
			delete data_release->second;
		}
		list_in.clear();
	}
	PancystarEngine::EngineFailReason BuildNewEngine(D3D12_COMMAND_LIST_TYPE engine_type);
};

class ThreadPoolGPUControl
{
	ThreadPoolGPU* main_contex;         //����Ⱦ�̳߳�
	ThreadPoolGPU* resource_load_contex;//��Դ�����̳߳�
	int GPU_thread_pool_self_add;
	std::unordered_map<uint32_t, ThreadPoolGPU*> GPU_thread_pool_empty;
	std::unordered_map<uint32_t, ThreadPoolGPU*> GPU_thread_pool_working;
private:
	ThreadPoolGPUControl();
	PancystarEngine::EngineFailReason Create();
public:
	static ThreadPoolGPUControl* threadpool_control_instance;
	static PancystarEngine::EngineFailReason SingleCreate()
	{
		if (threadpool_control_instance != NULL)
		{
			return PancystarEngine::succeed;
		}
		else
		{
			threadpool_control_instance = new ThreadPoolGPUControl();
			PancystarEngine::EngineFailReason check_failed = threadpool_control_instance->Create();
			return check_failed;
		}
	}
	static ThreadPoolGPUControl* GetInstance()
	{
		return threadpool_control_instance;
	}
	ThreadPoolGPU* GetMainContex()
	{
		return main_contex;
	}
	ThreadPoolGPU* GetResourceLoadContex()
	{
		return main_contex;
	}
	~ThreadPoolGPUControl();
	//todo:ΪCPU���̷߳���command alloctor
};
