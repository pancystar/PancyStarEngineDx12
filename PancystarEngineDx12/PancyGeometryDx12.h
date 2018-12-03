#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
#include"PancyMemoryBasic.h"
#include"PancyThreadBasic.h"
#define IndexType uint32_t
namespace PancystarEngine
{
	//2D�����ʽ
	struct Point2D
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 tex_color;  //���ڲ���������
		//DirectX::XMFLOAT4 tex_range;  //�������Ʋ������ε�����
	};
	struct PointUI
	{
		DirectX::XMFLOAT4 position;
		DirectX::XMFLOAT4 tex_color;  //���ڲ���������
		DirectX::XMUINT4  tex_id;     //UI��ID��
	};
	//��׼3D�����ʽ
	struct PointCommon
	{
		DirectX::XMFLOAT3 position;   //λ��
		DirectX::XMFLOAT3 normal;     //����
		DirectX::XMFLOAT3 tangent;    //����
		DirectX::XMUINT4  tex_id;     //ʹ�õ�����ID��
		DirectX::XMFLOAT4 tex_uv;     //���ڲ���������
	};
	//�������Ķ����ʽ
	struct PointSkinCommon
	{
		DirectX::XMFLOAT3 position;   //λ��
		DirectX::XMFLOAT3 normal;     //����
		DirectX::XMFLOAT3 tangent;    //����
		DirectX::XMUINT4  tex_id;     //ʹ�õ�����ID��
		DirectX::XMFLOAT4 tex_uv;     //���ڲ���������
		DirectX::XMUINT4  bone_id;    //����ID��
		DirectX::XMFLOAT4 bone_weight;//����Ȩ��
	};
	
	
	//�������������
	class GeometryBasic
	{
	protected:
		bool if_build_finish;//�Ƿ��Ѿ��ϴ����
		PancyFenceIdGPU upload_fence_value;
		//���������Ⱦbuffer
		SubMemoryPointer geometry_vertex_buffer;
		SubMemoryPointer geometry_index_buffer;
		SubMemoryPointer geometry_adjindex_buffer;
		D3D12_VERTEX_BUFFER_VIEW geometry_vertex_buffer_view;
		D3D12_INDEX_BUFFER_VIEW geometry_index_buffer_view;
		uint32_t all_vertex;
		uint32_t all_index;
		uint32_t all_index_adj;
		//������Ĵ�����Ϣ
		bool if_create_adj;
		bool if_buffer_created;
		
	public:
		GeometryBasic();
		PancystarEngine::EngineFailReason Create();
		virtual ~GeometryBasic();
		inline ComPtr<ID3D12Resource> GetVertexBuffer() 
		{
			int64_t res_size;
			return SubresourceControl::GetInstance()->GetResourceData(geometry_vertex_buffer, res_size)->GetResource();
		};
		inline ComPtr<ID3D12Resource> GetIndexBuffer()
		{
			int64_t res_size;
			return SubresourceControl::GetInstance()->GetResourceData(geometry_index_buffer, res_size)->GetResource();
		};
		inline ComPtr<ID3D12Resource> GetIndexAdjBuffer()
		{
			int64_t res_size;
			return SubresourceControl::GetInstance()->GetResourceData(geometry_adjindex_buffer, res_size)->GetResource();
		};
		inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()
		{
			return geometry_vertex_buffer_view;
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()
		{
			return geometry_index_buffer_view;
		};
		inline bool CheckIfCopyFinish() 
		{
			if (!if_build_finish) 
			{
				if (ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->CheckGpuBrokenFence(upload_fence_value))
				{
					if_build_finish = true;
				}
			}
			return if_build_finish;
		}
		inline void WaitCopyFinish() 
		{
			if (!if_build_finish)
			{
				ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->WaitGpuBrokenFence(upload_fence_value);
				if_build_finish = true;
			}
		}
	protected:
		virtual PancystarEngine::EngineFailReason InitGeometryDesc(
			bool &if_create_adj
		) = 0;
		virtual PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need, 
			uint32_t &all_index_need, 
			uint32_t &all_index_adj_need,
			SubMemoryPointer &geometry_vertex_buffer,
			SubMemoryPointer &geometry_index_buffer,
			SubMemoryPointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
		) = 0;
		PancystarEngine::EngineFailReason BuildDefaultBuffer(
			ID3D12GraphicsCommandList* cmdList,
			int64_t memory_alignment_size,
			int64_t memory_block_alignment_size,
			SubMemoryPointer &default_buffer,
			SubMemoryPointer &upload_buffer,
			const void* initData,
			const UINT BufferSize,
			D3D12_RESOURCE_STATES buffer_type
		);
	};
	GeometryBasic::GeometryBasic()
	{
		all_vertex = 0;
		all_index = 0;
		all_index_adj = 0;
		if_buffer_created = false;
		if_build_finish = false;
	}
	PancystarEngine::EngineFailReason GeometryBasic::Create()
	{
		PancystarEngine::EngineFailReason check_error;
		//�����Ƿ���Դ�Ѿ�����
		if (if_buffer_created)
		{
			return PancystarEngine::succeed;
		}
		//ע�Ἰ�����ʽ
		check_error = InitGeometryDesc(if_create_adj);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//ע�Ἰ������Դ
		check_error = InitGeometry(
			all_vertex, 
			all_index, 
			all_index_adj,
			geometry_vertex_buffer,
			geometry_index_buffer,
			geometry_adjindex_buffer,
			geometry_vertex_buffer_view,
			geometry_index_buffer_view
		);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		if_buffer_created = true;
		return PancystarEngine::succeed;
	}
	GeometryBasic::~GeometryBasic()
	{
	}
	PancystarEngine::EngineFailReason GeometryBasic::BuildDefaultBuffer(
		ID3D12GraphicsCommandList* cmdList,
		int64_t memory_alignment_size,
		int64_t memory_block_alignment_size,
		SubMemoryPointer &default_buffer,
		SubMemoryPointer &upload_buffer,
		const void* initData,
		const UINT BufferSize,
		D3D12_RESOURCE_STATES buffer_type
	) 
	{
		HRESULT hr;
		PancystarEngine::EngineFailReason check_error;
		//�ȴ���4M����Ĵ洢��
		UINT alignment_buffer_size = (BufferSize + memory_alignment_size) & ~(memory_alignment_size-1);//4M����
		std::string heapdesc_file_name = "json\\resource_heap\\PointBuffer" + std::to_string(alignment_buffer_size) + ".json";
		std::string dynamic_heapdesc_file_name = "json\\resource_heap\\DynamicBuffer" + std::to_string(alignment_buffer_size) + ".json";
		//������Դ�Ѹ�ʽ����
		if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(heapdesc_file_name)) 
		{
			//���¸�ʽ�ļ�
			Json::Value json_data_out;
			UINT resource_block_num = (4194304 * 20) / alignment_buffer_size;
			if (resource_block_num < 1)
			{
				resource_block_num = 1;
			}
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", alignment_buffer_size);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_DEFAULT");
			PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS");
			PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, heapdesc_file_name);
			//���ļ����Ϊ�Ѿ�����
			FileBuildRepeatCheck::GetInstance()->AddFileName(heapdesc_file_name);
		}
		//�����ϴ�����ʱ��Դ�Ѹ�ʽ����
		if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(dynamic_heapdesc_file_name))
		{
			//���¸�ʽ�ļ�
			Json::Value json_data_out;
			UINT resource_block_num = (4194304 * 5) / alignment_buffer_size;
			if (resource_block_num < 1) 
			{
				resource_block_num = 1;
			}
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "commit_block_num", resource_block_num);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "per_block_size", alignment_buffer_size);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_out, "heap_type_in", "D3D12_HEAP_TYPE_UPLOAD");
			PancyJsonTool::GetInstance()->AddJsonArrayValue(json_data_out, "heap_flag_in", "D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS");
			PancyJsonTool::GetInstance()->WriteValueToJson(json_data_out, dynamic_heapdesc_file_name);
			//���ļ����Ϊ�Ѿ�����
			FileBuildRepeatCheck::GetInstance()->AddFileName(dynamic_heapdesc_file_name);
		}
		//�����洢��Ԫ�ķ�����С
		UINT buffer_block_size = (BufferSize + memory_block_alignment_size) & ~(memory_block_alignment_size - 1);//128k����
		std::string bufferblock_file_name = "json\\resource_view\\PointBufferSub" + std::to_string(buffer_block_size) + ".json";
		std::string dynamic_bufferblock_file_name = "json\\resource_view\\DynamicBufferSub" + std::to_string(buffer_block_size) + ".json";
		//������Դ�Ѵ洢��Ԫ��ʽ����
		if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(bufferblock_file_name))
		{
			//���¸�ʽ�ļ�
			Json::Value json_data_resourceview;
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", heapdesc_file_name);
			Json::Value json_data_res_desc;
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", alignment_buffer_size);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", "DXGI_FORMAT_UNKNOWN");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", "D3D12_TEXTURE_LAYOUT_ROW_MAJOR");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", "D3D12_RESOURCE_FLAG_NONE");
			Json::Value json_data_sample_desc;
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
			//�ݹ�ص�
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
			//�����������
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_COPY_DEST");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", buffer_block_size);
			//д���ļ������Ϊ�Ѵ���
			PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, bufferblock_file_name);
			FileBuildRepeatCheck::GetInstance()->AddFileName(bufferblock_file_name);
		}
		//�����ϴ�����ʱ��Դ�Ѵ洢��Ԫ��ʽ����
		if (!FileBuildRepeatCheck::GetInstance()->CheckIfCreated(dynamic_bufferblock_file_name))
		{
			//���¸�ʽ�ļ�
			Json::Value json_data_resourceview;
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "ResourceType", dynamic_heapdesc_file_name);
			Json::Value json_data_res_desc;
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Dimension", "D3D12_RESOURCE_DIMENSION_BUFFER");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Alignment", 0);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Width", alignment_buffer_size);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Height", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "DepthOrArraySize", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "MipLevels", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Format", "DXGI_FORMAT_UNKNOWN");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Layout", "D3D12_TEXTURE_LAYOUT_ROW_MAJOR");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "Flags", "D3D12_RESOURCE_FLAG_NONE");
			Json::Value json_data_sample_desc;
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Count", 1);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_sample_desc, "Quality", 0);
			//�ݹ�ص�
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_res_desc, "SampleDesc", json_data_sample_desc);
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_DESC", json_data_res_desc);
			//�����������
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "D3D12_RESOURCE_STATES", "D3D12_RESOURCE_STATE_GENERIC_READ");
			PancyJsonTool::GetInstance()->SetJsonValue(json_data_resourceview, "per_block_size", buffer_block_size);
			//д���ļ������Ϊ�Ѵ���
			PancyJsonTool::GetInstance()->WriteValueToJson(json_data_resourceview, dynamic_bufferblock_file_name);
			FileBuildRepeatCheck::GetInstance()->AddFileName(dynamic_bufferblock_file_name);
		}
		check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(bufferblock_file_name, default_buffer);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = SubresourceControl::GetInstance()->BuildSubresourceFromFile(dynamic_bufferblock_file_name, upload_buffer);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//��CPU�ɷ��ʻ������������
		D3D12_SUBRESOURCE_DATA vertexData_buffer = {};
		vertexData_buffer.pData = initData;
		vertexData_buffer.RowPitch = BufferSize;
		vertexData_buffer.SlicePitch = vertexData_buffer.RowPitch;
		int64_t per_memory_size;
		auto dest_res = SubresourceControl::GetInstance()->GetResourceData(default_buffer, per_memory_size);
		auto copy_res = SubresourceControl::GetInstance()->GetResourceData(upload_buffer, per_memory_size);
		//�Ƚ�cpu�ϵ����ݿ������ϴ�������
		BYTE* pDataBegin;
		hr = copy_res->GetResource()->Map(0, NULL, reinterpret_cast<void**>(&pDataBegin));
		if (FAILED(hr))
		{
			PancystarEngine::EngineFailReason error_message(hr,"could not map the point buffer data for model load");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load model vertex/index data", error_message);
			return error_message;
		}
		memcpy(pDataBegin + upload_buffer.offset*per_memory_size, initData, BufferSize);
		//todo:�˴�������unmap�ᵼ�³��򲻿ɵ��ԣ�ͬ���͵�commitresource����Ӱ�죬ԭ����
		copy_res->GetResource()->Unmap(0, NULL);
		//���ϴ������������ݿ������Դ滺����
		cmdList->CopyBufferRegion(dest_res->GetResource().Get(), default_buffer.offset * per_memory_size, copy_res->GetResource().Get(), upload_buffer.offset*per_memory_size, buffer_block_size);
		//�޸Ļ�������ʽ
		cmdList->ResourceBarrier(
			1,
			&CD3DX12_RESOURCE_BARRIER::Transition(
				dest_res->GetResource().Get(),
				D3D12_RESOURCE_STATE_COPY_DEST,
				buffer_type
			)
		);
		return PancystarEngine::succeed;
	}
	//����ģ�ͼ�����
	template<typename T>
	class GeometryCommonModel : public GeometryBasic
	{
		T *vertex_data;
		UINT *index_data;
		bool if_save_CPU_data;//�Ƿ���cpu����
		
		//ģ�͵Ļ�������
		bool if_model_adj;
		uint32_t all_model_vertex;
		uint32_t all_model_index;
		
	public:
		GeometryCommonModel(
			T *vertex_data_in,
			UINT *index_data_in,
			const uint32_t &input_vert_num,
			const uint32_t &input_index_num,
			bool if_adj_in = false,
			bool if_save_cpu_data_in = false
		);
		~GeometryCommonModel();
	private:
		PancystarEngine::EngineFailReason InitGeometryDesc(
			bool &if_create_adj
		);
		PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need,
			uint32_t &all_index_need,
			uint32_t &all_index_adj_need,
			SubMemoryPointer &geometry_vertex_buffer,
			SubMemoryPointer &geometry_index_buffer,
			SubMemoryPointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
		);
	};
	template<typename T>
	GeometryCommonModel<T>::GeometryCommonModel(
		T *vertex_data_in,
		UINT *index_data_in,
		const uint32_t &input_vert_num,
		const uint32_t &input_index_num,
		bool if_adj_in,
		bool if_save_cpu_data_in
	) 
	{
		//����CPU����
		vertex_data = NULL;
		index_data = NULL;
		if (input_vert_num != 0) 
		{
			all_model_vertex = input_vert_num;
			vertex_data = new T[input_vert_num];
			memcpy(vertex_data, vertex_data_in, input_vert_num * sizeof(T));
		}
		if (input_index_num != 0) 
		{
			all_model_index = input_index_num;
			index_data = new UINT[input_index_num];
			memcpy(index_data, index_data_in, input_index_num * sizeof(UINT));
		}
		if_save_CPU_data = if_save_cpu_data_in;
		if_model_adj = if_adj_in;
	}
	template<typename T>
	GeometryCommonModel<T>::~GeometryCommonModel()
	{
		if (vertex_data != NULL) 
		{
			delete[] vertex_data;
			vertex_data = NULL;

		}
		if (index_data != NULL) 
		{
			delete[] index_data;
			index_data = NULL;
		}
	}
	template<typename T>
	PancystarEngine::EngineFailReason GeometryCommonModel<T>::InitGeometryDesc(
		bool &if_create_adj
	) 
	{
		if_create_adj = if_model_adj;
		return PancystarEngine::succeed;
	}
	template<typename T>
	PancystarEngine::EngineFailReason GeometryCommonModel<T>::InitGeometry(
		uint32_t &all_vertex_need,
		uint32_t &all_index_need,
		uint32_t &all_index_adj_need,
		SubMemoryPointer &geometry_vertex_buffer_in,
		SubMemoryPointer &geometry_index_buffer_in,
		SubMemoryPointer &geometry_adjindex_buffer_in,
		D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
		D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
	) 
	{
		//������ʱ���ϴ�������
		SubMemoryPointer geometry_vertex_buffer_upload;
		SubMemoryPointer geometry_index_buffer_upload;
		//��ȡ��ʱ�Ŀ���commandlist
		PancyRenderCommandList *copy_render_list;
		uint32_t copy_render_list_ID;
		
		auto copy_contex = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
		all_vertex_need = all_model_vertex;
		all_index_need = all_model_index;
		const UINT VertexBufferSize = all_vertex_need * sizeof(T);
		UINT IndexBufferSize;
		if (sizeof(IndexType) == sizeof(UINT)) 
		{
			IndexBufferSize = all_index_need * sizeof(UINT);
		}
		else if (sizeof(IndexType) == sizeof(uint16_t))
		{
			IndexBufferSize = all_index_need * sizeof(uint16_t);
		}
		else 
		{
			IndexBufferSize = 0;
			PancystarEngine::EngineFailReason check_error(E_FAIL,"unsurpported index buffer type: "+std::to_string(sizeof(IndexType)));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build geometry data ", check_error);
			return check_error;
		}
		
		//�������㻺����
		if (vertex_data != NULL) 
		{
			PancystarEngine::EngineFailReason check_error = BuildDefaultBuffer(copy_render_list->GetCommandList().Get(), 4194304, 131072, geometry_vertex_buffer_in, geometry_vertex_buffer_upload, vertex_data, VertexBufferSize, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//��������������
		if (index_data != NULL) 
		{
			PancystarEngine::EngineFailReason check_error = BuildDefaultBuffer(copy_render_list->GetCommandList().Get(),1048576,16384, geometry_index_buffer_in, geometry_index_buffer_upload, index_data, IndexBufferSize, D3D12_RESOURCE_STATE_INDEX_BUFFER);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		//�����Ⱦ���в��ύ����
		copy_render_list->UnlockPrepare();
		
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SubmitRenderlist(1, &copy_render_list_ID);
		//��GPU�ϲ�һ�������λ
		ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(upload_fence_value);
		//ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_DIRECT)->WaitGpuBrokenFence(upload_fence_value);
		//�ȴ�����������todo::������Դ���ٵȴ�
		WaitCopyFinish();
		//ɾ����ʱ������
		SubresourceControl::GetInstance()->FreeSubResource(geometry_vertex_buffer_upload);
		SubresourceControl::GetInstance()->FreeSubResource(geometry_index_buffer_upload);
		copy_render_list->UnlockPrepare();
		//ɾ��CPU����
		if (!if_save_CPU_data) 
		{
			if (vertex_data != NULL) 
			{
				delete[] vertex_data;
				vertex_data = NULL;
			}
			if (index_data != NULL) 
			{
				delete[] index_data;
				index_data = NULL;
			}
		}
		//todo �ڽ����������
		if (if_model_adj) 
		{
		}
		int64_t res_size;
		//�������㻺����ͼ
		auto res_vert_data = SubresourceControl::GetInstance()->GetResourceData(geometry_vertex_buffer_in, res_size)->GetResource();
		geometry_vertex_buffer_view_in.BufferLocation = res_vert_data->GetGPUVirtualAddress() + geometry_vertex_buffer_in.offset * res_size;
		geometry_vertex_buffer_view_in.StrideInBytes = sizeof(T);
		geometry_vertex_buffer_view_in.SizeInBytes = res_size;
		//��������������ͼ
		
		auto res_index_data = SubresourceControl::GetInstance()->GetResourceData(geometry_index_buffer_in, res_size)->GetResource();
		geometry_index_buffer_view_in.BufferLocation = res_index_data->GetGPUVirtualAddress() + geometry_index_buffer_in.offset * res_size;
		geometry_index_buffer_view_in.SizeInBytes = res_size;
		if (sizeof(IndexType) == sizeof(UINT))
		{
			geometry_index_buffer_view_in.Format = DXGI_FORMAT_R32_UINT;
		}
		else if (sizeof(IndexType) == sizeof(uint16_t))
		{
			geometry_index_buffer_view_in.Format = DXGI_FORMAT_R16_UINT;
		}
		return PancystarEngine::succeed;
	}
	class ModelResourceBasic 
	{
	};
}