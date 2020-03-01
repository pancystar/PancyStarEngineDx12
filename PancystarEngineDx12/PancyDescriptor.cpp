#include"PancyDescriptor.h"
using namespace PancystarEngine;

//����������Σ��������������������������ҳ�ķ���͹���
BindlessResourceViewSegmental::BindlessResourceViewSegmental(
	const pancy_object_id &max_descriptor_num_in,
	const pancy_object_id &segmental_offset_position_in,
	const pancy_object_id &per_descriptor_size_in,
	const ComPtr<ID3D12DescriptorHeap> descriptor_heap_data_in
)
{
	max_descriptor_num = max_descriptor_num_in;
	segmental_offset_position = segmental_offset_position_in;
	now_descriptor_pack_id_self_add = 0;
	now_pointer_offset = 0;
	per_descriptor_size = per_descriptor_size_in;
	descriptor_data.rehash(max_descriptor_num);
	now_pointer_refresh = max_descriptor_num_in;
	descriptor_heap_data = descriptor_heap_data_in.Get();
}
//����������ҳ��ָ����Ϣ�����������ѿ���������
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::BuildShaderResourceView(const BindlessResourceViewPointer &resource_view_pointer)
{
	for (int i = 0; i < resource_view_pointer.resource_view_num; ++i)
	{
		//���㵱ǰ���������������������ѵ�ƫ����(���׵�ַƫ��+ҳ�׵�ַƫ��+��ƫ��)
		pancy_object_id resource_view_heap_offset = segmental_offset_position + resource_view_pointer.resource_view_offset + i;
		//��������ƫ������Ӧ����ʵ��ַƫ����
		pancy_resource_size real_offset = static_cast<pancy_resource_size>(resource_view_heap_offset) * static_cast<pancy_resource_size>(per_descriptor_size);
		//������ʵ��ַƫ����������SRV������
		PancystarEngine::EngineFailReason check_error;
		//����������
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptor_heap_data->GetCPUDescriptorHandleForHeapStart());
		cpuHandle.Offset(real_offset);
		//�����Դ��ʽ
		auto real_resource = resource_view_pointer.describe_memory_data[i].GetResourceData();
		ResourceBlockGpu* now_gpu_resource = NULL;
		if (real_resource->GetResourceTypeName() == typeid(PancyBasicBuffer).name()) 
		{
			const PancyBasicBuffer *buffer_resource_data = dynamic_cast<const PancyBasicBuffer*>(real_resource);
			now_gpu_resource = buffer_resource_data->GetGpuResourceData();
		}
		else if(real_resource->GetResourceTypeName() == typeid(PancyBasicTexture).name())
		{
			const PancyBasicTexture *texture_resource_data = dynamic_cast<const PancyBasicTexture*>(real_resource);
			now_gpu_resource = texture_resource_data->GetGpuResourceData();
		}
		else 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The Resource is not a buffer/texture, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		if (now_gpu_resource == NULL) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The GPU data of Resource is empty, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		BasicDescriptorDesc new_descriptor_desc;
		new_descriptor_desc.basic_descriptor_type = PancyDescriptorType::DescriptorTypeShaderResourceView;
		new_descriptor_desc.shader_resource_view_desc = resource_view_pointer.SRV_desc[i];
		check_error = now_gpu_resource->BuildCommonDescriptorView(new_descriptor_desc, cpuHandle);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
//�����������￪��һ��bindless������ҳ
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::BuildBindlessShaderResourceViewPack(
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
	const std::vector<VirtualResourcePointer> &describe_memory_data,
	const pancy_object_id &SRV_pack_size,
	pancy_object_id &SRV_pack_id
)
{
	PancystarEngine::EngineFailReason check_error;
	//��鵱ǰ���е������������Ƿ����Կ��ٳ���ǰ�����������ҳ
	pancy_object_id now_empty_descriptor = max_descriptor_num - now_pointer_offset;
	if (now_empty_descriptor < SRV_pack_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "The Descriptor Segmental Is Not Enough to build a new descriptor pack");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build Descriptor pack from Segmental", error_message);
		return error_message;
	}
	//���ݵ�ǰ���������ε�ƫ�����������һ���µ���������
	BindlessResourceViewPointer new_resource_view_pack;
	new_resource_view_pack.resource_view_num = SRV_pack_size;
	new_resource_view_pack.resource_view_offset = now_pointer_offset;
	for (auto SRV_desc_data = SRV_desc.begin(); SRV_desc_data != SRV_desc.end(); ++SRV_desc_data)
	{
		//�������е���������ʽ
		new_resource_view_pack.SRV_desc.push_back(*SRV_desc_data);
	}
	for (auto resource_data = describe_memory_data.begin(); resource_data != describe_memory_data.end(); ++resource_data)
	{
		//�������е���Դ������ַ
		new_resource_view_pack.describe_memory_data.push_back(*resource_data);
	}
	//��ʼ����SRV
	check_error = BuildShaderResourceView(new_resource_view_pack);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�����µ�������ҳ��������������������ƫ���������ε�ָ��
	now_pointer_offset += SRV_pack_size;
	//Ϊ�µ�������������һ���µ�ID
	if (!now_descriptor_pack_id_reuse.empty())
	{
		SRV_pack_id = now_descriptor_pack_id_reuse.front();
		now_descriptor_pack_id_reuse.pop();
	}
	else
	{
		SRV_pack_id = now_descriptor_pack_id_self_add;
		now_descriptor_pack_id_self_add += 1;
	}
	//�������ɵ�������ӵ������м�¼
	descriptor_data.insert(std::pair<pancy_resource_id, BindlessResourceViewPointer>(SRV_pack_id, new_resource_view_pack));
	return PancystarEngine::succeed;
}
//������������ɾ��һ��bindless������ҳ
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::DeleteBindlessShaderResourceViewPack(const pancy_object_id &SRV_pack_id)
{
	//�ҵ���ǰ������ҳ����������
	auto descriptor_page = descriptor_data.find(SRV_pack_id);
	if (descriptor_page == descriptor_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the descriptor page ID: " + std::to_string(SRV_pack_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete descriptor page from Segmental", error_message);
		return error_message;
	}
	if (descriptor_page->second.resource_view_offset < now_pointer_refresh)
	{
		//�����ǰɾ����������ҳ�����������εĿ�ǰλ�ã�������һ��������������Ƭ��ʱ��ӵ�ǰλ�ÿ�ʼ����
		now_pointer_refresh = descriptor_page->second.resource_view_offset;
	}
	//���ٵ�ǰ������ҳ
	descriptor_data.erase(SRV_pack_id);
	return PancystarEngine::succeed;
}
//Ϊ��������ִ��һ����Ƭ�������
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::RefreshBindlessShaderResourceViewPack()
{
	PancystarEngine::EngineFailReason check_error;
	if (now_pointer_refresh >= max_descriptor_num)
	{
		//�ڴ�֮ǰ��������û�о����κ�ɾ������������Ҫ������Ƭ����
		return PancystarEngine::succeed;
	}
	now_pointer_offset = now_pointer_refresh;
	//�������������ڵ�����������ҳ������������Ƭ������
	for (auto descriptor_page_check = descriptor_data.begin(); descriptor_page_check != descriptor_data.end(); ++descriptor_page_check)
	{
		if (descriptor_page_check->second.resource_view_offset > now_pointer_refresh)
		{
			//�����������ҳ������Ҫ������λ�������һҳ���������������е���
			descriptor_page_check->second.resource_view_offset = now_pointer_offset;
			//�����µ�SRV
			check_error = BuildShaderResourceView(descriptor_page_check->second);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			//��ǵ�ǰ��ռ�õ�������λ��
			now_pointer_offset += descriptor_page_check->second.resource_view_num;
			if (now_pointer_offset > max_descriptor_num)
			{
				//���������г��ֵ����������������������ڵ���ǰ���쳣���
				PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor Segmental could not build desciptor more than : " + std::to_string(max_descriptor_num));
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh descriptor page from Segmental", error_message);
				return error_message;
			}
		}
	}
	//��ԭˢ�µ���
	now_pointer_refresh = max_descriptor_num;
	return PancystarEngine::succeed;
}
//Ϊ��������ɾ��һ��bindless������ҳ��ͬʱ����һ����������Ƭ
PancystarEngine::EngineFailReason BindlessResourceViewSegmental::DeleteBindlessShaderResourceViewPackAndRefresh(const pancy_object_id &SRV_pack_id)
{
	auto check_error = DeleteBindlessShaderResourceViewPack(SRV_pack_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = RefreshBindlessShaderResourceViewPack();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
const BindlessResourceViewPointer BindlessResourceViewSegmental::GetDescriptorPageOffset(const pancy_object_id &descriptor_page_id)
{
	auto check_descriptor_page = descriptor_data.find(descriptor_page_id);
	if (check_descriptor_page == descriptor_data.end())
	{
		BindlessResourceViewPointer error_pointer;
		error_pointer.resource_view_offset = 0;
		error_pointer.resource_view_num = 0;
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find the descriptor page ID: " + std::to_string(descriptor_page_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get descriptor page from Segmental", error_message);
		return error_pointer;
	}
	return check_descriptor_page->second;
}
//��������������Դ����������
bool BindlessDescriptorID::operator<(const BindlessDescriptorID& other)  const
{
	if (empty_resource_size != other.empty_resource_size)
	{
		return (empty_resource_size < other.empty_resource_size);
	}
	return (bindless_id < other.bindless_id);
}
//�������ѣ����ڴ��������Ϸ���������
PancyDescriptorHeap::PancyDescriptorHeap()
{


}
PancystarEngine::EngineFailReason PancyDescriptorHeap::Create(
	const D3D12_DESCRIPTOR_HEAP_DESC &descriptor_heap_desc,
	const std::string &descriptor_heap_name_in,
	const pancy_object_id &bind_descriptor_num_in,
	const pancy_object_id &bindless_descriptor_num_in,
	const pancy_object_id &per_segmental_size_in
)
{
	descriptor_desc = descriptor_heap_desc;
	descriptor_heap_name = descriptor_heap_name_in;
	bind_descriptor_num = bind_descriptor_num_in;
	bindless_descriptor_num = bindless_descriptor_num_in;
	per_segmental_size = per_segmental_size_in;
	//����ÿһ���������Ĵ�С
	per_descriptor_size = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->GetDescriptorHandleIncrementSize(descriptor_desc.Type);
	//�����������Ѹ�ʽ������������
	HRESULT hr = PancyDx12DeviceBasic::GetInstance()->GetD3dDevice()->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptor_heap_data));
	if (FAILED(hr))
	{
		PancystarEngine::EngineFailReason error_message(hr, "create descriptor heap error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap", error_message);
		return error_message;
	}
	//��ʼ�����е�ȫ��������ID
	for (int i = 0; i < bind_descriptor_num; ++i)
	{
		bind_descriptor_offset_reuse.push(i);
	}
	//Ϊbindless�����������ʼ��ÿ����������
	pancy_object_id globel_offset = bind_descriptor_num;
	pancy_object_id segmantal_id_self_add = 0;
	for (int i = 0; i < bindless_descriptor_num; i += per_segmental_size)
	{
		BindlessResourceViewSegmental *new_segmental = new BindlessResourceViewSegmental(per_segmental_size, globel_offset + i, per_descriptor_size, descriptor_heap_data);
		if (new_segmental == NULL)
		{
			//������������ʧ��
			PancystarEngine::EngineFailReason error_message(E_FAIL, "Build bindless texture segmental failed with NULL return");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build bindless texture segmental", error_message);
			return error_message;
		}
		if (new_segmental->GetEmptyDescriptorNum() != per_segmental_size)
		{
			//�����������εõ�������������������Ԥ�ڵ�����������
			PancystarEngine::EngineFailReason error_message(E_FAIL, "Build bindless texture segmental failed with Wrong dscriptor num,ask: " + std::to_string(per_segmental_size) + " but find: " + std::to_string(new_segmental->GetEmptyDescriptorNum()));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build bindless texture segmental", error_message);
			return error_message;
		}
		//���µ��������α������洢�������εı���
		BindlessDescriptorID new_descriptor_segmental_id;
		new_descriptor_segmental_id.bindless_id = segmantal_id_self_add;
		new_descriptor_segmental_id.empty_resource_size = per_segmental_size;
		bindless_descriptor_id_map.insert(std::pair<pancy_object_id, BindlessDescriptorID>(new_descriptor_segmental_id.bindless_id, new_descriptor_segmental_id));
		descriptor_segmental_map.insert(std::pair<BindlessDescriptorID, BindlessResourceViewSegmental*>(new_descriptor_segmental_id, new_segmental));
		segmantal_id_self_add += 1;
	}
	return PancystarEngine::succeed;
}
PancyDescriptorHeap::~PancyDescriptorHeap()
{
	for (auto release_data = descriptor_segmental_map.begin(); release_data != descriptor_segmental_map.end(); ++release_data)
	{
		delete release_data->second;
	}
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildBindlessShaderResourceViewPage(
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
	const std::vector<VirtualResourcePointer> &describe_memory_data,
	const pancy_object_id &SRV_pack_size,
	BindlessResourceViewID &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	if (SRV_pack_size <= 0)
	{
		//��Ҫ������SRV����С�ڵ���0
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build bindless texture with size:" + SRV_pack_size);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build SRV for bindless texture", error_message);
		return error_message;
	}
	//����ѡһ�����ʵ�bindless��������(Ҫ��ʣ���������㹻���Ҿ����������������еĿ���ֵ��С)
	BindlessResourceViewSegmental* RSV_segmental = NULL;
	BindlessDescriptorID check_min_size_id;
	check_min_size_id.bindless_id = 0;
	check_min_size_id.empty_resource_size = SRV_pack_size;
	auto min_resource_data = descriptor_segmental_map.lower_bound(check_min_size_id);
	if (min_resource_data != descriptor_segmental_map.end())
	{
		//���������д��ڷ���Ҫ����������Σ�ֱ���ڸ����������Ͽ���������ҳ
		descriptor_id.segmental_id = min_resource_data->first.bindless_id;
		RSV_segmental = min_resource_data->second;
	}
	else
	{
		//������������
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build bindless texture,the heap is full��ask number: " + std::to_string(SRV_pack_size));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build SRV for bindless texture", error_message);
		return error_message;
	}
	//�����������Ͽ���һ��������ҳ
	check_error = RSV_segmental->BuildBindlessShaderResourceViewPack(SRV_desc, describe_memory_data, SRV_pack_size, descriptor_id.page_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�޸�������ҳ�Ĵ�С(�ȴ�map��ɾ���ϵ�������ҳ���ٽ��µ�������ҳ���뵽map��,ʵ��ͨ��Mapά��������ҳ��С�Ĺ���)
	check_error = RefreshBindlessResourcesegmentalSize(min_resource_data->first.bindless_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::RefreshBindlessResourcesegmentalSize(const pancy_object_id &resourc_id)
{

	auto now_resource_data = bindless_descriptor_id_map.find(resourc_id);
	if (now_resource_data == bindless_descriptor_id_map.end())
	{
		//��Ҫˢ�µ���Դid������
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless resource segmental id" + std::to_string(resourc_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh bindless SRV size", error_message);
		return error_message;
	}
	auto RSV_segmental = descriptor_segmental_map.find(now_resource_data->second);
	if (RSV_segmental == descriptor_segmental_map.end())
	{
		//��Ҫˢ�µ���Դ������
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless resource segmental resource" + std::to_string(resourc_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh bindless SRV size", error_message);
		return error_message;
	}
	//��¼��ǰ��Ҫ�������Դ���µ�ַ��ɵ�ַ
	BindlessDescriptorID old_size_id;
	BindlessDescriptorID new_size_id;
	BindlessResourceViewSegmental *segmental_resource;
	new_size_id.bindless_id = resourc_id;
	new_size_id.empty_resource_size = RSV_segmental->second->GetEmptyDescriptorNum();
	old_size_id = now_resource_data->second;
	segmental_resource = RSV_segmental->second;
	//ˢ��ID
	bindless_descriptor_id_map.erase(old_size_id.bindless_id);
	bindless_descriptor_id_map.insert(std::pair<pancy_object_id, BindlessDescriptorID>(new_size_id.bindless_id, new_size_id));
	//ˢ������
	descriptor_segmental_map.erase(old_size_id);
	descriptor_segmental_map.insert(std::pair<BindlessDescriptorID, BindlessResourceViewSegmental*>(new_size_id, segmental_resource));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::DeleteBindlessShaderResourceViewPage(
	const BindlessResourceViewID &descriptor_id,
	bool is_refresh_segmental
)
{
	auto resource_id = bindless_descriptor_id_map.find(descriptor_id.segmental_id);
	if (resource_id == bindless_descriptor_id_map.end())
	{
		//δ�ҵ��������Դ
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete bindless SRV from segmental", error_message);
		return error_message;
	}
	auto descriptor_resource = descriptor_segmental_map.find(resource_id->second);
	if (descriptor_resource == descriptor_segmental_map.end())
	{
		//δ�ҵ��������Դ
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete bindless SRV from segmental", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error;
	if (is_refresh_segmental)
	{
		//Ҫ��ɾ��ҳ��Ϣ������ˢ�¶���Ϣ
		check_error = descriptor_resource->second->DeleteBindlessShaderResourceViewPackAndRefresh(descriptor_id.page_id);
	}
	else
	{
		//ɾ��ҳ��Ϣ������ˢ�¶���Ϣ
		check_error = descriptor_resource->second->DeleteBindlessShaderResourceViewPack(descriptor_id.page_id);
	}
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//���¼���ҳ�Ĵ�С��ˢ��ҳ��
	check_error = RefreshBindlessResourcesegmentalSize(descriptor_id.segmental_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::RefreshBindlessShaderResourceViewSegmental()
{
	PancystarEngine::EngineFailReason check_error;
	int size = bindless_descriptor_id_map.size();
	for (int i = 0; i < size; ++i)
	{
		BindlessDescriptorID new_id = bindless_descriptor_id_map[i];
		auto bindlessresource = descriptor_segmental_map.find(new_id);
		if (bindlessresource == descriptor_segmental_map.end())
		{
			//δ�ҵ��������Դ
			PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(i));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Refresh bindless SRV segmental", error_message);
			return error_message;
		}
		check_error = bindlessresource->second->RefreshBindlessShaderResourceViewPack();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = RefreshBindlessResourcesegmentalSize(i);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
//����ȫ��������
PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildGlobelDescriptor(
	const std::string &globel_name,
	const std::vector<BasicDescriptorDesc> &SRV_desc,
	const std::vector <VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer
)
{
	auto check_if_has_data = descriptor_globel_map.find(globel_name);
	//�ȼ���Ƿ��Ѿ�������ͬ����ȫ��������
	if (check_if_has_data != descriptor_globel_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "repeat build globel descriptor: " + globel_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build globel descriptor from heap", error_message);
		return error_message;
	}
	//����һ����ͨ�İ�������
	pancy_object_id bind_resource_id;
	PancystarEngine::EngineFailReason check_error;
	check_error = BuildBindDescriptor(SRV_desc, memory_data, if_build_multi_buffer, bind_resource_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//������������ȫ�ֱ������а�
	descriptor_globel_map[globel_name] = bind_resource_id;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::DeleteGlobelDescriptor(const std::string &globel_name)
{
	PancystarEngine::EngineFailReason check_error;
	auto check_if_has_data = descriptor_globel_map.find(globel_name);
	//�ȼ���Ƿ��Ѿ�������ȫ��������
	if (check_if_has_data == descriptor_globel_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find globel descriptor: " + globel_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete globel descriptor from heap", error_message);
		return error_message;
	}
	//��ɾ����������Դ
	check_error = DeleteBindDescriptor(check_if_has_data->second);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//��ɾ����������Դ����������
	descriptor_globel_map.erase(globel_name);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::GetGlobelDesciptorID(const std::string &globel_name, pancy_object_id &descriptor_id)
{
	PancystarEngine::EngineFailReason check_error;
	auto check_if_has_data = descriptor_globel_map.find(globel_name);
	//�ȼ���Ƿ��Ѿ�������ȫ��������
	if (check_if_has_data == descriptor_globel_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find globel descriptor: " + globel_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get globel descriptor from heap", error_message);
		return error_message;
	}
	descriptor_id = check_if_has_data->second;
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason PancyDescriptorHeap::BuildBindDescriptor(
	const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
	const std::vector<VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer,
	pancy_object_id &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> cpuHandle;
	CommonDescriptorPointer new_descriptor_data;
	//��⵱ǰ���������Ѹ�ʽ�Ƿ���������������һ��
	D3D12_DESCRIPTOR_HEAP_TYPE now_descriptor_heap = GetDescriptorHeapTypeOfDescriptor(now_descriptor_desc_in[0]);
	if (descriptor_desc.Type != now_descriptor_heap)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap type is not same as heap, could not build bind descriptor: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build bind descriptor from heap", error_message);
		return error_message;
	}
	//Ԥ���������������������е�λ��
	check_error = PreBuildBindDescriptor(
		now_descriptor_heap,
		if_build_multi_buffer,
		cpuHandle,
		new_descriptor_data
	);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (cpuHandle.size() != memory_data.size())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "descriptor_number: " + std::to_string(cpuHandle.size()) + "do not match resource number: " + std::to_string(memory_data.size()) + " checking if need to build multi buffer");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BuildBindDescriptor", error_message);
		return error_message;
	}
	//����������
	for (int i = 0; i < memory_data.size(); ++i)
	{
		//�洢��Դ���ݣ���������Դ���ݴ���������
		new_descriptor_data.resource_data.push_back(memory_data[i]);
		auto real_resource = memory_data[i].GetResourceData();
		ResourceBlockGpu* now_gpu_resource = NULL;
		if (real_resource->GetResourceTypeName() == typeid(PancyBasicBuffer).name())
		{
			const PancyBasicBuffer *buffer_resource_data = dynamic_cast<const PancyBasicBuffer*>(real_resource);
			now_gpu_resource = buffer_resource_data->GetGpuResourceData();
		}
		else if (real_resource->GetResourceTypeName() == typeid(PancyBasicTexture).name())
		{
			const PancyBasicTexture *texture_resource_data = dynamic_cast<const PancyBasicTexture*>(real_resource);
			now_gpu_resource = texture_resource_data->GetGpuResourceData();
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The Resource is not a buffer/texture, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		if (now_gpu_resource == NULL)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "The GPU data of Resource is empty, could not build Descriptor for it");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindlessResourceViewSegmental::BuildShaderResourceView", error_message);
			return error_message;
		}
		check_error = now_gpu_resource->BuildCommonDescriptorView(now_descriptor_desc_in[i], cpuHandle[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	//���´��������������뵽map��
	descriptor_id = new_descriptor_data.descriptor_offset[0];
	descriptor_bind_map[descriptor_id] = new_descriptor_data;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::PreBuildBindDescriptor(
	const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_type,
	const bool if_build_multi_buffer,
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> &descriptor_cpu_handle,
	CommonDescriptorPointer &new_descriptor_data
)
{
	pancy_object_id multibuffer_num = static_cast<pancy_object_id>(PancyDx12DeviceBasic::GetInstance()->GetFrameNum());
	PancystarEngine::EngineFailReason check_error;
	//����Ƿ��п����������λ��
	if (bind_descriptor_offset_reuse.size() == 0 || (if_build_multi_buffer && bind_descriptor_offset_reuse.size() < multibuffer_num))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor heap is full, could not build new bind descriptor: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build globel descriptor from heap", error_message);
		return error_message;
	}
	//�����Ҫ����������������(�������������������1��������ݵ�ǰ������Ⱦ֡����������������Դ)
	pancy_object_id build_descriptor_num = 1;
	if (if_build_multi_buffer)
	{
		build_descriptor_num = multibuffer_num;
		new_descriptor_data.if_multi_buffer = true;
	}
	//������Ҫ������������������������ÿ�������������������ڵ�����ƫ����
	descriptor_cpu_handle.clear();
	for (int i = 0; i < build_descriptor_num; ++i)
	{
		new_descriptor_data.descriptor_offset.push_back(bind_descriptor_offset_reuse.front());
		new_descriptor_data.descriptor_type = descriptor_type;
		//����������������λ��
		CD3DX12_CPU_DESCRIPTOR_HANDLE cpuHandle(descriptor_heap_data->GetCPUDescriptorHandleForHeapStart());
		pancy_resource_size real_descriptor_offset = static_cast<pancy_resource_size>(bind_descriptor_offset_reuse.front()) * static_cast<pancy_resource_size>(per_descriptor_size);
		cpuHandle.Offset(real_descriptor_offset);
		descriptor_cpu_handle.push_back(cpuHandle);
		//ɾ����ǰ���õ�һ��ID��
		bind_descriptor_offset_reuse.pop();
	}
	return PancystarEngine::succeed;
}
D3D12_DESCRIPTOR_HEAP_TYPE PancyDescriptorHeap::GetDescriptorHeapTypeOfDescriptor(const BasicDescriptorDesc &descriptor_desc)
{
	switch (descriptor_desc.basic_descriptor_type)
	{
	case PancyDescriptorType::DescriptorTypeShaderResourceView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	case PancyDescriptorType::DescriptorTypeUnorderedAccessView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	case PancyDescriptorType::DescriptorTypeConstantBufferView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		break;
	case PancyDescriptorType::DescriptorTypeRenderTargetView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		break;
	case PancyDescriptorType::DescriptorTypeDepthStencilView:
		return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		break;
	default:
		break;
	}
	return D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::DeleteBindDescriptor(const pancy_object_id &descriptor_id)
{
	auto check_if_has_data = descriptor_bind_map.find(descriptor_id);
	//�ȼ���ɾ�����������Ƿ����
	if (check_if_has_data == descriptor_bind_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find bind descriptor: " + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete bind descriptor from heap", error_message);
		return error_message;
	}
	//ɾ����ǰ������������
	descriptor_bind_map.erase(descriptor_id);
	//��ɾ����ϵ�������ID����������ID��������
	bind_descriptor_offset_reuse.push(descriptor_id);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BindGlobelDescriptor(
	const std::string &globel_name,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	//�Ȼ�ȡ�������ѵ�CPUָ��
	pancy_object_id bind_id_descriptor;
	check_error = GetGlobelDesciptorID(globel_name, bind_id_descriptor);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//��ȡȫ����������ƫ����
	check_error = BindCommonDescriptor(bind_id_descriptor, render_param_type, m_commandList, root_signature_offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BindCommonDescriptor(
	const pancy_object_id &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	//��������ID���Ƿ�Ϸ�
	if (descriptor_id >= bind_descriptor_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor id: " + std::to_string(descriptor_id) + " is bigger than the max id of descriptor heap: " + std::to_string(bind_descriptor_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindCommonDescriptor", error_message);
		return error_message;
	}
	//�Ȼ�ȡ�������ѵ�CPUָ��
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(descriptor_heap_data->GetGPUDescriptorHandleForHeapStart());
	//��ȡȫ������������Ϣ
	auto resource_id = descriptor_bind_map.find(descriptor_id);
	if (resource_id == descriptor_bind_map.end())
	{
		//δ�ҵ��������Դ
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bind descriptor :" + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BindCommonDescriptor", error_message);
		return error_message;
	}
	//�����������Ƿ��ǽ���֡�����������Ƿ���֡�����ϰ汾��rename������
	pancy_object_id real_offset_descriptor;
	if (resource_id->second.if_multi_buffer)
	{
		pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
		real_offset_descriptor = resource_id->second.descriptor_offset[now_frame_id];
	}
	else
	{
		real_offset_descriptor = resource_id->second.descriptor_offset[0];
	}
	//��ȡȫ����������ƫ����
	pancy_object_id id_offset = real_offset_descriptor * per_descriptor_size;
	srvHandle.Offset(id_offset);
	//��ʼ����������commandlist
	switch (render_param_type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		m_commandList->GetCommandList()->SetComputeRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could only bind descriptor to graph/cpmpute commondlist:" + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BindCommonDescriptor", error_message);
		return error_message;
		break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::GetCommonDescriptorCpuOffset(const pancy_object_id &descriptor_id, CD3DX12_CPU_DESCRIPTOR_HANDLE &Cpu_Handle)
{
	//��������ID���Ƿ�Ϸ�
	if (descriptor_id >= bind_descriptor_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the descriptor id: " + std::to_string(descriptor_id) + " is bigger than the max id of descriptor heap: " + std::to_string(bind_descriptor_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindCommonDescriptor", error_message);
		return error_message;
	}
	//�Ȼ�ȡ�������ѵ�CPUָ��
	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(descriptor_heap_data->GetCPUDescriptorHandleForHeapStart());
	//��ȡȫ������������Ϣ
	auto resource_id = descriptor_bind_map.find(descriptor_id);
	if (resource_id == descriptor_bind_map.end())
	{
		//δ�ҵ��������Դ
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bind descriptor :" + std::to_string(descriptor_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Bind bind descriptor from segmental", error_message);
		return error_message;
	}
	//�����������Ƿ��ǽ���֡�����������Ƿ���֡�����ϰ汾��rename������
	pancy_object_id real_offset_descriptor;
	if (resource_id->second.if_multi_buffer)
	{
		pancy_object_id now_frame_id = PancyDx12DeviceBasic::GetInstance()->GetLastFrame();
		real_offset_descriptor = resource_id->second.descriptor_offset[now_frame_id];
	}
	else
	{
		real_offset_descriptor = resource_id->second.descriptor_offset[0];
	}
	//��ȡȫ����������ƫ����
	pancy_object_id id_offset = real_offset_descriptor * per_descriptor_size;
	srvHandle.Offset(id_offset);
	Cpu_Handle = srvHandle;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeap::BindBindlessDescriptor(
	const BindlessResourceViewID &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	//�Ȼ�ȡ�������ѵ�CPUָ��
	if (descriptor_id.segmental_id >= bindless_descriptor_num)
	{
		//�������������λ�ó����˶ε�ַ�����ֵ
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the bindless descriptor segmental id: " + std::to_string(descriptor_id.segmental_id) + " is bigger than the max bindless segmental id of descriptor heap: " + std::to_string(bindless_descriptor_num));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindBindlessDescriptor", error_message);
		return error_message;
	}
	else if (descriptor_id.page_id > per_segmental_size)
	{
		//�����������ҳλ�ó�����ÿ�ε�ַ���������
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the bindless descriptor page id: " + std::to_string(descriptor_id.page_id) + " is bigger than the max bindless per_segmental_size id of descriptor heap: " + std::to_string(per_segmental_size));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("BindBindlessDescriptor", error_message);
		return error_message;
	}
	pancy_object_id bind_id_descriptor;
	//���ҵ�ǰ������ҳ��Ӧ������
	auto resource_id = bindless_descriptor_id_map.find(descriptor_id.segmental_id);
	if (resource_id == bindless_descriptor_id_map.end())
	{
		//δ�ҵ��������Դ
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Bind bindless SRV from segmental", error_message);
		return error_message;
	}
	auto descriptor_resource = descriptor_segmental_map.find(resource_id->second);
	if (descriptor_resource == descriptor_segmental_map.end())
	{
		//δ�ҵ��������Դ
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find bindless texture segmental:" + std::to_string(descriptor_id.segmental_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Bind bindless SRV from segmental", error_message);
		return error_message;
	}
	//��ȡ������ҳ����ʵƫ��
	const BindlessResourceViewPointer &descriptor_page_real_pos = descriptor_resource->second->GetDescriptorPageOffset(descriptor_id.page_id);
	//����������������ʼλ�������������е�ƫ�ƣ���ʼƫ��+���жμ��ƫ��+����ƫ�ƣ�
	bind_id_descriptor = bind_descriptor_num + descriptor_id.segmental_id * per_segmental_size + descriptor_page_real_pos.resource_view_offset;
	//�Ȼ�ȡ�������ѵ�CPUָ��
	CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle(descriptor_heap_data->GetGPUDescriptorHandleForHeapStart());
	//��ȡȫ����������ƫ����
	pancy_object_id id_offset = bind_id_descriptor * per_descriptor_size;
	srvHandle.Offset(id_offset);
	//��ʼ����������commandlist
	switch (render_param_type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		m_commandList->GetCommandList()->SetGraphicsRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		m_commandList->GetCommandList()->SetComputeRootDescriptorTable(root_signature_offset, srvHandle);
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could only bind descriptor to graph/cpmpute commondlist");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeap::BindBindlessDescriptor", error_message);
		return error_message;
		break;
	}
	return PancystarEngine::succeed;
}
//�������ѹ�����
PancyDescriptorHeapControl::PancyDescriptorHeapControl()
{
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
	JSON_REFLECT_INIT_ENUM(D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
	//����������������������
	PancystarEngine::EngineFailReason check_error;
	check_error = BuildNewDescriptorHeapFromJson("EngineResource\\BasicDescriptorHeap\\DesciptorHeapShaderResource.json", common_descriptor_heap_shader_resource);
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init Descriptor Heap Controler", check_error);
	}
	check_error = BuildNewDescriptorHeapFromJson("EngineResource\\BasicDescriptorHeap\\DesciptorHeapRenderTarget.json", common_descriptor_heap_render_target);
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init Descriptor Heap Controler", check_error);
	}
	check_error = BuildNewDescriptorHeapFromJson("EngineResource\\BasicDescriptorHeap\\DesciptorHeapDepthStencil.json", common_descriptor_heap_depth_stencil);
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Init Descriptor Heap Controler", check_error);
	}
}
PancyDescriptorHeapControl::~PancyDescriptorHeapControl()
{
	for (auto release_heap = descriptor_heap_map.begin(); release_heap != descriptor_heap_map.end(); ++release_heap)
	{
		delete release_heap->second;
	}
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildNewDescriptorHeapFromJson(const std::string &json_name, const Json::Value &root_value, pancy_resource_id &descriptor_heap_id)
{
	PancystarEngine::EngineFailReason check_error;
	PancyDescriptorHeap *descriptor_SRV = new PancyDescriptorHeap();
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc;
	pancy_json_value value_root;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "Flags", pancy_json_data_type::json_data_enum, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.Flags = static_cast<D3D12_DESCRIPTOR_HEAP_FLAGS>(value_root.int_value);
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "NodeMask", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.NodeMask = value_root.int_value;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "Type", pancy_json_data_type::json_data_enum, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	descriptor_heap_desc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(value_root.int_value);
	pancy_object_id bind_descriptor_num;
	pancy_object_id bindless_descriptor_num;
	pancy_object_id per_segmental_size;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "BindDescriptorNum", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	bind_descriptor_num = static_cast<pancy_object_id>(value_root.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "BindlessDescriptorNum", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	bindless_descriptor_num = static_cast<pancy_object_id>(value_root.int_value);

	check_error = PancyJsonTool::GetInstance()->GetJsonData(json_name, root_value, "PerSegmentalSize", pancy_json_data_type::json_data_int, value_root);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	per_segmental_size = static_cast<pancy_object_id>(value_root.int_value);
	descriptor_heap_desc.NumDescriptors = bind_descriptor_num + bindless_descriptor_num;
	check_error = descriptor_SRV->Create(descriptor_heap_desc, json_name, bind_descriptor_num, bindless_descriptor_num, per_segmental_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if (!descriptor_heap_id_reuse.empty())
	{
		descriptor_heap_id = descriptor_heap_id_reuse.front();
		descriptor_heap_id_reuse.pop();
	}
	else
	{
		descriptor_heap_id = descriptor_heap_id_self_add;
		descriptor_heap_id_self_add += 1;
	}
	descriptor_heap_map[descriptor_heap_id] = descriptor_SRV;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildNewDescriptorHeapFromJson(const std::string &json_file_name, pancy_resource_id &descriptor_heap_id)
{
	PancystarEngine::EngineFailReason check_error;
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(json_file_name, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = BuildNewDescriptorHeapFromJson(json_file_name, root_value, descriptor_heap_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::DeleteDescriptorHeap(const pancy_resource_id &descriptor_heap_id)
{
	auto descriptor_data = descriptor_heap_map.find(descriptor_heap_id);
	if (descriptor_data == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find descriptor heap: " + std::to_string(descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete descriptor heap from heap", error_message);
		return error_message;
	}
	//ɾ������������Դ������ID�Ż����������ѹ�����
	delete descriptor_heap_map[descriptor_heap_id];
	descriptor_heap_map.erase(descriptor_heap_id);
	descriptor_heap_id_reuse.push(descriptor_heap_id);
	return PancystarEngine::succeed;
}
pancy_resource_id PancyDescriptorHeapControl::GetCommonDescriptorHeapID(const BasicDescriptorDesc &descriptor_desc)
{
	return GetCommonDescriptorHeapID(descriptor_desc.basic_descriptor_type);
}
pancy_resource_id PancyDescriptorHeapControl::GetCommonDescriptorHeapID(const PancyDescriptorType &descriptor_type)
{
	switch (descriptor_type)
	{
	case PancyDescriptorType::DescriptorTypeShaderResourceView:
		return common_descriptor_heap_shader_resource;
		break;
	case PancyDescriptorType::DescriptorTypeUnorderedAccessView:
		return common_descriptor_heap_shader_resource;
		break;
	case PancyDescriptorType::DescriptorTypeConstantBufferView:
		return common_descriptor_heap_shader_resource;
		break;
	case PancyDescriptorType::DescriptorTypeRenderTargetView:
		return common_descriptor_heap_render_target;
		break;
	case PancyDescriptorType::DescriptorTypeDepthStencilView:
		return common_descriptor_heap_depth_stencil;
		break;
	default:
		break;
	}
	return common_descriptor_heap_shader_resource;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCommonDescriptor(
	const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
	const std::vector<VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer,
	BindDescriptorPointer &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(now_descriptor_desc_in[0]);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	descriptor_id.descriptor_heap_id = now_used_descriptor_heap;
	check_error = common_descriptor_heap->second->BuildBindDescriptor(now_descriptor_desc_in, memory_data, if_build_multi_buffer, descriptor_id.descriptor_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCommonBindlessShaderResourceView(
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
	const std::vector<VirtualResourcePointer> &describe_memory_data,
	const pancy_object_id &SRV_pack_size,
	BindlessDescriptorPointer &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	auto common_srv_heap = descriptor_heap_map.find(common_descriptor_heap_shader_resource);
	if (common_srv_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic SRV descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build SRV descriptor heap from heap", error_message);
		return error_message;
	}
	descriptor_id.descriptor_heap_id = common_descriptor_heap_shader_resource;
	check_error = common_srv_heap->second->BuildBindlessShaderResourceViewPage(SRV_desc, describe_memory_data, SRV_pack_size, descriptor_id.descriptor_pack_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BuildCommonGlobelDescriptor(
	const std::string &globel_srv_name,
	const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
	const std::vector<VirtualResourcePointer>& memory_data,
	const bool if_build_multi_buffer
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(now_descriptor_desc_in[0]);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BuildGlobelDescriptor(globel_srv_name, now_descriptor_desc_in, memory_data, if_build_multi_buffer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::GetCommonGlobelDescriptorID(
	PancyDescriptorType basic_descriptor_type,
	const std::string &globel_srv_name,
	BindDescriptorPointer &descriptor_id
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(basic_descriptor_type);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	descriptor_id.descriptor_heap_id = now_used_descriptor_heap;
	check_error = common_descriptor_heap->second->GetGlobelDesciptorID(globel_srv_name, descriptor_id.descriptor_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::GetBasicDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_heap_type, ID3D12DescriptorHeap **descriptor_heap_out)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id common_descriptor_heap_id = 999999999;
	switch (descriptor_heap_type)
	{
	case D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
		common_descriptor_heap_id = common_descriptor_heap_shader_resource;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
		common_descriptor_heap_id = common_descriptor_heap_render_target;
		break;
	case D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
		common_descriptor_heap_id = common_descriptor_heap_depth_stencil;
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not get common descriptor heap,type not defined");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Common Descriptor Heap", error_message);
		return error_message;
		break;
	}
	auto common_descriptor_heap = descriptor_heap_map.find(common_descriptor_heap_id);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Common Descriptor Heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->GetDescriptorHeapData(descriptor_heap_out);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindCommonGlobelDescriptor(
	PancyDescriptorType basic_descriptor_type,
	const std::string &globel_name,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id now_used_descriptor_heap = GetCommonDescriptorHeapID(basic_descriptor_type);
	auto common_descriptor_heap = descriptor_heap_map.find(now_used_descriptor_heap);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "engine haven't init Basic descriptor heap: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BindGlobelDescriptor(globel_name, render_param_type, m_commandList, root_signature_offset);
	if (check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindCommonDescriptor(
	const BindDescriptorPointer &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto common_descriptor_heap = descriptor_heap_map.find(descriptor_id.descriptor_heap_id);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find descriptor heap ID: " + std::to_string(descriptor_id.descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BindCommonDescriptor(descriptor_id.descriptor_id, render_param_type, m_commandList, root_signature_offset);
	if (check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindBindlessDescriptor(
	const BindlessDescriptorPointer &descriptor_id,
	const D3D12_COMMAND_LIST_TYPE &render_param_type,
	PancyRenderCommandList *m_commandList,
	const pancy_object_id &root_signature_offset
)
{
	PancystarEngine::EngineFailReason check_error;
	auto common_descriptor_heap = descriptor_heap_map.find(descriptor_id.descriptor_heap_id);
	if (common_descriptor_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "could not find descriptor heap ID: " + std::to_string(descriptor_id.descriptor_heap_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor heap from heap", error_message);
		return error_message;
	}
	check_error = common_descriptor_heap->second->BindBindlessDescriptor(descriptor_id.descriptor_pack_id, render_param_type, m_commandList, root_signature_offset);
	if (check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::BindCommonRenderTargetUncontiguous(
	const std::vector<pancy_object_id> rendertarget_list,
	const pancy_object_id depthstencil_descriptor,
	PancyRenderCommandList *m_commandList,
	const bool &if_have_rendertarget,
	const bool &if_have_depthstencil
)
{
	PancystarEngine::EngineFailReason check_error;
	pancy_object_id rtv_number = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE *rtvHandle;
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle;
	auto render_target_heap = descriptor_heap_map.find(common_descriptor_heap_render_target);
	auto depth_stencil_heap = descriptor_heap_map.find(common_descriptor_heap_depth_stencil);
	if (render_target_heap == descriptor_heap_map.end() || depth_stencil_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "haven't init common descriptor heap");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeapControl::BindCommonRenderTargetUncontiguous", error_message);
		return error_message;
	}
	//��ȡ������ȾĿ���ƫ����
	if (!if_have_rendertarget)
	{
		rtvHandle = NULL;
	}
	else
	{
		rtvHandle = new CD3DX12_CPU_DESCRIPTOR_HANDLE[rendertarget_list.size()];
		for (int i = 0; i < rendertarget_list.size(); ++i)
		{
			check_error = render_target_heap->second->GetCommonDescriptorCpuOffset(rendertarget_list[i], rtvHandle[i]);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		rtv_number = rendertarget_list.size();
	}
	//��ȡ��Ȼ�����ƫ����
	if (!if_have_depthstencil)
	{
		m_commandList->GetCommandList()->OMSetRenderTargets(rtv_number, rtvHandle, FALSE, NULL);
	}
	else
	{
		check_error = depth_stencil_heap->second->GetCommonDescriptorCpuOffset(depthstencil_descriptor, dsvHandle);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		m_commandList->GetCommandList()->OMSetRenderTargets(rtv_number, rtvHandle, FALSE, &dsvHandle);
	}
	delete[] rtvHandle;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyDescriptorHeapControl::GetCommonDepthStencilBufferOffset(
	const pancy_object_id depthstencil_descriptor,
	CD3DX12_CPU_DESCRIPTOR_HANDLE &dsvHandle
)
{
	PancystarEngine::EngineFailReason check_error;
	auto depth_stencil_heap = descriptor_heap_map.find(common_descriptor_heap_depth_stencil);
	if (depth_stencil_heap == descriptor_heap_map.end())
	{
		PancystarEngine::EngineFailReason error_message(S_OK, "haven't init common descriptor heap");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyDescriptorHeapControl::BindCommonRenderTargetUncontiguous", error_message);
		return error_message;
	}
	check_error = depth_stencil_heap->second->GetCommonDescriptorCpuOffset(depthstencil_descriptor, dsvHandle);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}

