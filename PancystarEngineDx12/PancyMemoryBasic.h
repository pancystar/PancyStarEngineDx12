#pragma once
#include"PancyDx12Basic.h"
#include"PancyThreadBasic.h"
#include<DirectXTex.h>
#include<LoaderHelpers.h>
#include<DDSTextureLoader.h>
#include<WICTextureLoader.h>
#define MaxHeapDivide 32
//�Դ�ָ��
struct VirtualMemoryPointer
{
	bool if_heap;//��Դ�Ƿ����ɶ��Ϸ������
	pancy_resource_id heap_type;      //������
	pancy_resource_id heap_list_id;   //��id��
	pancy_resource_id memory_block_id;//�ڴ��id��
	pancy_object_id memory_resource_id;//ֱ��ָ���ڴ��id��
	VirtualMemoryPointer()
	{
		if_heap = false;
		heap_type = 0;
		heap_list_id = 0;
		memory_block_id = 0;
		memory_resource_id = 0;
	}
};
//�Դ��
class MemoryBlockGpu
{
	pancy_resource_size memory_size;//�洢��Ĵ�С
	ComPtr<ID3D12Resource> resource_data;//�洢�������
	D3D12_HEAP_TYPE resource_usage;
	UINT8* map_pointer;
	D3D12_RESOURCE_STATES now_subresource_state;//��ǰ��Դ��ʹ�ø�ʽ
public:
	MemoryBlockGpu(
		const uint64_t &memory_size_in,
		ComPtr<ID3D12Resource> resource_data_in,
		const D3D12_HEAP_TYPE &resource_usage_in,
		const D3D12_RESOURCE_STATES &resource_state
	);
	~MemoryBlockGpu();
	inline ComPtr<ID3D12Resource> GetResource()
	{
		return resource_data;
	}
	inline uint64_t GetSize()
	{
		return memory_size;
	}
	//�鿴��ǰ��Դ��ʹ�ø�ʽ
	inline D3D12_RESOURCE_STATES GetResourceState()
	{
		return now_subresource_state;
	}
	//�޸ĵ�ǰ��Դ��ʹ�ø�ʽ
	inline void SetResourceState(const D3D12_RESOURCE_STATES &state)
	{
		now_subresource_state = state;
	}
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(const pancy_resource_size &pointer_offset, const void* copy_data, const pancy_resource_size data_size);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const pancy_resource_size &pointer_offset,
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason GetCpuMapPointer(UINT8** map_pointer_out)
	{
		if (resource_usage != D3D12_HEAP_TYPE::D3D12_HEAP_TYPE_UPLOAD)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "resource type is not upload, could not copy data to memory");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get CPU Pointer of memory block gpu", error_message);
			map_pointer_out = NULL;
			return error_message;
		}
		*map_pointer_out = map_pointer;
		return PancystarEngine::succeed;
	}
	PancystarEngine::EngineFailReason ReadFromBufferToCpu(const pancy_resource_size &pointer_offset, void* copy_data, const pancy_resource_size data_size);
};
//�����Դ��
class MemoryHeapGpu
{
	std::string heap_type_name;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	ComPtr<ID3D12Heap> heap_data;
	//std::unordered_set<pancy_resource_id> free_list;

	//�����Դ������
	std::unordered_map<pancy_resource_id, MemoryBlockGpu*> memory_heap_block;
	//�ճ����Դ�
	std::unordered_set<pancy_resource_id> empty_memory_block;
public:
	MemoryHeapGpu(const std::string &heap_type_name_in);
	//ÿ���Դ��Ĵ�С
	inline pancy_resource_size GetMemorySizePerBlock()
	{
		return size_per_block;
	}
	//�Դ�ѵ����Դ������
	inline pancy_resource_id GetMaxMemoryBlockNum()
	{
		return max_block_num;
	}
	//�Դ����δʹ�õ��Դ��С
	inline size_t GetFreeMemoryBlockNum()
	{
		return empty_memory_block.size();
	}
	PancystarEngine::EngineFailReason Create(const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//���Դ�ѿ�����Դ
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID
	);
	//��ȡ�Դ���Դ
	MemoryBlockGpu* GetMemoryResource(const pancy_resource_id &memory_block_ID);
	//�����Ӧid����Դ�Ƿ��Ѿ�������
	bool CheckIfFree(pancy_resource_id memory_block_ID);
	//�ͷ�һ����Ӧid����Դ
	PancystarEngine::EngineFailReason FreeMemoryReference(const pancy_resource_id &memory_block_ID);
	~MemoryHeapGpu();
};
//�����������Դ��
class MemoryHeapLinear
{
	//�Դ�ѵĸ�ʽ
	CD3DX12_HEAP_DESC heap_desc;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	//�Դ�ѵ�����
	std::string heap_type_name;

	//�����Դ�ѵ�����
	std::unordered_map<pancy_resource_id, MemoryHeapGpu*> memory_heap_data;
	//�ճ����Դ��
	std::unordered_set<pancy_resource_id> empty_memory_heap;
public:
	MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const pancy_resource_size &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//���Դ�ѿ�����Դ
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID,//�Դ���ַָ��
		pancy_resource_id &memory_heap_ID//�Դ�ε�ַָ��
	);
	MemoryBlockGpu* GetMemoryResource(
		const pancy_resource_id &memory_heap_ID,//�Դ�ε�ַָ��
		const pancy_resource_id &memory_block_ID//�Դ���ַָ��

	);
	//�ͷ�һ����Ӧid����Դ
	PancystarEngine::EngineFailReason FreeMemoryReference(
		const pancy_resource_id &memory_heap_ID,
		const pancy_resource_id &memory_block_ID
	);
	//��ȡ��ǰ�����Ϳ����Ķ�����
	inline pancy_object_id GetHeapNum()
	{
		return memory_heap_data.size();
	}
	//��ȡ��ǰ�����͵�ÿ���ѵĴ�С
	inline pancy_resource_size GetPerHeapSize()
	{
		return heap_desc.SizeInBytes;
	}
	~MemoryHeapLinear();
};
//��Դ������
class MemoryHeapGpuControl
{
	pancy_object_id resource_memory_id_self_add;
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, MemoryHeapLinear*> resource_heap_list;//�Դ�ѱ�
	std::unordered_map<pancy_object_id, MemoryBlockGpu*>resource_memory_list;//��ɢ���Դ���
	std::unordered_set<pancy_object_id> resource_memory_free_id;
	MemoryHeapGpuControl();
public:
	static MemoryHeapGpuControl* GetInstance()
	{
		static MemoryHeapGpuControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new MemoryHeapGpuControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildResourceCommit(
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		const CD3DX12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		VirtualMemoryPointer &virtual_pointer
	);
	PancystarEngine::EngineFailReason BuildResourceFromHeap(
		const std::string &HeapFileName,
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		VirtualMemoryPointer &virtual_pointer
	);
	MemoryBlockGpu *GetMemoryResource(const VirtualMemoryPointer &virtual_pointer);
	PancystarEngine::EngineFailReason FreeResource(const VirtualMemoryPointer &virtual_pointer);
	void GetHeapDesc(const pancy_resource_id &heap_id, pancy_object_id &heap_num, pancy_resource_size &per_heap_size);
	void GetHeapDesc(const std::string &heap_name, pancy_object_id &heap_num, pancy_resource_size &per_heap_size);
	~MemoryHeapGpuControl();
private:
	//�������ָ�����ϵ���Դ
	MemoryBlockGpu* GetMemoryFromList(const pancy_object_id &memory_block_ID);
	PancystarEngine::EngineFailReason FreeResourceCommit(const pancy_object_id &memory_block_ID);
	//�����ָ�����ϵ���Դ
	MemoryBlockGpu * GetMemoryResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//�Դ����ַָ��
		const pancy_resource_id &memory_heap_ID,//�Դ�ε�ַָ��
		const pancy_resource_id &memory_block_ID//�Դ���ַָ��
	);

	PancystarEngine::EngineFailReason LoadHeapFromFile(
		const std::string &HeapFileName,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
	PancystarEngine::EngineFailReason FreeResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//�Դ����ַָ��
		const pancy_resource_id &memory_heap_ID,//�Դ�ε�ַָ��
		const pancy_resource_id &memory_block_ID//�Դ���ַָ��
	);
	PancystarEngine::EngineFailReason BuildHeap(
		const std::string &HeapFileName,
		const pancy_resource_size &heap_size,
		const pancy_resource_size &per_block_size,
		const D3D12_HEAP_TYPE &heap_type_in,
		const D3D12_HEAP_FLAGS &heap_flag_in,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
};
//������Դ
struct SubMemoryPointer
{
	pancy_resource_id type_id;
	pancy_object_id list_id;
	pancy_object_id offset;
};
class SubMemoryData
{
	VirtualMemoryPointer buffer_data;//�Դ���Դָ��
	pancy_resource_size per_memory_size;//ÿ���������Ĵ�С
	std::unordered_set<pancy_object_id> empty_sub_memory;
	std::unordered_set<pancy_object_id> sub_memory_data;
public:
	SubMemoryData();
	PancystarEngine::EngineFailReason Create(
		const std::string &buffer_desc_file,
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		const pancy_object_id &per_memory_size_in
	);
	PancystarEngine::EngineFailReason BuildSubMemory(pancy_object_id &offset);
	PancystarEngine::EngineFailReason FreeSubMemory(const pancy_object_id &offset);
	//�鿴��ǰ������Դ�Ĵ�С
	inline pancy_object_id GetEmptySize()
	{
		return static_cast<pancy_object_id>(empty_sub_memory.size());
	}

	//��ȡ��Դ
	inline MemoryBlockGpu* GetResource()
	{
		return MemoryHeapGpuControl::GetInstance()->GetMemoryResource(buffer_data);
	}
	void GetLogMessage(std::vector<std::string> &log_message);
	void GetLogMessage(Json::Value &root_value, bool &if_empty);
	inline pancy_resource_size GetBlockSize()
	{
		return static_cast<pancy_resource_size>(per_memory_size);
	}
};
class SubresourceLiner
{
	std::string heap_name;
	std::string hash_name;
	D3D12_RESOURCE_DESC resource_desc;
	D3D12_RESOURCE_STATES resource_state;
	pancy_object_id per_memory_size;
	std::unordered_map<pancy_object_id, SubMemoryData*> submemory_list;
	//�ճ����Դ�
	std::unordered_set<pancy_object_id> empty_memory_heap;
	//��ǰ����id���Լ��Ѿ��ͷŵ�id��
	pancy_object_id max_id;
	std::unordered_set<pancy_object_id> free_id;
public:
	SubresourceLiner(
		const std::string &heap_name_in,
		const std::string &hash_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in
	);
	~SubresourceLiner();
	inline std::string GetHeapName()
	{
		return heap_name;
	}
	inline std::string GetResourceName()
	{
		return hash_name;
	}
	void GetLogMessage(std::vector<std::string> &log_message);
	void GetLogMessage(Json::Value &root_value);
	PancystarEngine::EngineFailReason BuildSubresource(
		pancy_object_id &new_memory_block_id,
		pancy_object_id &sub_memory_offset
	);
	PancystarEngine::EngineFailReason ReleaseSubResource(
		const pancy_object_id &new_memory_block_id,
		const pancy_object_id &sub_memory_offset
	);
	MemoryBlockGpu* GetSubResource(pancy_object_id sub_memory_id, pancy_resource_size &per_memory_size);
};
class SubresourceControl
{
	pancy_object_id subresource_id_self_add;
	std::unordered_map<pancy_resource_id, SubresourceLiner*> subresource_list_map;
	std::unordered_map<std::string, pancy_resource_id> subresource_init_list;
	std::unordered_set<pancy_object_id> subresource_free_id;
private:
	SubresourceControl();
public:
	~SubresourceControl();
	static SubresourceControl* GetInstance()
	{
		static SubresourceControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new SubresourceControl();
		}
		return this_instance;
	}
	PancystarEngine::EngineFailReason BuildSubresourceFromFile(
		const std::string &resource_name_in,
		SubMemoryPointer &submemory_pointer
	);
	PancystarEngine::EngineFailReason FreeSubResource(const SubMemoryPointer &submemory_pointer);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const SubMemoryPointer &submemory_pointer,
		const pancy_resource_size &pointer_offset,
		const void* copy_data,
		const pancy_resource_size data_size
	);
	PancystarEngine::EngineFailReason GetBufferCpuPointer(
		const SubMemoryPointer &submemory_pointer,
		UINT8** map_pointer_out
	);
	PancystarEngine::EngineFailReason WriteFromCpuToBuffer(
		const SubMemoryPointer &submemory_pointer,
		const pancy_resource_size &pointer_offset,
		std::vector<D3D12_SUBRESOURCE_DATA> &subresources,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		UINT64* pRowSizesInBytes,
		UINT* pNumRows
	);
	PancystarEngine::EngineFailReason CopyResource(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const SubMemoryPointer &dst_submemory,
		const pancy_resource_size &src_offset,
		const pancy_resource_size &dst_offset,
		const pancy_resource_size &data_size
	);
	PancystarEngine::EngineFailReason CopyResource(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const SubMemoryPointer &dst_submemory,
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pLayouts,
		const pancy_object_id &Layout_num
	);
	PancystarEngine::EngineFailReason ResourceBarrier(
		PancyRenderCommandList *commandlist,
		const SubMemoryPointer &src_submemory,
		const D3D12_RESOURCE_STATES &last_state,
		const D3D12_RESOURCE_STATES &now_state
	);
	PancystarEngine::EngineFailReason GetResourceState(const SubMemoryPointer &src_submemory, D3D12_RESOURCE_STATES &res_state);
	PancystarEngine::EngineFailReason CaptureTextureDataToWindows(
		const SubMemoryPointer &tex_data,
		const bool &if_cube_map,
		DirectX::ScratchImage *new_image
	);
	PancystarEngine::EngineFailReason GetSubResourceDesc(
		const SubMemoryPointer & tex_data,
		D3D12_RESOURCE_DESC &resource_desc
	);
	PancystarEngine::EngineFailReason BuildConstantBufferView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor
	);
	PancystarEngine::EngineFailReason BuildShaderResourceView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_SHADER_RESOURCE_VIEW_DESC  &SRV_desc
	);
	PancystarEngine::EngineFailReason BuildRenderTargetView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_RENDER_TARGET_VIEW_DESC  &RTV_desc
	);
	PancystarEngine::EngineFailReason BuildUnorderedAccessView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_UNORDERED_ACCESS_VIEW_DESC  &UAV_desc
	);
	PancystarEngine::EngineFailReason BuildDepthStencilView(
		const SubMemoryPointer &src_submemory,
		const D3D12_CPU_DESCRIPTOR_HANDLE &DestDescriptor,
		const D3D12_DEPTH_STENCIL_VIEW_DESC  &DSV_desc
	);
	PancystarEngine::EngineFailReason BuildVertexBufferView(
		const SubMemoryPointer &src_submemory,
		UINT StrideInBytes,
		D3D12_VERTEX_BUFFER_VIEW &VBV_out
	);
	PancystarEngine::EngineFailReason BuildIndexBufferView(
		const SubMemoryPointer &src_submemory,
		DXGI_FORMAT StrideInBytes,
		D3D12_INDEX_BUFFER_VIEW &IBV_out
	);
	void WriteSubMemoryMessageToFile(const std::string &log_file_name);
	//void WriteSubMemoryMessageToFile(Json::Value &root_value);
private:
	MemoryBlockGpu*  GetResourceData(const SubMemoryPointer &submemory_pointer, pancy_resource_size &per_memory_size);
	void InitSubResourceType(
		const std::string &hash_name,
		const std::string &heap_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		pancy_resource_id &subresource_type_id
	);
	PancystarEngine::EngineFailReason BuildSubresource(
		const std::string &hash_name,
		const std::string &heap_name_in,
		const D3D12_RESOURCE_DESC &resource_desc_in,
		const D3D12_RESOURCE_STATES &resource_state_in,
		const pancy_object_id &per_memory_size_in,
		SubMemoryPointer &submemory_pointer
	);
};

//��ͨ�İ�������
struct CommonDescriptorPointer
{
	//�Ƿ�ʹ��˫����
	bool if_multi_buffer;
	//������ҳ����ʼ��ַ
	std::vector<pancy_object_id> descriptor_offset;
	//����������
	D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type;
	CommonDescriptorPointer()
	{
		//��ʼ����ͨ������ָ��
		if_multi_buffer = false;
		descriptor_type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	}
};
//��󶨵�������ҳ
struct BindlessResourceViewPointer
{
	//������ҳ����ʼ��ַ
	pancy_object_id resource_view_offset;
	//������ҳ������������������
	pancy_object_id resource_view_num;
	//ÿһ���������ĸ�ʽ����
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> SRV_desc;
	//ÿһ����������ָ�����Դ
	std::vector<SubMemoryPointer> describe_memory_data;
	BindlessResourceViewPointer()
	{
		resource_view_offset = 0;
		resource_view_num = 0;
	}
};
//��������
class BindlessResourceViewSegmental
{
	ID3D12DescriptorHeap *descriptor_heap_data;//�������ѵ���ʵ��ַ
	pancy_object_id per_descriptor_size;       //ÿ���������Ĵ�С
	pancy_object_id segmental_offset_position; //��ǰ����������ȫ�ֵ�ƫ��
	pancy_object_id max_descriptor_num;        //��ǰ��������������ɵ�����������
	pancy_object_id now_pointer_offset;        //��ǰ����������ʹ�����ݵ�ƫ��
	pancy_object_id now_pointer_refresh;       //��ǰ�������������Ҫһ�������������������ʼ����λ��
	pancy_object_id now_descriptor_pack_id_self_add;//��ǰ��������Ϊÿ��������ҳ�����ID������������
	std::queue<pancy_object_id> now_descriptor_pack_id_reuse;//֮ǰ���ͷŵ������ڿ�������ʹ�õ�������ID

	std::unordered_map<pancy_resource_id, BindlessResourceViewPointer> descriptor_data;//ÿ�������������������ҳ
public:
	BindlessResourceViewSegmental(
		const pancy_object_id &max_descriptor_num,
		const pancy_object_id &segmental_offset_position_in,
		const pancy_object_id &per_descriptor_size_in,
		const ComPtr<ID3D12DescriptorHeap> descriptor_heap_data_in
	);
	inline pancy_object_id GetEmptyDescriptorNum()
	{
		return max_descriptor_num - now_pointer_offset;
	}
	//�����������￪��һ��bindless������ҳ
	PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPack(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer> &describe_memory_data,
		const pancy_object_id &SRV_pack_size,
		pancy_object_id &SRV_pack_id
	);
	//������������ɾ��һ��bindless������ҳ
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPack(const pancy_object_id &SRV_pack_id);
	//Ϊ�������ν���һ����������Ƭ���������
	PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewPack();
	//������������ɾ��һ��bindless������ҳ��ִ��һ���������
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPackAndRefresh(const pancy_object_id &SRV_pack_id);
	//��ȡһ��������ҳ�Ļ���ƫ��
	const BindlessResourceViewPointer GetDescriptorPageOffset(const pancy_object_id &descriptor_page_id);
private:
	//����������ҳ��ָ����Ϣ�����������ѿ���������
	PancystarEngine::EngineFailReason BuildShaderResourceView(const BindlessResourceViewPointer &resource_view_pointer);
};

//���������ҳ��id��
struct BindlessDescriptorID
{
	//ȫ��ID
	pancy_object_id bindless_id;
	//������Դ������
	pancy_object_id empty_resource_size;
	//����С�������
	bool operator<(const BindlessDescriptorID& other)  const;
};
//����������ε�id��
struct BindlessResourceViewID
{
	pancy_object_id segmental_id;
	pancy_object_id page_id;
};
enum PancyDescriptorType
{
	DescriptorTypeShaderResourceView = 0,
	DescriptorTypeConstantBufferView,
	DescriptorTypeUnorderedAccessView,
	DescriptorTypeRenderTargetView,
	DescriptorTypeDepthStencilView,
};
struct BasicDescriptorDesc
{
	//������������
	PancyDescriptorType basic_descriptor_type;
	//�洢���е���������ʽ
	D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc = {};
	D3D12_RENDER_TARGET_VIEW_DESC render_target_view_desc = {};
	D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
};
//���ڴ�����������
class PancyDescriptorHeap
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc;                                        //�������ѵĸ�ʽ
	std::string descriptor_heap_name;                                                  //�������ѵ�����
	pancy_object_id per_descriptor_size;                                               //ÿ���������Ĵ�С
	ComPtr<ID3D12DescriptorHeap> descriptor_heap_data;                                 //�������ѵ���ʵ����

	//�������ֲ�ͬ��������(ȫ����������bindless�������Լ���ͨ�İ�������)
	std::unordered_map<std::string, pancy_object_id> descriptor_globel_map;//�������ѵ�ȫ�������������ݼ���

	pancy_object_id bind_descriptor_num;                                               //�����������֧�ֵİ�����������
	std::queue<pancy_object_id> bind_descriptor_offset_reuse;                          //���������Ļ������õ�ID��
	std::unordered_map<pancy_object_id, CommonDescriptorPointer> descriptor_bind_map;  //�������ѵİ������������ݼ���

	pancy_object_id bindless_descriptor_num;                                                 //�����������֧�ֵ�bindless����������
	pancy_object_id per_segmental_size;                                                      //ÿһ���������εĴ�С
	std::unordered_map<pancy_object_id, BindlessDescriptorID> bindless_descriptor_id_map;    //�������ѵ�����bindless�������ε�id����
	std::map<BindlessDescriptorID, BindlessResourceViewSegmental*> descriptor_segmental_map; //�������ѵ�����bindless�������ε����ݼ���

public:
	PancyDescriptorHeap();
	~PancyDescriptorHeap();
	PancystarEngine::EngineFailReason Create(
		const D3D12_DESCRIPTOR_HEAP_DESC &descriptor_heap_desc,
		const std::string &descriptor_heap_name_in,
		const pancy_object_id &bind_descriptor_num_in,
		const pancy_object_id &bindless_descriptor_num_in,
		const pancy_object_id &per_segmental_size_in
	);
	inline PancystarEngine::EngineFailReason GetDescriptorHeapData(ID3D12DescriptorHeap **descriptor_heap_out)
	{
		*descriptor_heap_out = descriptor_heap_data.Get();
		return PancystarEngine::succeed;
	}
	//����ȫ��������
	PancystarEngine::EngineFailReason BuildGlobelDescriptor(
		const std::string &globel_name,
		const std::vector<BasicDescriptorDesc> &SRV_desc,
		const std::vector <SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer
	);
	PancystarEngine::EngineFailReason DeleteGlobelDescriptor(const std::string &globel_name);
	PancystarEngine::EngineFailReason GetGlobelDesciptorID(const std::string &globel_name, pancy_object_id &descriptor_id);
	PancystarEngine::EngineFailReason BindGlobelDescriptor(
		const std::string &globel_name,
		const D3D12_COMMAND_LIST_TYPE &render_param_type,
		PancyRenderCommandList *m_commandList,
		const pancy_object_id &root_signature_offset
	);
	//����˽�еİ�������
	PancystarEngine::EngineFailReason BuildBindDescriptor(
		const std::vector<BasicDescriptorDesc> &descriptor_desc,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer,
		pancy_object_id &descriptor_id
	);
	PancystarEngine::EngineFailReason DeleteBindDescriptor(const pancy_object_id &descriptor_id);
	PancystarEngine::EngineFailReason BindCommonDescriptor(
		const pancy_object_id &descriptor_id,
		const D3D12_COMMAND_LIST_TYPE &render_param_type,
		PancyRenderCommandList *m_commandList,
		const pancy_object_id &root_signature_offset
	);
	PancystarEngine::EngineFailReason GetCommonDescriptorCpuOffset(const pancy_object_id &descriptor_id, CD3DX12_CPU_DESCRIPTOR_HANDLE &Cpu_Handle);
	//����˽�е�bindless������ҳ
	PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPage(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer> &describe_memory_data,
		const pancy_object_id &SRV_pack_size,
		BindlessResourceViewID &descriptor_id
	);
	//ɾ��˽�е�bindless������ҳ(����ָ���Ƿ�ɾ����Ϻ��ҳ��Ƭ��������)
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPage(
		const BindlessResourceViewID &descriptor_id,
		bool is_refresh_segmental = true
	);
	//�������е��������Σ����Ľϴ����л���ͼ��Դ��ʱ����ϲ�������Ƭ��ɾ��ʹ��
	PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewSegmental();
	//���������������rootsignature
	PancystarEngine::EngineFailReason BindBindlessDescriptor(
		const BindlessResourceViewID &descriptor_id,
		const D3D12_COMMAND_LIST_TYPE &render_param_type,
		PancyRenderCommandList *m_commandList,
		const pancy_object_id &root_signature_offset
	);
private:
	//����ˢ�½����Դ�������εĴ�С��������������ɾ��ĵ�ʱ�򱻵���
	PancystarEngine::EngineFailReason RefreshBindlessResourcesegmentalSize(const pancy_object_id &resourc_id);
	//Ԥ��������������
	PancystarEngine::EngineFailReason PreBuildBindDescriptor(
		const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_type,
		const bool if_build_multi_buffer,
		std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> &descriptor_cpu_handle,
		CommonDescriptorPointer &new_descriptor_data
	);
	//��������������
	PancystarEngine::EngineFailReason BuildDescriptorData(
		const BasicDescriptorDesc &descriptor_data,
		const SubMemoryPointer &submemory_data,
		const CD3DX12_CPU_DESCRIPTOR_HANDLE &cpuHandle
	);
	//��ȡ�������Ѹ�ʽ
	D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapTypeOfDescriptor(const BasicDescriptorDesc &descriptor_desc);
};

//��������������ָ��
struct BindDescriptorPointer
{
	//��������ID
	pancy_resource_id descriptor_heap_id;
	//������ID
	pancy_object_id descriptor_id;
};
//���������������ָ��
struct BindlessDescriptorPointer
{
	//��������ID
	pancy_resource_id descriptor_heap_id;
	//������ID
	BindlessResourceViewID descriptor_pack_id;
};
//���ڹ������е���������
class PancyDescriptorHeapControl
{
	pancy_resource_id descriptor_heap_id_self_add;
	std::queue<pancy_resource_id> descriptor_heap_id_reuse;
	pancy_resource_id common_descriptor_heap_shader_resource;
	pancy_resource_id common_descriptor_heap_render_target;
	pancy_resource_id common_descriptor_heap_depth_stencil;
	std::unordered_map<pancy_resource_id, PancyDescriptorHeap*> descriptor_heap_map;
	PancyDescriptorHeapControl();
public:
	~PancyDescriptorHeapControl();
	static PancyDescriptorHeapControl* GetInstance()
	{
		static PancyDescriptorHeapControl* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyDescriptorHeapControl();
		}
		return this_instance;
	}
	//��ȡ��������������
	PancystarEngine::EngineFailReason GetBasicDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_desc, ID3D12DescriptorHeap **descriptor_heap_out);
	//ȫ��������
	PancystarEngine::EngineFailReason BuildCommonGlobelDescriptor(
		const std::string &globel_srv_name,
		const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer
	);
	PancystarEngine::EngineFailReason GetCommonGlobelDescriptorID(
		PancyDescriptorType basic_descriptor_type,
		const std::string &globel_srv_name,
		BindDescriptorPointer &descriptor_id
	);
	PancystarEngine::EngineFailReason BindCommonGlobelDescriptor(
		PancyDescriptorType basic_descriptor_type,
		const std::string &globel_name,
		const D3D12_COMMAND_LIST_TYPE &render_param_type,
		PancyRenderCommandList *m_commandList,
		const pancy_object_id &root_signature_offset
	);
	PancystarEngine::EngineFailReason BindCommonRenderTargetUncontiguous(
		const std::vector<pancy_object_id> rendertarget_list,
		const pancy_object_id depthstencil_descriptor,
		PancyRenderCommandList *m_commandList,
		const bool &if_have_rendertarget = true,
		const bool &if_have_depthstencil = true
	);
	//todo:Ŀǰ����RTV�ڽ�������ȡ�����޷�������ʱ����ȡ��depthstencil�ķ������ڲ��ԣ�����Դ������������Ҫɾ��
	PancystarEngine::EngineFailReason GetCommonDepthStencilBufferOffset(
		const pancy_object_id depthstencil_descriptor,
		CD3DX12_CPU_DESCRIPTOR_HANDLE &dsvHandle
	);
	//��������
	PancystarEngine::EngineFailReason BuildCommonDescriptor(
		const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
		const std::vector<SubMemoryPointer>& memory_data,
		const bool if_build_multi_buffer,
		BindDescriptorPointer &descriptor_id
	);
	PancystarEngine::EngineFailReason BindCommonDescriptor(
		const BindDescriptorPointer &descriptor_id,
		const D3D12_COMMAND_LIST_TYPE &render_param_type,
		PancyRenderCommandList *m_commandList,
		const pancy_object_id &root_signature_offset
	);
	//���������
	PancystarEngine::EngineFailReason BuildCommonBindlessShaderResourceView(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer> &describe_memory_data,
		const pancy_object_id &SRV_pack_size,
		BindlessDescriptorPointer &descriptor_id
	);
	PancystarEngine::EngineFailReason BindBindlessDescriptor(
		const BindlessDescriptorPointer &descriptor_id,
		const D3D12_COMMAND_LIST_TYPE &render_param_type,
		PancyRenderCommandList *m_commandList,
		const pancy_object_id &root_signature_offset
	);
	PancystarEngine::EngineFailReason ClearRenderTarget();
	//�����ɾ��һ����������
	PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_name, const Json::Value &root_value, pancy_resource_id &descriptor_heap_id);
	PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_file_name, pancy_resource_id &descriptor_heap_id);
	PancystarEngine::EngineFailReason DeleteDescriptorHeap(const pancy_resource_id &descriptor_heap_id);
private:
	pancy_resource_id GetCommonDescriptorHeapID(const BasicDescriptorDesc &descriptor_desc);
	pancy_resource_id GetCommonDescriptorHeapID(const PancyDescriptorType &descriptor_type);
};

