#pragma once
#include"PancystarEngineBasicDx12.h"
#include <typeinfo.h>
#include<type_traits>
#include"PancyJsonTool.h"
#define Init_Json_Data_Vatriable(var_name) AddAnyVariable(#var_name, var_name)
//�������ݵķ��䱣������
struct JsonReflectData
{
	PancyJsonMemberType data_type;  //��ʶ���ݵĻ�����ʽ
	std::string data_name;          //��ʶ���ݵ�����
	std::string parent_name;        //��ʶ���ݵĸ��ṹ������
	std::string data_type_name;     //��ʶ���ݵ�ϸ����������
	pancy_object_id array_size = 1;     //��ʶ���ݵ��������ɴ�С(��������vector)
	pancy_object_id real_used_size = 1; //��ʶ���ݵ���ʵʹ�ô�С(��������vector)
	void* data_pointer = NULL;             //���ݵ�����
};
//����������
class PancyJsonReflect
{
	//��json���ݰ󶨵���ͨ�ṹ������ݽṹ
	std::unordered_map<std::string, JsonReflectData> value_map;                //ÿ����Ա������Ӧ��ֵ
	std::unordered_map<std::string, std::string> parent_list;                  //ÿ����Ա�����ĸ��ڵ�
	std::unordered_map<std::string, std::vector<std::string>> child_value_list;//ÿ����Ա�������ӱ���
	std::unordered_map<std::string, PancyJsonReflect*> child_node_list;        //ÿ������ṹ����ӽڵ㴦���
	std::unordered_map<std::string, pancy_resource_size> child_size_list;      //ÿ������ṹ��ĳ�Ա������С��¼��

public:
	PancyJsonReflect();
	void Create();
	//��json�ļ��м�������
	PancystarEngine::EngineFailReason LoadFromJsonFile(const std::string &Json_file);
	//��json�ڴ��м�������
	PancystarEngine::EngineFailReason LoadFromJsonMemory(const std::string &value_name, const Json::Value &root_value);
	//�����Ա���ݴ洢��json�ļ�
	PancystarEngine::EngineFailReason SaveToJsonFile(const std::string &json_name);
	//�����Ա���ݴ洢��json�ڴ�
	PancystarEngine::EngineFailReason SaveToJsonMemory(Json::Value &root_value);
	//�����Ա���ݿ�����ָ��ָ��
	virtual PancystarEngine::EngineFailReason CopyMemberData(void *dst_pointer, const std::string &data_type_name, const pancy_resource_size &size) = 0;
	//�����Ա������ӵ�vector��
	virtual PancystarEngine::EngineFailReason CopyVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_resource_size &index, const pancy_resource_size &size) = 0;
	//��ָ��ָ������ݿ��������Ա����
	virtual PancystarEngine::EngineFailReason ResetMemoryByArrayData(void *array_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) = 0;
	//��ָ��vector�����ݿ��������Ա����
	virtual PancystarEngine::EngineFailReason ResetMemoryByVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) = 0;
private:
	//�����ӽڵ�ӳ��
	void BuildChildValueMap();
	//��json�ڵ��м�������
	PancystarEngine::EngineFailReason LoadFromJsonNode(const std::string &parent_name, const std::string &value_name, const Json::Value &root_value);
	//��json�����м�������
	PancystarEngine::EngineFailReason LoadFromJsonArray(const std::string &value_name, const Json::Value &root_value);
	//������д�뵽json�ڵ�
	PancystarEngine::EngineFailReason SaveToJsonNode(const std::string &parent_name, Json::Value &root_value);
	//���ݽṹ�巴��Ϊ���࿪�ٷ�����Ϣ
	virtual PancystarEngine::EngineFailReason InitChildReflectClass() = 0;
	//ע�����еķ������
	virtual void InitBasicVariable() = 0;
	//��ȡ�ڵ�ĸ��ڵ�����
	const std::string GetParentName(const std::string &name_in);
	//���û�������
	PancystarEngine::EngineFailReason SetIntValue(JsonReflectData &reflect_data, const int64_t &int_value);
	PancystarEngine::EngineFailReason SetDoubleValue(JsonReflectData &reflect_data, const double &double_value);
	PancystarEngine::EngineFailReason SetBoolValue(JsonReflectData &reflect_data, const bool &bool_value);
	PancystarEngine::EngineFailReason SetStringValue(JsonReflectData &reflect_data, const std::string &string_value);
	//�����������
	PancystarEngine::EngineFailReason SetArrayValue(JsonReflectData &reflect_data, const Json::Value &now_child_value);
	PancystarEngine::EngineFailReason SetIntArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const int64_t &int_value);
	PancystarEngine::EngineFailReason SetDoubleArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const double &double_value);
	PancystarEngine::EngineFailReason SetBoolArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const bool &bool_value);
	PancystarEngine::EngineFailReason SetStringArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const std::string &string_value);
	PancystarEngine::EngineFailReason SetNodeArrayValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const Json::Value &now_child_value);
	//����vector����
	template<class T1, class T2>
	PancystarEngine::EngineFailReason SetVectorValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const T2 &input_value);
	//��ȡ��������
	PancystarEngine::EngineFailReason SaveSingleValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	//��ȡ�������
	template<typename ArrayType, typename JsonType>
	PancystarEngine::EngineFailReason SaveArrayValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	PancystarEngine::EngineFailReason SaveArrayEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	//��ȡvector����
	template<typename ArrayType, typename JsonType>
	PancystarEngine::EngineFailReason SaveVectorValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	PancystarEngine::EngineFailReason SaveVectorEnumMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value);
	//����ַ��������Ƿ����Ѿ�ע����Ľṹ���ö��
	bool CheckIfStruct(const std::string &variable_type_name);
	bool CheckIfStructList(const std::string &variable_type_name);
	PancystarEngine::EngineFailReason TranslateStringToEnum(const std::string &basic_string, std::string &enum_type, std::string &enum_value_string, int32_t &enum_value_data);
	//����������������ת��Ϊ��������
	std::string TranslateFullNameToRealName(const std::string &full_name);
protected:
	template<typename T>
	PancystarEngine::EngineFailReason AddAnyVariable(
		const std::string &name,
		T &variable_data
	);
	PancystarEngine::EngineFailReason AddVariable(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name);
	PancystarEngine::EngineFailReason AddArray(const std::string &name, void*variable_data, const size_t &variable_type, const std::string &variable_type_name, const pancy_object_id &array_size);
	PancystarEngine::EngineFailReason AddReflectData(const PancyJsonMemberType &type_data, const std::string &name, const std::string &variable_type_name, void*variable_data, const pancy_object_id &array_size = 1);
	PancystarEngine::EngineFailReason AddChildStruct(PancyJsonReflect*data_pointer, const std::string &name, const pancy_resource_size &data_size);
	template<typename DataClassDesc, typename ReflectClassDesc>
	void AddChildReflectClass();
};
template<typename T>
PancystarEngine::EngineFailReason PancyJsonReflect::AddAnyVariable(
	const std::string &name,
	T &variable_data
)
{
	if constexpr (std::is_array<T>::value)
	{
		int32_t array_size = sizeof(variable_data) / sizeof(variable_data[0]);
		return AddArray(name, (void*)(variable_data), typeid(&variable_data[0]).hash_code(), typeid(&variable_data[0]).name(), array_size);
	}
	else
	{
		return AddVariable(name, (void*)(&variable_data), typeid(variable_data).hash_code(), typeid(variable_data).name());
	}
	return PancystarEngine::succeed;
}
template<typename DataClassDesc, typename ReflectClassDesc>
void PancyJsonReflect::AddChildReflectClass()
{
	PancyJsonReflect *pointer = new ReflectClassDesc();
	pointer->Create();
	std::string now_array_name = typeid(DataClassDesc*).name();
	std::string now_vector_name = typeid(std::vector<DataClassDesc>).name();
	auto now_size = sizeof(DataClassDesc);
	AddChildStruct(pointer, now_array_name, now_size);
	AddChildStruct(pointer, now_vector_name, now_size);
}
template<class T1, class T2>
PancystarEngine::EngineFailReason PancyJsonReflect::SetVectorValue(JsonReflectData &reflect_data, const pancy_object_id &offset_value, const T2 &input_value)
{
	std::vector<T1> *now_data_pointer = reinterpret_cast<std::vector<T1>*>(reflect_data.data_pointer);
	if (now_data_pointer->size() != offset_value)
	{
		//ƫ��������ȷ�������i��Ԫ��Ҫ��֤�Ѿ�����i����Ա
		PancystarEngine::EngineFailReason error_message(E_FAIL, "reflect vector have wrong offset: " + reflect_data.data_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SetIntArrayValue", error_message);
		return error_message;
	}
	now_data_pointer->push_back(static_cast<T1>(input_value));
	return PancystarEngine::succeed;
}
template<typename ArrayType, typename JsonType>
PancystarEngine::EngineFailReason PancyJsonReflect::SaveArrayValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	if (typeid(ArrayType*).name() != reflect_data.data_type_name)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "array type dismatch: " + reflect_data.data_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveArrayValueMemberToJson", error_message);
		return error_message;
	}
	ArrayType *pointer = reinterpret_cast<ArrayType*>(reflect_data.data_pointer);
	for (int32_t data_offset_index = 0; data_offset_index < reflect_data.real_used_size; ++data_offset_index)
	{
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<JsonType>(pointer[data_offset_index]));
	}
	return PancystarEngine::succeed;
}
template<typename ArrayType, typename JsonType>
PancystarEngine::EngineFailReason PancyJsonReflect::SaveVectorValueMemberToJson(const JsonReflectData &reflect_data, Json::Value &root_value)
{
	if (typeid(std::vector<ArrayType>).name() != reflect_data.data_type_name)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "array type dismatch: " + reflect_data.data_name);
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflect::SaveArrayValueMemberToJson", error_message);
		return error_message;
	}
	std::vector<ArrayType> *pointer = reinterpret_cast<std::vector<ArrayType>*>(reflect_data.data_pointer);
	for (int32_t data_offset_index = 0; data_offset_index < reflect_data.real_used_size; ++data_offset_index)
	{
		PancyJsonTool::GetInstance()->AddJsonArrayValue(root_value, TranslateFullNameToRealName(reflect_data.data_name), static_cast<JsonType>((*pointer)[data_offset_index]));
	}
	return PancystarEngine::succeed;
}
//ģ�巴����
template<typename ReflectDataType>
class PancyJsonReflectTemplate :public PancyJsonReflect
{
protected:
	ReflectDataType reflect_data;
public:
	PancyJsonReflectTemplate();
	PancystarEngine::EngineFailReason CopyMemberData(void * dst_pointer, const std::string &data_type_name, const pancy_resource_size & size) override;
	PancystarEngine::EngineFailReason CopyVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_resource_size &index, const pancy_resource_size &size) override;

	PancystarEngine::EngineFailReason ResetMemoryByArrayData(void *array_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) override;
	PancystarEngine::EngineFailReason ResetMemoryByVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size) override;
};
template<typename ReflectDataType>
PancyJsonReflectTemplate<ReflectDataType>::PancyJsonReflectTemplate()
{

}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::CopyMemberData(void *dst_pointer, const std::string &data_type_name, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(ReflectDataType*).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not copy memory for json reflect,type dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::CopyMemberData", error_message);
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not copy memory for json reflect,size dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::CopyMemberData", error_message);
		return error_message;
	}
	ReflectDataType *pointer = reinterpret_cast<ReflectDataType*>(dst_pointer);
	*pointer = reflect_data;
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::CopyVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_resource_size &index, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(std::vector<ReflectDataType>).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not push back vector for json reflect,type dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::CopyVectorData", error_message);
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not push back vector for json reflect,size dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::CopyVectorData", error_message);
		return error_message;
	}
	std::vector<ReflectDataType> *pointer = reinterpret_cast<std::vector<ReflectDataType>*>(vector_pointer);
	if (pointer->size() != index)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not add vector memory for json reflect,index number dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::CopyMemberData", error_message);
		return error_message;
	}
	pointer->push_back(reflect_data);
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::ResetMemoryByArrayData(void *array_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(ReflectDataType*).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not copy memory for json reflect,type dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::ResetMemoryByArrayData", error_message);
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not copy memory for json reflect,size dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::ResetMemoryByArrayData", error_message);
		return error_message;
	}
	ReflectDataType *real_data_pointer = reinterpret_cast<ReflectDataType*>(array_pointer);
	reflect_data = real_data_pointer[index];
	return PancystarEngine::succeed;
}
template<typename ReflectDataType>
PancystarEngine::EngineFailReason PancyJsonReflectTemplate<ReflectDataType>::ResetMemoryByVectorData(void *vector_pointer, const std::string &data_type_name, const pancy_object_id &index, const pancy_resource_size &size)
{
	std::string check_data_type_name = typeid(std::vector<ReflectDataType>).name();
	if (check_data_type_name != data_type_name)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not push back vector for json reflect,type dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::ResetMemoryByVectorData", error_message);
		return error_message;
	}
	if (size != sizeof(ReflectDataType))
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not push back vector for json reflect,size dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::ResetMemoryByVectorData", error_message);
		return error_message;
	}
	std::vector<ReflectDataType> *pointer = reinterpret_cast<std::vector<ReflectDataType>*>(vector_pointer);
	if (pointer->size() <= index)
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "could not add vector memory for json reflect,index number dismatch");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("PancyJsonReflectTemplate::ResetMemoryByVectorData", error_message);
		return error_message;
	}
	reflect_data = (*pointer)[index];
	return PancystarEngine::succeed;
}
