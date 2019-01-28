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
			ID3D12GraphicsCommandList* cmdList,
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
		//��ȡ��������
		template<typename T>
		PancystarEngine::EngineFailReason LoadMeshData(const std::string &file_name) 
		{
			instream.open(file_name, ios::binary);
			int32_t vertex_num;
			int32_t index_num;
			instream.read(reinterpret_cast<char*>(&vertex_num), sizeof(vertex_num));
			instream.read(reinterpret_cast<char*>(&index_num), sizeof(index_num));

			T *vertex_data = new T[vertex_num];
			IndexType *index_data = new IndexType[index_num];
			int32_t vertex_size = vertex_num * sizeof(vertex_data[0]);
			int32_t index_size = vertex_num * sizeof(index_data[0]);
			instream.read(reinterpret_cast<char*>(vertex_data), vertex_size);
			instream.read(reinterpret_cast<char*>(index_data), index_size);
			PancySubModel *new_submodel = new PancySubModel();
			check_error = new_submodel->Create(vertex_data, index_data, vertex_num, index_num, 0);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			model_resource_list.push_back(new_submodel);
			delete[] vertex_data;
			delete[] index_data;
			instream.close();
		}
	};
	PancyBasicModel::PancyBasicModel(const std::string &desc_file_in) : PancyBasicVirtualResource(desc_file_in)
	{
	}
	PancyBasicModel::~PancyBasicModel() 
	{
		for (auto data_submodel = model_resource_list.begin(); data_submodel != model_resource_list.end(); ++data_submodel)
		{
			delete *data_submodel;
		}
		model_resource_list.clear();
		material_list.clear();
		for (auto id_tex = texture_list.begin(); id_tex != texture_list.end(); ++id_tex)
		{
			PancystarEngine::PancyTextureControl::GetInstance()->DeleteResurceReference(*id_tex);
		}
	}
	PancystarEngine::EngineFailReason PancyBasicModel::LoadSkinTree(string filename)
	{
		instream.open(filename, ios::binary);
		if (!instream.is_open())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "open file " + filename + " error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Model From File", error_message);
			return error_message;
		}
		//��ȡƫ�ƾ���
		int bone_num_need;
		instream.read(reinterpret_cast<char*>(&bone_num), sizeof(bone_num));
		instream.read(reinterpret_cast<char*>(offset_matrix_array), bone_num * sizeof(DirectX::XMFLOAT4X4));
		//�ȶ�ȡ��һ����ջ��
		char data[11];
		instream.read(reinterpret_cast<char*>(data), sizeof(data));
		root_skin = new skin_tree();
		//�ݹ��ؽ�������
		ReadBoneTree(root_skin);
		//�ر��ļ�
		instream.close();
		return PancystarEngine::succeed;
	}
	void PancyBasicModel::ReadBoneTree(skin_tree *now)
	{
		char data[11];
		instream.read(reinterpret_cast<char*>(now), sizeof(*now));
		now->brother = NULL;
		now->son = NULL;
		instream.read(data, sizeof(data));
		while (strcmp(data, "*heaphead*") == 0)
		{
			//��ջ���ţ������ӽڵ�
			skin_tree *now_point = new skin_tree();
			ReadBoneTree(now_point);
			now_point->brother = now->son;
			now->son = now_point;
			instream.read(data, sizeof(data));
		}

	}
	PancystarEngine::EngineFailReason PancyBasicModel::InitResource(const std::string &resource_desc_file) 
	{
		PancystarEngine::EngineFailReason check_error;
		std::string path_name = "";
		std::string file_name = "";
		std::string tile_name = "";
		DivideFilePath(resource_desc_file, path_name, file_name, tile_name);
		pancy_json_value rec_value;
		Json::Value root_value;
		check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_desc_file, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//�Ƿ������������
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfHaveSkinAnimation", pancy_json_data_type::json_data_bool, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if_skinmesh = rec_value.bool_value;
		//�Ƿ�������㶯��
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "IfHavePoinAnimation", pancy_json_data_type::json_data_bool, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		if_skinmesh = rec_value.bool_value;
		//ģ�͵�pbr����
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "PbrType", pancy_json_data_type::json_data_enum, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		model_pbr_type = static_cast<PbrMaterialType>(rec_value.int_value);
		//��ȡģ�͵���������
		int32_t model_part_num;
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "model_num", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		model_part_num = rec_value.int_value;
		for (int i = 0; i < model_part_num; ++i) 
		{
			std::string model_vertex_data_name = path_name + file_name + std::to_string(i) + ".vertex";
			if (if_skinmesh) 
			{
				check_error = LoadMeshData<PancystarEngine::PointSkinCommon8>(model_vertex_data_name);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
			}
			else if (if_pointmesh) 
			{
				check_error = LoadMeshData<PancystarEngine::PointCatchCommon>(model_vertex_data_name);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
			}
			else 
			{
				check_error = LoadMeshData<PancystarEngine::PointCommon>(model_vertex_data_name);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
			}
		}
		//��ȡģ�͵���������
		int32_t model_texture_num;
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, root_value, "texture_num", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		model_texture_num = rec_value.int_value;
		for (int i = 0; i < model_texture_num; ++i) 
		{
			std::string texture_name_now = path_name + file_name+"_tex" + std::to_string(i) + ".json";
			pancy_object_id texture_id;
			//��������������ü���
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(texture_name_now, texture_id);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			check_error = PancystarEngine::PancyTextureControl::GetInstance()->AddResurceReference(texture_id);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			texture_list.push_back(texture_id);
		}
		//��ȡģ�͵Ĳ�������
		Json::Value material_value = root_value.get("material", Json::Value::null);
		for (int i = 0; i < material_value.size(); ++i) 
		{
			std::unordered_map<TexType, pancy_object_id> now_material_need;
			int32_t material_id;
			//����id
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "materialID", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			material_id = rec_value.int_value;
			//����������
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "Albedotex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse, rec_value.int_value));
			//��������
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "Normaltex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, rec_value.int_value));
			//AO����
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "Ambienttex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, rec_value.int_value));
			//PBR����
			if (model_pbr_type == PbrMaterialType::PbrType_MetallicRoughness) 
			{
				//����������
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "MetallicTex", pancy_json_data_type::json_data_int, rec_value);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_metallic, rec_value.int_value));
				//�ֲڶ�����
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "RoughnessTex", pancy_json_data_type::json_data_int, rec_value);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_roughness, rec_value.int_value));
			}
			else if (model_pbr_type == PbrMaterialType::PbrType_SpecularSmoothness) 
			{
				//�����&ƽ��������
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, material_value[i], "SpecularSmoothTex", pancy_json_data_type::json_data_int, rec_value);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_specular_smoothness, rec_value.int_value));
			}
		}
		//��ȡ��������
		if (if_skinmesh) 
		{
			std::string bone_data_name = path_name + file_name + ".bone";
			//��ȡ������Ϣ
			check_error = LoadSkinTree(bone_data_name);
			if (!check_error.CheckIfSucceed()) 
			{
				return check_error;
			}
			//��ȡ������Ϣ
			Json::Value skin_animation_value = root_value.get("SkinAnimation", Json::Value::null);
			for (int i = 0; i < skin_animation_value.size(); ++i) 
			{
				animation_set new_animation;
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, skin_animation_value, i, pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				std::string now_animation_name = path_name + rec_value.string_value;
				instream.open(now_animation_name);
				int32_t animation_bone_num;
				instream.read(reinterpret_cast<char*>(&animation_bone_num),sizeof(animation_bone_num));
				for (int i = 0; i < animation_bone_num; ++i) 
				{
					animation_data new_bone_data;
					//������Ϣ
					int32_t bone_name_size;
					instream.read(reinterpret_cast<char*>(&bone_name_size), sizeof(bone_name_size));
					char *name = new char[bone_name_size];
					instream.read(name, bone_name_size * sizeof(char));
					new_bone_data.bone_name += name;
					delete[] name;
					//��ת����
					int32_t rotation_key_num;
					instream.read(reinterpret_cast<char*>(&rotation_key_num), sizeof(rotation_key_num));
					quaternion_animation *new_rotation_key = new quaternion_animation[rotation_key_num];
					int32_t rotation_key_size = sizeof(new_rotation_key[0]) * rotation_key_num;
					instream.read(reinterpret_cast<char*>(new_rotation_key), rotation_key_size);
					for (int j = 0; j < rotation_key_num; ++j) 
					{
						new_bone_data.rotation_key.push_back(new_rotation_key[i]);
					}
					//ƽ������
					int32_t translation_key_num;
					instream.read(reinterpret_cast<char*>(&translation_key_num), sizeof(translation_key_num));
					vector_animation *new_translation_key = new vector_animation[translation_key_num];
					int32_t translation_key_size = sizeof(new_translation_key[0]) * translation_key_num;
					instream.read(reinterpret_cast<char*>(new_translation_key), translation_key_size);
					for (int j = 0; j < translation_key_num; ++j)
					{
						new_bone_data.translation_key.push_back(new_translation_key[i]);
					}
					//��������
					int32_t scaling_key_num;
					instream.read(reinterpret_cast<char*>(&scaling_key_num), sizeof(scaling_key_num));
					vector_animation *new_scaling_key = new vector_animation[scaling_key_num];
					int32_t scaling_key_size = sizeof(new_scaling_key[0]) * scaling_key_num;
					instream.read(reinterpret_cast<char*>(new_scaling_key), scaling_key_size);
					for (int j = 0; j < scaling_key_num; ++j)
					{
						new_bone_data.scaling_key.push_back(new_scaling_key[i]);
					}
					new_animation.data_animition.push_back(new_bone_data);
					//ɾ����ʱ����
					delete[] new_rotation_key;
					delete[] new_translation_key;
					delete[] new_scaling_key;
				}
				//��������Ϣ�����
				skin_animation_name.insert(std::pair<std::string, pancy_resource_id>(now_animation_name,i));
				skin_animation_map.insert(std::pair<pancy_resource_id, animation_set>(i, new_animation));
				instream.close();
			}
		}
		//��ȡ���㶯��
		if (if_pointmesh) 
		{
			Json::Value point_animation_value = root_value.get("PointAnimation", Json::Value::null);
			for (int i = 0; i < point_animation_value.size(); ++i)
			{
				check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_desc_file, point_animation_value, i, pancy_json_data_type::json_data_string, rec_value);
				if (!check_error.CheckIfSucceed())
				{
					return check_error;
				}
				std::string now_animation_name = path_name + rec_value.string_value;
				instream.open(now_animation_name);
				instream.read(reinterpret_cast<char*>(&all_frame_num), sizeof(all_frame_num));
				instream.read(reinterpret_cast<char*>(&all_frame_num), sizeof(all_frame_num));
				instream.read(reinterpret_cast<char*>(&buffer_size), sizeof(buffer_size));
				instream.read(reinterpret_cast<char*>(&fps_point_catch), sizeof(fps_point_catch));
				int32_t size_need = buffer_size * sizeof(mesh_animation_data);
				mesh_animation_data *new_point_catch_data = new mesh_animation_data[buffer_size];
				instream.read(reinterpret_cast<char*>(new_point_catch_data), size_need);
				instream.close();
				/*
				��������
				*/
				delete[] new_point_catch_data;
			}
		}
	}
}