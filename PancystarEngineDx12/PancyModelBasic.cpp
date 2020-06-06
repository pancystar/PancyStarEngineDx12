#include"PancyModelBasic.h"
using namespace PancystarEngine;
//ģ�Ͳ���
PancyRenderMesh::PancyRenderMesh()
{
	model_mesh = NULL;
}
PancyRenderMesh::~PancyRenderMesh()
{
	if (model_mesh != NULL)
	{
		delete model_mesh;
	}
}
//ģ����
PancyBasicModel::PancyBasicModel()
{
}
PancystarEngine::EngineFailReason PancyBasicModel::Create(const std::string &resource_name)
{
	Json::Value root_value;
	auto check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_name, root_value);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	check_error = InitResource(root_value, resource_name);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancyBasicModel::~PancyBasicModel()
{
	for (auto data_submodel = model_resource_list.begin(); data_submodel != model_resource_list.end(); ++data_submodel)
	{
		delete *data_submodel;
	}
	model_resource_list.clear();
}
//��������������
PancystarEngine::EngineFailReason PancyBasicModel::LoadSkinTree(const string &filename)
{
	instream.open(filename, ios::binary);
	if (!instream.is_open())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "open file " + filename + " error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Model From File", error_message);
		return error_message;
	}
	//��ȡƫ�ƾ���
	instream.read(reinterpret_cast<char*>(&bone_num), sizeof(bone_num));
	instream.read(reinterpret_cast<char*>(offset_matrix_array), bone_num * sizeof(DirectX::XMFLOAT4X4));
	//�ȶ�ȡ��һ����ջ��
	char data[11];
	instream.read(reinterpret_cast<char*>(data), sizeof(data));
	//�ݹ��ؽ�������
	bone_object_num = bone_num;
	auto check_error = ReadBoneTree(root_id);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	bone_parent_data[root_id] = 0;
	//�ر��ļ�
	instream.close();
	return PancystarEngine::succeed;
}
//todo::ȡ��ʹ������ṹ
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
		bone_number = NouseBoneStruct;
		brother = NULL;
		son = NULL;
		DirectX::XMStoreFloat4x4(&basic_matrix, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&animation_matrix, DirectX::XMMatrixIdentity());
		DirectX::XMStoreFloat4x4(&now_matrix, DirectX::XMMatrixIdentity());
	}
};
PancystarEngine::EngineFailReason PancyBasicModel::ReadBoneTree(int32_t &now_build_id)
{
	char data[11];
	skin_tree now_bone_data;
	instream.read(reinterpret_cast<char*>(&now_bone_data), sizeof(now_bone_data));
	//���ݶ�ȡ���Ĺ����ڵ㴴��һ�������ṹ��������
	bone_struct new_bone;
	bool if_used_skin = false;;
	if (now_bone_data.bone_number == NouseBoneStruct)
	{
		//��ǰ������δ������Ƥ��Ϣ,��������һ��ID��
		now_build_id = bone_object_num;
		bone_object_num += 1;
	}
	else
	{
		if_used_skin = true;
		now_build_id = now_bone_data.bone_number;
	}
	new_bone.bone_name = now_bone_data.bone_ID;
	new_bone.bone_ID_son = NouseBoneStruct;
	new_bone.bone_ID_brother = NouseBoneStruct;
	new_bone.if_used_for_skin = if_used_skin;
	bone_tree_data.insert(std::pair<int32_t, bone_struct>(now_build_id, new_bone));
	bone_name_index.insert(std::pair<std::string, int32_t>(new_bone.bone_name, now_build_id));
	instream.read(data, sizeof(data));
	while (strcmp(data, "*heaphead*") == 0)
	{
		//��ջ���ţ������ӽڵ�
		int32_t now_son_ID = NouseBoneStruct;
		//�ݹ鴴���ӽڵ�
		auto check_error = ReadBoneTree(now_son_ID);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		auto now_parent_data = bone_tree_data.find(now_build_id);
		auto now_data = bone_tree_data.find(now_son_ID);
		if (now_parent_data == bone_tree_data.end())
		{
			//����ĸ��ڵ㲻����
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Parent bone ID " + std::to_string(now_build_id));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Model Bone Data From File", error_message);
			return error_message;
		}
		else if (now_data == bone_tree_data.end())
		{
			//���ɵ��ӽڵ㲻����
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Son Bone ID " + std::to_string(now_son_ID));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Model Bone Data From File", error_message);
			return error_message;
		}
		else
		{
			//��ǰ�ڵ���ֵܽڵ�洢֮ǰ���ڵ���ӽڵ�
			now_data->second.bone_ID_brother = now_parent_data->second.bone_ID_son;
			//֮ǰ���ڵ���ӽڵ��Ϊ��ǰ�ڵ�
			now_parent_data->second.bone_ID_son = now_son_ID;
			//��¼��ǰ�ڵ�ĸ��ڵ�
			bone_parent_data[now_son_ID] = now_build_id+1;
			instream.read(data, sizeof(data));
		}
	}
	return PancystarEngine::succeed;
}
void PancyBasicModel::Interpolate(quaternion_animation& pOut, const quaternion_animation &pStart, const quaternion_animation &pEnd, const float &pFactor)
{
	float cosom = pStart.main_key[0] * pEnd.main_key[0] + pStart.main_key[1] * pEnd.main_key[1] + pStart.main_key[2] * pEnd.main_key[2] + pStart.main_key[3] * pEnd.main_key[3];
	quaternion_animation end = pEnd;
	if (cosom < static_cast<float>(0.0))
	{
		cosom = -cosom;
		end.main_key[0] = -end.main_key[0];
		end.main_key[1] = -end.main_key[1];
		end.main_key[2] = -end.main_key[2];
		end.main_key[3] = -end.main_key[3];
	}
	float sclp, sclq;
	if ((static_cast<float>(1.0) - cosom) > static_cast<float>(0.0001))
	{
		float omega, sinom;
		omega = acos(cosom);
		sinom = sin(omega);
		sclp = sin((static_cast<float>(1.0) - pFactor) * omega) / sinom;
		sclq = sin(pFactor * omega) / sinom;
	}
	else
	{
		sclp = static_cast<float>(1.0) - pFactor;
		sclq = pFactor;
	}

	pOut.main_key[0] = sclp * pStart.main_key[0] + sclq * end.main_key[0];
	pOut.main_key[1] = sclp * pStart.main_key[1] + sclq * end.main_key[1];
	pOut.main_key[2] = sclp * pStart.main_key[2] + sclq * end.main_key[2];
	pOut.main_key[3] = sclp * pStart.main_key[3] + sclq * end.main_key[3];
}
void PancyBasicModel::Interpolate(vector_animation& pOut, const vector_animation &pStart, const vector_animation &pEnd, const float &pFactor)
{
	for (int i = 0; i < 3; ++i)
	{
		pOut.main_key[i] = pStart.main_key[i] + pFactor * (pEnd.main_key[i] - pStart.main_key[i]);
	}
}
void PancyBasicModel::FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<quaternion_animation> &input)
{
	if (input_time < 0)
	{
		st = 0;
		ed = 0;
		return;
	}
	if (input_time > input[input.size() - 1].time)
	{
		st = static_cast<int>(input.size()) - 1;
		ed = static_cast<int>(input.size()) - 1;
		return;
	}
	for (int i = 0; i < input.size() - 1; ++i)
	{
		if (input_time >= input[i].time && input_time <= input[i + 1].time)
		{
			st = i;
			ed = i + 1;
			return;
		}
	}
	st = static_cast<int>(input.size()) - 1;
	ed = static_cast<int>(input.size()) - 1;
}
void PancyBasicModel::FindAnimStEd(const float &input_time, int &st, int &ed, const std::vector<vector_animation> &input)
{
	if (input_time < 0)
	{
		st = 0;
		ed = 0;
		return;
	}
	if (input_time > input[input.size() - 1].time)
	{
		st = static_cast<int>(input.size()) - 1;
		ed = static_cast<int>(input.size()) - 1;
		return;
	}
	for (int i = 0; i < input.size() - 1; ++i)
	{
		if (input_time >= input[i].time && input_time <= input[i + 1].time)
		{
			st = i;
			ed = i + 1;
			return;
		}
	}
	st = static_cast<int>(input.size()) - 1;
	ed = static_cast<int>(input.size()) - 1;
}
PancystarEngine::EngineFailReason PancyBasicModel::GetBoneByAnimation(
	const pancy_resource_id &animation_ID,
	const float &animation_time,
	std::vector<DirectX::XMFLOAT4X4> &bone_final_matrix
)
{
	PancystarEngine::EngineFailReason check_error;
	//���ж�ģ�������Ƿ�֧�ֹ�������
	if (!if_skinmesh)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "model: don't have skin mesh message");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicModel::GetBoneByAnimation", error_message);
		return error_message;
	}
	std::vector<DirectX::XMFLOAT4X4> matrix_animation_save;
	matrix_animation_save.resize(bone_object_num);
	for (int i = 0; i < bone_object_num; ++i)
	{
		//ʹ�õ�λ���󽫶����������
		DirectX::XMStoreFloat4x4(&matrix_animation_save[i], DirectX::XMMatrixIdentity());
	}
	check_error = UpdateAnimData(animation_ID, animation_time, matrix_animation_save);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	DirectX::XMFLOAT4X4 matrix_identi;
	DirectX::XMStoreFloat4x4(&matrix_identi, DirectX::XMMatrixIdentity());

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//����һ����ʱ�洢���й����ݹ����ľ������顣����������������ݽ���
	//����Ƥ�����������Ҫ����Ŀռ�洢����Ƥ�������м���Ϣ��
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	std::vector<DirectX::XMFLOAT4X4> matrix_combine_save;
	matrix_combine_save.resize(bone_object_num);
	for (int i = 0; i < bone_object_num; ++i)
	{
		DirectX::XMStoreFloat4x4(&matrix_combine_save[i], DirectX::XMMatrixIdentity());
	}
	std::vector<DirectX::XMFLOAT4X4> matrix_out_save;
	matrix_out_save.resize(bone_num);
	bone_final_matrix.resize(bone_num);
	UpdateRoot(root_id, matrix_identi, matrix_animation_save, matrix_combine_save, matrix_out_save);
	//�����º�Ķ���������ƫ��
	for (int i = 0; i < bone_num; ++i)
	{
		//ʹ�õ�λ���󽫻�Ͼ������
		DirectX::XMStoreFloat4x4(&bone_final_matrix[i], DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&offset_matrix_array[i]) * DirectX::XMLoadFloat4x4(&matrix_out_save[i])));
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicModel::UpdateAnimData(
	const pancy_resource_id &animation_ID,
	const float &time_in,
	std::vector<DirectX::XMFLOAT4X4> &matrix_out
)
{
	//���ݶ�����ID�Ų��Ҷ�Ӧ�Ķ�������
	auto skin_anim_data = skin_animation_map.find(animation_ID);
	if (skin_anim_data == skin_animation_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find skin_animation ID " + std::to_string(animation_ID));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update mesh animation", error_message);
		return error_message;
	}
	animation_set now_animation_use = skin_anim_data->second;
	float input_time = time_in * now_animation_use.animation_length;
	for (int i = 0; i < now_animation_use.data_animition.size(); ++i)
	{
		animation_data now = now_animation_use.data_animition[i];
		DirectX::XMMATRIX rec_trans, rec_scal;
		DirectX::XMFLOAT4X4 rec_rot;

		int start_anim, end_anim;
		FindAnimStEd(input_time, start_anim, end_anim, now.rotation_key);
		//��Ԫ����ֵ��Ѱ�ұ任����
		quaternion_animation rotation_now;
		if (start_anim == end_anim || end_anim >= now.rotation_key.size())
		{
			rotation_now = now.rotation_key[start_anim];
		}
		else
		{
			Interpolate(rotation_now, now.rotation_key[start_anim], now.rotation_key[end_anim], (input_time - now.rotation_key[start_anim].time) / (now.rotation_key[end_anim].time - now.rotation_key[start_anim].time));
		}
		GetQuatMatrix(rec_rot, rotation_now);
		//���ű任
		FindAnimStEd(input_time, start_anim, end_anim, now.scaling_key);
		vector_animation scalling_now;
		if (start_anim == end_anim)
		{
			scalling_now = now.scaling_key[start_anim];
		}
		else
		{
			Interpolate(scalling_now, now.scaling_key[start_anim], now.scaling_key[end_anim], (input_time - now.scaling_key[start_anim].time) / (now.scaling_key[end_anim].time - now.scaling_key[start_anim].time));
		}
		rec_scal = DirectX::XMMatrixScaling(scalling_now.main_key[0], scalling_now.main_key[1], scalling_now.main_key[2]);
		//ƽ�Ʊ任
		FindAnimStEd(input_time, start_anim, end_anim, now.translation_key);
		vector_animation translation_now;
		if (start_anim == end_anim)
		{
			translation_now = now.translation_key[start_anim];
		}
		else
		{
			Interpolate(translation_now, now.translation_key[start_anim], now.translation_key[end_anim], (input_time - now.translation_key[start_anim].time) / (now.translation_key[end_anim].time - now.translation_key[start_anim].time));
		}
		rec_trans = DirectX::XMMatrixTranslation(translation_now.main_key[0], translation_now.main_key[1], translation_now.main_key[2]);
		//��⵱ǰ�����ڵ��Ӧ�Ĺ��������Ƿ�����
		auto bone_data = bone_tree_data.find(now.bone_ID);
		if (bone_data == bone_tree_data.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Bone ID " + std::to_string(now.bone_ID) + " in model: ");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update mesh animation", error_message);
			return error_message;
		}
		XMStoreFloat4x4(&matrix_out[now.bone_ID], rec_scal * XMLoadFloat4x4(&rec_rot) * rec_trans);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicModel::UpdateRoot(
	int32_t root_id,
	const DirectX::XMFLOAT4X4 &matrix_parent,
	const std::vector<DirectX::XMFLOAT4X4> &matrix_animation,
	std::vector<DirectX::XMFLOAT4X4> &matrix_combine_save,
	std::vector<DirectX::XMFLOAT4X4> &matrix_out
)
{
	PancystarEngine::EngineFailReason check_error;
	//���ҵ�ǰ�ĸ��ڵ��Ƿ����
	auto now_root_bone_data = bone_tree_data.find(root_id);
	if (now_root_bone_data == bone_tree_data.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find bone ID" + std::to_string(root_id) + " in model: ");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update Bone tree", error_message);
		return error_message;
	}
	//��ȡ��ǰ�����Ķ�������
	DirectX::XMMATRIX rec = DirectX::XMLoadFloat4x4(&matrix_animation[root_id]);
	DirectX::XMStoreFloat4x4(&matrix_combine_save[root_id], rec * DirectX::XMLoadFloat4x4(&matrix_parent));
	//������Ƥ��Ҫ�Ĺ��������䵼��
	if (now_root_bone_data->second.if_used_for_skin)
	{
		matrix_out[root_id] = matrix_combine_save[root_id];
	}
	//�����ֵܽڵ㼰�ӽڵ�
	if (now_root_bone_data->second.bone_ID_brother != NouseBoneStruct)
	{
		check_error = UpdateRoot(now_root_bone_data->second.bone_ID_brother, matrix_parent, matrix_animation, matrix_combine_save, matrix_out);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	if (now_root_bone_data->second.bone_ID_son != NouseBoneStruct)
	{
		check_error = UpdateRoot(now_root_bone_data->second.bone_ID_son, matrix_combine_save[root_id], matrix_animation, matrix_combine_save, matrix_out);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
void PancyBasicModel::GetQuatMatrix(DirectX::XMFLOAT4X4 &resMatrix, const quaternion_animation& pOut)
{
	resMatrix._11 = static_cast<float>(1.0) - static_cast<float>(2.0) * (pOut.main_key[1] * pOut.main_key[1] + pOut.main_key[2] * pOut.main_key[2]);
	resMatrix._21 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[1] - pOut.main_key[2] * pOut.main_key[3]);
	resMatrix._31 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[2] + pOut.main_key[1] * pOut.main_key[3]);
	resMatrix._41 = 0.0f;

	resMatrix._12 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[1] + pOut.main_key[2] * pOut.main_key[3]);
	resMatrix._22 = static_cast<float>(1.0) - static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[0] + pOut.main_key[2] * pOut.main_key[2]);
	resMatrix._32 = static_cast<float>(2.0) * (pOut.main_key[1] * pOut.main_key[2] - pOut.main_key[0] * pOut.main_key[3]);
	resMatrix._42 = 0.0f;

	resMatrix._13 = static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[2] - pOut.main_key[1] * pOut.main_key[3]);
	resMatrix._23 = static_cast<float>(2.0) * (pOut.main_key[1] * pOut.main_key[2] + pOut.main_key[0] * pOut.main_key[3]);
	resMatrix._33 = static_cast<float>(1.0) - static_cast<float>(2.0) * (pOut.main_key[0] * pOut.main_key[0] + pOut.main_key[1] * pOut.main_key[1]);
	resMatrix._43 = 0.0f;

	resMatrix._14 = 0.0f;
	resMatrix._24 = 0.0f;
	resMatrix._34 = 0.0f;
	resMatrix._44 = 1.0f;
}
pancy_object_id PancyBasicModel::FindParentByLayer(
	const pancy_object_id& bone_id,
	const pancy_object_id& layer,
	const std::vector<uint32_t>& bone_node_parent_data
){
	pancy_object_id now_parent_id = bone_id;
	for (pancy_object_id now_layer = 0; now_layer < layer; ++now_layer)
	{
		if (now_parent_id == bone_node_parent_data[now_parent_id]) 
		{
			return now_parent_id;
		}
		else 
		{
			now_parent_id = bone_node_parent_data[now_parent_id];
		}
	}
	return now_parent_id;
}
//��Դ����
PancystarEngine::EngineFailReason PancyBasicModel::InitResource(const Json::Value &root_value, const std::string &resource_name)
{
	PancystarEngine::EngineFailReason check_error;
	std::string path_name = "";
	std::string file_name = "";
	std::string tile_name = "";
	DivideFilePath(resource_name, path_name, file_name, tile_name);
	pancy_json_value rec_value;
	//�Ƿ������������
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "IfHaveSkinAnimation", pancy_json_data_type::json_data_bool, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if_skinmesh = rec_value.bool_value;
	//�Ƿ�������㶯��
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "IfHavePoinAnimation", pancy_json_data_type::json_data_bool, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	if_pointmesh = rec_value.bool_value;
	//��ȡģ�͵���������
	int32_t model_part_num;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "model_num", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_part_num = rec_value.int_value;
	for (int i = 0; i < model_part_num; ++i)
	{
		std::string model_vertex_data_name = path_name + file_name + std::to_string(i) + ".vertex";
		std::string model_index_data_name = path_name + file_name + std::to_string(i) + ".index";
		if (if_skinmesh)
		{
			check_error = LoadMeshData<PancystarEngine::PointSkinCommon8>(model_vertex_data_name, model_index_data_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		else if (if_pointmesh)
		{
			check_error = LoadMeshData<PancystarEngine::PointCatchCommon>(model_vertex_data_name, model_index_data_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		else
		{
			check_error = LoadMeshData<PancystarEngine::PointCommon>(model_vertex_data_name, model_index_data_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
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
		for (Json::ArrayIndex i = 0; i < skin_animation_value.size(); ++i)
		{
			animation_set new_animation;
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, skin_animation_value, i, pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			std::string now_animation_name = path_name + rec_value.string_value;
			instream.open(now_animation_name, ios::binary);
			int32_t animation_bone_num;
			float animation_length;
			instream.read(reinterpret_cast<char*>(&animation_length), sizeof(animation_length));
			instream.read(reinterpret_cast<char*>(&animation_bone_num), sizeof(animation_bone_num));
			new_animation.animation_length = animation_length;
			for (int l = 0; l < animation_bone_num; ++l)
			{
				animation_data new_bone_data;
				//������Ϣ
				int32_t bone_name_size = -1;
				instream.read(reinterpret_cast<char*>(&bone_name_size), sizeof(bone_name_size));
				char *name = new char[bone_name_size];
				instream.read(name, bone_name_size * sizeof(char));
				new_bone_data.bone_name += name;
				auto now_used_bone_ID = bone_name_index.find(new_bone_data.bone_name);
				if (now_used_bone_ID == bone_name_index.end())
				{
					PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find bone: " + new_bone_data.bone_name + " in model: " + resource_name);
					PancystarEngine::EngineFailLog::GetInstance()->AddLog("Load Skin Mesh Animation Data", error_message);
					return error_message;
				}
				new_bone_data.bone_ID = now_used_bone_ID->second;
				delete[] name;
				//��ת����
				int32_t rotation_key_num = 0;
				instream.read(reinterpret_cast<char*>(&rotation_key_num), sizeof(rotation_key_num));
				quaternion_animation *new_rotation_key = new quaternion_animation[rotation_key_num];
				int32_t rotation_key_size = sizeof(new_rotation_key[0]) * rotation_key_num;

				instream.read(reinterpret_cast<char*>(new_rotation_key), rotation_key_size);
				for (int j = 0; j < rotation_key_num; ++j)
				{
					new_bone_data.rotation_key.push_back(new_rotation_key[j]);
				}
				//ƽ������
				int32_t translation_key_num = 0;
				instream.read(reinterpret_cast<char*>(&translation_key_num), sizeof(translation_key_num));
				vector_animation *new_translation_key = new vector_animation[translation_key_num];
				int32_t translation_key_size = sizeof(new_translation_key[0]) * translation_key_num;
				instream.read(reinterpret_cast<char*>(new_translation_key), translation_key_size);
				for (int j = 0; j < translation_key_num; ++j)
				{
					new_bone_data.translation_key.push_back(new_translation_key[j]);
				}

				//��������
				int32_t scaling_key_num = 0;
				instream.read(reinterpret_cast<char*>(&scaling_key_num), sizeof(scaling_key_num));
				vector_animation *new_scaling_key = new vector_animation[scaling_key_num];
				int32_t scaling_key_size = sizeof(new_scaling_key[0]) * scaling_key_num;
				instream.read(reinterpret_cast<char*>(new_scaling_key), scaling_key_size);
				for (int j = 0; j < scaling_key_num; ++j)
				{
					new_bone_data.scaling_key.push_back(new_scaling_key[j]);
				}
				new_animation.data_animition.push_back(new_bone_data);
				//ɾ����ʱ����
				delete[] new_rotation_key;
				delete[] new_translation_key;
				delete[] new_scaling_key;
			}
			//��������Ϣ�����
			skin_animation_name.insert(std::pair<std::string, pancy_resource_id>(now_animation_name, i));
			skin_animation_map.insert(std::pair<pancy_resource_id, animation_set>(i, new_animation));
			instream.close();
		}
		//�����еĶ���������֯��һ��ͳһ��buffer
		std::vector<AnimationNodeData> animation_buffer_data;
		for (auto& skin_anim_data : skin_animation_map)
		{
			int32_t max_resample_data = 0;
			for (auto each_bone_data_anim : skin_anim_data.second.data_animition)
			{
				if (each_bone_data_anim.translation_key.size() == each_bone_data_anim.rotation_key.size() && each_bone_data_anim.rotation_key.size() == each_bone_data_anim.scaling_key.size())
				{
					if (max_resample_data == 0 || max_resample_data == each_bone_data_anim.translation_key.size())
					{
						max_resample_data = static_cast<int32_t>(each_bone_data_anim.translation_key.size());
						for (int32_t key_index = 0; key_index < max_resample_data; ++key_index)
						{
							AnimationNodeData new_animation_data;
							new_animation_data.translation_key.x = each_bone_data_anim.translation_key[key_index].main_key[0];
							new_animation_data.translation_key.y = each_bone_data_anim.translation_key[key_index].main_key[1];
							new_animation_data.translation_key.z = each_bone_data_anim.translation_key[key_index].main_key[2];

							new_animation_data.rotation_key.x = each_bone_data_anim.rotation_key[key_index].main_key[0];
							new_animation_data.rotation_key.y = each_bone_data_anim.rotation_key[key_index].main_key[1];
							new_animation_data.rotation_key.z = each_bone_data_anim.rotation_key[key_index].main_key[2];
							new_animation_data.rotation_key.w = each_bone_data_anim.rotation_key[key_index].main_key[3];

							new_animation_data.scaling_key.x = each_bone_data_anim.scaling_key[key_index].main_key[0];
							new_animation_data.scaling_key.y = each_bone_data_anim.scaling_key[key_index].main_key[1];
							new_animation_data.scaling_key.z = each_bone_data_anim.scaling_key[key_index].main_key[2];
							animation_buffer_data.push_back(new_animation_data);
						}
					}
					else
					{
						//todo:����һ�������Ĳ����ʺ�����������һ��
						if (each_bone_data_anim.translation_key.size() == 1)
						{
							for (int32_t key_index = 0; key_index < max_resample_data; ++key_index)
							{
								AnimationNodeData new_animation_data;
								new_animation_data.translation_key.x = each_bone_data_anim.translation_key[0].main_key[0];
								new_animation_data.translation_key.y = each_bone_data_anim.translation_key[0].main_key[1];
								new_animation_data.translation_key.z = each_bone_data_anim.translation_key[0].main_key[2];

								new_animation_data.rotation_key.x = each_bone_data_anim.rotation_key[0].main_key[0];
								new_animation_data.rotation_key.y = each_bone_data_anim.rotation_key[0].main_key[1];
								new_animation_data.rotation_key.z = each_bone_data_anim.rotation_key[0].main_key[2];
								new_animation_data.rotation_key.w = each_bone_data_anim.rotation_key[0].main_key[3];

								new_animation_data.scaling_key.x = each_bone_data_anim.scaling_key[0].main_key[0];
								new_animation_data.scaling_key.y = each_bone_data_anim.scaling_key[0].main_key[1];
								new_animation_data.scaling_key.z = each_bone_data_anim.scaling_key[0].main_key[2];
								animation_buffer_data.push_back(new_animation_data);
							}
						}
						else
						{
							PancystarEngine::EngineFailReason error_message(E_FAIL, "could not resample animation");
							PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicModel::InitResource", error_message);
							return error_message;
						}
					}
				}
				else
				{
					int32_t now_resample = static_cast<int32_t>(max(each_bone_data_anim.translation_key.size(), each_bone_data_anim.rotation_key.size()));
					now_resample = static_cast<int32_t>(max(now_resample, each_bone_data_anim.scaling_key.size()));
					if (max_resample_data != 0 && now_resample != max_resample_data)
					{
						PancystarEngine::EngineFailReason error_message(E_FAIL, "could not resample animation");
						PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicModel::InitResource", error_message);
						return error_message;
					}
					for (int32_t key_index = 0; key_index < max_resample_data; ++key_index)
					{
						AnimationNodeData new_animation_data;
						if (each_bone_data_anim.translation_key.size() == now_resample)
						{
							new_animation_data.translation_key.x = each_bone_data_anim.translation_key[key_index].main_key[0];
							new_animation_data.translation_key.y = each_bone_data_anim.translation_key[key_index].main_key[1];
							new_animation_data.translation_key.z = each_bone_data_anim.translation_key[key_index].main_key[2];
						}
						else if (each_bone_data_anim.translation_key.size() == 1)
						{
							new_animation_data.translation_key.x = each_bone_data_anim.translation_key[0].main_key[0];
							new_animation_data.translation_key.y = each_bone_data_anim.translation_key[0].main_key[1];
							new_animation_data.translation_key.z = each_bone_data_anim.translation_key[0].main_key[2];
						}
						else
						{
							PancystarEngine::EngineFailReason error_message(E_FAIL, "could not resample animation");
							PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicModel::InitResource", error_message);
							return error_message;
						}

						if (each_bone_data_anim.rotation_key.size() == now_resample)
						{
							new_animation_data.rotation_key.x = each_bone_data_anim.rotation_key[key_index].main_key[0];
							new_animation_data.rotation_key.y = each_bone_data_anim.rotation_key[key_index].main_key[1];
							new_animation_data.rotation_key.z = each_bone_data_anim.rotation_key[key_index].main_key[2];
							new_animation_data.rotation_key.w = each_bone_data_anim.rotation_key[key_index].main_key[3];
						}
						else if (each_bone_data_anim.rotation_key.size() == 1)
						{
							new_animation_data.rotation_key.x = each_bone_data_anim.rotation_key[0].main_key[0];
							new_animation_data.rotation_key.y = each_bone_data_anim.rotation_key[0].main_key[1];
							new_animation_data.rotation_key.z = each_bone_data_anim.rotation_key[0].main_key[2];
							new_animation_data.rotation_key.w = each_bone_data_anim.rotation_key[0].main_key[3];
						}
						else
						{
							PancystarEngine::EngineFailReason error_message(E_FAIL, "could not resample animation");
							PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicModel::InitResource", error_message);
							return error_message;
						}

						if (each_bone_data_anim.scaling_key.size() == now_resample)
						{
							new_animation_data.scaling_key.x = each_bone_data_anim.scaling_key[key_index].main_key[0];
							new_animation_data.scaling_key.y = each_bone_data_anim.scaling_key[key_index].main_key[1];
							new_animation_data.scaling_key.z = each_bone_data_anim.scaling_key[key_index].main_key[2];
						}
						else if (each_bone_data_anim.scaling_key.size() == 1)
						{
							new_animation_data.scaling_key.x = each_bone_data_anim.scaling_key[0].main_key[0];
							new_animation_data.scaling_key.y = each_bone_data_anim.scaling_key[0].main_key[1];
							new_animation_data.scaling_key.z = each_bone_data_anim.scaling_key[0].main_key[2];
						}
						else
						{
							PancystarEngine::EngineFailReason error_message(E_FAIL, "could not resample animation");
							PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyBasicModel::InitResource", error_message);
							return error_message;
						}
						animation_buffer_data.push_back(new_animation_data);
					}
				}
			}
		}
		//�����洢�������ݵĻ�������Դ(��̬������)
		auto real_buffer_size = animation_buffer_data.size() * sizeof(AnimationNodeData);
		check_error = BuildBufferResourceFromMemory(file_name+"_animation", model_animation_buffer, &animation_buffer_data[0], real_buffer_size, true);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//�����������ݵĸ��ڵ㻺������Դ��ע��������Ҫ�����������Ĺ�����ͷ������β�����������᷽��Computeshader���в��д���
		std::vector<uint32_t> bone_node_parent_data;
		bone_node_parent_data.resize(bone_parent_data.size()+2);
		for (auto now_bone_node : bone_parent_data) 
		{
			bone_node_parent_data[static_cast<size_t>(now_bone_node.first) + static_cast<size_t>(1)] = now_bone_node.second;
		}
		std::vector<uint32_t> bone_node_parent_multilayer_data;
		bone_node_parent_multilayer_data.resize(MaxSkinDeSampleTime * bone_node_parent_data.size());
		int32_t pow2_save[] = { 1,2,4,8,16,32,64,128,256 };
		for (uint32_t now_node_index = 0;  now_node_index < bone_node_parent_data.size(); ++now_node_index)
		{
			for (int32_t layer_index = 1; layer_index < MaxSkinDeSampleTime; ++layer_index)
			{
				int32_t globle_index = MaxSkinDeSampleTime * now_node_index + layer_index - 1;
				int32_t now_index_search = pow2_save[layer_index-1];
				bone_node_parent_multilayer_data[globle_index] = FindParentByLayer(now_node_index, now_index_search, bone_node_parent_data);
			}
		}
		//�����洢���������ݵĻ�������Դ(��̬������)
		auto real_parent_buffer_size = bone_node_parent_multilayer_data.size() * sizeof(uint32_t);
		check_error = BuildBufferResourceFromMemory(file_name + "_boneparent", model_bonetree_buffer, &bone_node_parent_multilayer_data[0], real_parent_buffer_size, true);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//����ƫ�ƾ��󻺳�����Դ(ͬ����Ҫͷ������β����)
		std::vector<DirectX::XMFLOAT4X4> bone_offset_mat_data;
		DirectX::XMFLOAT4X4 identity_mat;
		DirectX::XMStoreFloat4x4(&identity_mat,DirectX::XMMatrixIdentity());
		bone_offset_mat_data.push_back(identity_mat);
		for (int bone_id = 0; bone_id < bone_object_num; ++bone_id)
		{
			bone_offset_mat_data.push_back(identity_mat);
		}
		bone_offset_mat_data.push_back(identity_mat);
		for (int bone_id = 0; bone_id < bone_num; ++bone_id)
		{
			bone_offset_mat_data[bone_id+1] = offset_matrix_array[bone_id];
		}
		//�����洢ƫ�ƾ������ݵĻ�������Դ(��̬������)
		auto real_offset_buffer_size = bone_offset_mat_data.size() * sizeof(DirectX::XMFLOAT4X4);
		check_error = BuildBufferResourceFromMemory(file_name + "_boneoffset", model_boneoffset_buffer, &bone_offset_mat_data[0], real_offset_buffer_size, true);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		int a = 0;
	}
	
	
	//��ȡ���㶯��
	/*
	if (if_pointmesh)
	{
		Json::Value point_animation_value = root_value.get("PointAnimation", Json::Value::null);
		for (int i = 0; i < point_animation_value.size(); ++i)
		{
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, point_animation_value, i, pancy_json_data_type::json_data_string, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			std::string now_animation_name = path_name + rec_value.string_value;
			instream.open(now_animation_name, ios::binary);
			instream.read(reinterpret_cast<char*>(&all_frame_num), sizeof(all_frame_num));
			instream.read(reinterpret_cast<char*>(&mesh_animation_buffer_size), sizeof(mesh_animation_buffer_size));
			instream.read(reinterpret_cast<char*>(&mesh_animation_ID_buffer_size), sizeof(mesh_animation_ID_buffer_size));
			int32_t buffer_size = mesh_animation_buffer_size * sizeof(mesh_animation_data);
			int32_t buffer_id_size = mesh_animation_ID_buffer_size * sizeof(VertexAnimationID);
			mesh_animation_data *new_point_catch_data = new mesh_animation_data[buffer_size];
			instream.read(reinterpret_cast<char*>(new_point_catch_data), buffer_size);
			VertexAnimationID *new_point_id_data = new VertexAnimationID[buffer_id_size];
			instream.read(reinterpret_cast<char*>(new_point_id_data), buffer_id_size);
			instream.close();
			pancy_object_id point_aimation_buffer_pointer;
			pancy_object_id point_aimation_id_buffer_pointer;
			Json::Value point_animation_buffer_type;
			std::string buffer_subresource_name;
			check_error = PancyBasicBufferControl::GetInstance()->BuildBufferTypeJson(Buffer_ShaderResource_static, buffer_size, buffer_subresource_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			PancyJsonTool::GetInstance()->SetJsonValue(point_animation_buffer_type, "BufferType", "Buffer_ShaderResource_static");
			PancyJsonTool::GetInstance()->SetJsonValue(point_animation_buffer_type, "SubResourceFile", buffer_subresource_name);
			check_error = PancyBasicBufferControl::GetInstance()->LoadResource(file_name, point_animation_buffer_type, point_aimation_buffer_pointer, true);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}

			Json::Value point_animation_buffer_id_type;
			check_error = PancyBasicBufferControl::GetInstance()->BuildBufferTypeJson(Buffer_ShaderResource_static, buffer_id_size, buffer_subresource_name);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			PancyJsonTool::GetInstance()->SetJsonValue(point_animation_buffer_id_type, "BufferType", "Buffer_ShaderResource_static");
			PancyJsonTool::GetInstance()->SetJsonValue(point_animation_buffer_id_type, "SubResourceFile", buffer_subresource_name);
			check_error = PancyBasicBufferControl::GetInstance()->LoadResource(file_name, point_animation_buffer_id_type, point_aimation_id_buffer_pointer, true);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			delete[] new_point_catch_data;
			delete[] new_point_id_data;
			Point_animation_name.insert(std::pair<std::string, pancy_object_id>(now_animation_name, point_animation_id_self_add));
			Point_animation_buffer.insert(std::pair<pancy_object_id, pancy_object_id>(point_animation_id_self_add, point_aimation_buffer_pointer));
			Point_animation_id_buffer.insert(std::pair<pancy_object_id, pancy_object_id>(point_animation_id_self_add, point_aimation_id_buffer_pointer));
			point_animation_id_self_add += 1;
		}
	}
	*/
	return PancystarEngine::succeed;
}
bool PancyBasicModel::CheckIfLoadSucceed()
{
	//������еļ�������Դ�Ƿ��Ѿ��������
	for (int i = 0; i < model_resource_list.size(); ++i)
	{
		bool if_mesh_succeed_load = model_resource_list[i]->CheckIfLoadSucceed();
		if (!if_mesh_succeed_load)
		{
			return false;
		}
	}
	return true;
}
