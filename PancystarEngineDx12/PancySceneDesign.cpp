#include"PancySceneDesign.h"
#define MEMORY_64MB 67108876
#define MEMORY_128MB 134217728
SceneRoot::SceneRoot()
{
	If_dsv_loaded = false;
	time_game = 0;
	scene_center_pos = DirectX::XMFLOAT3(0, 0, 0);
}
PancystarEngine::EngineFailReason SceneRoot::Create(int32_t width_in, int32_t height_in)
{
	back_buffer_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	frame_constant_buffer.resize(back_buffer_num);

	PancystarEngine::EngineFailReason check_error;
	check_error = ResetScreen(width_in, height_in);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = Init();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SceneRoot::ResetScreen(int32_t width_in, int32_t height_in)
{
	PancystarEngine::EngineFailReason check_error;
	Scene_width = width_in;
	Scene_height = height_in;
	std::vector<D3D12_HEAP_FLAGS> heap_flags;
	//�����µ���Ļ�ռ������ʽ
	D3D12_RESOURCE_DESC default_tex_RGB_dxdesc;
	D3D12_RESOURCE_DESC default_tex_float_desc;
	D3D12_RESOURCE_DESC depth_stencil_desc;
	default_tex_RGB_dxdesc.Alignment = 0;
	default_tex_RGB_dxdesc.DepthOrArraySize = 1;
	default_tex_RGB_dxdesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	default_tex_RGB_dxdesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	default_tex_RGB_dxdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	default_tex_RGB_dxdesc.Height = Scene_height;
	default_tex_RGB_dxdesc.Width = Scene_width;
	default_tex_RGB_dxdesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	default_tex_RGB_dxdesc.MipLevels = 1;
	default_tex_RGB_dxdesc.SampleDesc.Count = 1;
	default_tex_RGB_dxdesc.SampleDesc.Quality = 0;
	//����rgb8���͵Ĵ��ڴ�С�����ʽ
	default_tex_desc_RGB.heap_flag_in = D3D12_HEAP_FLAG_DENY_BUFFERS | D3D12_HEAP_FLAG_DENY_NON_RT_DS_TEXTURES;
	default_tex_desc_RGB.heap_type = D3D12_HEAP_TYPE_DEFAULT;
	default_tex_desc_RGB.if_force_srgb = false;
	default_tex_desc_RGB.if_gen_mipmap = false;
	default_tex_desc_RGB.max_size = 0;
	default_tex_desc_RGB.texture_data_file = "";
	default_tex_desc_RGB.texture_res_desc = default_tex_RGB_dxdesc;
	default_tex_desc_RGB.texture_type = PancystarEngine::Texture_Render_Target;
	//����srgb8���͵Ĵ��ڴ�С�����ʽ
	default_tex_desc_SRGB = default_tex_desc_RGB;
	default_tex_desc_SRGB.texture_res_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//����rgb16���͵Ĵ��ڴ�С�����ʽ
	default_tex_desc_float = default_tex_desc_RGB;
	default_tex_desc_float.texture_res_desc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	//�������ģ�建����
	default_tex_desc_depthstencil = default_tex_desc_RGB;
	default_tex_desc_depthstencil.texture_res_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	default_tex_desc_depthstencil.texture_res_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	default_tex_desc_depthstencil.heap_flag_in = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;
	//ɾ���ɵ����ģ�建����
	/*
	if (If_dsv_loaded)
	{
		for (int i = 0; i < back_buffer_num; ++i)
		{
			PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(Default_depthstencil_buffer[i]);
		}
	}
	*/
	//todo:������ֱ�Ӵ���������
	std::vector<SubMemoryPointer> back_buffer_data;
	std::vector<BasicDescriptorDesc> back_buffer_desc;

	for (int i = 0; i < back_buffer_num; ++i)
	{
		//�������ģ�建����
		std::string depth_stencil_use = "screentarget\\screen_" + std::to_string(width_in) + "_" + std::to_string(height_in) + "_DSV.json";
		pancy_object_id default_dsv_resource;
		auto check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(depth_stencil_use, default_dsv_resource);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		Default_depthstencil_buffer.push_back(default_dsv_resource);
		//�������ģ�建����������
		SubMemoryPointer tex_resource_data;
		D3D12_DEPTH_STENCIL_VIEW_DESC DSV_desc;
		D3D12_RESOURCE_DESC res_desc = {};
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(default_dsv_resource, tex_resource_data);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->GetTexDesc(default_dsv_resource, res_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		DSV_desc.Flags = D3D12_DSV_FLAG_NONE;
		DSV_desc.Format = res_desc.Format;
		DSV_desc.Texture2D.MipSlice = 0;
		DSV_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		back_buffer_data.push_back(tex_resource_data);
		BasicDescriptorDesc new_descriptor_desc;
		new_descriptor_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeDepthStencilView;
		new_descriptor_desc.depth_stencil_view_desc = DSV_desc;
		back_buffer_desc.push_back(new_descriptor_desc);
	}
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonGlobelDescriptor("DefaultDepthBufferSRV", back_buffer_desc, back_buffer_data,true);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	check_error = ScreenChange();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SceneRoot::GetGlobelCbuffer(
	const pancy_object_id &PSO_id, 
	const std::string &cbuffer_name, 
	PancyConstantBuffer ** cbuffer_data
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	//����pso��id�Ų��Ҷ�Ӧ��Cbuffer����
	auto PSO_cbuffer_list = frame_constant_buffer[now_frame].find(PSO_id);
	if (PSO_cbuffer_list == frame_constant_buffer[now_frame].end()) 
	{
		//ָ����pso��δ����cbuffer
		std::string pso_name_pre;
		//�ȼ��pso�Ƿ����
		check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, pso_name_pre);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		//���һ��PSO��cbuffer��
		std::unordered_map<std::string, PancyConstantBuffer *> new_pso_cbuffer_list;
		frame_constant_buffer[now_frame].insert(std::pair<pancy_object_id, std::unordered_map<std::string, PancyConstantBuffer *>>(PSO_id, new_pso_cbuffer_list));
		PSO_cbuffer_list = frame_constant_buffer[now_frame].find(PSO_id);
	}
	//����cbuffer������Ѱ�ҳ���������
	auto cbuffer_out = PSO_cbuffer_list->second.find(cbuffer_name);
	if (cbuffer_out == PSO_cbuffer_list->second.end())
	{
		//cbufferδ����������һ������������
		std::string pso_name;
		check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, pso_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string pso_divide_path;
		std::string pso_divide_name;
		std::string pso_divide_tail;
		PancystarEngine::DivideFilePath(pso_name, pso_divide_path, pso_divide_name, pso_divide_tail);
		PancyConstantBuffer *new_cbuffer = new PancyConstantBuffer(cbuffer_name, pso_divide_name);
		const Json::Value *cbuffer_desc_root;
		PancyEffectGraphic::GetInstance()->GetCbuffer(PSO_id, cbuffer_name, cbuffer_desc_root);
		check_error = new_cbuffer->Create(*cbuffer_desc_root);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		PSO_cbuffer_list->second.insert(std::pair<std::string, PancyConstantBuffer *>(cbuffer_name, new_cbuffer));
		cbuffer_out = PSO_cbuffer_list->second.find(cbuffer_name);
	}
	*cbuffer_data = cbuffer_out->second;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason SceneRoot::GetGlobelCbuffer(
	const pancy_object_id &PSO_id,
	const std::string &cbuffer_name,
	std::vector<PancyConstantBuffer*> &cbuffer_data
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	//todo��Ŀǰ���ڼ��ص�ʱ��˳�����ȫ��cbuffer�Ĵ����Ժ�ȫ��cbuffer��Ҫ��ǰ����
	std::vector<SubMemoryPointer> back_buffer_data;
	std::vector<BasicDescriptorDesc> back_buffer_desc;
	bool if_create = false;
	for (int i = 0; i < frame_num; ++i) 
	{
		//����pso��id�Ų��Ҷ�Ӧ��Cbuffer����
		auto PSO_cbuffer_list = frame_constant_buffer[i].find(PSO_id);
		if (PSO_cbuffer_list == frame_constant_buffer[i].end())
		{
			if_create = true;
			//ָ����pso��δ����cbuffer
			std::string pso_name_pre;
			//�ȼ��pso�Ƿ����
			check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, pso_name_pre);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//���һ��PSO��cbuffer��
			std::unordered_map<std::string, PancyConstantBuffer *> new_pso_cbuffer_list;
			frame_constant_buffer[i].insert(std::pair<pancy_object_id, std::unordered_map<std::string, PancyConstantBuffer *>>(PSO_id, new_pso_cbuffer_list));
			PSO_cbuffer_list = frame_constant_buffer[i].find(PSO_id);
		}
		//����cbuffer������Ѱ�ҳ���������
		auto cbuffer_out = PSO_cbuffer_list->second.find(cbuffer_name);
		if (cbuffer_out == PSO_cbuffer_list->second.end())
		{
			//cbufferδ����������һ������������
			std::string pso_name;
			check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, pso_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			std::string pso_divide_path;
			std::string pso_divide_name;
			std::string pso_divide_tail;
			PancystarEngine::DivideFilePath(pso_name, pso_divide_path, pso_divide_name, pso_divide_tail);
			PancyConstantBuffer *new_cbuffer = new PancyConstantBuffer(cbuffer_name, pso_divide_name);
			const Json::Value *cbuffer_desc_root;
			PancyEffectGraphic::GetInstance()->GetCbuffer(PSO_id, cbuffer_name, cbuffer_desc_root);
			check_error = new_cbuffer->Create(*cbuffer_desc_root);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			PSO_cbuffer_list->second.insert(std::pair<std::string, PancyConstantBuffer *>(cbuffer_name, new_cbuffer));
			cbuffer_out = PSO_cbuffer_list->second.find(cbuffer_name);
		}
		cbuffer_data.push_back(cbuffer_out->second);
		SubMemoryPointer check_pointer;
		cbuffer_out->second->GetBufferSubResource(check_pointer);
		back_buffer_data.push_back(check_pointer);
		BasicDescriptorDesc new_desc;
		new_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeConstantBufferView;
		back_buffer_desc.push_back(new_desc);
	}
	//����������
	if (if_create) 
	{
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildCommonGlobelDescriptor(cbuffer_name, back_buffer_desc, back_buffer_data, true);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
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
HRESULT engine_windows_main::game_create(SceneRoot   *new_scene_in)
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
	PancystarEngine::PancyDescriptorHeapControl::GetInstance();
	PancyInput::SingleCreate(hwnd, hInstance);
	PancyCamera::GetInstance();
	check_error = PancystarEngine::PancySkinAnimationControl::SingleCreate(MEMORY_128MB, MEMORY_64MB);
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
	//�����̳߳ع���
	check_error = ThreadPoolGPUControl::SingleCreate();
	if (!check_error.CheckIfSucceed())
	{
		return E_FAIL;
	}
	//��������
	new_scene = new_scene_in;
	//new_scene = new scene_test_simple();

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
	delete PancystarEngine::PancySkinAnimationControl::GetInstance();
	delete PancystarEngine::RenderParamSystem::GetInstance();
	//delete PancystarEngine::DescriptorControl::GetInstance();
	delete PancyDx12DeviceBasic::GetInstance();
	PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
	delete PancystarEngine::EngineFailLog::GetInstance();
	delete ThreadPoolGPUControl::GetInstance();
	delete PancyShaderControl::GetInstance();
	delete PancyRootSignatureControl::GetInstance();
	delete PancyEffectGraphic::GetInstance();
	delete PancyJsonTool::GetInstance();
	delete PancystarEngine::PancyDescriptorHeapControl::GetInstance();
	delete PancystarEngine::FileBuildRepeatCheck::GetInstance();
	delete PancyInput::GetInstance();
	delete PancyCamera::GetInstance();
	
	return msg.wParam;
}