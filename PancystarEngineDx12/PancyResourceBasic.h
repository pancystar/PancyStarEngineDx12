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
		) :dynamic_buffer_resource(
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
			const pancy_resource_size &data_size_in,
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
		//ע�Ტ������Դ
		virtual PancystarEngine::EngineFailReason InitResourceJson(const std::string &resource_name_in, const Json::Value &root_value_in) = 0;
		//���ڴ��м�����Դ
		virtual PancystarEngine::EngineFailReason InitResourceMemory(void *resource_data, const std::string &resource_type, const pancy_resource_size &resource_size) = 0;
		//ֱ�Ӵ��ļ��м�����Դ����json�ļ���
		virtual PancystarEngine::EngineFailReason InitResourceDirect(const std::string &file_name) = 0;
	};
	template<typename ResourceDescStruct>
	class PancyCommonVirtualResource : public PancyBasicVirtualResource
	{
		ResourceDescStruct resource_desc;
	public:
		PancyCommonVirtualResource(const bool &if_could_reload_in);
		inline ResourceDescStruct& GetResourceDesc() 
		{
			return resource_desc;
		};
		virtual ~PancyCommonVirtualResource();
	private:
		//��json���м�����Դ
		PancystarEngine::EngineFailReason InitResourceJson(const std::string &resource_name_in, const Json::Value &root_value_in) override;
		//���ڴ��м�����Դ
		PancystarEngine::EngineFailReason InitResourceMemory(void *resource_data, const std::string &resource_type, const pancy_resource_size &resource_size) override;
		//ֱ�Ӵ��ļ��м�����Դ����json�ļ���
		PancystarEngine::EngineFailReason InitResourceDirect(const std::string &file_name) override;
		//������Դ��ʽ������Դ����
		virtual PancystarEngine::EngineFailReason LoadResoureDataByDesc(const ResourceDescStruct &ResourceDescStruct) = 0;
		//���������ļ���ȡ��Դ������ȡ��Դ��ʽ
		virtual PancystarEngine::EngineFailReason LoadResoureDataByOtherFile(const std::string &file_name, ResourceDescStruct &resource_desc);
	};
	template<typename ResourceDescStruct>
	PancyCommonVirtualResource<ResourceDescStruct>::PancyCommonVirtualResource(const bool &if_could_reload_in) : PancyBasicVirtualResource(if_could_reload_in)
	{
	}
	template<typename ResourceDescStruct>
	PancyCommonVirtualResource<ResourceDescStruct>::~PancyCommonVirtualResource()
	{
	}
	template<typename ResourceDescStruct>
	PancystarEngine::EngineFailReason PancyCommonVirtualResource<ResourceDescStruct>::InitResourceJson(const std::string &resource_name_in, const Json::Value &root_value_in)
	{
		PancystarEngine::EngineFailReason check_error;
		auto reflect_class = PancyJsonReflectControl::GetInstance()->GetJsonReflect(typeid(ResourceDescStruct).name());
		if (reflect_class == NULL)
		{
			PancystarEngine::EngineFailReason error_message(0, std::string("class: ") + typeid(ResourceDescStruct).name() + " haven't init to reflect class");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyCommonVirtualResource::InitResourceJson", error_message);
			return error_message;
		}
		check_error = reflect_class->LoadFromJsonMemory(resource_name_in, root_value_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = reflect_class->CopyMemberData(&resource_desc, typeid(&resource_desc).name(), sizeof(resource_desc));
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = LoadResoureDataByDesc(resource_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<typename ResourceDescStruct>
	PancystarEngine::EngineFailReason PancyCommonVirtualResource<ResourceDescStruct>::InitResourceMemory(void *resource_data, const std::string &resource_type, const pancy_resource_size &resource_size)
	{
		//�����������ͼ�飬���ɹ��󿽱�����
		if ((typeid(ResourceDescStruct*).name() != resource_type) || (resource_size != sizeof(ResourceDescStruct)))
		{
			PancystarEngine::EngineFailReason error_message(0, "class type dismatch: " + resource_type + " haven't init to reflect class");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyCommonVirtualResource::InitResourceMemory", error_message);
			return error_message;
		}
		resource_desc = *reinterpret_cast<ResourceDescStruct*>(resource_data);
		auto check_error = LoadResoureDataByDesc(resource_desc);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<typename ResourceDescStruct>
	PancystarEngine::EngineFailReason PancyCommonVirtualResource<ResourceDescStruct>::InitResourceDirect(const std::string &file_name) 
	{
		auto check_error = LoadResoureDataByOtherFile(file_name, resource_desc);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<typename ResourceDescStruct>
	PancystarEngine::EngineFailReason PancyCommonVirtualResource<ResourceDescStruct>::LoadResoureDataByOtherFile(const std::string &file_name, ResourceDescStruct &resource_desc)
	{
		//Ĭ������£��������κη�json�ļ��ļ���
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse file: " + file_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicVirtualResource::LoadResourceDirect", error_message);
		return error_message;
	}
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
		inline PancyBasicVirtualResource *GetResourceData()
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
	//todo��weak_ptr������ʱʹ����Դ�����
	class VirtualWeakResourcePointer 
	{
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
				res_pointer.MakeShared(check_data->second);
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
			auto check_data = resource_name_list.find(desc_file_in);
			if (check_data != resource_name_list.end())
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "repeat load resource : " + desc_file_in, PancystarEngine::LogMessageType::LOG_MESSAGE_WARNING);
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
		check_error = AddResourceToControl(desc_file_in, new_data, res_pointer, if_allow_repeat);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<class ResourceType>
	PancystarEngine::EngineFailReason PancyGlobelResourceControl::LoadResource(
		const std::string &name_resource_in,
		void *resource_data,
		const std::string &resource_type,
		const pancy_resource_size &resource_size,
		VirtualResourcePointer &id_need,
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
		check_error = new_data->Create(name_resource_in, resource_data, resource_type, resource_size);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = AddResourceToControl(name_resource_in, new_data, id_need, if_allow_repeat);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
};

