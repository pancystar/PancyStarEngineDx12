#pragma once
#include"PancyMemoryBasic.h"
#include"PancyResourceJsonReflect.h"
namespace PancystarEngine
{
	//upload��������Դ��
	struct UploadResourceBlock 
	{
		//�洢���ٿռ�ǰ���ָ��λ�ã����ڲ���ÿ�ο��ٶ�����β���������Կ���ǰ��λ��+�洢���Ĵ�С����һ�����ڿ��ٺ��λ��
		pancy_resource_size pointer_before_alloc;
		pancy_resource_size pointer_after_alloc;
		ResourceBlockGpu dynamic_buffer_resource;
		ResourceBlockGpu *static_gpu_resource;
		UploadResourceBlock(
			const pancy_resource_size &pointer_before_alloc_in,
			const pancy_resource_size &pointer_after_alloc_in,
			const uint64_t &memory_size_in,
			ComPtr<ID3D12Resource> resource_data_in,
			const D3D12_HEAP_TYPE &resource_usage_in,
			const D3D12_RESOURCE_STATES &resource_state,
			ResourceBlockGpu *static_gpu_resource_input
		):dynamic_buffer_resource(
			memory_size_in,
			resource_data_in,
			resource_usage_in,
			resource_state
		)
		{
			pointer_before_alloc = pointer_before_alloc_in;
			pointer_after_alloc = pointer_after_alloc_in;
			static_gpu_resource = static_gpu_resource_input;
		}
	};
	class PancyDynamicRingBuffer 
	{
		ComPtr<ID3D12Heap> ringbuffer_heap_data;
		pancy_resource_size buffer_size;
		pancy_resource_size pointer_head_offset;
		pancy_resource_size pointer_tail_offset;
		std::queue<UploadResourceBlock*> ResourceUploadingMap;
		PancyDynamicRingBuffer();
	public:
		static PancyDynamicRingBuffer* GetInstance()
		{
			static PancyDynamicRingBuffer* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyDynamicRingBuffer();
			}
			return this_instance;
		}
		~PancyDynamicRingBuffer();
		//�������ݵ��Դ�
		PancystarEngine::EngineFailReason CopyDataToGpu(
			PancyRenderCommandList *commandlist,
			void* data_pointer,
			const pancy_resource_size &data_size,
			ResourceBlockGpu &gpu_resource_pointer
		);
		PancystarEngine::EngineFailReason CopyDataToGpu(
			PancyRenderCommandList *commandlist,
			std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
			UINT64* pRowSizesInBytes,
			UINT* pNumRows,
			const pancy_resource_size &data_size,
			ResourceBlockGpu &gpu_resource_pointer
		);
	private:
		PancystarEngine::EngineFailReason AllocNewDynamicData(
			pancy_resource_size data_size,
			ResourceBlockGpu &gpu_resource_pointer,
			UploadResourceBlock **new_block
		);
		//���س�ʼ����Ϣ
		PancystarEngine::EngineFailReason LoadInitData();
		//ˢ���ϵĻ��������ͷŲ���Ҫ����Դ��ռ�Ŀռ�
		PancystarEngine::EngineFailReason RefreshOldDynamicData();
	};
	//������Դ
	class PancyBasicVirtualResource
	{
	protected:
		bool if_could_reload;//��Դ�Ƿ������ظ�����
		PancyJsonReflect *resource_desc_value = NULL;
		std::string resource_type_name;
	protected:
		std::string resource_name;
		std::atomic<pancy_object_id> reference_count;
	public:
		PancyBasicVirtualResource(const bool &if_could_reload_in);
		virtual ~PancyBasicVirtualResource();
		//��json�ļ��м�����Դ
		PancystarEngine::EngineFailReason Create(const std::string &resource_name_in);
		//��json�ڴ��м�����Դ
		PancystarEngine::EngineFailReason Create(const std::string &resource_name_in, const Json::Value &root_value_in);
		//ֱ�Ӵӽṹ�������Դ
		PancystarEngine::EngineFailReason Create(const std::string &resource_name_in, void *resource_data, const std::string &resource_type, const pancy_resource_size &resource_size);
		inline const std::string &GetResourceTypeName() const
		{
			return resource_type_name;
		}
		//�޸����ü���
		void AddReference();
		void DeleteReference();
		inline int32_t GetReferenceCount()
		{
			return reference_count;
		}
		inline std::string GetResourceName()
		{
			return resource_name;
		}
		//��⵱ǰ����Դ�Ƿ��Ѿ�������GPU
		virtual bool CheckIfResourceLoadFinish() = 0;
	private:
		virtual void BuildJsonReflect(PancyJsonReflect **pointer_data) = 0;
		//ע�Ტ������Դ
		virtual PancystarEngine::EngineFailReason InitResource() = 0;
		//ֱ�Ӵ��ļ��м�����Դ����json�ļ���
		virtual PancystarEngine::EngineFailReason LoadResourceDirect(const std::string &file_name);
	};
	//������Դ��ģ������ָ��
	class VirtualResourcePointer
	{
		bool if_NULL;
		pancy_object_id resource_id;
		PancyBasicVirtualResource *data_pointer = NULL;
	public:
		VirtualResourcePointer();
		VirtualResourcePointer(const pancy_object_id &resource_id_in);
		VirtualResourcePointer(const VirtualResourcePointer & copy_data);
		~VirtualResourcePointer();
		VirtualResourcePointer& operator=(const VirtualResourcePointer& b);
		PancystarEngine::EngineFailReason MakeShared(const pancy_object_id &resource_id_in);
		inline pancy_object_id GetResourceId() const
		{
			return resource_id;
		}
		inline const PancyBasicVirtualResource *GetResourceData() const
		{
			if (if_NULL)
			{
				return NULL;
			}
			else
			{
				return data_pointer;
			}
		}
	};
	class PancyGlobelResourceControl
	{
		std::unordered_map<pancy_object_id, PancyBasicVirtualResource*> basic_resource_array;
	private:
		std::unordered_map<std::string, pancy_object_id> resource_name_list;
		std::unordered_set<pancy_object_id> free_id_list;
		PancyGlobelResourceControl();
	public:
		static PancyGlobelResourceControl* GetInstance()
		{
			static PancyGlobelResourceControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyGlobelResourceControl();
			}
			return this_instance;
		}
		virtual ~PancyGlobelResourceControl();
		PancystarEngine::EngineFailReason GetResourceById(const pancy_object_id &resource_id, PancyBasicVirtualResource **data_pointer);

		PancystarEngine::EngineFailReason AddResurceReference(const pancy_object_id &resource_id);
		PancystarEngine::EngineFailReason DeleteResurceReference(const pancy_object_id &resource_id);

		template<typename ResourceType>
		PancystarEngine::EngineFailReason LoadResource(
			const std::string &desc_file_in,
			VirtualResourcePointer &id_need,
			bool if_allow_repeat = false
		);
		template<class ResourceType>
		PancystarEngine::EngineFailReason LoadResource(
			const std::string &name_resource_in,
			const Json::Value &root_value,
			VirtualResourcePointer &id_need,
			bool if_allow_repeat = false
		);
		template<class ResourceType>
		PancystarEngine::EngineFailReason LoadResource(
			const std::string &name_resource_in,
			void *resource_data, 
			const std::string &resource_type,
			const pancy_resource_size &resource_size,
			VirtualResourcePointer &id_need,
			bool if_allow_repeat = false
		);
	private:
		PancystarEngine::EngineFailReason AddResourceToControl(
			const std::string &name_resource_in,
			PancyBasicVirtualResource *new_data,
			VirtualResourcePointer &res_pointer,
			const bool &if_allow_repeat
		);
	};
	template<class ResourceType>
	PancystarEngine::EngineFailReason PancyGlobelResourceControl::LoadResource(
		const std::string &name_resource_in,
		const Json::Value &root_value,
		VirtualResourcePointer &res_pointer,
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
				PancystarEngine::EngineFailReason error_message(E_FAIL, "repeat load resource : " + name_resource_in, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Resource", error_message);
				return error_message;
			}
		}
		//����һ���µ���Դ
		PancyBasicVirtualResource *new_data = new ResourceType(if_allow_repeat);
		check_error = new_data->Create(name_resource_in, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = AddResourceToControl(name_resource_in, new_data, res_pointer, if_allow_repeat);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<typename ResourceType>
	PancystarEngine::EngineFailReason PancyGlobelResourceControl::LoadResource(
		const std::string &desc_file_in,
		VirtualResourcePointer &res_pointer,
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
				PancystarEngine::EngineFailReason error_message(E_FAIL, "repeat load resource : " + name_resource_in, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Resource", error_message);
				return error_message;
			}
		}
		//����һ���µ���Դ
		PancyBasicVirtualResource *new_data = new ResourceType(if_allow_repeat);
		check_error = new_data->Create(desc_file_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = AddResourceToControl(name_resource_in, new_data, res_pointer, if_allow_repeat);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<class ResourceType>
	PancystarEngine::EngineFailReason LoadResource(
		const std::string &name_resource_in,
		void *resource_data,
		const std::string &resource_type,
		const pancy_resource_size &resource_size,
		VirtualResourcePointer &id_need,
		bool if_allow_repeat = false
	) 
	{
		PancystarEngine::EngineFailReason check_error;
		//��Դ�����ж��ظ�
		if (!if_allow_repeat)
		{
			auto check_data = resource_name_list.find(name_resource_in);
			if (check_data != resource_name_list.end())
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "repeat load resource : " + name_resource_in, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Resource", error_message);
				return error_message;
			}
		}
		//����һ���µ���Դ
		PancyBasicVirtualResource *new_data = new ResourceType(if_allow_repeat);
		check_error = new_data->Create(desc_file_in, resource_data, resource_type, resource_size);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = AddResourceToControl(name_resource_in, new_data, res_pointer, if_allow_repeat);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
};

