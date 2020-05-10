#pragma once
#include"PancyRenderParam.h"
namespace PancystarEngine 
{
	enum MaterialShaderResourceType 
	{
		MaterialShaderResourceBufferBind = 0,
		MaterialShaderResourceTextureBind,
		MaterialShaderResourceBufferBindLess,
		MaterialShaderResourceTextureBindLess
	};
	//buffer��Դ����������ʽ
	struct PancyBufferDescriptorDesc 
	{
		//��������
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		UINT Shader4ComponentMapping = 0;
		//buffer����
		UINT64 FirstElement = 0;
		UINT NumElements = 0;
		UINT StructureByteStride = 0;
		D3D12_BUFFER_SRV_FLAGS Flags = D3D12_BUFFER_SRV_FLAG_NONE;
	};
	class PancyJsonMaterialBufferDescriptor :public PancyJsonReflectTemplate<PancyBufferDescriptorDesc>
	{
	public:
		PancyJsonMaterialBufferDescriptor();
	private:
		void InitBasicVariable() override;
	};
	//buffer��Դ��shader�󶨸�ʽ
	struct PancyMaterialShaderResourceDataBuffer
	{
		MaterialShaderResourceType shader_resource_type = MaterialShaderResourceBufferBind;
		std::string shader_resource_slot_name;
		std::vector<std::string> shader_resource_path;
		std::vector<PancyBufferDescriptorDesc> buffer_descriptor_desc;//buffer��ÿ��buffer�ĸ�ʽ(��bindless��ֻ��һ��)
	};
	class PancyJsonMaterialBufferShaderResource:public PancyJsonReflectTemplate<PancyMaterialShaderResourceDataBuffer>
	{
	public:
		PancyJsonMaterialBufferShaderResource();
	private:
		void InitBasicVariable() override;
	};
	//texture��Դ����������ʽ
	struct PancyTextureDescriptorDesc
	{
		//��������
		DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
		D3D12_SRV_DIMENSION ViewDimension = D3D12_SRV_DIMENSION_UNKNOWN;
		UINT Shader4ComponentMapping = 0;
		//��������
		UINT MostDetailedMip = 0;
		UINT MipLevels = 0;
		UINT PlaneSlice = 0;
		UINT First2DArrayFace = 0;
		UINT NumCubes = 0;
		UINT FirstArraySlice = 0;
		UINT ArraySize = 0;
		UINT UnusedField_NothingToDefine = 0;
		FLOAT ResourceMinLODClamp = 0;
	};
	class PancyJsonMaterialTexureDescriptor :public PancyJsonReflectTemplate<PancyTextureDescriptorDesc>
	{
	public:
		PancyJsonMaterialTexureDescriptor();
	private:
		void InitBasicVariable() override;
	};
	//texture��Դ��shader�󶨸�ʽ
	struct PancyMaterialShaderResourceDataTexture
	{
		MaterialShaderResourceType shader_resource_type = MaterialShaderResourceBufferBind;
		std::string shader_resource_slot_name;
		std::vector<std::string> shader_resource_path;
		std::vector<PancyTextureDescriptorDesc> texture_descriptor_desc;//�����ÿ��buffer�ĸ�ʽ(��bindless��ֻ��һ��)
	};
	class PancyJsonMaterialTexureShaderResource :public PancyJsonReflectTemplate<PancyMaterialShaderResourceDataTexture>
	{
	public:
		PancyJsonMaterialTexureShaderResource();
	private:
		void InitBasicVariable() override;
	};
	//��ͨ���ʵ������ʽ
	struct PancyCommonMaterialDesc
	{
		std::string pipeline_state_name;
		//�����õ��Ļ�����
		std::vector<PancyMaterialShaderResourceDataBuffer> marterial_buffer_slot_data;
		//�����õ�������
		std::vector<PancyMaterialShaderResourceDataTexture> marterial_texture_slot_data;
	};
	class CommonMaterialDescJsonReflect :public PancyJsonReflectTemplate<PancyCommonMaterialDesc>
	{
	public:
		CommonMaterialDescJsonReflect();
	private:
		void InitBasicVariable() override;
	};
	class PancyMaterialBasic : public PancyCommonVirtualResource<PancyCommonMaterialDesc>
	{
		std::vector<VirtualResourcePointer> ShaderResourceData;
		//��Ⱦ���������������
		std::unordered_map<std::string, BindDescriptorPointer> bind_shader_resource;         //˽��������(����ר�õİ���Դ����Ҫ����򴴽�)
		std::unordered_map<std::string, BindlessDescriptorPointer> bindless_shader_resource; //���������(����ר�õĽ����Դ����Ҫ����򴴽�)
	public:
		PancyMaterialBasic(const bool &if_could_reload_in);
		//������Դ��ʽ������Դ����
		PancystarEngine::EngineFailReason LoadResoureDataByDesc(const PancyCommonMaterialDesc &ResourceDescStruct) override;
		//����һ����Ⱦparam
		PancystarEngine::EngineFailReason BuildRenderParam(PancyRenderParamID &render_param_id);
		bool CheckIfResourceLoadFinish() override;;
	private:
		PancystarEngine::EngineFailReason BuildBufferDescriptorByDesc(
			const std::string &resource_file_name,
			const PancyBufferDescriptorDesc &descriptor_desc,
			std::vector<VirtualResourcePointer> &resource_pointer_array,
			std::vector<BasicDescriptorDesc> &resource_descriptor_descr_array
			);
		PancystarEngine::EngineFailReason BuildTextureDescriptorByDesc(
			const std::string &resource_file_name,
			const PancyTextureDescriptorDesc &descriptor_desc,
			std::vector<VirtualResourcePointer> &resource_pointer_array,
			std::vector<BasicDescriptorDesc> &resource_descriptor_descr_array
		);
	};
	PancystarEngine::EngineFailReason LoadMaterialFromFile(
		const std::string &name_resource_in,
		VirtualResourcePointer &id_need
	);
	void InitMaterialJsonReflect();
}
