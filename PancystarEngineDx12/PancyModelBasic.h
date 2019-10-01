#pragma once
#include"PancyTextureDx12.h"
#include"PancyGeometryDx12.h"
#include"PancyShaderDx12.h"
namespace PancystarEngine
{
#define MaxBoneNum 100
#define NouseAssimpStruct -12138
#define VertexAnimationID uint32_t
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
	struct skin_tree
	{
		char bone_ID[128];
		int bone_number;
		DirectX::XMFLOAT4X4 basic_matrix;
		DirectX::XMFLOAT4X4 animation_matrix;
		DirectX::XMFLOAT4X4 now_matrix;
		skin_tree *brother;
		skin_tree *son;
		skin_tree()
		{
			bone_ID[0] = '\0';
			bone_number = NouseAssimpStruct;
			brother = NULL;
			son = NULL;
			DirectX::XMStoreFloat4x4(&basic_matrix, DirectX::XMMatrixIdentity());
			DirectX::XMStoreFloat4x4(&animation_matrix, DirectX::XMMatrixIdentity());
			DirectX::XMStoreFloat4x4(&now_matrix, DirectX::XMMatrixIdentity());
		}
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
	
	class PancySubModel
	{
		PancystarEngine::GeometryBasic *model_mesh;
		pancy_object_id material_use;
	public:
		PancySubModel();
		~PancySubModel();
		template<typename T>
		PancystarEngine::EngineFailReason Create(const T* vertex_need, const IndexType* index_need, const int32_t &vert_num, const int32_t &index_num, const pancy_object_id& material_id)
		{
			model_mesh = new PancystarEngine::GeometryCommonModel<T>(vertex_need, index_need, vert_num, index_num, false, true);
			auto check_error = model_mesh->Create();
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			material_use = material_id;
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
		inline pancy_object_id GetMaterial()
		{
			return material_use;
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
		inline PancystarEngine::EngineFailReason GetLoadState(ResourceStateType &load_state)
		{
			return model_mesh->CheckGeometryState(load_state);
		}
		inline pancy_object_id GetVertexBuffer() 
		{
			return model_mesh->GetVertexBufferID();
		};
	};
	//����ģ��
	class PancyBasicModel : public PancyBasicVirtualResource
	{
		//ģ�͵�������Ϣ
		std::vector<PancySubModel*> model_resource_list;     //ģ�͵�ÿ���Ӳ���
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> material_list;
		std::unordered_map<pancy_object_id, std::vector<pancy_object_id>> material_id_list;
		std::vector<pancy_object_id> texture_list;
		//ģ�͵Ķ�����Ϣ
		bool if_skinmesh;
		bool if_pointmesh;
		//ģ�͵�pbr��ʽ
		PbrMaterialType model_pbr_type;
		//ģ�͵İ�Χ�Լ��α���Ϣ
		BoundingData model_size;
		DirectX::XMFLOAT4X4 model_translation;
		PancystarEngine::GeometryBasic *model_boundbox;
		//����������Ϣ
		int32_t root_id;
		std::unordered_map<std::string,pancy_object_id> bone_name_index;//���ݹ���������������ID
		std::unordered_map<pancy_object_id, bone_struct> bone_tree_data;//��������
		int bone_num;//������Ƥ�Ĺ�������
		int bone_object_num;//���еĹ�������(������������Ƥ�Ĳ���)
		DirectX::XMFLOAT4X4 offset_matrix_array[MaxBoneNum];
		std::unordered_map<std::string, pancy_resource_id> skin_animation_name;
		std::unordered_map<pancy_resource_id, animation_set> skin_animation_map;
		float now_animation_play_station;//��ǰ���ڲ��ŵĶ���
		DirectX::XMFLOAT4X4 bind_pose_matrix;//����ģ��λ�õĸ�����ƫ�ƾ���
		skin_tree *model_move_skin;//��ǰ����ģ��λ�õĸ�����
		bool if_animation_choose;
		//���㶯����Ϣ
		SubMemoryPointer vertex_anim_buffer;
		PancyFenceIdGPU upload_fence_value;
		uint32_t mesh_animation_buffer_size;
		uint32_t mesh_animation_ID_buffer_size;
		uint32_t all_frame_num;
		pancy_object_id point_animation_id_self_add=0;
		std::unordered_map<std::string, pancy_object_id> Point_animation_name;
		std::unordered_map<pancy_object_id, pancy_object_id> Point_animation_buffer;
		std::unordered_map<pancy_object_id, pancy_object_id> Point_animation_id_buffer;
		//�ļ���ȡ��
		ifstream instream;
	public:
		PancyBasicModel(const std::string &resource_name,const Json::Value &root_value);
		//��ȡ��Ⱦ����
		inline pancy_object_id GetSubModelNum()
		{
			return model_resource_list.size();
		};
		inline PancystarEngine::EngineFailReason GetRenderMesh(const pancy_object_id &submesh_id, PancySubModel **render_mesh)
		{
			if (submesh_id >= model_resource_list.size()) 
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "submesh id:" + std::to_string(submesh_id) + " bigger than the submodel num:" + std::to_string(model_resource_list.size()) + " of model: " + resource_name);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find submesh from model ", error_message);
				return error_message;
			}
			*render_mesh = model_resource_list[submesh_id];
			return PancystarEngine::succeed;
		}
		//��ȡ������Ϣ
		inline size_t GetMaterialNum()
		{
			return material_list.size();
		}
		inline PancystarEngine::EngineFailReason GetMateriaTexture(const pancy_object_id &material_id, const TexType &texture_type, pancy_object_id &texture_id)
		{
			if (material_id > material_list.size())
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "material id:" + std::to_string(material_id) + " bigger than the submodel num:" + std::to_string(model_resource_list.size()) + " of model: " + resource_name);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find texture from model ", error_message);
				return error_message;
			}
			auto material_data = material_list.find(material_id);
			auto texture_data = material_data->second.find(texture_type);
			if (texture_data == material_data->second.end())
			{
				PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find the texture id:" + std::to_string(texture_type) + " in material id:" + std::to_string(material_id) + "in model " + resource_name);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find texture from model ", error_message);
				return error_message;
			}
			texture_id = texture_list[texture_data->second];
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
		//��ȡģ�͵�pbr��Ϣ
		inline PbrMaterialType GetModelPbrDesc()
		{
			return model_pbr_type;
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
		/*
		//��ȡ���㶯���Ļ�����
		inline SubMemoryPointer GetPointAnimationBuffer(int32_t &buffer_size_in, int32_t &stride_size_in)
		{
			buffer_size_in = buffer_size;
			stride_size_in = sizeof(mesh_animation_data);
			return vertex_anim_buffer;
		}
		//��ȡ���㶯����֡����
		inline void GetPointAnimationFrame(const float &animation_time,uint32_t &now_frame, uint32_t &perframe_size_in)
		{
			perframe_size_in = perframe_size;
			now_frame = now_animation_play_station * all_frame_num;
		}
		*/
		//��ȡ��Ⱦ��������ÿ������Ķ�����Դ
		PancystarEngine::EngineFailReason GetShaderResourcePerObject(
			std::vector<SubMemoryPointer> &resource_data_per_frame_out,
			std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_out
		);
		virtual ~PancyBasicModel();
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state);
		void CheckIfResourceLoadToGpu(ResourceStateType &now_res_state);
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
			PancySubModel *new_submodel = new PancySubModel();
			check_error = new_submodel->Create(vertex_data, index_data, vertex_num, index_num, 0);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			model_resource_list.push_back(new_submodel);
			delete[] vertex_data;
			delete[] index_data;
			return PancystarEngine::succeed;
		}
	};
	//ģ�͹�����
	class PancyModelControl : public PancystarEngine::PancyBasicResourceControl 
	{
	private:
		PancyModelControl(const std::string &resource_type_name_in);
	public:
		static PancyModelControl* GetInstance()
		{
			static PancyModelControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyModelControl("Model Resource Control");
			}
			return this_instance;
		}
		PancystarEngine::EngineFailReason GetRenderMesh(const pancy_object_id &model_id, const pancy_object_id &submesh_id, PancySubModel **render_mesh);
		PancystarEngine::EngineFailReason GetShaderResourcePerObject(
			const pancy_object_id &model_id,
			std::vector<SubMemoryPointer> &resource_data_per_frame_out,
			std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_out
		);
		PancystarEngine::EngineFailReason GetModelBoneMatrix(
			const pancy_object_id &model_id,
			const pancy_resource_id &animation_ID,
			const float &animation_time,
			std::vector<DirectX::XMFLOAT4X4> &matrix_out
		);
	private:
		PancystarEngine::EngineFailReason BuildResource(
			const Json::Value &root_value,
			const std::string &name_resource_in,
			PancyBasicVirtualResource** resource_out
		);
		
	};
	
}