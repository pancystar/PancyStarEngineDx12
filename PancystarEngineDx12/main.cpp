#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
enum SwapChainBitDepth
{
	_8 = 0,
	_10,
	_16,
	SwapChainBitDepthCount
};


class PancyDx12Basic
{
	UINT now_frame_use;
	ThreadPoolGPU *main_thread;//���߳�
	//��Դ�Ѵ�С	
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize;
	std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
	//����״̬
	ComPtr<ID3D12PipelineState> m_pipelineState;
	uint32_t renderlist_ID;
public:
	PancyDx12Basic()
	{
		for (uint32_t i = 0; i < PancyDx12DeviceBasic::GetInstance()->GetFrameNum(); ++i)
		{
			m_renderTargets.push_back(NULL);
		}
	}
	PancystarEngine::EngineFailReason Create(HWND hwnd_window_in, int width_in, int height_in);
	void Render();
	void Release();
private:
	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
	void PopulateCommandList();
	void WaitForPreviousFrame();
	inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
	{
		return max(0, min(ax2, bx2) - max(ax1, bx1)) * max(0, min(ay2, by2) - max(ay1, by1));
	}
};

void PancyDx12Basic::PopulateCommandList()
{
	HRESULT hr;
	PancystarEngine::EngineFailReason check_error;
	// Command list allocators can only be reset when the associated 
	// command lists have finished execution on the GPU; apps should use 
	// fences to determine GPU execution progress.
	//hr = m_commandAllocator->Reset();

	check_error = main_thread->FreeAlloctor(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT);
	PancyRenderCommandList *m_commandList;
	
	check_error = main_thread->GetEmptyRenderlist(m_pipelineState.Get(),D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,&m_commandList, renderlist_ID);
	// However, when ExecuteCommandList() is called on a particular command 
	// list, that command list can then be reset at any time and must be before 
	// re-recording.
	//hr = m_commandList->GetCommandList()->Reset(m_commandAllocator.Get(), m_pipelineState.Get());

	// Indicate that the back buffer will be used as a render target.
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[now_frame_use].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), now_frame_use, m_rtvDescriptorSize);

	// Record commands.
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	// Indicate that the back buffer will now be used to present.
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[now_frame_use].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
	//hr = m_commandList->GetCommandList()->Close();
}
void PancyDx12Basic::WaitForPreviousFrame()
{
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
	// sample illustrates how to use fences for efficient resource usage and to
	// maximize GPU utilization.

	// Signal and increment the fence value.
	auto  check_error = main_thread->WaitWorkRenderlist(renderlist_ID);
	/*
	const UINT64 fence = PancyDx12DeviceBasic::GetInstance()->GetDirectQueueFenceValue();
	
	hr = PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect()->Signal(PancyDx12DeviceBasic::GetInstance()->GetDirectQueueFence().Get(), fence);
	PancyDx12DeviceBasic::GetInstance()->AddDirectQueueFenceValue();
	// Wait until the previous frame is finished.
	if (PancyDx12DeviceBasic::GetInstance()->GetDirectQueueFence()->GetCompletedValue() < PancyDx12DeviceBasic::GetInstance()->GetDirectQueueFenceValue())
	{
		hr = PancyDx12DeviceBasic::GetInstance()->GetDirectQueueFence()->SetEventOnCompletion(fence, PancyDx12DeviceBasic::GetInstance()->GetDirectQueueFenceEvent());
		WaitForSingleObject(PancyDx12DeviceBasic::GetInstance()->GetDirectQueueFenceEvent(), INFINITE);
	}
	*/
	now_frame_use = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->GetCurrentBackBufferIndex();
}

void PancyDx12Basic::Render()
{
	HRESULT hr;
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	auto  check_error = main_thread->SubmitRenderlist(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT,1,&renderlist_ID);
	// Execute the command list.
	//ID3D12CommandList* ppCommandLists[] = { m_commandList.Get() };
	//PancyDx12DeviceBasic::GetInstance()->GetCommandQueueDirect()->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	hr = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->Present(1, 0);

	WaitForPreviousFrame();
}
void PancyDx12Basic::GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
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
PancystarEngine::EngineFailReason PancyDx12Basic::Create(HWND hwnd_window_in, int width_in, int height_in)
{
	HRESULT hr;
	main_thread = new ThreadPoolGPU(0);
	auto check_error = main_thread->Create();
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	//ѡȡ��ǰ����Ļ�����
	now_frame_use = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->GetCurrentBackBufferIndex();
	//����һ����Դ��
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create DescriptorHeap Error When init D3D basic");
		return error_message;
	}
	//��ȡ��Դ�ѵĴ�С
	m_rtvDescriptorSize = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//����Դ���ϴ���RTV
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT n = 0; n < PancyDx12DeviceBasic::GetInstance()->GetFrameNum(); n++)
	{
		hr = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->GetBuffer(n, IID_PPV_ARGS(&m_renderTargets[n]));
		PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRenderTargetView(m_renderTargets[n].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}
	/*
	//����commandallocator
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create CommandAllocator Error When init D3D basic");
		return error_message;
	}
	//����commondlist
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create CommandList Error When init D3D basic");
		return error_message;
	}
	m_commandList->Close();
	
	//����fence
	hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "Create Fence Error When init D3D basic");
		return error_message;
	}
	m_fenceValue = 1;
	m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (m_fenceEvent == nullptr)
	{
		PancystarEngine::EngineFailReason error_message(HRESULT_FROM_WIN32(GetLastError()), "Create FenceEvent Error When init D3D basic");
		return error_message;
	}
	*/
	return PancystarEngine::succeed;
}
void PancyDx12Basic::Release() 
{
	
	WaitForPreviousFrame();
	delete main_thread;
	//CloseHandle(m_fenceEvent);
}





class engine_windows_main
{
	HWND         hwnd;                                                  //ָ��windows��ľ����
	MSG          msg;                                                   //�洢��Ϣ�Ľṹ��
	WNDCLASS     wndclass;
	int32_t      window_width;
	int32_t      window_height;
	HINSTANCE    hInstance;
	HINSTANCE    hPrevInstance;
	PSTR         szCmdLine;
	int32_t      iCmdShow;
	PancyDx12Basic *new_device;
public:
	engine_windows_main(HINSTANCE hInstance_need, HINSTANCE hPrevInstance_need, PSTR szCmdLine_need, int iCmdShow_need);
	HRESULT game_create();
	HRESULT game_loop();
	WPARAM game_end();
	static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
};
LRESULT CALLBACK engine_windows_main::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:                // ���̰�����Ϣ
		if (wParam == VK_ESCAPE)    // ESC��
			DestroyWindow(hwnd);    // ���ٴ���, ������һ��WM_DESTROY��Ϣ
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
	case WM_SIZE:

		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
engine_windows_main::engine_windows_main(HINSTANCE hInstance_need, HINSTANCE hPrevInstance_need, PSTR szCmdLine_need, int iCmdShow_need)
{
	hwnd = NULL;
	hInstance = hInstance_need;
	hPrevInstance = hPrevInstance_need;
	szCmdLine = szCmdLine_need;
	iCmdShow = iCmdShow_need;
	window_width = 800;
	window_height = 600;
}
HRESULT engine_windows_main::game_create()
{
	//��䴰������
	wndclass.style = CS_HREDRAW | CS_VREDRAW;                   //����������ͣ��˴�������ֱ��ˮƽƽ�ƻ��ߴ�С�ı�ʱʱ��ˢ�£���msdnԭ�Ľ��ܣ�Redraws the entire window if a movement or size adjustment changes the width of the client area.
	wndclass.lpfnWndProc = WndProc;                                   //ȷ�����ڵĻص������������ڻ��windows�Ļص���Ϣʱ���ڴ�����Ϣ�ĺ�����
	wndclass.cbClsExtra = 0;                                         //Ϊ������ĩβ���������ֽڡ�
	wndclass.cbWndExtra = 0;                                         //Ϊ�������ʵ��ĩβ���������ֽڡ�
	wndclass.hInstance = hInstance;                                 //�����ô�����Ĵ��ڵľ����
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);          //�������ͼ������
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);              //������Ĺ������
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);     //������ı�����ˢ�����
	wndclass.lpszMenuName = NULL;                                      //������Ĳ˵���
	wndclass.lpszClassName = TEXT("pancystar_engine");           //����������ơ�

	//ȡ��dpi����Ϸ������
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	if (!RegisterClass(&wndclass))                                      //ע�ᴰ���ࡣ
	{
		MessageBox(NULL, TEXT("This program requires Windows NT!"),
			TEXT("pancystar_engine"), MB_ICONERROR);
		return E_FAIL;
	}
	//��ȡ��Ⱦ���������Ĵ�С
	RECT R = { 0, 0, window_width, window_height };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;
	//��������
	hwnd = CreateWindow(TEXT("pancystar_engine"), // window class name�����������õĴ���������֡�
		TEXT("pancystar_engine"),                 // window caption��Ҫ�����Ĵ��ڵı��⡣
		WS_DLGFRAME | WS_SYSMENU,                 // window style��Ҫ�����Ĵ��ڵ����͡�
		CW_USEDEFAULT,                            // initial x position���ڵĳ�ʼλ��ˮƽ���ꡣ
		CW_USEDEFAULT,                            // initial y position���ڵĳ�ʼλ�ô�ֱ���ꡣ
		width,                                    // initial x size���ڵ�ˮƽλ�ô�С��
		height,                                   // initial y size���ڵĴ�ֱλ�ô�С��
		NULL,                                     // parent window handle�丸���ڵľ����
		NULL,                                     // window menu handle��˵��ľ����
		hInstance,                                // program instance handle���ڳ����ʵ�������
		NULL);                                    // creation parameters�������ڵ�ָ��
	if (hwnd == NULL)
	{
		return E_FAIL;
	}
	RECT new_info;
	GetWindowRect(hwnd, &new_info);
	PancyDx12DeviceBasic::SingleCreate(hwnd, window_width, window_height);

	//MoveWindow(hwnd,new_info.left+100, new_info.top+100, width, height,true);
	new_device = new PancyDx12Basic();
	auto check_error = new_device->Create(hwnd, window_width, window_height);
	ShowWindow(hwnd, SW_SHOW);                    // ��������ʾ�������ϡ�
	UpdateWindow(hwnd);                           // ˢ��һ�鴰�ڣ�ֱ��ˢ�£�����windows��Ϣѭ����������ʾ����
	return S_OK;
}
HRESULT engine_windows_main::game_loop()
{
	//��Ϸѭ��
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			new_device->Render();
			PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
			TranslateMessage(&msg);//��Ϣת��
			DispatchMessage(&msg);//��Ϣ���ݸ����ڹ��̺���
		}
		else
		{
			new_device->Render();
		}
	}
	return S_OK;
}
WPARAM engine_windows_main::game_end()
{
	new_device->Release();
	delete new_device;
	delete PancyDx12DeviceBasic::GetInstance();
	return msg.wParam;
}
//windows���������
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	PSTR szCmdLine, int iCmdShow)
{
	//_CrtSetBreakAlloc(224);
	engine_windows_main *engine_main = new engine_windows_main(hInstance, hPrevInstance, szCmdLine, iCmdShow);
	engine_main->game_create();
	engine_main->game_loop();
	auto msg_end = engine_main->game_end();
	PancystarEngine::EngineFailLog::GetInstance();
	delete engine_main;
	delete PancystarEngine::EngineFailLog::GetInstance();
#ifdef CheckWindowMemory
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	//_CrtDumpMemoryLeaks();
	
	return msg_end;
}

