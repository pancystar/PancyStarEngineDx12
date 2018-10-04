#include"PancyResourceBasic.h"
using namespace PancystarEngine;
//������Դ
PancyBasicVirtualResource::PancyBasicVirtualResource(std::string desc_file_in)
{
	resource_name = desc_file_in;
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
PancystarEngine::EngineFailReason PancyBasicVirtualResource::Create()
{
	auto check_error = InitResource(resource_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
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
PancystarEngine::EngineFailReason PancyBasicResourceControl::LoadResource(std::string desc_file_in)
{
	//��Դ�����ж��ظ�
	auto check_data = resource_name_list.find(desc_file_in);
	if (check_data != resource_name_list.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"repeat load resource : " + desc_file_in,PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Resource", error_message);
		return error_message;
	}
	//����һ���µ���Դ
	PancyBasicVirtualResource *new_data;
	auto check_error = BuildResource(desc_file_in,&new_data);
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
	}
	else 
	{
		id_now = basic_resource_array.size();
	}
	//�������-id����������
	resource_name_list.insert(std::pair<std::string, pancy_object_id>(desc_file_in, id_now));
	//���뵽��Դ�б�
	basic_resource_array.insert(std::pair<pancy_object_id, PancyBasicVirtualResource*>(id_now, new_data));
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