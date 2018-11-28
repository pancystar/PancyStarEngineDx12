#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
#include"PancyGeometryDx12.h"
#include"PancyShaderDx12.h"
#include"PancyTextureDx12.h"
#include"PancyThreadBasic.h"
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
	//��Դ�Ѵ�С	
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	UINT m_rtvDescriptorSize;
	std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
	//����״̬
	ComPtr<ID3D12PipelineState> m_pipelineState;
	uint32_t renderlist_ID;
	//ģ�Ͳ���
	PancystarEngine::GeometryBasic *test_model;
	//PancyShaderBasic *shader_test;
	//�ӿ�
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT view_rect;
	//֡�ȴ�fence����
	PancyFenceIdGPU broken_fence_id;
	//��Դ�󶨲���
	ResourceViewPointer table_offset[3];
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
	PancystarEngine::EngineFailReason check_error;
	
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
	PancyRenderCommandList *m_commandList;
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetData();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data,&m_commandList, renderlist_ID);
	// Set necessary state.
	auto rootsignature_data = PancyRootSignatureControl::GetInstance()->GetRootSignature("json\\root_signature\\test_root_signature.json")->GetRootSignature();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());


	//������������
	ID3D12DescriptorHeap *heap_pointer;
	heap_pointer = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(table_offset[0].resource_view_pack_id.descriptor_heap_type_id).Get();
	m_commandList->GetCommandList()->SetDescriptorHeaps(1, &heap_pointer);
	//�����������ѵ�ƫ��
	for (int i = 0; i < 3; ++i) 
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle;
		//CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(heap_pointer->GetGPUDescriptorHandleForHeapStart());
		auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(table_offset[i], srvHandle);
		//srvHandle.Offset(heap_offset);
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(i, srvHandle);
	}
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[now_frame_use].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), now_frame_use, m_rtvDescriptorSize);
	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->DrawInstanced(3, 1, 0, 0);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_renderTargets[now_frame_use].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
}
void PancyDx12Basic::WaitForPreviousFrame()
{
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(broken_fence_id);
	now_frame_use = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->GetCurrentBackBufferIndex();
}

void PancyDx12Basic::Render()
{
	HRESULT hr;
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList();

	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1,&renderlist_ID);
	
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
	
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
	view_port.TopLeftX = 0;
	view_port.TopLeftY = 0;
	view_port.Width = static_cast<FLOAT>(width_in);
	view_port.Height = static_cast<FLOAT>(height_in);
	view_port.MaxDepth = 1.0f;
	view_port.MinDepth = 0.0f;
	view_rect.left = 0;
	view_rect.top = 0;
	view_rect.right = width_in;
	view_rect.bottom = height_in;
	//������ʱ����
	PancystarEngine::Point2D point[3];
	point[0].position = DirectX::XMFLOAT4(0.0f, 0.25f * 1.77f, 0.0f,1);
	point[1].position = DirectX::XMFLOAT4(0.25f, -0.25f * 1.77f, 0.0f,1);
	point[2].position = DirectX::XMFLOAT4(-0.25f, -0.25f * 1.77f, 0.0f,1);

	point[0].tex_color = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f,0.0f);
	point[1].tex_color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	point[2].tex_color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	UINT index[3] = {0,1,2};
	test_model = new PancystarEngine::GeometryCommonModel<PancystarEngine::Point2D>(point, index,3,3);
	auto check_error = test_model->Create();
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	/*
	shader_test = new PancyShaderBasic("shader\\testcolor.hlsl","VSMain","vs_5_0");
	check_error = shader_test->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
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
		D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {};
		rtv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		rtv_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateRenderTargetView(m_renderTargets[n].Get(), &rtv_desc, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
	}
	//����һ��pso
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_test.json");
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	
	//����һ��cbuffer
	std::unordered_map<std::string, std::string> Cbuffer_Heap_desc;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetCbufferHeapName(Cbuffer_Heap_desc);
	std::vector<DescriptorTableDesc> descriptor_use_data;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetDescriptorHeapUse(descriptor_use_data);
	SubMemoryPointer cbuffer[2];
	int count = 0;
	for (auto cbuffer_data = Cbuffer_Heap_desc.begin(); cbuffer_data != Cbuffer_Heap_desc.end(); ++cbuffer_data) 
	{
		check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(cbuffer_data->second, cbuffer[count]);
		count += 1;
	}
	ResourceViewPack globel_var;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_use_data[0].descriptor_heap_name, globel_var);
	//ResourceViewPointer new_rsv;
	table_offset[0].resource_view_pack_id = globel_var;
	table_offset[0].resource_view_offset_id = descriptor_use_data[0].table_offset[0];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset[0], cbuffer[0]);
	table_offset[1].resource_view_pack_id = globel_var;
	table_offset[1].resource_view_offset_id = descriptor_use_data[0].table_offset[1];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset[1], cbuffer[1]);
	//����һ������
	pancy_object_id tex_id;
	SubMemoryPointer texture_need;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\test222.json", tex_id);
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_id, texture_need);
	table_offset[2].resource_view_pack_id = globel_var;
	table_offset[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(table_offset[2], texture_need, SRV_desc);

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
	delete test_model;
	//delete shader_test;
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
	window_width = 1280;
	window_height = 720;
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
	//ע�ᵥ��
	PancyShaderControl::GetInstance();
	PancyRootSignatureControl::GetInstance();
	PancyEffectGraphic::GetInstance();
	PancyJsonTool::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	PancyDescriptorHeapControl::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	SubresourceControl::GetInstance();
	//����
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyDx12DeviceBasic::SingleCreate(hwnd, window_width, window_height);
	if (!check_error.CheckIfSucceed()) 
	{
		return E_FAIL;
	}

	check_error = ThreadPoolGPUControl::SingleCreate();
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
	//MoveWindow(hwnd,new_info.left+100, new_info.top+100, width, height,true);
	new_device = new PancyDx12Basic();
	check_error = new_device->Create(hwnd, window_width, window_height);
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
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
	//_CrtSetBreakAlloc(4969);
	engine_windows_main *engine_main = new engine_windows_main(hInstance, hPrevInstance, szCmdLine, iCmdShow);
	HRESULT hr = engine_main->game_create();
	if (FAILED(hr)) 
	{
		return 0;
	}
	engine_main->game_loop();
	auto msg_end = engine_main->game_end();
	PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
	delete engine_main;
	delete PancystarEngine::EngineFailLog::GetInstance();
	delete ThreadPoolGPUControl::GetInstance();
	delete PancyShaderControl::GetInstance();
	delete PancyRootSignatureControl::GetInstance();
	delete PancyEffectGraphic::GetInstance();
	delete PancyJsonTool::GetInstance();
	delete MemoryHeapGpuControl::GetInstance();
	delete PancyDescriptorHeapControl::GetInstance();
	delete PancystarEngine::PancyTextureControl::GetInstance();
	delete SubresourceControl::GetInstance();
	if (InputLayoutDesc::GetInstance() != NULL)
	{
		delete InputLayoutDesc::GetInstance();
	}
#ifdef CheckWindowMemory
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
#endif
	
	//_CrtDumpMemoryLeaks();
	
	return static_cast<int>(msg_end);
}

