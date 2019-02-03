#include"PancyResourceBasic.h"
using namespace PancystarEngine;
//������Դ
PancyBasicVirtualResource::PancyBasicVirtualResource(const std::string &resource_name_in, Json::Value root_value_in)
{
	now_res_state = ResourceStateType::resource_state_not_init;
	resource_name = resource_name_in;
	root_value = root_value_in;
	reference_count.store(0);
}
PancyBasicVirtualResource::~PancyBasicVirtualResource()
{
}
void PancyBasicVirtualResource::AddReference()
{
	reference_count.fetch_add(1);
}
void PancyBasicVirtualResource::DeleteReference()
{
	if (reference_count > 0)
	{
		reference_count.fetch_sub(1);
	}
	else
	{
		reference_count.store(0);
	}
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::CopyCpuResourceToGpu(
	void* cpu_resource,
	const pancy_resource_size &resource_size_in, 
	const pancy_resource_size &resource_offset_in
)
{
	if (now_res_state == ResourceStateType::resource_state_not_init)
	{
		//GPU��Դδ���ػ�ȡ����ʧ�ܣ�����������
		PancystarEngine::EngineFailReason error_message(E_FAIL,"The Resource Haven't load or load failed");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update Resource"+ resource_name +" From Cpu to Gpu", error_message);
		return error_message;
	}
	else if (now_res_state == ResourceStateType::resource_state_load_GPU_memory_finish) 
	{
		//GPU��Դ�����سɹ�����״̬����ΪCPU��ɴ����������ԭ�ȵ�����
		now_res_state == ResourceStateType::resource_state_load_CPU_memory_finish;
	}
	return UpdateResourceToGPU(now_res_state, cpu_resource, resource_size_in, resource_offset_in);
}
ResourceStateType PancyBasicVirtualResource::GetResourceState()
{
	CheckIfResourceLoadToGpu(now_res_state);
	return now_res_state;
}
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create()
{
	auto check_error = InitResource(root_value, resource_name, now_res_state);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//�հ׵��麯��
PancystarEngine::EngineFailReason PancyBasicVirtualResource::UpdateResourceToGPU(
	ResourceStateType &now_res_state,
	void* resource,
	const pancy_resource_size &resource_size_in,
	const pancy_resource_size &resource_offset_in
)
{
}
//������Դ������
PancyBasicResourceControl::PancyBasicResourceControl(const std::string &resource_type_name_in)
{
	resource_type_name = resource_type_name_in;
}
PancyBasicResourceControl::~PancyBasicResourceControl()
{
	for (auto data_resource = basic_resource_array.begin(); data_resource != basic_resource_array.end(); ++data_resource)
	{
		delete data_resource->second;
	}
	basic_resource_array.clear();
	resource_name_list.clear();
	free_id_list.clear();
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::LoadResource(
	const std::string &name_resource_in, 
	const Json::Value &root_value, 
	pancy_object_id &id_need, 
	bool if_allow_repeat
)
{
	PancystarEngine::EngineFailReason check_error;
	//��Դ�����ж��ظ�
	if (!if_allow_repeat) 
	{
		auto check_data = resource_name_list.find(name_resource_in);
		if (check_data != resource_name_list.end())
		{
			id_need = check_data->second;
			PancystarEngine::EngineFailReason error_message(E_FAIL, "repeat load resource : " + name_resource_in, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Resource", error_message);
			AddResurceReference(id_need);
			return error_message;
		}
	}
	//����һ���µ���Դ
	PancyBasicVirtualResource *new_data;
	check_error = BuildResource(root_value, name_resource_in, &new_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = new_data->Create();
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	int id_now;
	//�ж��Ƿ��п��е�id���
	if (free_id_list.size() > 0)
	{
		id_now = *free_id_list.begin();
		free_id_list.erase(id_now);
	}
	else
	{
		id_now = basic_resource_array.size();
	}
	if (!if_allow_repeat)
	{
		//�������-id����������
		resource_name_list.insert(std::pair<std::string, pancy_object_id>(name_resource_in, id_now));
	}
	//���뵽��Դ�б�
	basic_resource_array.insert(std::pair<pancy_object_id, PancyBasicVirtualResource*>(id_now, new_data));
	id_need = id_now;
	AddResurceReference(id_need);
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::LoadResource(const std::string &desc_file_in, pancy_object_id &id_need, bool if_allow_repeat)
{
	PancystarEngine::EngineFailReason check_error;
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(desc_file_in, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = LoadResource(desc_file_in, root_value, id_need, if_allow_repeat);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::AddResurceReference(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add resource reference in resource control " + resource_type_name, error_message);
		return error_message;
	}
	data_now->second->AddReference();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::DeleteResurceReference(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete resource reference in resource control " + resource_type_name, error_message);
		return error_message;
	}
	data_now->second->DeleteReference();
	//���ü���Ϊ0,ɾ������Դ
	if (data_now->second->GetReferenceCount() == 0)
	{
		//ɾ����Դ��Ӧ����
		resource_name_list.erase(data_now->second->GetResourceName());
		//��ӵ�������Դ
		free_id_list.insert(data_now->first);
		//ɾ����Դ
		delete data_now->second;
		basic_resource_array.erase(data_now);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::CopyCpuResourceToGpu(
	const pancy_object_id &resource_id, 
	void* cpu_resource, 
	const pancy_resource_size &resource_size,
	const pancy_resource_size &resource_offset_in
)
{
	PancystarEngine::EngineFailReason check_error;
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Copy Cpu Resource To Gpu in resource control " + resource_type_name, error_message);
		return error_message;
	}
	check_error = data_now->second->CopyCpuResourceToGpu(cpu_resource, resource_size,resource_offset_in);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicResourceControl::GetResourceState(const pancy_object_id &resource_id, ResourceStateType &resource_state)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Resource State in resource control " + resource_type_name, error_message);
		return error_message;
	}
	resource_state = data_now->second->GetResourceState();
	return PancystarEngine::succeed;
}
PancyBasicVirtualResource* PancyBasicResourceControl::GetResource(const pancy_object_id &resource_id)
{
	auto data_now = basic_resource_array.find(resource_id);
	if (data_now == basic_resource_array.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource: " + resource_id, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get resource reference in resource control " + resource_type_name, error_message);
		return NULL;
	}
	return data_now->second;
}