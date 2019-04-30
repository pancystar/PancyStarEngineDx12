#include"PancyModelBasic.h"
using namespace PancystarEngine;
//ģ�Ͳ���
PancySubModel::PancySubModel()
{
	model_mesh = NULL;
	material_use = 0;

}
PancySubModel::~PancySubModel()
{
	if (model_mesh != NULL)
	{
		delete model_mesh;
	}
}
//ģ����
PancyBasicModel::PancyBasicModel(const std::string &resource_name, const Json::Value &root_value) : PancyBasicVirtualResource(resource_name, root_value)
{
	//���ݽ���֡����������������������Ⱦ��������
	pancy_object_id Frame_num = PancyDx12DeviceBasic::GetInstance()->GetFrameNum();
	descriptor_map.resize(Frame_num);
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
	for (int32_t i = 0; i < descriptor_map.size(); ++i) 
	{
		for (auto descriptor_data = descriptor_map[i].begin(); descriptor_data != descriptor_map[i].end(); ++descriptor_data) 
		{
			delete descriptor_data->second;
		}
	}
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
	int bone_num_need;
	instream.read(reinterpret_cast<char*>(&bone_num), sizeof(bone_num));
	instream.read(reinterpret_cast<char*>(offset_matrix_array), bone_num * sizeof(DirectX::XMFLOAT4X4));
	//�ȶ�ȡ��һ����ջ��
	char data[11];
	instream.read(reinterpret_cast<char*>(data), sizeof(data));
	//root_skin = new skin_tree();
	//�ݹ��ؽ�������
	//ReadBoneTree(root_skin);
	
	bone_object_num = bone_num;
	/*
	bone_struct root_bone;
	root_bone.bone_ID_son = NouseAssimpStruct;
	root_bone.bone_ID_brother = NouseAssimpStruct;
	root_bone.bone_name = "pancy_root";
	root_bone.if_used_for_skin = false;
	bone_tree_data.insert(std::pair<int32_t, bone_struct>(bone_object_num, root_bone));
	root_id = bone_object_num;
	bone_object_num += 1;
	bone_name_index.insert(std::pair<std::string, int32_t>(root_bone.bone_name, root_id));
	*/
	auto check_error = ReadBoneTree(root_id);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	//�ر��ļ�
	instream.close();
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyBasicModel::ReadBoneTree(int32_t &now_build_id)
{
	char data[11];
	skin_tree now_bone_data;
	instream.read(reinterpret_cast<char*>(&now_bone_data), sizeof(now_bone_data));
	//���ݶ�ȡ���Ĺ����ڵ㴴��һ�������ṹ��������
	bone_struct new_bone;
	bool if_used_skin = false;;
	if (now_bone_data.bone_number == NouseAssimpStruct) 
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
	new_bone.bone_ID_son = NouseAssimpStruct;
	new_bone.bone_ID_brother = NouseAssimpStruct;
	new_bone.if_used_for_skin = if_used_skin;
	bone_tree_data.insert(std::pair<int32_t, bone_struct>(now_build_id, new_bone));
	bone_name_index.insert(std::pair<std::string, int32_t>(new_bone.bone_name, now_build_id));
	instream.read(data, sizeof(data));
	while (strcmp(data, "*heaphead*") == 0)
	{
		//��ջ���ţ������ӽڵ�
		int32_t now_son_ID = NouseAssimpStruct;
		//�ݹ鴴���ӽڵ�
		auto check_error = ReadBoneTree(now_son_ID);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		auto now_parent_data = bone_tree_data.find(now_build_id);
		auto now_data = bone_tree_data.find(now_son_ID);
		if (now_parent_data == bone_tree_data.end() )
		{
			//����ĸ��ڵ㲻����
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Parent bone ID "+std::to_string(now_build_id));
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
		st = input.size() - 1;
		ed = input.size() - 1;
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
	st = input.size() - 1;
	ed = input.size() - 1;
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
		st = input.size() - 1;
		ed = input.size() - 1;
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
	st = input.size() - 1;
	ed = input.size() - 1;
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "model: " +resource_name +"don't have skin mesh message");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Update mesh animation", error_message);
		return error_message;
	}
	std::vector<DirectX::XMFLOAT4X4> matrix_animation_save;
	matrix_animation_save.resize(bone_object_num);
	for (int i = 0; i < bone_object_num; ++i)
	{
		//ʹ�õ�λ���󽫶����������
		DirectX::XMStoreFloat4x4(&matrix_animation_save[i],DirectX::XMMatrixIdentity());
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
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find Bone ID " + std::to_string(now.bone_ID) +" in model: "+ resource_name);
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
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find bone ID"+ std::to_string(root_id) + " in model: " + resource_name);
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
	if (now_root_bone_data->second.bone_ID_brother != NouseAssimpStruct) 
	{
		check_error = UpdateRoot(now_root_bone_data->second.bone_ID_brother, matrix_parent, matrix_animation, matrix_combine_save, matrix_out);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	if (now_root_bone_data->second.bone_ID_son != NouseAssimpStruct) 
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
//��Դ����
PancystarEngine::EngineFailReason PancyBasicModel::InitResource(const Json::Value &root_value, const std::string &resource_name, ResourceStateType &now_res_state)
{
	PancystarEngine::EngineFailReason check_error;
	std::string path_name = "";
	std::string file_name = "";
	std::string tile_name = "";
	DivideFilePath(resource_name, path_name, file_name, tile_name);
	pancy_json_value rec_value;
	/*
	Json::Value root_value;
	check_error = PancyJsonTool::GetInstance()->LoadJsonFile(resource_desc_file, root_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	*/
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
	//ģ�͵�pbr����
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "PbrType", pancy_json_data_type::json_data_enum, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_pbr_type = static_cast<PbrMaterialType>(rec_value.int_value);
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
	//��ȡģ�͵���������
	int32_t model_texture_num;
	check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, root_value, "texture_num", pancy_json_data_type::json_data_int, rec_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	model_texture_num = rec_value.int_value;
	for (int i = 0; i < model_texture_num; ++i)
	{
		std::string texture_name_now = path_name + file_name + "_tex" + std::to_string(i) + ".dds";
		pancy_object_id texture_id;
		//��������������ü���
		Json::Value new_texture;
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "FileName", texture_name_now);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "IfAutoBuildMipMap", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "IfForceSrgb", 0);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "IfFromFile", 1);
		PancyJsonTool::GetInstance()->SetJsonValue(new_texture, "MaxSize", 0);
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->LoadResource(texture_name_now, new_texture, texture_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		/*
		check_error = PancystarEngine::PancyTextureControl::GetInstance()->AddResurceReference(texture_id);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		*/
		texture_list.push_back(texture_id);
	}
	//��ȡģ�͵Ĳ�������
	Json::Value material_value = root_value.get("material", Json::Value::null);
	for (int i = 0; i < material_value.size(); ++i)
	{
		std::unordered_map<TexType, pancy_object_id> now_material_need;
		std::vector<pancy_object_id> now_material_id_need;
		int32_t material_id;
		//����id
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "materialID", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		material_id = rec_value.int_value;
		//����������
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Albedotex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_diffuse, rec_value.int_value));
		now_material_id_need.push_back(rec_value.int_value);
		//��������
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Normaltex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_normal, rec_value.int_value));
		now_material_id_need.push_back(rec_value.int_value);
		//AO����
		check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "Ambienttex", pancy_json_data_type::json_data_int, rec_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_ambient, rec_value.int_value));
		now_material_id_need.push_back(rec_value.int_value);
		//PBR����
		if (model_pbr_type == PbrMaterialType::PbrType_MetallicRoughness)
		{
			//����������
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "MetallicTex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_metallic, rec_value.int_value));
			now_material_id_need.push_back(rec_value.int_value);
			//�ֲڶ�����
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "RoughnessTex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_roughness, rec_value.int_value));
			now_material_id_need.push_back(rec_value.int_value);
		}
		else if (model_pbr_type == PbrMaterialType::PbrType_SpecularSmoothness)
		{
			//�����&ƽ��������
			check_error = PancyJsonTool::GetInstance()->GetJsonData(resource_name, material_value[i], "SpecularSmoothTex", pancy_json_data_type::json_data_int, rec_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			now_material_need.insert(std::pair<TexType, pancy_object_id>(TexType::tex_specular_smoothness, rec_value.int_value));
			now_material_id_need.push_back(rec_value.int_value);
		}
		material_list.insert(std::pair<pancy_object_id, std::unordered_map<TexType, pancy_object_id>>(material_id, now_material_need));
		material_id_list.insert(std::pair<pancy_object_id, std::vector<pancy_object_id>>(material_id, now_material_id_need));
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
	}
	//��ȡ���㶯��
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
			instream.read(reinterpret_cast<char*>(&perframe_size), sizeof(perframe_size));
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
	now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
	return PancystarEngine::succeed;
}
void PancyBasicModel::CheckIfResourceLoadToGpu(ResourceStateType &now_res_state)
{
	PancystarEngine::EngineFailReason check_error;
	if (now_res_state == ResourceStateType::resource_state_load_CPU_memory_finish)
	{
		//������е�������Դ�Ƿ��Ѿ��������
		for (int i = 0; i < texture_list.size(); ++i)
		{
			ResourceStateType now_texture_state;
			check_error = PancyTextureControl::GetInstance()->GetResourceState(texture_list[i], now_texture_state);
			if (!check_error.CheckIfSucceed() || now_texture_state == ResourceStateType::resource_state_not_init)
			{
				now_res_state = ResourceStateType::resource_state_not_init;
				return;
			}
			else if (now_texture_state == ResourceStateType::resource_state_load_CPU_memory_finish)
			{
				now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
				return;
			}
		}
		//������еļ�������Դ�Ƿ��Ѿ��������
		for (int i = 0; i < model_resource_list.size(); ++i)
		{
			ResourceStateType now_texture_state;
			check_error = model_resource_list[i]->GetLoadState(now_texture_state);
			if (!check_error.CheckIfSucceed() || now_texture_state == ResourceStateType::resource_state_not_init)
			{
				now_res_state = ResourceStateType::resource_state_not_init;
				return;
			}
			else if (now_texture_state == ResourceStateType::resource_state_load_CPU_memory_finish)
			{
				now_res_state = ResourceStateType::resource_state_load_CPU_memory_finish;
				return;
			}
		}
		now_res_state = ResourceStateType::resource_state_load_GPU_memory_finish;
	}
}
PancystarEngine::EngineFailReason PancyBasicModel::GetRenderDescriptor(
	pancy_object_id PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	DescriptorObject **descriptor_out
)
{
	pancy_object_id now_render_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	PancystarEngine::EngineFailReason check_error;
	auto descriptor_list = descriptor_map[now_render_frame].find(PSO_id);
	if (descriptor_list == descriptor_map[now_render_frame].end())
	{
		//����PSO��ID�Ż�ȡPSO�����ƺ��������ĸ�ʽ����
		std::string PSO_name;
		std::string descriptor_name;
		check_error = PancyEffectGraphic::GetInstance()->GetPSOName(PSO_id, PSO_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		check_error = PancyEffectGraphic::GetInstance()->GetPSODescriptorName(PSO_id, descriptor_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//�����е�ģ����Դ����
		std::vector<SubMemoryPointer> res_pack;
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> SRV_pack;
		if (if_pointmesh)
		{

		}
		bool if_need_resource_barrier = false;
		for (int i = 0; i < material_id_list.size(); ++i)
		{
			for (int j = 0; j < material_id_list[i].size(); ++j) 
			{
				D3D12_SHADER_RESOURCE_VIEW_DESC new_SRV_desc;
				SubMemoryPointer now_texture;
				PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(texture_list[material_id_list[i][j]], now_texture);
				res_pack.push_back(now_texture);
				D3D12_RESOURCE_STATES test_state;
				SubresourceControl::GetInstance()->GetResourceState(now_texture, test_state);
				if (test_state != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE) 
				{
					if_need_resource_barrier = true;
				}
				PancystarEngine::PancyTextureControl::GetInstance()->GetSRVDesc(texture_list[material_id_list[i][j]], new_SRV_desc);
				SRV_pack.push_back(new_SRV_desc);
			}
		}
		if (if_need_resource_barrier) 
		{
			//��Щ��Ⱦ��Դ��δת��ΪSRV��ʽ���������߳�ͳһת��
			PancyRenderCommandList *m_commandList;
			PancyThreadIdGPU commdlist_id_use;
			check_error = ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->GetEmptyRenderlist(NULL, &m_commandList, commdlist_id_use);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			for (int i = 0; i < material_id_list.size(); ++i)
			{
				for (int j = 0; j < material_id_list[i].size(); ++j)
				{
					SubMemoryPointer now_texture;
					PancystarEngine::PancyTextureControl::GetInstance()->GetTexResource(texture_list[material_id_list[i][j]], now_texture);
					D3D12_RESOURCE_STATES test_state;
					SubresourceControl::GetInstance()->GetResourceState(now_texture, test_state);
					if (test_state != D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
					{
						SubresourceControl::GetInstance()->ResourceBarrier(m_commandList, now_texture,D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATES::D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
					};
				}
			}
			m_commandList->UnlockPrepare();
			ThreadPoolGPUControl::GetInstance()->GetMainContex()->GetThreadPool(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT)->SubmitRenderlist(1,&commdlist_id_use);
		}
		//����һ���µ�����������
		DescriptorObjectList *new_descriptor_List;
		new_descriptor_List = new DescriptorObjectList(PSO_name, descriptor_name);
		check_error = new_descriptor_List->Create(
			cbuffer_name_per_object_in,
			cbuffer_per_frame_in,
			resource_data_per_frame_in,
			resource_desc_per_frame_in,
			res_pack,
			SRV_pack
		);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		descriptor_map[now_render_frame].insert(std::pair<pancy_object_id, DescriptorObjectList*>(PSO_id, new_descriptor_List));
		descriptor_list = descriptor_map[now_render_frame].find(PSO_id);
	}
	check_error = descriptor_list->second->GetEmptyList(descriptor_out);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void PancyBasicModel::ResetRenderList()
{
	pancy_object_id now_render_frame = PancyDx12DeviceBasic::GetInstance()->GetNowFrame();
	PancystarEngine::EngineFailReason check_error;
	for (auto descriptor_data = descriptor_map[now_render_frame].begin(); descriptor_data != descriptor_map[now_render_frame].end(); ++descriptor_data) 
	{
		descriptor_data->second->Reset();
	}
}

//��������
DescriptorObject::DescriptorObject()
{
	PSO_pointer = NULL;
	rootsignature = NULL;
	descriptor_heap_use = NULL;
}
DescriptorObject::~DescriptorObject()
{
	PancyDescriptorHeapControl::GetInstance()->FreeResourceView(descriptor_block_id);
	for (auto release_data = per_object_cbuffer.begin(); release_data != per_object_cbuffer.end(); ++release_data)
	{
		delete release_data->second;
	}
	per_object_cbuffer.clear();
}
PancystarEngine::EngineFailReason DescriptorObject::Create(
	const std::string &PSO_name,
	const std::string &descriptor_name,
	const std::vector<std::string> &cbuffer_name_per_object,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame,
	const std::vector<SubMemoryPointer> &resource_data_per_frame,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_object,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
)
{
	PancystarEngine::EngineFailReason check_error;
	PSO_name_descriptor = PSO_name;
	//����һ����Ӧ���͵���������
	ResourceViewPointer new_point;
	pancy_object_id globel_offset = 0;
	check_error = PancyDescriptorHeapControl::GetInstance()->BuildResourceViewFromFile(descriptor_name, descriptor_block_id, resource_view_num);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//����Ⱦ��Ҫ�İ���Դָ��һ����ȫ����ȡ������
	pancy_object_id PSO_id_need;
	//PSO����
	check_error = PancyEffectGraphic::GetInstance()->GetPSO(PSO_name, PSO_id_need);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyEffectGraphic::GetInstance()->GetPSOResource(PSO_id_need,&PSO_pointer);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//rootsignature����
	check_error = PancyEffectGraphic::GetInstance()->GetRootSignatureResource(PSO_id_need, &rootsignature);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�󶨵�������������
	check_error = PancyDescriptorHeapControl::GetInstance()->GetDescriptorHeap(descriptor_block_id,&descriptor_heap_use);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	//�󶨵��������ѵ�ƫ��
	std::vector<pancy_object_id> descriptor_distribute;
	check_error = PancyEffectGraphic::GetInstance()->GetDescriptorDistribute(PSO_id_need, descriptor_distribute);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	pancy_object_id now_start_offset = 0;
	for (int i = 0; i < descriptor_distribute.size(); ++i) 
	{
		ResourceViewPointer distribute_point;
		distribute_point.resource_view_pack_id = descriptor_block_id;
		CD3DX12_GPU_DESCRIPTOR_HANDLE now_gpu_handle;
		distribute_point.resource_view_offset_id = now_start_offset;
		check_error = PancyDescriptorHeapControl::GetInstance()->GetOffsetNum(distribute_point, now_gpu_handle);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		descriptor_offset.push_back(now_gpu_handle);
		now_start_offset += descriptor_distribute[i];
	}
	
	//�������������Ϣ
	new_point.resource_view_pack_id = descriptor_block_id;
	//���鴫�����Դ�������������������Ƿ�ƥ��(�����bindless texture��Ҫ����Դ����С�ڵ�����������)
	pancy_object_id check_descriptor_size = cbuffer_name_per_object.size() + cbuffer_per_frame.size() + resource_data_per_frame.size() + resource_data_per_object.size();
	if (resource_data_per_object.size() == 0 && check_descriptor_size != resource_view_num)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource num: " +
			std::to_string(check_descriptor_size) +
			" dismatch resource view num: " +
			std::to_string(resource_view_num) +
			" in PSO: " + PSO_name
		);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor object", error_message);
		return error_message;
	}
	else if (resource_view_num < check_descriptor_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the resource num: " +
			std::to_string(check_descriptor_size) +
			" bigger than resource view num: " +
			std::to_string(resource_view_num) +
			" in PSO: " + PSO_name
		);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Build descriptor object", error_message);
		return error_message;
	}
	//�ȸ��ݳ��������������ƣ���object���еĳ�����������
	for (int i = 0; i < cbuffer_name_per_object.size(); ++i)
	{
		auto cbuffer_check = per_object_cbuffer.find(cbuffer_name_per_object[i]);
		if (cbuffer_check == per_object_cbuffer.end()) 
		{
			std::string pso_divide_path;
			std::string pso_divide_name;
			std::string pso_divide_tail;
			PancystarEngine::DivideFilePath(PSO_name, pso_divide_path, pso_divide_name, pso_divide_tail);
			PancystarEngine::PancyConstantBuffer *new_cbuffer = new PancystarEngine::PancyConstantBuffer(cbuffer_name_per_object[i], pso_divide_name);
			check_error = new_cbuffer->Create();
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			SubMemoryPointer submemory;
			check_error = new_cbuffer->GetBufferSubResource(submemory);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			new_point.resource_view_offset_id = globel_offset + i;
			check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_point, submemory);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			per_object_cbuffer.insert(std::pair<std::string, PancystarEngine::PancyConstantBuffer*>(cbuffer_name_per_object[i], new_cbuffer));
		}
		
	}
	globel_offset += cbuffer_name_per_object.size();
	//��ÿ֡���еĳ���������
	for (int i = 0; i < cbuffer_per_frame.size(); ++i)
	{
		SubMemoryPointer submemory;
		check_error = cbuffer_per_frame[i]->GetBufferSubResource(submemory);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildCBV(new_point, submemory);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	globel_offset += cbuffer_per_frame.size();
	//��ÿ֡���е�shader��Դ
	for (int i = 0; i < resource_data_per_frame.size(); ++i)
	{
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_point, resource_data_per_frame[i], resource_desc_per_frame_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	globel_offset += resource_data_per_frame.size();
	//��ÿ��object���е�shader��Դ
	for (int i = 0; i < resource_data_per_object.size(); ++i)
	{
		new_point.resource_view_offset_id = globel_offset + i;
		check_error = PancyDescriptorHeapControl::GetInstance()->BuildSRV(new_point, resource_data_per_object[i], resource_desc_per_object_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferMatrix(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4X4 &data_in,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end()) 
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"Could not find Cbuffer: "+ cbuffer_name +" in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer Matrix",error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetMatrix(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferFloat4(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMFLOAT4 &data_in,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer float4", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetFloat4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferUint4(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const DirectX::XMUINT4 &data_in,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer uint4", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetUint4(variable_name, data_in, offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObject::SetCbufferStructData(
	const std::string &cbuffer_name,
	const std::string &variable_name,
	const void* data_in,
	const pancy_resource_size &data_size,
	const pancy_resource_size &offset
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto cbuffer_data = per_object_cbuffer.find(cbuffer_name);
	if (cbuffer_data == per_object_cbuffer.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not find Cbuffer: " + cbuffer_name + " in DescriptorObject of PSO: " + PSO_name_descriptor);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Set Cbuffer struct", error_message);
		return error_message;
	}
	check_error = cbuffer_data->second->SetStruct(variable_name, data_in, data_size,offset);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
//��������
DescriptorObjectList::DescriptorObjectList(
	const std::string &PSO_name_in,
	const std::string &descriptor_name_in
)
{
	PSO_name = PSO_name_in;
	descriptor_name = descriptor_name_in;
}
DescriptorObjectList::~DescriptorObjectList()
{
	//ɾ�����е�����������
	while (!empty_list.empty())
	{
		auto data = empty_list.front();
		delete data;
		empty_list.pop();
		
	}
	while (!used_list.empty())
	{
		auto data = used_list.front();
		delete data;
		used_list.pop();
		
	}
}
PancystarEngine::EngineFailReason DescriptorObjectList::Create(
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_object_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_object_in
)
{
	PancystarEngine::EngineFailReason check_error;
	//����Դ��Ϣ����
	for (int i = 0; i < cbuffer_name_per_object_in.size(); ++i)
	{
		//���鴫���ÿ�����������������Ƿ�Ϸ�
		pancy_object_id PSO_id;
		check_error = PancyEffectGraphic::GetInstance()->GetPSO(PSO_name, PSO_id);
		if (!check_error.CheckIfSucceed()) 
		{
			return check_error;
		}
		check_error = PancyEffectGraphic::GetInstance()->CheckCbuffer(PSO_id, cbuffer_name_per_object_in[i]);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		cbuffer_name_per_object.push_back(cbuffer_name_per_object_in[i]);
	}
	for (int i = 0; i < cbuffer_per_frame_in.size(); ++i)
	{
		cbuffer_per_frame.push_back(cbuffer_per_frame_in[i]);
	}
	for (int i = 0; i < resource_data_per_frame_in.size(); ++i)
	{
		resource_data_per_frame.push_back(resource_data_per_frame_in[i]);
	}
	for (int i = 0; i < resource_data_per_object_in.size(); ++i)
	{
		resource_data_per_object.push_back(resource_data_per_object_in[i]);
	}
	for (int i = 0; i < resource_desc_per_frame_in.size(); ++i)
	{
		resource_desc_per_frame.push_back(resource_desc_per_frame_in[i]);
	}
	for (int i = 0; i < resource_desc_per_object_in.size(); ++i)
	{
		resource_desc_per_per_object.push_back(resource_desc_per_object_in[i]);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason DescriptorObjectList::GetEmptyList(DescriptorObject** descripto_res)
{
	if (empty_list.size() > 0)
	{
		auto empty_descriptor = empty_list.front();
		empty_list.pop();
		used_list.push(empty_descriptor);
		*descripto_res = empty_descriptor;
	}
	else
	{
		DescriptorObject *new_descriptor_obj;
		new_descriptor_obj = new DescriptorObject();
		auto check_error = new_descriptor_obj->Create(
			PSO_name,
			descriptor_name,
			cbuffer_name_per_object,
			cbuffer_per_frame,
			resource_data_per_frame,
			resource_desc_per_frame,
			resource_data_per_object,
			resource_desc_per_per_object
		);
		if (!check_error.CheckIfSucceed())
		{
			*descripto_res = NULL;
			return check_error;
		}
		used_list.push(new_descriptor_obj);
		*descripto_res = new_descriptor_obj;
	}
	return PancystarEngine::succeed;
}
void DescriptorObjectList::Reset()
{
	//���Ѿ�ʹ����ϵ���������ԭ
	while (!used_list.empty())
	{
		auto empty_descriptor = used_list.front();
		used_list.pop();
		empty_list.push(empty_descriptor);
	}
}
//ģ�͹�����
static PancyModelControl* this_instance = NULL;
PancyModelControl::PancyModelControl(const std::string &resource_type_name_in) :PancyBasicResourceControl(resource_type_name_in)
{

}
PancystarEngine::EngineFailReason PancyModelControl::GetRenderMesh(const pancy_object_id &model_id, const pancy_object_id &submesh_id, PancySubModel **render_mesh)
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(model_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Render mesh From Model", error_message);
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	check_error = model_pointer->GetRenderMesh(submesh_id, render_mesh);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelControl::GetRenderDescriptor(
	const pancy_object_id &model_id,
	const pancy_object_id &PSO_id,
	const std::vector<std::string> &cbuffer_name_per_object_in,
	const std::vector<PancystarEngine::PancyConstantBuffer *> &cbuffer_per_frame_in,
	const std::vector<SubMemoryPointer> &resource_data_per_frame_in,
	const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &resource_desc_per_frame_in,
	DescriptorObject **descriptor_out
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL,"could not find resource ID: " + std::to_string(model_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Render Descriptor From Model", error_message);
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	check_error = model_pointer->GetRenderDescriptor(
		PSO_id, 
		cbuffer_name_per_object_in, 
		cbuffer_per_frame_in, 
		resource_data_per_frame_in, 
		resource_desc_per_frame_in, 
		descriptor_out
	);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelControl::ResetModelRenderDescriptor(const pancy_object_id &model_id)
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(model_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Render Descriptor From Model", error_message);
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	model_pointer->ResetRenderList();
	return PancystarEngine::succeed;

}
PancystarEngine::EngineFailReason PancyModelControl::GetModelBoneMatrix(
	const pancy_object_id &model_id,
	const pancy_resource_id &animation_ID,
	const float &animation_time,
	std::vector<DirectX::XMFLOAT4X4> &matrix_out
) 
{
	PancystarEngine::EngineFailReason check_error;
	auto resource_data = GetResource(model_id);
	if (resource_data == NULL)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find resource ID: " + std::to_string(model_id));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Get Render Descriptor From Model", error_message);
		return error_message;
	}
	PancyBasicModel *model_pointer = dynamic_cast<PancyBasicModel*>(resource_data);
	check_error = model_pointer->GetBoneByAnimation(animation_ID, animation_time, matrix_out);
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyModelControl::BuildResource(
	const Json::Value &root_value,
	const std::string &name_resource_in,
	PancyBasicVirtualResource** resource_out
)
{
	*resource_out = new PancyBasicModel(name_resource_in, root_value);
	return PancystarEngine::succeed;
}
