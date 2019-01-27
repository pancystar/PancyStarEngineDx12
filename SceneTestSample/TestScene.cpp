#include"TestScene.h"
PancystarEngine::EngineFailReason scene_test_simple::ScreenChange()
{
	view_port.TopLeftX = 0;
	view_port.TopLeftY = 0;
	view_port.Width = static_cast<FLOAT>(Scene_width);
	view_port.Height = static_cast<FLOAT>(Scene_height);
	view_port.MaxDepth = 1.0f;
	view_port.MinDepth = 0.0f;
	view_rect.left = 0;
	view_rect.top = 0;
	view_rect.right = Scene_width;
	view_rect.bottom = Scene_height;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::PretreatPbrDescriptor()
{
	PancystarEngine::EngineFailReason check_error;
	std::unordered_map<std::string, std::string> Cbuffer_Heap_desc;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json")->GetCbufferHeapName(Cbuffer_Heap_desc);
	std::vector<DescriptorTableDesc> descriptor_use_data;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_pbr.json")->GetDescriptorHeapUse(descriptor_use_data);
	int count = 0;
	for (auto cbuffer_data = Cbuffer_Heap_desc.begin(); cbuffer_data != Cbuffer_Heap_desc.end(); ++cbuffer_data)
	{
		check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(cbuffer_data->second, cbuffer_model[count]);
		count += 1;
	}
	ResourceViewPack globel_var;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_use_data[0].descriptor_heap_name, globel_var);
	//�ȴ�������cbufferview
	table_offset_model[0].resource_view_pack_id = globel_var;
	table_offset_model[0].resource_view_offset_id = descriptor_use_data[0].table_offset[0];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset_model[0], cbuffer_model[0]);
	table_offset_model[1].resource_view_pack_id = globel_var;
	table_offset_model[1].resource_view_offset_id = descriptor_use_data[0].table_offset[1];
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(table_offset_model[1], cbuffer_model[1]);
	//��������srv
	SubMemoryPointer texture_need;
	//tex_id = tex_brdf_id;
	table_offset_model[2].resource_view_pack_id = globel_var;
	table_offset_model[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	ResourceViewPointer new_rvp = table_offset_model[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	//���淴�价����
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_spec_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_spec_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����价����
	new_rvp.resource_view_offset_id += 1;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_diffuse_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_diffuse_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//brdfԤ��������
	new_rvp.resource_view_offset_id += 1;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, texture_need);
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_brdf_id, SRV_desc);
	SRV_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//ģ�����������
	table_offset_model[3].resource_view_pack_id = globel_var;
	table_offset_model[3].resource_view_offset_id = descriptor_use_data[0].table_offset[3];
	new_rvp = table_offset_model[3];
	for (int i = 0; i < model_deal->GetSubModelNum(); ++i) 
	{
		pancy_object_id now_tex_id = model_deal->GetSubModelTexture(i,TexType::tex_diffuse);
		//����������
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//��������
		now_tex_id = model_deal->GetSubModelTexture(i, TexType::tex_normal);
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(now_tex_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(now_tex_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//����������
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_metallic_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_metallic_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_rvp.resource_view_offset_id += 1;
		//�ֲڶ�����
		PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_roughness_id, texture_need);
		PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_roughness_id, SRV_desc);
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_rvp, texture_need, SRV_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::Init()
{
	PancystarEngine::EngineFailReason check_error;
	//����ȫ��������
	PancystarEngine::Point2D point[4];
	point[0].position = DirectX::XMFLOAT4(-1.0f, -1.0f, 0.0f, 1.0f);
	point[1].position = DirectX::XMFLOAT4(-1.0f, +1.0f, 0.0f, 1.0f);
	point[2].position = DirectX::XMFLOAT4(+1.0f, +1.0f, 0.0f, 1.0f);
	point[3].position = DirectX::XMFLOAT4(+1.0f, -1.0f, 0.0f, 1.0f);
	point[0].tex_color = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 0.0f);
	point[1].tex_color = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	point[2].tex_color = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f);
	point[3].tex_color = DirectX::XMFLOAT4(1.0f, 1.0f, 0.0f, 0.0f);
	UINT index[] = { 0,1,2 ,0,2,3};
	test_model = new PancystarEngine::GeometryCommonModel<PancystarEngine::Point2D>(point, index, 4, 6);
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

	//ģ�ͼ��ز���
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_pbr.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����һ��cbuffer

	//����һ��pso
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_sky.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	std::unordered_map<std::string, std::string> Cbuffer_Heap_desc;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_sky.json")->GetCbufferHeapName(Cbuffer_Heap_desc);
	std::vector<DescriptorTableDesc> descriptor_use_data;
	PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_sky.json")->GetDescriptorHeapUse(descriptor_use_data);
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
	//Ԥ����brdf
	check_error = PretreatBrdf();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}

	//������Ҫ��pbr����
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Cubemap.json", tex_ibl_spec_id);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\IrradianceMap.json", tex_ibl_diffuse_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Sphere002_metallic.json", tex_metallic_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("data\\Sphere002_roughness.json", tex_roughness_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//Ϊpbrģ�͵���Ⱦ����descriptor
	check_error = PretreatPbrDescriptor();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	



	SubMemoryPointer texture_need;
	//tex_id = tex_brdf_id;
	
	
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_ibl_spec_id, texture_need);
	table_offset[2].resource_view_pack_id = globel_var;
	table_offset[2].resource_view_offset_id = descriptor_use_data[0].table_offset[2];
	D3D12_SHADER_RESOURCE_VIEW_DESC SRV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(tex_ibl_spec_id, SRV_desc);
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(table_offset[2], texture_need, SRV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason scene_test_simple::PretreatBrdf()
{
	CD3DX12_VIEWPORT view_port_brdf;
	CD3DX12_RECT view_rect_brdf;
	view_port_brdf.TopLeftX = 0;
	view_port_brdf.TopLeftY = 0;
	view_port_brdf.Width = 1024.0;
	view_port_brdf.Height = 1024.0;
	view_port_brdf.MaxDepth = 1.0f;
	view_port_brdf.MinDepth = 0.0f;
	view_rect_brdf.left = 0;
	view_rect_brdf.top = 0;
	view_rect_brdf.right = 1024;
	view_rect_brdf.bottom = 1024;
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyEffectGraphic::GetInstance()->BuildPso("json\\pipline_state_object\\pso_brdfgen.json");
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����brdfԤ��������
	//todo:commandalloctor���֡��Ҫ�����̳߳�
	//todo������resourcedesc������heap���ֿ�Ĵ�С
	//pancy_object_id tex_brdf_id;
	SubMemoryPointer texture_brdf_need;
	check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource("json\\texture\\1024_1024_R16B16G16A16FLOAT.json", tex_brdf_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//������ȾĿ��
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(tex_brdf_id, texture_brdf_need);
	D3D12_RENDER_TARGET_VIEW_DESC RTV_desc;
	PancystarEngine::PancyTextureControl::GetInstance()->GetRTVDesc(tex_brdf_id, RTV_desc);
	RTV_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	std::string dsv_descriptor_name = "json\\descriptor_heap\\RTV_1_descriptor_heap.json";
	ResourceViewPointer RTV_pointer;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(dsv_descriptor_name, RTV_pointer.resource_view_pack_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	RTV_pointer.resource_view_offset_id = 0;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildRTV(RTV_pointer, texture_brdf_need, RTV_desc);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����Ԥ��Ⱦ����
	PancyRenderCommandList *m_commandList;
	auto pso_data = PancyEffectGraphic::GetInstance()->GetPSO("json\\pipline_state_object\\pso_brdfgen.json");
	PancyThreadIdGPU commdlist_id_use;
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(pso_data->GetData(), &m_commandList, commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port_brdf);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect_brdf);
	auto rootsignature_data = pso_data->GetRootSignature()->GetResource();
	m_commandList->GetCommandList()->SetGraphicsRootSignature(rootsignature_data.Get());
	//������ȾĿ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	int64_t per_mem_size;
	auto rtv_res_data = SubresourceControl::GetInstance()->GetResourceData(texture_brdf_need, per_mem_size);
	ComPtr<ID3D12Resource> screen_rendertarget = rtv_res_data->GetResource();
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(RTV_pointer, rtvHandle);
	
	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, NULL);
	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	//��Ⱦ������
	m_commandList->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_commandList->GetCommandList()->IASetVertexBuffers(0, 1, &test_model->GetVertexBufferView());
	m_commandList->GetCommandList()->IASetIndexBuffer(&test_model->GetIndexBufferView());
	m_commandList->GetCommandList()->DrawIndexedInstanced(test_model->GetIndexNum(), 1, 0, 0, 0);
	
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1, &commdlist_id_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void scene_test_simple::Display()
{
	HRESULT hr;
	renderlist_ID.clear();
	auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->FreeAlloctor();
	ClearScreen();
	//PopulateCommandList(model_sky);
	PopulateCommandList(model_cube);
	PopulateCommandListSky();
	PopulateCommandListModelDeal();
	check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(renderlist_ID.size(), &renderlist_ID[0]);
	ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SetGpuBrokenFence(broken_fence_id);
	hr = PancyDx12DeviceBasic::GetInstance()->GetSwapchain()->Present(1, 0);
	WaitForPreviousFrame();
}
void scene_test_simple::DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix)
{
}
void scene_test_simple::ClearScreen()
{
	PancyRenderCommandList *m_commandList;
	PancyThreadIdGPU commdlist_id_use;
	auto check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &m_commandList, commdlist_id_use);
	m_commandList->GetCommandList()->RSSetViewports(1, &view_port);
	m_commandList->GetCommandList()->RSSetScissorRects(1, &view_rect);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12Resource> screen_rendertarget = PancyDx12DeviceBasic::GetInstance()->GetBackBuffer(rtvHandle);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//�޸���Դ��ʽΪdsv
	int32_t now_render_num = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	SubMemoryPointer sub_res_dsv;
	int64_t per_memory_size;
	PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(Default_depthstencil_buffer[now_render_num], sub_res_dsv);
	auto memory_data = SubresourceControl::GetInstance()->GetResourceData(sub_res_dsv, per_memory_size);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	//��ȡ��Ȼ�����
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto heap_offset = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(Default_depthstencil_view[now_render_num], dsvHandle);

	m_commandList->GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	m_commandList->GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	m_commandList->GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH| D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(screen_rendertarget.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->GetCommandList()->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(memory_data->GetResource().Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_PRESENT));
	m_commandList->UnlockPrepare();
	renderlist_ID.push_back(commdlist_id_use);
}
void scene_test_simple::WaitForPreviousFrame()
{
	auto  check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(broken_fence_id);
}
void scene_test_simple::Update(float delta_time)
{
	PancystarEngine::EngineFailReason check_error;
	updateinput(delta_time);
	DirectX::XMFLOAT4X4 world_mat,uv_mat;
	DirectX::XMStoreFloat4x4(&uv_mat,DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&world_mat,DirectX::XMMatrixIdentity());
	PancyModelAssimp *render_object_sky = dynamic_cast<PancyModelAssimp*>(model_sky);
	render_object_sky->update(world_mat, uv_mat, delta_time);

	DirectX::XMStoreFloat4x4(&uv_mat,  DirectX::XMMatrixScaling(1000,1000,0));
	DirectX::XMStoreFloat4x4(&world_mat, DirectX::XMMatrixScaling(100,2,100)*DirectX::XMMatrixTranslation(0, -5, 0));
	PancyModelAssimp *render_object_cube = dynamic_cast<PancyModelAssimp*>(model_cube);
	render_object_cube->update(world_mat, uv_mat, delta_time);



	int64_t per_memory_size;
	auto data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer[0], per_memory_size);
	DirectX::XMFLOAT4X4 sky_world_mat[4];
	DirectX::XMFLOAT4X4 view_mat, inv_view_mat;
	PancyCamera::GetInstance()->CountViewMatrix(&view_mat);
	PancyCamera::GetInstance()->CountInvviewMatrix(&inv_view_mat);
	//������cbuffer
	DirectX::XMMATRIX proj_mat = DirectX::XMMatrixPerspectiveFovLH(DirectX::XM_PIDIV4, 1280.0f / 720.0f, 0.1f, 1000.0f);
	DirectX::XMStoreFloat4x4(&sky_world_mat[0], DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(100,100,100)));
	DirectX::XMStoreFloat4x4(&sky_world_mat[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&sky_world_mat[0]) * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&sky_world_mat[2], DirectX::XMMatrixIdentity());
	DirectX::XMVECTOR x_delta;
	DirectX::XMStoreFloat4x4(&sky_world_mat[3], DirectX::XMMatrixInverse(&x_delta, DirectX::XMLoadFloat4x4(&sky_world_mat[0])));
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer[0].offset* per_memory_size, sky_world_mat, sizeof(sky_world_mat));
	//��䴦��ģ�͵�cbuffer
	data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer_model[0], per_memory_size);
	DirectX::XMFLOAT4X4 pbr_world_mat[4];
	DirectX::XMStoreFloat4x4(&pbr_world_mat[0], DirectX::XMMatrixTranspose(DirectX::XMMatrixScaling(1, 1, 1)));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[1], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&pbr_world_mat[0]) * DirectX::XMLoadFloat4x4(&view_mat) * proj_mat));
	DirectX::XMStoreFloat4x4(&pbr_world_mat[2], DirectX::XMMatrixIdentity());
	DirectX::XMStoreFloat4x4(&pbr_world_mat[3], DirectX::XMMatrixInverse(&x_delta, DirectX::XMLoadFloat4x4(&pbr_world_mat[0])));
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[0].offset* per_memory_size, pbr_world_mat, sizeof(pbr_world_mat));

	data_submemory = SubresourceControl::GetInstance()->GetResourceData(cbuffer_model[1], per_memory_size);
	per_view_pack view_buffer_data;
	DirectX::XMStoreFloat4x4(&view_buffer_data.view_matrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&view_mat)));
	DirectX::XMStoreFloat4x4(&view_buffer_data.projectmatrix, DirectX::XMMatrixTranspose(proj_mat));
	DirectX::XMStoreFloat4x4(&view_buffer_data.invview_matrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&inv_view_mat)));
	DirectX::XMFLOAT3 view_pos;
	PancyCamera::GetInstance()->GetViewPosition(&view_pos);
	view_buffer_data.view_position.x = view_pos.x;
	view_buffer_data.view_position.y = view_pos.y;
	view_buffer_data.view_position.z = view_pos.z;
	view_buffer_data.view_position.w = 1.0f;
	check_error = data_submemory->WriteFromCpuToBuffer(cbuffer_model[1].offset* per_memory_size, &view_buffer_data, sizeof(view_buffer_data));
}
scene_test_simple::~scene_test_simple()
{
	WaitForPreviousFrame();
	delete test_model;
	delete model_cube;
	delete model_sky;
	delete model_deal;
}