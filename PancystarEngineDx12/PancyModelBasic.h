#pragma once
#include"PancyTextureDx12.h"
#include"PancyGeometryDx12.h"
#include"PancyShaderDx12.h"
namespace PancystarEngine
{
#define MaxBoneNum 100
#define NouseBoneStruct -12138
#define VertexAnimationID uint32_t
//GPU�����������ĵ�������
#define MaxSkinDeSampleTime 8
	enum TexType
	{
		tex_diffuse = 0,
		tex_normal,
		tex_metallic_roughness,
		tex_metallic,
		tex_roughness,
		tex_specular_smoothness,
		tex_ambient,
		tex_subsurface_color,
		tex_subsurface_value,
	};
	
	struct BoundingData
	{
		DirectX::XMFLOAT3 min_box_pos;
		DirectX::XMFLOAT3 max_box_pos;
	};
	struct bone_struct 
	{
		std::string bone_name;
		int32_t bone_ID_brother;
		int32_t bone_ID_son;
		bool if_used_for_skin;
		bone_struct() 
		{
			bone_name = "";
			bone_ID_brother = 99999999;
			bone_ID_son     = 99999999;
			if_used_for_skin = false;
		}
	};
	enum PbrMaterialType
	{
		PbrType_MetallicRoughness = 0,
		PbrType_SpecularSmoothness
	};
	//�任����
	struct vector_animation
	{
		float time;               //֡ʱ��
		float main_key[3];        //֡����
	};
	//�任��Ԫ��
	struct quaternion_animation
	{
		float time;               //֡ʱ��
		float main_key[4];        //֡����
	};
	struct animation_data
	{
		std::string bone_name;                              //���α任���ݶ�Ӧ�Ĺ�������
		int32_t bone_ID;                                    //���α任���ݶ�Ӧ�Ĺ������
		std::vector<vector_animation> translation_key;      //����ƽ�Ʊ任����
		std::vector<vector_animation> scaling_key;          //���������任����
		std::vector<quaternion_animation> rotation_key;     //������ת�任������
	};
	struct animation_set
	{
		float animation_length;                             //�����ĳ���
		std::vector<animation_data> data_animition;         //�ö���������
	};
	
	enum PancyRenderMeshVertexType
	{
		PancyMeshVertexCommon = 0,
		PancyMeshVertexSkin,
		PancyMeshVertexPointCatch
	};
	struct AnimationNodeData 
	{
		DirectX::XMFLOAT4 translation_key;
		DirectX::XMFLOAT4 rotation_key;
		DirectX::XMFLOAT4 scaling_key;
	};
	class PancyRenderMesh
	{
		//����ĸ�ʽ����
		PancyRenderMeshVertexType RenderType;
		//ģ����������
		PancystarEngine::GeometryBasic *model_mesh;
		//todo:��Χ����Ϣ
		BoundingData mesh_bound;
	public:
		PancyRenderMesh();
		~PancyRenderMesh();
		template<typename T>
		PancystarEngine::EngineFailReason Create(const T* vertex_need, const IndexType* index_need, const int32_t &vert_num, const int32_t &index_num)
		{
			model_mesh = new PancystarEngine::GeometryCommonModel<T>(vertex_need, index_need, vert_num, index_num, false, true);
			auto check_error = model_mesh->Create();
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			return PancystarEngine::succeed;
		}
		template<typename T>
		inline PancystarEngine::EngineFailReason GetSubModelData(
			std::vector<T> &vertex_data_in,
			std::vector<IndexType> &index_data_in
		)
		{
			PancystarEngine::GeometryCommonModel<T> *model_real = dynamic_cast<PancystarEngine::GeometryCommonModel<T> *>(model_mesh);
			return model_real->GetModelData(vertex_data_in, index_data_in);
		}
		inline D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView()
		{
			return model_mesh->GetVertexBufferView();
		};
		inline D3D12_INDEX_BUFFER_VIEW GetIndexBufferView()
		{
			return model_mesh->GetIndexBufferView();
		};
		inline int32_t GetVertexNum()
		{
			return model_mesh->GetVetexNum();
		}
		inline int32_t GetIndexNum()
		{
			return model_mesh->GetIndexNum();
		}
		inline bool CheckIfLoadSucceed()
		{
			return model_mesh->CheckIfCreateSucceed();
		}
		inline VirtualResourcePointer& GetVertexBuffer()
		{
			return model_mesh->GetVertexBufferResource();
		};
	};
	//����ģ��
	class PancyBasicModel
	{
		//ģ�͵�������Ϣ
		std::vector<PancyRenderMesh*> model_resource_list;     //ģ�͵�ÿ���Ӳ���
		//ģ�͵Ķ�����Ϣ
		bool if_skinmesh;
		bool if_pointmesh;
		//ģ�͵İ�Χ�Լ��α���Ϣ
		BoundingData model_size;
		DirectX::XMFLOAT4X4 model_translation;
		PancystarEngine::GeometryBasic *model_boundbox;
		//����������Ϣ
		int32_t root_id;
		std::unordered_map<std::string,pancy_object_id> bone_name_index;//���ݹ���������������ID
		std::unordered_map<pancy_object_id, bone_struct> bone_tree_data;//��������
		std::unordered_map<pancy_object_id, pancy_object_id> bone_parent_data;
		int bone_num;//������Ƥ�Ĺ�������
		int bone_object_num;//���еĹ�������(������������Ƥ�Ĳ���)
		DirectX::XMFLOAT4X4 offset_matrix_array[MaxBoneNum];
		std::unordered_map<std::string, pancy_resource_id> skin_animation_name;
		std::unordered_map<pancy_resource_id, animation_set> skin_animation_map;
		float now_animation_play_station;//��ǰ���ڲ��ŵĶ���
		bool if_animation_choose;
		//�ļ���ȡ��
		ifstream instream;
		//��������buffer
		VirtualResourcePointer model_animation_buffer;
		VirtualResourcePointer model_bonetree_buffer;
		VirtualResourcePointer model_boneoffset_buffer;
	public:
		PancyBasicModel();
		PancystarEngine::EngineFailReason Create(const std::string &resource_name);
		//��ȡ��Ⱦ����
		inline pancy_object_id GetSubModelNum()
		{
			return static_cast<pancy_object_id>(model_resource_list.size());
		};
		inline PancystarEngine::EngineFailReason GetRenderMesh(const pancy_object_id &submesh_id, PancyRenderMesh **render_mesh)
		{
			if (submesh_id >= model_resource_list.size()) 
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "submesh id:" + std::to_string(submesh_id) + " bigger than the submodel num:" + std::to_string(model_resource_list.size()) + " of model: ");
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find submesh from model ", error_message);
				return error_message;
			}
			*render_mesh = model_resource_list[submesh_id];
			return PancystarEngine::succeed;
		}
		//���鶯����Ϣ
		inline bool CheckIfSkinMesh()
		{
			return if_skinmesh;
		}
		inline bool CheckIfPointMesh()
		{
			return if_pointmesh;
		}
		//��ȡ��Χ����Ϣ
		inline PancystarEngine::GeometryBasic *GetBoundBox()
		{
			return model_boundbox;
		}
		//��ȡָ��������ָ��ʱ��Ĺ��������Լ�����������ƫ�ƾ�������
		PancystarEngine::EngineFailReason GetBoneByAnimation(
			const pancy_resource_id &animation_ID,
			const float &animation_time, 
			std::vector<DirectX::XMFLOAT4X4> &matrix_out
		);
		virtual ~PancyBasicModel();
		bool CheckIfLoadSucceed();
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name);
		//��ȡ������
		PancystarEngine::EngineFailReason LoadSkinTree(const string &filename);
		PancystarEngine::EngineFailReason ReadBoneTree(int32_t &now_build_id);
		//���¹����Ķ�������
		PancystarEngine::EngineFailReason UpdateAnimData(
			const pancy_resource_id &animation_ID,
			const float &time_in,
			std::vector<DirectX::XMFLOAT4X4> &matrix_out
		);
		//֡���ֵ(��Ԫ��)
		void Interpolate(quaternion_animation& pOut, const quaternion_animation &pStart, const quaternion_animation &pEnd, const float &pFactor);
		//֡���ֵ(����)
		void Interpolate(vector_animation& pOut, const vector_animation &pStart, const vector_animation &pEnd, const float &pFactor);
		//����ָ����ʱ�䣬��ȡ��ǰ�������ؼ�֡����(��Ԫ��)
		void FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<quaternion_animation> &input);
		//����ָ����ʱ�䣬��ȡ��ǰ�������ؼ�֡����(����)
		void FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<vector_animation> &input);
		//������Ԫ����ȡ�任����
		void GetQuatMatrix(DirectX::XMFLOAT4X4 &resMatrix, const quaternion_animation& pOut);
		PancystarEngine::EngineFailReason UpdateRoot(
			int32_t root_id,
			const DirectX::XMFLOAT4X4 &matrix_parent,
			const std::vector<DirectX::XMFLOAT4X4> &matrix_animation,
			std::vector<DirectX::XMFLOAT4X4> &matrix_combine_save,
			std::vector<DirectX::XMFLOAT4X4> &matrix_out
		);
		//��ȡ��������
		template<typename T>
		PancystarEngine::EngineFailReason LoadMeshData(const std::string &file_name_vertex, const std::string &file_name_index)
		{
			PancystarEngine::EngineFailReason check_error;
			
			int32_t vertex_num;
			int32_t index_num;
			
			
			instream.open(file_name_vertex, ios::binary);
			instream.read(reinterpret_cast<char*>(&vertex_num), sizeof(vertex_num));
			T *vertex_data = new T[vertex_num];
			int32_t vertex_size = vertex_num * sizeof(vertex_data[0]);
			instream.read(reinterpret_cast<char*>(vertex_data), vertex_size);
			instream.close();

			instream.open(file_name_index, ios::binary);
			instream.read(reinterpret_cast<char*>(&index_num), sizeof(index_num));
			IndexType *index_data = new IndexType[index_num];
			int32_t index_size = index_num * sizeof(index_data[0]);
			
			instream.read(reinterpret_cast<char*>(index_data), index_size);
			instream.close();
			PancyRenderMesh *new_submodel = new PancyRenderMesh();
			check_error = new_submodel->Create(vertex_data, index_data, vertex_num, index_num);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			model_resource_list.push_back(new_submodel);
			delete[] vertex_data;
			delete[] index_data;
			return PancystarEngine::succeed;
		}
		//���ݲ㼶��ȡ���ڵ�
		pancy_object_id FindParentByLayer(
			const pancy_object_id& bone_id,
			const pancy_object_id& layer,
			const std::vector<uint32_t> &bone_node_parent_data
		);
	};
}