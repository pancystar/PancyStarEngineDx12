#pragma once
#include"PancyTextureDx12.h"
#include"PancyGeometryDx12.h"
namespace PancystarEngine
{
#define MaxBoneNum 100
#define NouseAssimpStruct -12138
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
		skin_tree *bone_point;                              //���α任���ݶ�Ӧ�Ĺ�����ָ��
		std::vector<vector_animation> translation_key;      //����ƽ�Ʊ任����
		std::vector<vector_animation> scaling_key;          //���������任����
		std::vector<quaternion_animation> rotation_key;     //������ת�任������
	};
	struct animation_set
	{
		float animation_length;                             //�����ĳ���
		std::vector<animation_data> data_animition;         //�ö���������
	};
	//���㶯������
	struct mesh_animation_data
	{
		DirectX::XMFLOAT3 position;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT3 tangent;
		mesh_animation_data()
		{
			position = DirectX::XMFLOAT3(0, 0, 0);
			normal = DirectX::XMFLOAT3(0, 0, 0);
			tangent = DirectX::XMFLOAT3(0, 0, 0);
		}
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
	};
	class PancyBasicModel : public PancyBasicVirtualResource
	{
		std::vector<PancySubModel*> model_resource_list;     //ģ�͵�ÿ���Ӳ���
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> material_list;
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
		skin_tree *root_skin;
		int bone_num;
		int root_bone_num = 0;
		DirectX::XMFLOAT4X4 bone_matrix_array[MaxBoneNum];
		DirectX::XMFLOAT4X4 offset_matrix_array[MaxBoneNum];
		DirectX::XMFLOAT4X4 final_matrix_array[MaxBoneNum];
		int tree_node_num[MaxBoneNum][MaxBoneNum];
		std::unordered_map<std::string, pancy_resource_id> skin_animation_name;
		std::unordered_map<pancy_resource_id, animation_set> skin_animation_map;
		float now_animation_play_station;//��ǰ���ڲ��ŵĶ���
		DirectX::XMFLOAT4X4 bind_pose_matrix;//����ģ��λ�õĸ�����ƫ�ƾ���
		skin_tree *model_move_skin;//��ǰ����ģ��λ�õĸ�����
		bool if_animation_choose;
		//���㶯����Ϣ
		SubMemoryPointer vertex_anim_buffer;
		PancyFenceIdGPU upload_fence_value;
		int32_t buffer_size;
		uint32_t perframe_size;
		uint32_t all_frame_num;
		uint32_t fps_point_catch;
		PancystarEngine::EngineFailReason BuildDefaultBuffer(
			PancyNowGraphicsCommandList* cmdList,
			int64_t memory_alignment_size,
			int64_t memory_block_alignment_size,
			SubMemoryPointer &default_buffer,
			SubMemoryPointer &upload_buffer,
			const void* initData,
			const UINT BufferSize,
			D3D12_RESOURCE_STATES buffer_type
		);
		//�ļ���ȡ��
		ifstream instream;
	public:
		PancyBasicModel(const std::string &desc_file_in);
		//��ȡ��Ⱦ����
		inline pancy_object_id GetSubModelNum()
		{
			return model_resource_list.size();
		};
		void GetRenderMesh(std::vector<PancySubModel*> &render_mesh);
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

		inline PancystarEngine::GeometryBasic *GetBoundBox()
		{
			return model_boundbox;
		}
		//��ȡָ��������ָ��ʱ��Ĺ��������Լ�����������ƫ�ƾ�������
		void GetBoneByAnimation(const pancy_resource_id &animation_name,const float &animation_time, DirectX::XMFLOAT4X4 *bone_matrix,DirectX::XMFLOAT4X4 &model_pos_matrix);
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
		
		virtual ~PancyBasicModel();
	private:
		PancystarEngine::EngineFailReason InitResource(const std::string &resource_desc_file);
		//��ȡ������
		PancystarEngine::EngineFailReason LoadSkinTree(string filename);
		void ReadBoneTree(skin_tree *now);
		void FreeBoneTree(skin_tree *now);
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
			int32_t index_size = vertex_num * sizeof(index_data[0]);
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
	private:
		PancystarEngine::EngineFailReason BuildResource(const std::string &desc_file_in, PancyBasicVirtualResource** resource_out);
	};
	
}