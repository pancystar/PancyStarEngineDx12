#pragma once
#include"PancystarEngineBasicDx12.h"
//��Ⱦ��Դ����ģ��(��Դresource����Դ������resource control����Դ������resource view����Դ������������resource view control)
/*
�̳��ڿ�����������Դ������������غ���������Դ���أ�
���Ǳ���Ҫͨ��BuildResource������������ϵ���Դ���뵽���С�
*/
/*
64λint��Ϊid�ţ�����id����
*/
namespace PancystarEngine
{
	class reference_basic 
	{
		std::atomic<uint64_t> reference_count;
	public:
		reference_basic() 
		{
			reference_count = 0;
		}
		inline void AddReference() 
		{
			reference_count.fetch_add(1);
		}
		inline void DeleteReference() 
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
		inline uint64_t GetReference() 
		{
			return reference_count.load();
		}
	};
	class PancyBasicVirtualResource
	{
	protected:
		uint32_t resource_id;
		std::atomic<int32_t> reference_count;
	public:
		PancyBasicVirtualResource(const uint32_t &resource_id_in);
		virtual ~PancyBasicVirtualResource();
		PancystarEngine::EngineFailReason Create(std::string desc_file_in);
		void AddReference();
		void DeleteReference();
		inline int32_t GetReference()
		{
			return reference_count;
		}
	private:
		virtual PancystarEngine::EngineFailReason InitResource(std::string resource_desc_file) = 0;
	};
	template<class T>
	class PancyBasicResourceControl
	{
		std::string resource_type_name;
	private:
		//��Դ�б���Ϊ˽�У�������buildresource���з���
		uint64_t resource_id_self_add;
		std::unordered_map<uint64_t, PancyBasicVirtualResource*> basic_resource_array;
	public:
		PancyBasicResourceControl(const std::string &resource_type_name_in);
		virtual ~PancyBasicResourceControl();
		PancystarEngine::EngineFailReason AddResurceReference(const uint64_t &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const uint64_t &resource_id);
		PancystarEngine::EngineFailReason GetResource(const uint64_t &resource_id, PancyBasicVirtualResource** resource_out = NULL);
		PancystarEngine::EngineFailReason LoadResource(std::string desc_file_in, uint64_t &resource_id_out);
	};
	//������Դ������
	template<class T>
	PancyBasicResourceControl<T>::PancyBasicResourceControl(const std::string &resource_type_name_in)
	{
		resource_type_name = resource_type_name_in;
		resource_id_self_add = 0;
	}
	template<class T>
	PancyBasicResourceControl<T>::~PancyBasicResourceControl()
	{
		for (auto data_resource = basic_resource_array.begin(); data_resource != basic_resource_array.end(); ++data_resource)
		{
			delete data_resource->second;
		}
		basic_resource_array.clear();
	}
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::LoadResource(std::string desc_file_in, uint64_t &resource_id_out)
	{
		//����һ���µ���Դ
		PancyBasicVirtualResource *new_data = new T();
		PancystarEngine::EngineFailReason check_error = new_data->Create(desc_file_in);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		//���뵽��Դ�б�
		basic_resource_array.insert(std::pair<uint32_t, PancyBasicVirtualResource*>(resource_id_self_add, new_data));
		resource_id_out = resource_id_self_add;
		//����ID��+1
		resource_id_self_add += 1;
		return PancystarEngine::succeed;
	}
	/*
	PancystarEngine::EngineFailReason PancyBasicResourceControl::BuildResource(PancyBasicVirtualResource* data_input, uint64_t &resource_id_out)
	{
		//����һ���µ���Դ
		if (data_input == NULL)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not build resource with NULL data ID: " + std::to_string(resource_id_self_add), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("build resource data in resource control " + resource_type_name, error_message);
			return error_message;
		}
		//���뵽��Դ�б�
		basic_resource_array.insert(std::pair<uint32_t, PancyBasicVirtualResource*>(resource_id_self_add, data_input));
		resource_id_out = resource_id_self_add;
		//����ID��+1
		resource_id_self_add += 1;
		return PancystarEngine::succeed;
	}
	*/
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::AddResurceReference(const uint64_t &resource_id)
	{
		auto data_now = basic_resource_array.find(resource_id);
		if (data_now == basic_resource_array.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Add resource reference in resource control " + resource_type_name, error_message);
			return error_message;
		}
		data_now->second->AddReference();
		return PancystarEngine::succeed;
	}
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::DeleteResurceReference(const uint64_t &resource_id)
	{
		auto data_now = basic_resource_array.find(resource_id);
		if (data_now == basic_resource_array.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Delete resource reference in resource control " + resource_type_name, error_message);
			return error_message;
		}
		data_now->second->DeleteReference();
		//���ü���Ϊ0,ɾ������Դ
		if (data_now->second->GetReference() == 0)
		{
			delete data_now->second;
			basic_resource_array.erase(data_now);
		}
		return PancystarEngine::succeed;
	}
	template<class T>
	PancystarEngine::EngineFailReason PancyBasicResourceControl<T>::GetResource(const uint64_t &resource_id, PancyBasicVirtualResource** resource_out)
	{
		auto data_now = basic_resource_array.find(resource_id);
		if (data_now == basic_resource_array.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(resource_id), PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get resource reference in resource control " + resource_type_name, error_message);
			return error_message;
		}
		if (resource_out != NULL)
		{
			*resource_out = data_now->second;
		}
		return PancystarEngine::succeed;
	}
}