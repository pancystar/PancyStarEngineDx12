#pragma once
#include"PancystarEngineBasicDx12.h"
//��Ⱦ��Դ����ģ��(��Դresource����Դ������resource control����Դ������resource view����Դ������������resource view control)
/*
�̳��ڿ�����������Դ������������غ���������Դ���أ�
���Ǳ���Ҫͨ��BuildResource������������ϵ���Դ���뵽���С�
*/
namespace PancystarEngine
{
	class PancyBasicVirtualResource
	{
	protected:
		uint32_t resource_id;
		std::atomic<int32_t> reference_count;
	public:
		PancyBasicVirtualResource(const uint32_t &resource_id_in);
		PancystarEngine::EngineFailReason Create();
		void AddReference();
		void DeleteReference();
		inline int32_t GetReference()
		{
			return reference_count;
		}
	protected:
		virtual PancystarEngine::EngineFailReason InitResource() = 0;
	};
	class PancyBasicResourceControl
	{
		std::string resource_type_name;
	private:
		//��Դ�б���Ϊ˽�У�������buildresource���з���
		uint32_t resource_id_self_add;
		std::unordered_map<uint32_t, PancyBasicVirtualResource*> basic_resource_array;
	public:
		PancyBasicResourceControl(const std::string &resource_type_name_in);
		PancystarEngine::EngineFailReason AddResurceReference(const uint32_t &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const uint32_t &resource_id);
		PancystarEngine::EngineFailReason GetResource(const uint32_t &resource_id, PancyBasicVirtualResource** resource_out = NULL);
	protected:
		PancystarEngine::EngineFailReason BuildResource(PancyBasicVirtualResource* data_input, uint32_t &resource_id_out);
	};
	class PancyBasicResourceView
	{
	protected:
		uint32_t resource_used_id;
		uint32_t resource_view_id;
	private:
		bool if_build;
		PancyBasicResourceControl *resource_control_use;
	public:
		PancyBasicResourceView();
		PancystarEngine::EngineFailReason Create
		(
			const uint32_t &resource_id_in,
			const uint32_t &resource_view_id_in,
			PancyBasicResourceControl *resource_control_use_in
		);
		virtual ~PancyBasicResourceView();
	protected:
		virtual PancystarEngine::EngineFailReason InitResourceView() = 0;
	};
	class PancyBasicResourceViewControl
	{
		std::string resource_type_name;
	private:
		//��Դ�б���Ϊ˽�У�������buildresource���з���
		uint32_t resource_view_id_self_add;
		std::unordered_map<uint32_t, PancyBasicResourceView*> resource_view_array;
		PancyBasicResourceControl *resource_control_use;
	public:
		PancyBasicResourceViewControl(const std::string &resource_type_name_in, PancyBasicResourceControl *resource_control_use);
		PancystarEngine::EngineFailReason DeleteResourceView(const uint32_t &resource_view_id);
	protected:
		PancystarEngine::EngineFailReason BuildResourceView(PancyBasicResourceView* data_input, uint32_t &resource_view_id_out);
	};
}