#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyDx12Basic.h"
#include"PancyMemoryBasic.h"
#include"PancyBufferDx12.h"
#include"PancyThreadBasic.h"
#define IndexType uint32_t
namespace PancystarEngine
{
	//�������ʽ(����������ʾ)
	struct PointPositionSingle
	{
		DirectX::XMFLOAT4 position;
	};
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
	//�������Ķ����ʽ(���֧�ֵ�����4����)
	struct PointSkinCommon4
	{
		DirectX::XMFLOAT3 position;   //λ��
		DirectX::XMFLOAT3 normal;     //����
		DirectX::XMFLOAT3 tangent;    //����
		DirectX::XMUINT4  tex_id;     //ʹ�õ�����ID��
		DirectX::XMFLOAT4 tex_uv;     //���ڲ���������
		DirectX::XMUINT4  bone_id;    //����ID��
		DirectX::XMFLOAT4 bone_weight;//����Ȩ��
	};
	//�������Ķ����ʽ(���֧�ֵ�����8����)
	struct PointSkinCommon8
	{
		DirectX::XMFLOAT3 position;     //λ��
		DirectX::XMFLOAT3 normal;       //����
		DirectX::XMFLOAT3 tangent;      //����
		DirectX::XMUINT4  tex_id;       //ʹ�õ�����ID��
		DirectX::XMFLOAT4 tex_uv;       //���ڲ���������
		DirectX::XMUINT4  bone_id;      //����ID��
		DirectX::XMFLOAT4 bone_weight0; //����Ȩ��
		DirectX::XMFLOAT4 bone_weight1; //����Ȩ��
	};
	//�����㶯���Ķ����ʽ
	struct PointCatchCommon
	{
		DirectX::XMFLOAT3 position;   //λ��
		DirectX::XMFLOAT3 normal;     //����
		DirectX::XMFLOAT3 tangent;    //����
		DirectX::XMUINT4  tex_id;     //ʹ�õ�����ID��
		DirectX::XMFLOAT4 tex_uv;     //���ڲ���������
		DirectX::XMUINT4  anim_id;    //��Ӧ�Ķ�������
	};
	//�������������
	class GeometryBasic
	{
	protected:
		//���������Ⱦbuffer
		VirtualResourcePointer geometry_vertex_buffer;
		VirtualResourcePointer geometry_index_buffer;
		VirtualResourcePointer geometry_adjindex_buffer;
		D3D12_VERTEX_BUFFER_VIEW geometry_vertex_buffer_view;
		D3D12_INDEX_BUFFER_VIEW geometry_index_buffer_view;
		D3D12_INDEX_BUFFER_VIEW geometry_adj_index_buffer_view;
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
		//��������Ƿ���ȷ����
		bool CheckIfCreateSucceed();
		//��ȡ���������ݵĴ�С
		inline uint32_t GetVetexNum()
		{
			return all_vertex;
		}
		inline uint32_t GetIndexNum()
		{
			return all_index;
		}
		inline uint32_t GetIndexAdjNum()
		{
			return all_index_adj;
		}
		//��ȡ���������ݵ���Ⱦָ��(������)
		inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()
		{
			return geometry_vertex_buffer_view;
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()
		{
			return geometry_index_buffer_view;
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexAdjBufferView()
		{
			return geometry_adj_index_buffer_view;
		};
		//��ȡ���������ݵ�����ָ��
		inline VirtualResourcePointer& GetVertexBufferResource()
		{
			return geometry_vertex_buffer;
		};
		inline VirtualResourcePointer& GetIndexBufferResource()
		{
			return geometry_index_buffer;
		};
		inline VirtualResourcePointer& GetAdjIndexBufferResource()
		{
			return geometry_adjindex_buffer;
		};
	protected:
		virtual PancystarEngine::EngineFailReason InitGeometryDesc(
			bool &if_create_adj
		) = 0;
		virtual PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need, 
			uint32_t &all_index_need, 
			uint32_t &all_index_adj_need,
			VirtualResourcePointer &geometry_vertex_buffer,
			VirtualResourcePointer &geometry_index_buffer,
			VirtualResourcePointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_adj_buffer_view_in
		) = 0;
	};
	
	//����ģ�ͼ�����
	template<typename T>
	class GeometryCommonModel : public GeometryBasic
	{
		T *vertex_data;
		IndexType *index_data;
		bool if_save_CPU_data;//�Ƿ���cpu����
		
		//ģ�͵Ļ�������
		bool if_model_adj;
		uint32_t all_model_vertex;
		uint32_t all_model_index;
		
	public:
		GeometryCommonModel(
			const T *vertex_data_in,
			const IndexType *index_data_in,
			const uint32_t &input_vert_num,
			const uint32_t &input_index_num,
			bool if_adj_in = false,
			bool if_save_cpu_data_in = false
		);
		PancystarEngine::EngineFailReason GetModelData(
			std::vector<T> &vertex_data_in,
			std::vector<IndexType> &index_data_in
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
			VirtualResourcePointer &geometry_vertex_buffer,
			VirtualResourcePointer &geometry_index_buffer,
			VirtualResourcePointer &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_adj_buffer_view_in
		);
	};
	template<typename T>
	GeometryCommonModel<T>::GeometryCommonModel(
		const T *vertex_data_in,
		const IndexType *index_data_in,
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
			index_data = new IndexType[input_index_num];
			memcpy(index_data, index_data_in, input_index_num * sizeof(IndexType));
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
		VirtualResourcePointer &geometry_vertex_buffer_in,
		VirtualResourcePointer &geometry_index_buffer_in,
		VirtualResourcePointer &geometry_adjindex_buffer_in,
		D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
		D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in,
		D3D12_INDEX_BUFFER_VIEW &geometry_index_adj_buffer_view_in
	) 
	{
		PancystarEngine::EngineFailReason check_error;
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
		PancyCommonBufferDesc buffer_resource_desc;
		buffer_resource_desc.buffer_type = Buffer_Vertex;
		buffer_resource_desc.buffer_res_desc.Alignment = 0;
		buffer_resource_desc.buffer_res_desc.DepthOrArraySize = 1;
		buffer_resource_desc.buffer_res_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		buffer_resource_desc.buffer_res_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		buffer_resource_desc.buffer_res_desc.Height = 1;
		buffer_resource_desc.buffer_res_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		buffer_resource_desc.buffer_res_desc.MipLevels = 1;
		buffer_resource_desc.buffer_res_desc.SampleDesc.Count = 1;
		buffer_resource_desc.buffer_res_desc.SampleDesc.Quality = 0;
		buffer_resource_desc.buffer_res_desc.Width = VertexBufferSize;
		auto check_error = BuildBufferResource(
			"vertex_buffer",
			buffer_resource_desc,
			geometry_vertex_buffer_in,
			true
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//��������������
		buffer_resource_desc.buffer_type = Buffer_Index;
		buffer_resource_desc.buffer_res_desc.Width = IndexBufferSize;
		auto check_error = BuildBufferResource(
			"index_buffer",
			buffer_resource_desc,
			geometry_index_buffer_in,
			true
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//todo �ڽ����������
		if (if_model_adj)
		{
		}
		//��ȡ���ڿ�����commond list
		PancyRenderCommandList *copy_render_list;
		PancyThreadIdGPU copy_render_list_ID;
		check_error = ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->GetEmptyRenderlist(NULL, &copy_render_list, copy_render_list_ID);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//������Դ����(���㻺����)
		ResourceBlockGpu* vertex_buffer_gpu_resource = GetBufferResourceData(geometry_vertex_buffer_in, check_error);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyDynamicRingBuffer::GetInstance()->CopyDataToGpu(copy_render_list, vertex_data, VertexBufferSize, *vertex_buffer_gpu_resource);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//������Դ����(����������)
		ResourceBlockGpu* index_buffer_gpu_resource = GetBufferResourceData(geometry_index_buffer_in, check_error);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyDynamicRingBuffer::GetInstance()->CopyDataToGpu(copy_render_list, index_data, IndexBufferSize, *index_buffer_gpu_resource);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		copy_render_list->UnlockPrepare();
		//�ύ��Ⱦ����
		ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SubmitRenderlist(1, &copy_render_list_ID);
		//����ȴ���λ
		PancyFenceIdGPU WaitFence;
		ThreadPoolGPUControl::GetInstance()->GetResourceLoadContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE_COPY)->SetGpuBrokenFence(WaitFence);
		check_error = vertex_buffer_gpu_resource->SetResourceCopyBrokenFence(WaitFence);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
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
		//�������㻺����ͼ
		check_error = vertex_buffer_gpu_resource->BuildVertexBufferView(0, VertexBufferSize,sizeof(T), geometry_vertex_buffer_view_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//��������������ͼ
		DXGI_FORMAT index_format;
		if (sizeof(IndexType) == sizeof(UINT))
		{
			index_format = DXGI_FORMAT_R32_UINT;
		}
		else if (sizeof(IndexType) == sizeof(uint16_t))
		{
			index_format = DXGI_FORMAT_R16_UINT;
		}
		check_error = index_buffer_gpu_resource->BuildIndexBufferView(0, IndexBufferSize, index_format, geometry_vertex_buffer_view_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		return PancystarEngine::succeed;
	}
	template<typename T>
	PancystarEngine::EngineFailReason GeometryCommonModel<T>::GetModelData(
		std::vector<T> &vertex_data_in,
		std::vector<IndexType> &index_data_in
	) 
	{
		vertex_data_in.clear();
		index_data_in.clear();
		if (!if_save_CPU_data) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"the model doesn't save the data to cpu");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("GeometryCommonModel<T>::GetModelData", error_message);
			return error_message;
		}
		for (int i = 0; i < all_vertex; ++i)
		{
			vertex_data_in.push_back(vertex_data[i]);
		}
		for (int i = 0; i < all_index; ++i)
		{
			index_data_in.push_back(index_data[i]);
		}
		return PancystarEngine::succeed;
	}
	class ModelResourceBasic 
	{
	};
}