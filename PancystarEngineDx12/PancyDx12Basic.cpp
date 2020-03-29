#include"PancyDx12Basic.h"
#include"PancyTextureDx12.h"
PancyDx12DeviceBasic* PancyDx12DeviceBasic::d3dbasic_instance = NULL;

PancyDx12DeviceBasic::PancyDx12DeviceBasic(HWND hwnd_window_in, uint32_t width_in, uint32_t height_in)
{
	FrameCount = 2;
	for (uint32_t i = 0; i < FrameCount; ++i)
	{
		m_renderTargets.push_back(NULL);
	}
	width = width_in;
	height = height_in;
	hwnd_window = hwnd_window_in;
	//Direct_queue_fence_value.store(0);
}
PancystarEngine::EngineFailReason PancyDx12DeviceBasic::Init()
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;
	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif
	hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgi_factory));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create DXGIFactory Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	ComPtr<IDXGIAdapter1> hardwareAdapter;
	GetHardwareAdapter(dxgi_factory.Get(), &hardwareAdapter);
	hr = D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Dx12 Device Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	//����ֱ����Ⱦ����
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hr = m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&command_queue_direct));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Command Queue(Direct) Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	command_queue_direct->SetName(PancystarEngine::PancyString("direct queue").GetUnicodeString().c_str());
	//����������Ⱦ����
	D3D12_COMMAND_QUEUE_DESC queueDesc_copy = {};
	queueDesc_copy.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc_copy.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	hr = m_device->CreateCommandQueue(&queueDesc_copy, IID_PPV_ARGS(&command_queue_copy));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Command Queue(Copy) Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	command_queue_direct->SetName(PancystarEngine::PancyString("copy queue").GetUnicodeString().c_str());
	//����������Ⱦ����
	D3D12_COMMAND_QUEUE_DESC queueDesc_compute = {};
	queueDesc_compute.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc_compute.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	hr = m_device->CreateCommandQueue(&queueDesc_compute, IID_PPV_ARGS(&command_queue_compute));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Command Queue(Compute) Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init dx12 basic state", error_message);
		return error_message;
	}
	command_queue_direct->SetName(PancystarEngine::PancyString("Compute queue").GetUnicodeString().c_str());
	//���������������ٶ���ʱ�����queue״̬��fence
	//�������ڽ���gpuͬ����Ϣ��fence
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue_fence_direct));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDx12DeviceBasic::Init", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue_fence_compute));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDx12DeviceBasic::Init", error_message);
		return error_message;
	}
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&queue_fence_copy));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Fence Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDx12DeviceBasic::Init", error_message);
		return error_message;
	}
	//������Ļ
	check_error = ResetScreen(width, height);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����һ����Դ��
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FrameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create DescriptorHeap Error When init D3D basic");
		return error_message;

	}
	//��ȡ��Դ�ѵ�ƫ�ƴ�С
	auto rtv_offset = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//����Դ���ϴ���RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT n = 0; n < FrameCount; n++)
	{
		hr = dx12_swapchain->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		m_device->CreateRenderTargetView(m_renderTargets[n].Get(), &rtv_desc, rtvHandle);
		rtvHandle.Offset(1, rtv_offset);
	}
	//��ֹalt+�س�ȫ��
	dxgi_factory->MakeWindowAssociation(hwnd_window, DXGI_MWA_NO_ALT_ENTER);
	//���֧�ֵĸ�ʽ
	D3D12_FEATURE_DATA_D3D12_OPTIONS formatInfo = { };
	m_device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &formatInfo, sizeof(formatInfo));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDx12DeviceBasic::ResetScreen(uint32_t window_width_in, uint32_t window_height_in)
{
	HRESULT hr;
	//���´���������
	if (dx12_swapchain != NULL)
	{
		dx12_swapchain.Reset();
	}
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;
	ComPtr<IDXGISwapChain1> swapChain;
	hr = dxgi_factory->CreateSwapChainForHwnd(
		command_queue_direct.Get(),
		hwnd_window,
		&swapChainDesc,
		nullptr,
		nullptr,
		&swapChain
	);
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Swap Chain Error When init D3D basic");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Change window size", error_message);
		return error_message;
	}
	swapChain.As(&dx12_swapchain);
	last_frame_use = dx12_swapchain->GetCurrentBackBufferIndex();
	current_frame_use = dx12_swapchain->GetCurrentBackBufferIndex();
	return PancystarEngine::succeed;
}
void PancyDx12DeviceBasic::FlushGpu()
{
	uint64_t fenceValueForSignal = 1;
	//�ȴ���Ⱦ���н���
	HANDLE wait_thread_ID_direct;
	wait_thread_ID_direct = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (wait_thread_ID_direct == nullptr)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Create fence event Error When build commandlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
	}
	command_queue_direct->Signal(queue_fence_direct.Get(), fenceValueForSignal);
	if (queue_fence_direct->GetCompletedValue() < 1)
	{
		queue_fence_direct->SetEventOnCompletion(fenceValueForSignal, wait_thread_ID_direct);
		WaitForSingleObject(wait_thread_ID_direct, INFINITE);
	}
	CloseHandle(wait_thread_ID_direct);
	//�ȴ�������н���
	HANDLE wait_thread_ID_compute;
	wait_thread_ID_compute = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (wait_thread_ID_compute == nullptr)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Create fence event Error When build commandlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
	}
	command_queue_compute->Signal(queue_fence_compute.Get(), fenceValueForSignal);
	if (queue_fence_compute->GetCompletedValue() < 1)
	{
		queue_fence_compute->SetEventOnCompletion(fenceValueForSignal, wait_thread_ID_compute);
		WaitForSingleObject(wait_thread_ID_compute, INFINITE);
	}
	CloseHandle(wait_thread_ID_compute);
	//�ȴ��������н���
	HANDLE wait_thread_ID_copy;
	wait_thread_ID_copy = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (wait_thread_ID_copy == nullptr)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Create fence event Error When build commandlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("create render command list", error_message);
	}
	command_queue_copy->Signal(queue_fence_copy.Get(), fenceValueForSignal);
	if (queue_fence_copy->GetCompletedValue() < 1)
	{
		queue_fence_copy->SetEventOnCompletion(fenceValueForSignal, wait_thread_ID_copy);
		WaitForSingleObject(wait_thread_ID_copy, INFINITE);
	}
	CloseHandle(wait_thread_ID_copy);
}
void PancyDx12DeviceBasic::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
{
	ComPtr<IDXGIAdapter1> adapter;
	*ppAdapter = nullptr;
	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			//����CPU��Ⱦ����
			continue;
		}
		//�����Ƿ�֧��dx12
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}
	*ppAdapter = adapter.Detach();
}


