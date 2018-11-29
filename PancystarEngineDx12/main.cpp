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
class scene_root
{
protected:
	DirectX::XMFLOAT3         scene_center_pos;//��������
	float                     time_game;       //��Ϸʱ��
public:
	scene_root();
	virtual PancystarEngine::EngineFailReason Create(int32_t width_in, int32_t height_in) = 0;
	virtual void Display() = 0;
	virtual void DisplayNopost() = 0;
	virtual void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix) = 0;
	virtual void Update(float delta_time) = 0;
	virtual ~scene_root() 
	{
	};
};
scene_root::scene_root()
{
	time_game = 0;
	scene_center_pos = DirectX::XMFLOAT3(0, 0, 0);
}
class scene_test_simple : public scene_root
{
	//����״̬
	ComPtr<ID3D12PipelineState> m_pipelineState;
	uint32_t renderlist_ID;
	//ģ�Ͳ���
	PancystarEngine::GeometryBasic *test_model;
	//�ӿ�
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT view_rect;
	//֡�ȴ�fence����
	PancyFenceIdGPU broken_fence_id;
	//��Դ�󶨲���
	ResourceViewPointer table_offset[3];
public:
	scene_test_simple()
	{
	}
	~scene_test_simple();
	PancystarEngine::EngineFailReason Create(int32_t width_in, int32_t height_in);
	PancystarEngine::EngineFailReason ResetScreen(int32_t width_in, int32_t height_in);
	void Display();
	void DisplayNopost() {};
	void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix);
	void Update(float delta_time);
private:
	void PopulateCommandList();
	void WaitForPreviousFrame();
	inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
	{
		return max(0, min(ax2, bx2) - max(ax1, bx1)) * max(0, min(ay2, by2) - max(ay1, by1));
	}
};
PancystarEngine::EngineFailReason scene_test_simple::ResetScreen(int32_t width_in, int32_t height_in)
{
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
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::Create(int32_t width_in, int32_t height_in)
{
	PancystarEngine::EngineFailReason check_error;
	check_error = ResetScreen(width_in, height_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	//������ʱ����
	PancystarEngine::Point2D point[3];
	point[0].position = DirectX::XMFLOAT4(0.0f, 0.25f * 1.77f, 0.0f, 1);
	point[1].position = DirectX::XMFLOAT4(0.25f, -0.25f * 1.77f, 0.0f, 1);
	point[2].position = DirectX::XMFLOAT4(-0.25f, -0.25f * 1.77f, 0.0f, 1);

	point[0].tex_color = DirectX::XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);
	point[1].tex_color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	point[2].tex_color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	UINT index[3] = { 0,1,2 };
	test_model = new PancystarEngine::GeometryCommonModel<PancystarEngine::Point2D>(point, index, 3, 3);
	check_error = test_model->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
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
	//check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset[1], cbuffer[1]);
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
	//���cbuffer
	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 world_mat[2];
	DirectX::XMStoreFloat4x4(&world_mat[0],DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&world_mat[1], DirectX::XMMatrixTranslation(0.2,0.2,0));
	CD3DX12_RANGE readRange(0, 0);
	UINT8* m_pCbvDataBegin;
	data_submemory->GetResource()->Map(0, &readRange, reinterpret_cast<void**>(&m_pCbvDataBegin));
	memcpy(m_pCbvDataBegin+(cbuffer[0].offset* per_memory_size), &world_mat,sizeof(world_mat));
	DirectX::XMFLOAT4X4 *p = reinterpret_cast<DirectX::XMFLOAT4X4*>(m_pCbvDataBegin);
	data_submemory->GetResource()->Unmap(0,NULL);
	return PancystarEngine::succeed;
}
void scene_test_simple::Display()
{
	HRESULT hr;
	PopulateCommandList();
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &renderlist_ID);
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
	hr = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->Present(1, 0);
	WaitForPreviousFrame();
}
void scene_test_simple::DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix)
{
}
void scene_test_simple::PopulateCommandList()
{
	PancystarEngine::EngineFailReason check_error;

	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
	PancyRenderCommandList *m_commandList;
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_test.json")->GetData();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data, &m_commandList, renderlist_ID);
	
	
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->DrawInstanced(3, 1, 0, 0);

	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	m_commandList->UnlockPrepare();
}
void scene_test_simple::WaitForPreviousFrame()
{
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(broken_fence_id);
}
void scene_test_simple::Update(float delta_time)
{
}
scene_test_simple::~scene_test_simple()
{
	WaitForPreviousFrame();
	delete test_model;
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
	scene_root   *new_scene;
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
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = TEXT("pancystar_engine");

	//ȡ��dpi����Ϸ������
	SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
	if (!RegisterClass(&wndclass))
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
	hwnd = CreateWindow(TEXT("pancystar_engine"),
		TEXT("pancystar_engine"),
		WS_DLGFRAME | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		width,
		height,
		NULL,
		NULL,
		hInstance,
		NULL);
	if (hwnd == NULL)
	{
		return E_FAIL;
	}
	RECT new_info;
	GetWindowRect(hwnd, &new_info);
	
	//����directx�豸
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyDx12DeviceBasic::SingleCreate(hwnd, window_width, window_height);
	if (!check_error.CheckIfSucceed()) 
	{
		return E_FAIL;
	}
	//ע�ᵥ��
	PancyShaderControl::GetInstance();
	PancyRootSignatureControl::GetInstance();
	PancyEffectGraphic::GetInstance();
	PancyJsonTool::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	PancyDescriptorHeapControl::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	SubresourceControl::GetInstance();
	//�����̳߳ع���
	check_error = ThreadPoolGPUControl::SingleCreate();
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
	//MoveWindow(hwnd,new_info.left+100, new_info.top+100, width, height,true);
	//��������
	new_scene = new scene_test_simple();
	check_error = new_scene->Create(window_width, window_height);
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
			//new_scene->Render();
			new_scene->Update(0);
			new_scene->Display();
			TranslateMessage(&msg);//��Ϣת��
			DispatchMessage(&msg);//��Ϣ���ݸ����ڹ��̺���
		}
		else
		{
			//new_scene->Render();
			new_scene->Update(0);
			new_scene->Display();
		}
	}
	return S_OK;
}
WPARAM engine_windows_main::game_end()
{
	delete new_scene;
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
	delete PancystarEngine::FileBuildRepeatCheck::GetInstance();
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

