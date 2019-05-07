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
		//SubMemoryPointer geometry_vertex_buffer;
		//SubMemoryPointer geometry_index_buffer;
		//SubMemoryPointer geometry_adjindex_buffer;
		pancy_object_id geometry_vertex_buffer;
		pancy_object_id geometry_index_buffer;
		pancy_object_id geometry_adjindex_buffer;
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
		/*
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
		*/
		inline uint32_t GetVetexNum()
		{
			return all_vertex;
		}
		inline uint32_t GetIndexNum()
		{
			return all_index;
		}
		inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()
		{
			return geometry_vertex_buffer_view;
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()
		{
			return geometry_index_buffer_view;
		};
		inline pancy_object_id GetVertexBufferID() 
		{
			return geometry_vertex_buffer;
		};
		PancystarEngine::EngineFailReason CheckGeometryState(ResourceStateType &now_state);
	protected:
		virtual PancystarEngine::EngineFailReason InitGeometryDesc(
			bool &if_create_adj
		) = 0;
		virtual PancystarEngine::EngineFailReason InitGeometry(
			uint32_t &all_vertex_need, 
			uint32_t &all_index_need, 
			uint32_t &all_index_adj_need,
			pancy_object_id &geometry_vertex_buffer,
			pancy_object_id &geometry_index_buffer,
			pancy_object_id &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
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
			pancy_object_id &geometry_vertex_buffer,
			pancy_object_id &geometry_index_buffer,
			pancy_object_id &geometry_adjindex_buffer,
			D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
			D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
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
		pancy_object_id &geometry_vertex_buffer_in,
		pancy_object_id &geometry_index_buffer_in,
		pancy_object_id &geometry_adjindex_buffer_in,
		D3D12_VERTEX_BUFFER_VIEW &geometry_vertex_buffer_view_in,
		D3D12_INDEX_BUFFER_VIEW &geometry_index_buffer_view_in
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
		std::string vertex_subresource_name;
		std::string index_subresource_name;
		check_error = PancyBasicBufferControl::GetInstance()->BuildBufferTypeJson(PancystarEngine::Buffer_Vertex, VertexBufferSize, vertex_subresource_name);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		Json::Value json_vertex;
		PancyJsonTool::GetInstance()->SetJsonValue(json_vertex, "BufferType", "Buffer_Vertex");
		PancyJsonTool::GetInstance()->SetJsonValue(json_vertex, "SubResourceFile", vertex_subresource_name);
		check_error = PancyBasicBufferControl::GetInstance()->LoadResource("VertexBuffer", json_vertex, geometry_vertex_buffer_in,true);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyBasicBufferControl::GetInstance()->CopyCpuResourceToGpu(geometry_vertex_buffer_in, vertex_data, VertexBufferSize,0);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//��������������
		check_error = PancyBasicBufferControl::GetInstance()->BuildBufferTypeJson(PancystarEngine::Buffer_Index, IndexBufferSize, index_subresource_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		Json::Value json_index;
		PancyJsonTool::GetInstance()->SetJsonValue(json_index, "BufferType", "Buffer_Index");
		PancyJsonTool::GetInstance()->SetJsonValue(json_index, "SubResourceFile", index_subresource_name);
		check_error = PancyBasicBufferControl::GetInstance()->LoadResource("IndexBuffer", json_index, geometry_index_buffer_in, true);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyBasicBufferControl::GetInstance()->CopyCpuResourceToGpu(geometry_index_buffer_in, index_data, IndexBufferSize, 0);
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
		//todo �ڽ����������
		if (if_model_adj) 
		{
		}
		//�������㻺����ͼ
		SubMemoryPointer vertex_buffer_res;
		check_error = PancyBasicBufferControl::GetInstance()->GetBufferSubResource(geometry_vertex_buffer_in, vertex_buffer_res);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = SubresourceControl::GetInstance()->BuildVertexBufferView(vertex_buffer_res, sizeof(T), geometry_vertex_buffer_view_in);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//��������������ͼ
		SubMemoryPointer index_buffer_res;
		DXGI_FORMAT index_format;
		check_error = PancyBasicBufferControl::GetInstance()->GetBufferSubResource(geometry_index_buffer_in, index_buffer_res);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if (sizeof(IndexType) == sizeof(UINT))
		{
			index_format = DXGI_FORMAT_R32_UINT;
		}
		else if (sizeof(IndexType) == sizeof(uint16_t))
		{
			index_format = DXGI_FORMAT_R16_UINT;
		}
		check_error = SubresourceControl::GetInstance()->BuildIndexBufferView(index_buffer_res, index_format, geometry_index_buffer_view_in);
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
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get model point data", error_message);
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