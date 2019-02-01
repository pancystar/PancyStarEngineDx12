#pragma once
#include"PancyResourceBasic.h"
#include"PancyThreadBasic.h"

namespace PancystarEngine 
{
	enum PancyBufferType 
	{
		Buffer_ShaderResource = 0,
		Buffer_CommondBUffer = 1,
		Buffer_VertexIndex = 2
	};
	class PancyBasicBuffer : public PancyBasicVirtualResource
	{
		PancyBufferType  buffer_type;
		SubMemoryPointer buffer_data;     //buffer����ָ��
		SubMemoryPointer update_tex_data; //buffer�ϴ�����ָ��
	private:
		PancystarEngine::EngineFailReason InitResource(const Json::Value &root_value, const std::string &resource_name);
	};
}