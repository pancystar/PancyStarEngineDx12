#include"PancyResourceJsonReflect.h"
PancyJsonReflect::PancyJsonReflect()
{
}
const std::string PancyJsonReflect::GetParentName(const std::string &name_in)
{
	int32_t parent_value_tail = name_in.find_last_of('.');
	int32_t parent_pointer_tail = name_in.rfind("->");
	int32_t max_value = max(parent_value_tail, parent_pointer_tail);
	if (max_value <= 0)
	{
		return "";
	}
	return name_in.substr(0, max_value);
}
PancystarEngine::EngineFailReason PancyJsonReflect::AddChildStruct(PancyJsonReflect*data_pointer, const std::string &name, const pancy_resource_size &data_size)
{
	child_node_list[name] = data_pointer;
	child_size_list[name] = data_size;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::AddReflectData(const PancyJsonMemberType &type_data, const std::string &name, const std::string &variable_type_name, void*variable_data, const pancy_object_id &array_size)
{
	JsonReflectData new_data;
	new_data.array_size = array_size;
	new_data.data_type = type_data;
	new_data.data_name = name;
	new_data.parent_name = GetParentName(name);
	new_data.data_pointer = variable_data;
	new_data.data_type_name = variable_type_name;
	//���ڵ��Ƿ�����ظ���ӵ����
	if (parent_list.find(name) != parent_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + name + " has been inited before, do not repeat init");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("init Variable of json reflect class", error_message);
		return error_message;
	}
	if (value_map.find(name) != value_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + name + " has been inited before, do not repeat init");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("init Variable of json reflect class", error_message);
		return error_message;
	}
	//���ܴ����ڵ�Ϊnode����
	std::string next_parent = new_data.parent_name;
	while (next_parent != "")
	{
		std::string parent_next_parent = GetParentName(next_parent);
		if (value_map.find(next_parent) == value_map.end())
		{
			JsonReflectData new_parent_data;
			new_parent_data.data_type = PancyJsonMemberType::json_member_node;
			new_parent_data.data_name = next_parent;
			new_parent_data.parent_name = parent_next_parent;
			new_parent_data.data_pointer = variable_data;
			value_map.insert(std::pair<std::string, JsonReflectData>(next_parent, new_parent_data));
			parent_list.insert(std::pair<std::string, std::string>(next_parent, parent_next_parent));
		}
		next_parent = parent_next_parent;
	}
	parent_list.insert(std::pair<std::string, std::string>(name, new_data.parent_name));
	value_map.insert(std::pair<std::string, JsonReflectData>(name, new_data));
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::AddVariable(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name)
{
	PancystarEngine::EngineFailReason check_error;
	PancyJsonMemberType now_variable_type;
	now_variable_type = PancyJsonTool::GetInstance()->GetVariableJsonType(variable_type);
	if (now_variable_type == PancyJsonMemberType::json_member_unknown)
	{
		if (CheckIfStruct(variable_type_name))
		{
			now_variable_type = PancyJsonMemberType::json_member_node;
		}
		else if (CheckIfStructList(variable_type_name))
		{
			now_variable_type = PancyJsonMemberType::json_member_node_list;
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not recognize type: " + variable_type_name + " check for init or try AddArray()");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::AddVariable", error_message);
			return error_message;
		}
	}
	check_error = AddReflectData(now_variable_type, name, variable_type_name, variable_data);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::AddArray(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name, const pancy_object_id &array_size)
{
	PancystarEngine::EngineFailReason check_error;
	PancyJsonMemberType now_variable_type;
	now_variable_type = PancyJsonTool::GetInstance()->GetVariableJsonType(variable_type);
	if (now_variable_type == PancyJsonMemberType::json_member_unknown)
	{
		if (CheckIfStruct(variable_type_name))
		{
			now_variable_type = PancyJsonMemberType::json_member_node_array;
		}
		else
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not recognize array type: " + variable_type_name + " check for init or try AddVariable()");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::AddArray", error_message);
			return error_message;
		}
	}
	check_error = AddReflectData(now_variable_type, name, variable_type_name, variable_data, array_size);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
bool PancyJsonReflect::CheckIfStruct(const std::string &variable_type_name)
{
	if (
		variable_type_name[0] == 's' &&
		variable_type_name[1] == 't' &&
		variable_type_name[2] == 'r' &&
		variable_type_name[3] == 'u' &&
		variable_type_name[4] == 'c' &&
		variable_type_name[5] == 't'
		)
	{
		return true;
	}
	return false;
}
bool PancyJsonReflect::CheckIfStructList(const std::string &variable_type_name)
{
	if (child_node_list.find(variable_type_name) != child_node_list.end())
	{
		return true;
	}
	return false;
}
void PancyJsonReflect::Create()
{
	InitChildReflectClass();
	InitBasicVariable();
	BuildChildValueMap();
}
void PancyJsonReflect::BuildChildValueMap()
{
	for (auto now_member_value = parent_list.begin(); now_member_value != parent_list.end(); ++now_member_value)
	{
		child_value_list.emplace(now_member_value->first, std::vector<std::string>());
	}
	for (auto now_member_value = parent_list.begin(); now_member_value != parent_list.end(); ++now_member_value)
	{
		if (now_member_value->second != "")
		{
			child_value_list[now_member_value->second].push_back(now_member_value->first);
		}
	}
}
PancystarEngine::EngineFailReason PancyJsonReflect::TranslateStringToEnum(const std::string &basic_string, std::string &enum_type, std::string &enum_value_string, int32_t &enum_value_data)
{
	auto divide_part = basic_string.find("::");
	if (divide_part < 0)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not translate enum variable: " + basic_string);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::TranslateStringToEnum", error_message);
		return error_message;
	}
	enum_type = basic_string.substr(0, divide_part);
	enum_value_string = basic_string.substr(divide_part + 2);
	enum_value_data = PancyJsonTool::GetInstance()->GetEnumValue(enum_type, enum_value_string);
	if (enum_value_data == -1)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "haven't init enum variable: " + enum_type + " :: " + enum_value_string);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::TranslateStringToEnum", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
//������ͨ����
PancystarEngine::EngineFailReason PancyJsonReflect::SetIntValue(JsonReflectData &reflect_data, const int64_t &int_value)
{
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_uint8:
	{
		uint8_t* data_pointer = reinterpret_cast<uint8_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint8_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_uint16:
	{
		uint16_t* data_pointer = reinterpret_cast<uint16_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint16_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_uint32:
	{
		uint32_t* data_pointer = reinterpret_cast<uint32_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint32_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_uint64:
	{
		uint64_t* data_pointer = reinterpret_cast<uint64_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<uint64_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int8:
	{
		int8_t* data_pointer = reinterpret_cast<int8_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int8_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int16:
	{
		int16_t* data_pointer = reinterpret_cast<int16_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int16_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int32:
	{
		int32_t* data_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int32_t>(int_value);
	}
	break;
	case PancyJsonMemberType::json_member_int64:
	{
		int64_t* data_pointer = reinterpret_cast<int64_t*>(reflect_data.data_pointer);
		*data_pointer = static_cast<int64_t>(int_value);
	}
	break;
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a int value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntValue", error_message);
		return error_message;
	}
	break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetDoubleValue(JsonReflectData &reflect_data, const double &double_value)
{
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_float:
	{
		float* data_pointer = reinterpret_cast<float*>(reflect_data.data_pointer);
		*data_pointer = static_cast<float>(double_value);
	}
	break;
	case PancyJsonMemberType::json_member_double:
	{
		double* data_pointer = reinterpret_cast<double*>(reflect_data.data_pointer);
		*data_pointer = static_cast<double>(double_value);
	}
	break;
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a double value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetDoubleValue", error_message);
		return error_message;
	}
	break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetBoolValue(JsonReflectData &reflect_data, const bool &bool_value)
{
	if (reflect_data.data_type != PancyJsonMemberType::json_member_bool)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a bool value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetBoolValue", error_message);
		return error_message;
	}
	bool* data_pointer = reinterpret_cast<bool*>(reflect_data.data_pointer);
	*data_pointer = bool_value;
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetStringValue(JsonReflectData &reflect_data, const std::string &string_value)
{
	PancystarEngine::EngineFailReason check_error;
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_string:
	{
		//��ͨ���ַ���
		std::string* data_pointer = reinterpret_cast<std::string*>(reflect_data.data_pointer);
		*data_pointer = static_cast<std::string>(string_value);
	}
	break;
	case PancyJsonMemberType::json_member_enum:
	{
		//ö�ٱ����ַ���
		std::string value_enum_type;
		std::string value_enum_value_name;
		int32_t enum_value;
		check_error = TranslateStringToEnum(string_value, value_enum_type, value_enum_value_name, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//����ö�ٱ��������ͣ���ö�ٱ�����ֵ
		if (value_enum_type != reflect_data.data_type_name)
		{
			//ö�����Ͳ�ƥ��
			PancystarEngine::EngineFailReason error_message(E_FAIL, "enum member: " + reflect_data.data_name + " :type dismatch: " + value_enum_type + " Dismatch�� " + reflect_data.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringValue", error_message);
			return error_message;
		}
		auto check_error = PancyJsonTool::GetInstance()->SetEnumMemberValue(value_enum_type, reflect_data.data_pointer, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//int32_t* data_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer);
		//*data_pointer = enum_value;
	}
	break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a string value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringValue", error_message);
		return error_message;
		break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetIntArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const int64_t &int_value)
{
	PancystarEngine::EngineFailReason check_error;
	JsonReflectData single_member;
	//��������ǲ���vector���͵�����
	bool if_vector_value = true;
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_int8_list:
	{
		check_error = SetVectorValue<int8_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int16_list:
	{
		check_error = SetVectorValue<int16_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int32_list:
	{
		check_error = SetVectorValue<int32_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int64_list:
	{
		check_error = SetVectorValue<int64_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint8_list:
	{
		check_error = SetVectorValue<uint8_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint16_list:
	{
		check_error = SetVectorValue<uint16_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint32_list:
	{
		check_error = SetVectorValue<uint32_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint64_list:
	{
		check_error = SetVectorValue<uint64_t>(reflect_data, offset_value, int_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	default:
	{
		if_vector_value = false;
	}
	}
	if (if_vector_value)
	{
		//�Ѿ��ɹ�����vector�����˳�
		return PancystarEngine::succeed;
	}
	//�ȼ�������Ƿ�Խ��
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_int8_array:
		single_member.data_type = PancyJsonMemberType::json_member_int8;
		single_member.data_pointer = reinterpret_cast<int8_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_int16_array:
		single_member.data_type = PancyJsonMemberType::json_member_int16;
		single_member.data_pointer = reinterpret_cast<int16_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_int32_array:
		single_member.data_type = PancyJsonMemberType::json_member_int32;
		single_member.data_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_int64_array:
		single_member.data_type = PancyJsonMemberType::json_member_int64;
		single_member.data_pointer = reinterpret_cast<int64_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint8_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint8;
		single_member.data_pointer = reinterpret_cast<uint8_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint16_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint16;
		single_member.data_pointer = reinterpret_cast<uint16_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint32_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint32;
		single_member.data_pointer = reinterpret_cast<uint32_t*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_uint64_array:
		single_member.data_type = PancyJsonMemberType::json_member_uint64;
		single_member.data_pointer = reinterpret_cast<uint64_t*>(reflect_data.data_pointer) + offset_value;
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a int array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
		break;
	}

	check_error = SetIntValue(single_member, int_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetDoubleArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const double &double_value)
{
	PancystarEngine::EngineFailReason check_error;
	JsonReflectData single_member;
	//��������ǲ���vector���͵�����
	bool if_vector_value = true;
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_double_list:
	{
		check_error = SetVectorValue<double>(reflect_data, offset_value, double_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_float_list:
	{
		check_error = SetVectorValue<float>(reflect_data, offset_value, double_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	default:
	{
		if_vector_value = false;
	}
	}
	if (if_vector_value)
	{
		//�Ѿ��ɹ�����vector�����˳�
		return PancystarEngine::succeed;
	}
	//�ȼ�������Ƿ�Խ��
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_double_array:
		single_member.data_type = PancyJsonMemberType::json_member_double;
		single_member.data_pointer = reinterpret_cast<double*>(reflect_data.data_pointer) + offset_value;
		break;
	case PancyJsonMemberType::json_member_float_array:
		single_member.data_type = PancyJsonMemberType::json_member_float;
		single_member.data_pointer = reinterpret_cast<float*>(reflect_data.data_pointer) + offset_value;
		break;
	default:
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a double array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
		break;
	}
	check_error = SetDoubleValue(single_member, double_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetBoolArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const bool &bool_value)
{
	JsonReflectData single_member;
	//��������ǲ���vector���͵�����
	if (reflect_data.data_type == PancyJsonMemberType::json_member_bool_list)
	{
		return SetVectorValue<bool>(reflect_data, offset_value, bool_value);
	}
	if (reflect_data.data_type != PancyJsonMemberType::json_member_bool_array)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a bool array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	//�ȼ�������Ƿ�Խ��
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	single_member.data_type = PancyJsonMemberType::json_member_bool;
	single_member.data_pointer = reinterpret_cast<bool*>(reflect_data.data_pointer) + offset_value;
	PancystarEngine::EngineFailReason check_error;
	check_error = SetBoolValue(single_member, bool_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetStringArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const std::string &string_value)
{
	PancystarEngine::EngineFailReason check_error;
	JsonReflectData single_member;
	//��������ǲ���vector���͵�����
	if (reflect_data.data_type == PancyJsonMemberType::json_member_string_list)
	{
		return SetVectorValue<std::string>(reflect_data, offset_value, string_value);
	}
	else if (reflect_data.data_type == PancyJsonMemberType::json_member_enum_list)
	{
		//todo:enum������
		std::string value_enum_type;
		std::string value_enum_value_name;
		int32_t enum_value;
		check_error = TranslateStringToEnum(string_value, value_enum_type, value_enum_value_name, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string now_dealed_enum_type;
		PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, now_dealed_enum_type);
		if (now_dealed_enum_type != value_enum_type)
		{
			//ö�����Ͳ�ƥ��
			PancystarEngine::EngineFailReason error_message(E_FAIL, "enum member: " + reflect_data.data_name + " :type dismatch: " + now_dealed_enum_type + " Dismatch�� " + value_enum_type);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringArrayValue", error_message);
			return error_message;
		}
		return PancyJsonTool::GetInstance()->SetEnumVectorValue(value_enum_type, reflect_data.data_pointer, offset_value, enum_value);
	}
	//�ȼ�������Ƿ�Խ��
	if (offset_value >= reflect_data.array_size)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	if (reflect_data.data_type == PancyJsonMemberType::json_member_string_array)
	{
		single_member.data_type = PancyJsonMemberType::json_member_string;
		single_member.data_pointer = reinterpret_cast<std::string*>(reflect_data.data_pointer) + offset_value;
		PancystarEngine::EngineFailReason check_error;
		check_error = SetStringValue(single_member, string_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else if (reflect_data.data_type == PancyJsonMemberType::json_member_enum_array)
	{
		std::string value_enum_type, value_enum_value_name;
		int32_t enum_value;
		//�Ƚ��ַ������ݽ���Ϊö������
		check_error = TranslateStringToEnum(string_value, value_enum_type, value_enum_value_name, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//��ȡ����ö�ٱ�������ʵ��������
		std::string member_enum_type_name;
		check_error = PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, member_enum_type_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		//��������Ƿ�ƥ��
		if (value_enum_type != member_enum_type_name)
		{
			//ö�����Ͳ�ƥ��
			PancystarEngine::EngineFailReason error_message(E_FAIL, "enum member: " + reflect_data.data_name + " :type dismatch: " + value_enum_type + " Dismatch�� " + reflect_data.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringArrayValue", error_message);
			return error_message;
		}
		auto check_error = PancyJsonTool::GetInstance()->SetEnumArrayValue(value_enum_type, reflect_data.data_pointer, offset_value, enum_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a string array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetStringArrayValue", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SetNodeArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const Json::Value &now_child_value)
{
	PancystarEngine::EngineFailReason check_error;
	//���ݽڵ�����ʹ���һ���µĴ������������ڵ�����
	auto child_reflect_pointer = child_node_list.find(reflect_data.data_type_name);
	if (child_reflect_pointer == child_node_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node reflect: " + reflect_data.data_type_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetNodeArrayValue", error_message);
		return error_message;
	}
	check_error = child_reflect_pointer->second->LoadFromJsonNode("reflect_data", reflect_data.data_name + "array:" + std::to_string(offset_value), now_child_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	auto now_data_type_size = child_size_list.find(reflect_data.data_type_name);
	if (now_data_type_size == child_size_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct size reflect: " + reflect_data.data_type_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetNodeArrayValue", error_message);
		return error_message;
	}
	if (reflect_data.data_type == PancyJsonMemberType::json_member_node_list)
	{
		//vector���͵Ľڵ�����
		check_error = child_reflect_pointer->second->CopyVectorData(reflect_data.data_pointer, reflect_data.data_type_name, offset_value, now_data_type_size->second);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	else if (reflect_data.data_type == PancyJsonMemberType::json_member_node_array)
	{
		//�ȼ�������Ƿ�Խ��
		if (offset_value >= reflect_data.array_size)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(offset_value));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetNodeArrayValue", error_message);
			return error_message;
		}
		//�ȸ���ƫ�������ڵ������ָ��ƫ�Ƶ���ȷ��λ��
		char* now_array_pointer = reinterpret_cast<char*>(reflect_data.data_pointer);
		now_array_pointer = now_array_pointer + offset_value * now_data_type_size->second;
		//������������ݿ�����ָ��λ��
		child_reflect_pointer->second->CopyMemberData(now_array_pointer, reflect_data.data_type_name, now_data_type_size->second);
	}
	else
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + reflect_data.data_name + " is not a node array value");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetNodeArrayValue", error_message);
		return error_message;
	}
	return PancystarEngine::succeed;
}
//�����������
PancystarEngine::EngineFailReason PancyJsonReflect::SetArrayValue(JsonReflectData &reflect_data, const Json::Value &now_child_value)
{
	PancystarEngine::EngineFailReason check_error;
	reflect_data.real_used_size = now_child_value.size();
	for (int32_t array_index = 0; array_index < now_child_value.size(); ++array_index)
	{
		//�ȼ�������Ƿ�Խ��
		if (array_index >= reflect_data.array_size)
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the array variable: " + reflect_data.data_name + " size only have " + std::to_string(reflect_data.array_size) + " could not get index" + std::to_string(array_index));
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
			return error_message;
		}
		switch (now_child_value[array_index].type())
		{
		case Json::intValue:
		{
			check_error = SetIntArrayValue(reflect_data, array_index, now_child_value[array_index].asInt64());
		}
		break;
		case Json::uintValue:
		{
			check_error = SetIntArrayValue(reflect_data, array_index, now_child_value[array_index].asInt64());
		}
		break;
		case Json::realValue:
		{
			check_error = SetDoubleArrayValue(reflect_data, array_index, now_child_value[array_index].asDouble());
		}
		break;
		case Json::booleanValue:
		{
			check_error = SetBoolArrayValue(reflect_data, array_index, now_child_value[array_index].asBool());
		}
		break;
		case Json::stringValue:
		{
			check_error = SetStringArrayValue(reflect_data, array_index, now_child_value[array_index].asString());
		}
		break;
		case Json::objectValue:
		{
			check_error = SetNodeArrayValue(reflect_data, array_index, now_child_value[array_index]);
		}
		break;
		//todo:�ڵ�����
		default:
			break;
		}
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonFile(const std::string &Json_file)
{
	Json::Value jsonRoot;
	auto check_error = PancyJsonTool::GetInstance()->LoadJsonFile(Json_file, jsonRoot);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return LoadFromJsonNode("reflect_data", Json_file, jsonRoot);
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonMemory(const std::string &value_name, const Json::Value &root_value)
{
	return LoadFromJsonNode("reflect_data", value_name, root_value);
}

//��ȡ��ͨ����
PancystarEngine::EngineFailReason PancyJsonReflect::SaveSingleValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	switch (reflect_data.data_type)
	{
	case PancyJsonMemberType::json_member_int8:
	{
		int8_t* now_value_pointer = reinterpret_cast<int8_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<int64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_int16:
	{
		int16_t* now_value_pointer = reinterpret_cast<int16_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<int64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_int32:
	{
		int32_t* now_value_pointer = reinterpret_cast<int32_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<int64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_int64:
	{
		int64_t* now_value_pointer = reinterpret_cast<int64_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_uint8:
	{
		uint8_t* now_value_pointer = reinterpret_cast<uint8_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<uint64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_uint16:
	{
		uint16_t* now_value_pointer = reinterpret_cast<uint16_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<uint64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_uint32:
	{
		uint32_t* now_value_pointer = reinterpret_cast<uint32_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<uint64_t>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_uint64:
	{
		uint64_t* now_value_pointer = reinterpret_cast<uint64_t*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_float:
	{
		float* now_value_pointer = reinterpret_cast<float*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<double>(*now_value_pointer));
		break;
	}
	case PancyJsonMemberType::json_member_double:
	{
		double* now_value_pointer = reinterpret_cast<double*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_bool:
	{
		bool* now_value_pointer = reinterpret_cast<bool*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_string:
	{
		string* now_value_pointer = reinterpret_cast<string*>(reflect_data.data_pointer);
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), *now_value_pointer);
		break;
	}
	case PancyJsonMemberType::json_member_enum:
	{
		PancystarEngine::EngineFailReason check_error;
		int32_t enum_value_int = -1;
		check_error = PancyJsonTool::GetInstance()->GetEnumMemberValue(reflect_data.data_type_name, reflect_data.data_pointer, enum_value_int);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string enum_name_final = PancyJsonTool::GetInstance()->GetEnumName(reflect_data.data_type_name, enum_value_int);
		if (enum_name_final == "")
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse enum value to string: " + reflect_data.data_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveSingleValueMemberToJson", error_message);
			return error_message;
		}
		enum_name_final = reflect_data.data_type_name + "::" + enum_name_final;
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), enum_name_final);
		break;
	}
	default:
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not recognize JSON reflect type: " + reflect_data.data_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveSingleValueMemberToJson", error_message);
		return error_message;
	}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveArrayEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	for (int32_t now_enum_index = 0; now_enum_index < reflect_data.real_used_size; ++now_enum_index)
	{
		std::string member_enum_type_name;
		check_error = PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, member_enum_type_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		int32_t enum_value_int = -1;
		check_error = PancyJsonTool::GetInstance()->GetEnumArrayValue(member_enum_type_name, reflect_data.data_pointer, now_enum_index, enum_value_int);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string enum_name_final = PancyJsonTool::GetInstance()->GetEnumName(member_enum_type_name, enum_value_int);
		if (enum_name_final == "")
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse enum value to string: " + reflect_data.data_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveArrayEnumMemberToJson", error_message);
			return error_message;
		}
		enum_name_final = member_enum_type_name + "::" + enum_name_final;
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), enum_name_final);
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveVectorEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	for (int32_t now_enum_index = 0; now_enum_index < reflect_data.real_used_size; ++now_enum_index)
	{
		std::string member_enum_type_name;
		check_error = PancyJsonTool::GetInstance()->GetEnumNameByPointerName(reflect_data.data_type_name, member_enum_type_name);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		int32_t enum_value_int = -1;
		check_error = PancyJsonTool::GetInstance()->GetEnumVectorValue(reflect_data.data_type_name, reflect_data.data_pointer, now_enum_index, enum_value_int);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		std::string enum_name_final = PancyJsonTool::GetInstance()->GetEnumName(reflect_data.data_type_name, enum_value_int);
		if (enum_name_final == "")
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not parse enum value to string: " + reflect_data.data_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveVectorEnumMemberToJson", error_message);
			return error_message;
		}
		enum_name_final = member_enum_type_name + "::" + enum_name_final;
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), enum_name_final);
	}
	return PancystarEngine::succeed;
}

PancystarEngine::EngineFailReason PancyJsonReflect::SaveToJsonMemory(Json::Value &root_value)
{
	auto child_value_member = child_value_list.find("reflect_data");
	if (child_value_member == child_value_list.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: reflect_data");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonMemory", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error;
	for (int32_t child_value_index = 0; child_value_index < child_value_member->second.size(); ++child_value_index)
	{
		std::string now_child_name = child_value_member->second[child_value_index];
		check_error = SaveToJsonNode(now_child_name, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveToJsonFile(const std::string &json_name)
{
	Json::Value file_value;
	auto check_error = SaveToJsonMemory(file_value);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	check_error = PancyJsonTool::GetInstance()->WriteValueToJson(file_value, json_name);
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::SaveToJsonNode(const std::string &parent_name, Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	//Json::Value root_value;
	auto now_reflect_data = value_map.find(parent_name);
	if (now_reflect_data == value_map.end())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + parent_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
		return error_message;
	}
	switch (now_reflect_data->second.data_type)
	{
	case PancyJsonMemberType::json_member_int8:
	case PancyJsonMemberType::json_member_int16:
	case PancyJsonMemberType::json_member_int32:
	case PancyJsonMemberType::json_member_int64:
	case PancyJsonMemberType::json_member_uint8:
	case PancyJsonMemberType::json_member_uint16:
	case PancyJsonMemberType::json_member_uint32:
	case PancyJsonMemberType::json_member_uint64:
	case PancyJsonMemberType::json_member_float:
	case PancyJsonMemberType::json_member_double:
	case PancyJsonMemberType::json_member_bool:
	case PancyJsonMemberType::json_member_string:
	case PancyJsonMemberType::json_member_enum:
	{
		check_error = SaveSingleValueMemberToJson(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int8_array:
	{
		check_error = SaveArrayValueMemberToJson<int8_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int16_array:
	{
		check_error = SaveArrayValueMemberToJson<int16_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int32_array:
	{
		check_error = SaveArrayValueMemberToJson<int32_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int64_array:
	{
		check_error = SaveArrayValueMemberToJson<int64_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint8_array:
	{
		check_error = SaveArrayValueMemberToJson<uint8_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint16_array:
	{
		check_error = SaveArrayValueMemberToJson<uint16_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint32_array:
	{
		check_error = SaveArrayValueMemberToJson<uint32_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint64_array:
	{
		check_error = SaveArrayValueMemberToJson<uint64_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_float_array:
	{
		check_error = SaveArrayValueMemberToJson<float, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_double_array:
	{
		check_error = SaveArrayValueMemberToJson<double, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_bool_array:
	{
		check_error = SaveArrayValueMemberToJson<bool, bool>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_string_array:
	{
		check_error = SaveArrayValueMemberToJson<std::string, std::string>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_enum_array:
	{
		check_error = SaveArrayEnumMemberToJson(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int8_list:
	{
		check_error = SaveVectorValueMemberToJson<int8_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int16_list:
	{
		check_error = SaveVectorValueMemberToJson<int16_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int32_list:
	{
		check_error = SaveVectorValueMemberToJson<int32_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_int64_list:
	{
		check_error = SaveVectorValueMemberToJson<int64_t, int64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint8_list:
	{
		check_error = SaveVectorValueMemberToJson<uint8_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint16_list:
	{
		check_error = SaveVectorValueMemberToJson<uint16_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint32_list:
	{
		check_error = SaveVectorValueMemberToJson<uint32_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_uint64_list:
	{
		check_error = SaveVectorValueMemberToJson<uint64_t, uint64_t>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_float_list:
	{
		check_error = SaveVectorValueMemberToJson<float, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_double_list:
	{
		check_error = SaveVectorValueMemberToJson<double, double>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_bool_list:
	{
		check_error = SaveVectorValueMemberToJson<bool, bool>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_string_list:
	{
		check_error = SaveVectorValueMemberToJson<std::string, std::string>(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_enum_list:
	{
		check_error = SaveVectorEnumMemberToJson(now_reflect_data->second, root_value);
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		break;
	}
	case PancyJsonMemberType::json_member_node:
	{
		auto child_value_member = child_value_list.find(parent_name);
		if (child_value_member == child_value_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + parent_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		Json::Value struct_root_value;
		for (int32_t child_value_index = 0; child_value_index < child_value_member->second.size(); ++child_value_index)
		{
			std::string now_child_name = child_value_member->second[child_value_index];
			Json::Value child_root_value;
			check_error = SaveToJsonNode(now_child_name, struct_root_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		PancyJsonTool::GetInstance()->SetJsonValue(root_value, TranslateFullNameToRealName(now_reflect_data->second.data_name), struct_root_value);
		break;
	}
	case PancyJsonMemberType::json_member_node_array:
	{
		//���ݽڵ�����ͻ�ȡ��Ӧ�Ĵ������������ڵ�����
		auto child_reflect_pointer = child_node_list.find(now_reflect_data->second.data_type_name);
		if (child_reflect_pointer == child_node_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node reflect: " + now_reflect_data->second.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		auto child_size_pointer = child_size_list.find(now_reflect_data->second.data_type_name);
		if (child_reflect_pointer == child_node_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node size reflect: " + now_reflect_data->second.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		for (int32_t node_index_array = 0; node_index_array < now_reflect_data->second.real_used_size; ++node_index_array)
		{
			Json::Value array_member_value;
			check_error = child_reflect_pointer->second->ResetMemoryByArrayData(now_reflect_data->second.data_pointer, now_reflect_data->second.data_type_name, node_index_array, child_size_pointer->second);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			check_error = child_reflect_pointer->second->SaveToJsonMemory(array_member_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(now_reflect_data->second.data_name), array_member_value);
		}
		break;
	}
	case PancyJsonMemberType::json_member_node_list:
	{
		//���ݽڵ�����ͻ�ȡ��Ӧ�Ĵ������������ڵ�����
		auto child_reflect_pointer = child_node_list.find(now_reflect_data->second.data_type_name);
		if (child_reflect_pointer == child_node_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node reflect: " + now_reflect_data->second.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		auto child_size_pointer = child_size_list.find(now_reflect_data->second.data_type_name);
		if (child_reflect_pointer == child_node_list.end())
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find struct node size reflect: " + now_reflect_data->second.data_type_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveToJsonNode", error_message);
			return error_message;
		}
		for (int32_t node_index_array = 0; node_index_array < now_reflect_data->second.real_used_size; ++node_index_array)
		{
			Json::Value array_member_value;
			check_error = child_reflect_pointer->second->ResetMemoryByVectorData(now_reflect_data->second.data_pointer, now_reflect_data->second.data_type_name, node_index_array, child_size_pointer->second);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			check_error = child_reflect_pointer->second->SaveToJsonMemory(array_member_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
			PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(now_reflect_data->second.data_name), array_member_value);
		}
		break;
	}
	default:
		break;
	}
	return PancystarEngine::succeed;
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonArray(const std::string &value_name, const Json::Value &root_value)
{
	auto now_reflect_data = value_map.find(value_name);
	if (now_reflect_data == value_map.end())
	{
		//δ�ҵ���Ӧ�ķ�������
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + value_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::LoadFromJson", error_message);
		return error_message;
	}
	return SetArrayValue(now_reflect_data->second, root_value);
}
PancystarEngine::EngineFailReason PancyJsonReflect::LoadFromJsonNode(const std::string &parent_name, const std::string &value_name, const Json::Value &root_value)
{
	PancystarEngine::EngineFailReason check_error;
	std::vector<std::string> all_member_name = root_value.getMemberNames();
	std::vector<Json::ValueType> check_value;
	for (int member_index = 0; member_index < all_member_name.size(); ++member_index)
	{
		//��ȡ�ӽڵ��json����
		Json::Value now_child_value = root_value.get(all_member_name[member_index], Json::Value::null);
		//���Ҷ�Ӧ�ڵ�ķ�����Ϣ
		std::string now_combine_value_common = parent_name;//ͨ��.���ӵĽṹ����Ϣ
		std::string now_combine_value_pointer = parent_name;//ͨ��->���ӵĽṹ��ָ����Ϣ
		if (parent_name != "")
		{
			now_combine_value_common += ".";
			now_combine_value_pointer += "->";
		}
		now_combine_value_common += all_member_name[member_index];
		now_combine_value_pointer += all_member_name[member_index];
		//�洢��ǰ�ڵ�����������ƣ�������Щnode����.������ӽڵ㣬����Щ��Ҫ����->���
		std::string now_node_name = now_combine_value_common;
		auto now_reflect_data = value_map.find(now_combine_value_common);
		if (now_reflect_data == value_map.end())
		{
			now_reflect_data = value_map.find(now_combine_value_pointer);
			if (now_reflect_data == value_map.end())
			{
				//δ�ҵ���Ӧ�ķ�������
				PancystarEngine::EngineFailReason error_message(E_FAIL, "could not find JSON reflect variable: " + now_combine_value_common + " while load json node " + value_name);
				PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::LoadFromJson", error_message);
				return error_message;
			}
			now_node_name = now_combine_value_pointer;
		}
		auto json_data_type = now_child_value.type();
		//���ݷ������ݵ����ͣ��������ݵ����뷽ʽ
		switch (json_data_type)
		{
		case Json::nullValue:
		{
			//�𻵵�����
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the variable: " + all_member_name[member_index] + " could not be recognized by json tool");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::LoadFromJson", error_message);
			return error_message;
		}
		break;
		case Json::intValue:
		{
			int64_t int_data = now_child_value.asInt64();
			check_error = SetIntValue(now_reflect_data->second, int_data);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::uintValue:
		{
			int64_t int_data = now_child_value.asInt64();
			check_error = SetIntValue(now_reflect_data->second, int_data);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::realValue:
		{
			double double_value = now_child_value.asDouble();
			check_error = SetDoubleValue(now_reflect_data->second, double_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::stringValue:
		{
			std::string string_value = now_child_value.asString();
			check_error = SetStringValue(now_reflect_data->second, string_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::booleanValue:
		{
			bool bool_value = now_child_value.asDouble();
			check_error = SetBoolValue(now_reflect_data->second, bool_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::arrayValue:
		{
			check_error = LoadFromJsonArray(now_node_name, now_child_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		case Json::objectValue:
		{
			//�ڵ�����
			check_error = LoadFromJsonNode(now_node_name, value_name, now_child_value);
			if (!check_error.CheckIfSucceed())
			{
				return check_error;
			}
		}
		break;
		default:
			break;
		}
	}
	return PancystarEngine::succeed;
}
std::string PancyJsonReflect::TranslateFullNameToRealName(const std::string &full_name)
{
	int32_t self_offset = 1;
	int32_t offset_value = full_name.rfind(".");
	int32_t offset_pointer = full_name.rfind("->");
	if (offset_value < offset_pointer)
	{
		self_offset = 2;
		offset_value = offset_pointer;
	}
	std::string real_name;
	if (offset_value != -1)
	{
		real_name = full_name.substr(self_offset + offset_value, real_name.size() - (self_offset + offset_value));
	}
	else
	{
		real_name = full_name;
	}
	return real_name;
}
