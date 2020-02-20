#pragma once
#include"PancyDx12Basic.h"
#include"PancyThreadBasic.h"
#include<DirectXTex.h>
#include<LoaderHelpers.h>
#include<DDSTextureLoader.h>
#include<WICTextureLoader.h>
#define MaxHeapDivide 32
//显存指针
struct VirtualMemoryPointer
{
	bool if_heap;//资源是否是由堆上分配而来
	pancy_resource_id heap_type;      //堆类型
	pancy_resource_id heap_list_id;   //堆id号
	pancy_resource_id memory_block_id;//内存块id号
	pancy_object_id memory_resource_id;//直接指向内存的id号
	VirtualMemoryPointer()
	{
		if_heap = false;
		heap_type = 0;
		heap_list_id = 0;
		memory_block_id = 0;
		memory_resource_id = 0;
	}
};
//显存块
class MemoryBlockGpu
{
	pancy_resource_size memory_size;//存储块的大小
	ComPtr<ID3D12Resource> resource_data;//存储块的数据
	D3D12_HEAP_TYPE resource_usage;
	UINT8* map_pointer;
	D3D12_RESOURCE_STATES now_subresource_state;//当前资源的使用格式
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
	//查看当前资源的使用格式
	inline D3D12_RESOURCE_STATES GetResourceState()
	{
		return now_subresource_state;
	}
	//修改当前资源的使用格式
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
//保留显存堆
class MemoryHeapGpu
{
	std::string heap_type_name;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	ComPtr<ID3D12Heap> heap_data;
	//std::unordered_set<pancy_resource_id> free_list;

	//所有显存的数据
	std::unordered_map<pancy_resource_id, MemoryBlockGpu*> memory_heap_block;
	//空出的显存
	std::unordered_set<pancy_resource_id> empty_memory_block;
public:
	MemoryHeapGpu(const std::string &heap_type_name_in);
	//每个显存块的大小
	inline pancy_resource_size GetMemorySizePerBlock()
	{
		return size_per_block;
	}
	//显存堆的总显存块数量
	inline pancy_resource_id GetMaxMemoryBlockNum()
	{
		return max_block_num;
	}
	//显存堆尚未使用的显存大小
	inline size_t GetFreeMemoryBlockNum()
	{
		return empty_memory_block.size();
	}
	PancystarEngine::EngineFailReason Create(const CD3DX12_HEAP_DESC &heap_desc_in, const uint64_t &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID
	);
	//获取显存资源
	MemoryBlockGpu* GetMemoryResource(const pancy_resource_id &memory_block_ID);
	//检验对应id的资源是否已经被分配
	bool CheckIfFree(pancy_resource_id memory_block_ID);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(const pancy_resource_id &memory_block_ID);
	~MemoryHeapGpu();
};
//线性增长的显存堆
class MemoryHeapLinear
{
	//显存堆的格式
	CD3DX12_HEAP_DESC heap_desc;
	pancy_resource_size size_per_block;
	pancy_resource_id max_block_num;
	//显存堆的名称
	std::string heap_type_name;

	//所有显存堆的数据
	std::unordered_map<pancy_resource_id, MemoryHeapGpu*> memory_heap_data;
	//空出的显存堆
	std::unordered_set<pancy_resource_id> empty_memory_heap;
public:
	MemoryHeapLinear(const std::string &heap_type_name_in, const CD3DX12_HEAP_DESC &heap_desc_in, const pancy_resource_size &size_per_block_in, const pancy_resource_id &max_block_num_in);
	//从显存堆开辟资源
	PancystarEngine::EngineFailReason BuildMemoryResource(
		const D3D12_RESOURCE_DESC &resource_desc,
		const D3D12_RESOURCE_STATES &resource_state,
		pancy_resource_id &memory_block_ID,//显存块地址指针
		pancy_resource_id &memory_heap_ID//显存段地址指针
	);
	MemoryBlockGpu* GetMemoryResource(
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针

	);
	//释放一个对应id的资源
	PancystarEngine::EngineFailReason FreeMemoryReference(
		const pancy_resource_id &memory_heap_ID,
		const pancy_resource_id &memory_block_ID
	);
	//获取当前堆类型开启的堆数量
	inline pancy_object_id GetHeapNum()
	{
		return memory_heap_data.size();
	}
	//获取当前堆类型的每个堆的大小
	inline pancy_resource_size GetPerHeapSize()
	{
		return heap_desc.SizeInBytes;
	}
	~MemoryHeapLinear();
};
//资源管理器
class MemoryHeapGpuControl
{
	pancy_object_id resource_memory_id_self_add;
	std::unordered_map<std::string, pancy_resource_id> resource_init_list;
	std::unordered_map<pancy_resource_id, MemoryHeapLinear*> resource_heap_list;//显存堆表
	std::unordered_map<pancy_object_id, MemoryBlockGpu*>resource_memory_list;//离散的显存块表
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
	//不存放在指定堆上的资源
	MemoryBlockGpu* GetMemoryFromList(const pancy_object_id &memory_block_ID);
	PancystarEngine::EngineFailReason FreeResourceCommit(const pancy_object_id &memory_block_ID);
	//存放在指定堆上的资源
	MemoryBlockGpu * GetMemoryResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
	);

	PancystarEngine::EngineFailReason LoadHeapFromFile(
		const std::string &HeapFileName,
		pancy_resource_id &resource_id,
		uint64_t heap_alignment_size = 0
	);
	PancystarEngine::EngineFailReason FreeResourceFromHeap(
		const pancy_resource_id &memory_heap_list_ID,//显存域地址指针
		const pancy_resource_id &memory_heap_ID,//显存段地址指针
		const pancy_resource_id &memory_block_ID//显存块地址指针
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
//二级资源
struct SubMemoryPointer
{
	pancy_resource_id type_id;
	pancy_object_id list_id;
	pancy_object_id offset;
};
class SubMemoryData
{
	VirtualMemoryPointer buffer_data;//显存资源指针
	pancy_resource_size per_memory_size;//每个缓冲区的大小
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
	//查看当前空闲资源的大小
	inline pancy_object_id GetEmptySize()
	{
		return static_cast<pancy_object_id>(empty_sub_memory.size());
	}

	//获取资源
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
	//空出的显存
	std::unordered_set<pancy_object_id> empty_memory_heap;
	//当前最大的id号以及已经释放的id号
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

//普通的绑定描述符
struct CommonDescriptorPointer
{
	//是否使用双缓冲
	bool if_multi_buffer;
	//描述符页的起始地址
	std::vector<pancy_object_id> descriptor_offset;
	//描述符类型
	D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type;
	CommonDescriptorPointer()
	{
		//初始化普通描述符指针
		if_multi_buffer = false;
		descriptor_type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	}
};
//解绑定的描述符页
struct BindlessResourceViewPointer
{
	//描述符页的起始地址
	pancy_object_id resource_view_offset;
	//描述符页所包含的描述符数量
	pancy_object_id resource_view_num;
	//每一个描述符的格式数据
	std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> SRV_desc;
	//每一个描述符所指向的资源
	std::vector<SubMemoryPointer> describe_memory_data;
	BindlessResourceViewPointer()
	{
		resource_view_offset = 0;
		resource_view_num = 0;
	}
};
//描述符段
class BindlessResourceViewSegmental
{
	ID3D12DescriptorHeap *descriptor_heap_data;//描述符堆的真实地址
	pancy_object_id per_descriptor_size;       //每个描述符的大小
	pancy_object_id segmental_offset_position; //当前描述符段在全局的偏移
	pancy_object_id max_descriptor_num;        //当前描述符段最大容纳的描述符数量
	pancy_object_id now_pointer_offset;        //当前描述符段已使用数据的偏移
	pancy_object_id now_pointer_refresh;       //当前描述符段如果需要一次整理操作，其合理的起始整理位置
	pancy_object_id now_descriptor_pack_id_self_add;//当前描述符段为每个描述符页分配的ID的自增长号码
	std::queue<pancy_object_id> now_descriptor_pack_id_reuse;//之前被释放掉，现在可以重新使用的描述符ID

	std::unordered_map<pancy_resource_id, BindlessResourceViewPointer> descriptor_data;//每个被分配出来的描述符页
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
	//从描述符段里开辟一组bindless描述符页
	PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPack(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer> &describe_memory_data,
		const pancy_object_id &SRV_pack_size,
		pancy_object_id &SRV_pack_id
	);
	//从描述符段里删除一组bindless描述符页
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPack(const pancy_object_id &SRV_pack_id);
	//为描述符段进行一次描述符碎片的整理操作
	PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewPack();
	//从描述符段里删除一组bindless描述符页并执行一次整理操作
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPackAndRefresh(const pancy_object_id &SRV_pack_id);
	//获取一个描述符页的基础偏移
	const BindlessResourceViewPointer GetDescriptorPageOffset(const pancy_object_id &descriptor_page_id);
private:
	//根据描述符页的指针信息，在描述符堆开辟描述符
	PancystarEngine::EngineFailReason BuildShaderResourceView(const BindlessResourceViewPointer &resource_view_pointer);
};

//解绑定描述符页的id号
struct BindlessDescriptorID
{
	//全局ID
	pancy_object_id bindless_id;
	//空余资源的数量
	pancy_object_id empty_resource_size;
	//重载小于运算符
	bool operator<(const BindlessDescriptorID& other)  const;
};
//解绑定描述符段的id号
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
	//描述符的类型
	PancyDescriptorType basic_descriptor_type;
	//存储所有的描述符格式
	D3D12_SHADER_RESOURCE_VIEW_DESC shader_resource_view_desc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC unordered_access_view_desc = {};
	D3D12_RENDER_TARGET_VIEW_DESC render_target_view_desc = {};
	D3D12_DEPTH_STENCIL_VIEW_DESC depth_stencil_view_desc = {};
};
//用于处理描述符堆
class PancyDescriptorHeap
{
	D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc;                                        //描述符堆的格式
	std::string descriptor_heap_name;                                                  //描述符堆的名称
	pancy_object_id per_descriptor_size;                                               //每个描述符的大小
	ComPtr<ID3D12DescriptorHeap> descriptor_heap_data;                                 //描述符堆的真实数据

	//管理三种不同的描述符(全局描述符，bindless描述符以及普通的绑定描述符)
	std::unordered_map<std::string, pancy_object_id> descriptor_globel_map;//描述符堆的全局描述符的数据集合

	pancy_object_id bind_descriptor_num;                                               //描述符堆最大支持的绑定描述符数量
	std::queue<pancy_object_id> bind_descriptor_offset_reuse;                          //绑定描述符的回收利用的ID号
	std::unordered_map<pancy_object_id, CommonDescriptorPointer> descriptor_bind_map;  //描述符堆的绑定描述符的数据集合

	pancy_object_id bindless_descriptor_num;                                                 //描述符堆最大支持的bindless描述符数量
	pancy_object_id per_segmental_size;                                                      //每一个描述符段的大小
	std::unordered_map<pancy_object_id, BindlessDescriptorID> bindless_descriptor_id_map;    //描述符堆的所有bindless描述符段的id集合
	std::map<BindlessDescriptorID, BindlessResourceViewSegmental*> descriptor_segmental_map; //描述符堆的所有bindless描述符段的数据集合

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
	//创建全局描述符
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
	//创建私有的绑定描述符
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
	//创建私有的bindless描述符页
	PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPage(
		const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
		const std::vector<SubMemoryPointer> &describe_memory_data,
		const pancy_object_id &SRV_pack_size,
		BindlessResourceViewID &descriptor_id
	);
	//删除私有的bindless描述符页(可以指定是否删除完毕后对页碎片进行整理)
	PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPage(
		const BindlessResourceViewID &descriptor_id,
		bool is_refresh_segmental = true
	);
	//整理所有的描述符段，消耗较大，在切换地图资源的时候配合不整理碎片的删除使用
	PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewSegmental();
	//将解绑定描述符绑定至rootsignature
	PancystarEngine::EngineFailReason BindBindlessDescriptor(
		const BindlessResourceViewID &descriptor_id,
		const D3D12_COMMAND_LIST_TYPE &render_param_type,
		PancyRenderCommandList *m_commandList,
		const pancy_object_id &root_signature_offset
	);
private:
	//重新刷新解绑定资源描述符段的大小，当描述符段增删查改的时候被调用
	PancystarEngine::EngineFailReason RefreshBindlessResourcesegmentalSize(const pancy_object_id &resourc_id);
	//预创建描述符数据
	PancystarEngine::EngineFailReason PreBuildBindDescriptor(
		const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_type,
		const bool if_build_multi_buffer,
		std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> &descriptor_cpu_handle,
		CommonDescriptorPointer &new_descriptor_data
	);
	//创建描述符数据
	PancystarEngine::EngineFailReason BuildDescriptorData(
		const BasicDescriptorDesc &descriptor_data,
		const SubMemoryPointer &submemory_data,
		const CD3DX12_CPU_DESCRIPTOR_HANDLE &cpuHandle
	);
	//获取描述符堆格式
	D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapTypeOfDescriptor(const BasicDescriptorDesc &descriptor_desc);
};

//绑定描述符的虚拟指针
struct BindDescriptorPointer
{
	//描述符堆ID
	pancy_resource_id descriptor_heap_id;
	//描述符ID
	pancy_object_id descriptor_id;
};
//解绑定描述符的虚拟指针
struct BindlessDescriptorPointer
{
	//描述符堆ID
	pancy_resource_id descriptor_heap_id;
	//描述符ID
	BindlessResourceViewID descriptor_pack_id;
};
//用于管理所有的描述符堆
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
	//获取基础的描述符堆
	PancystarEngine::EngineFailReason GetBasicDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_desc, ID3D12DescriptorHeap **descriptor_heap_out);
	//全局描述符
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
	//todo:目前由于RTV在交换链中取出来无法管理，暂时给出取出depthstencil的方法用于测试，在资源管理器重做后要删除
	PancystarEngine::EngineFailReason GetCommonDepthStencilBufferOffset(
		const pancy_object_id depthstencil_descriptor,
		CD3DX12_CPU_DESCRIPTOR_HANDLE &dsvHandle
	);
	//绑定描述符
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
	//解绑定描述符
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
	//添加与删除一个描述符堆
	PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_name, const Json::Value &root_value, pancy_resource_id &descriptor_heap_id);
	PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_file_name, pancy_resource_id &descriptor_heap_id);
	PancystarEngine::EngineFailReason DeleteDescriptorHeap(const pancy_resource_id &descriptor_heap_id);
private:
	pancy_resource_id GetCommonDescriptorHeapID(const BasicDescriptorDesc &descriptor_desc);
	pancy_resource_id GetCommonDescriptorHeapID(const PancyDescriptorType &descriptor_type);
};

