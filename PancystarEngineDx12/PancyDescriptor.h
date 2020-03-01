#pragma once
#define AnimationSize
#include"PancyModelBasic.h"
#include"PancyResourceBasic.h"
#define threadBlockSize 128
namespace PancystarEngine
{
	//��ͨ�İ�������
	struct CommonDescriptorPointer
	{
		//�Ƿ�ʹ��˫����
		bool if_multi_buffer;
		//������ҳ����ʼ��ַ
		std::vector<pancy_object_id> descriptor_offset;
		//����������
		D3D12_DESCRIPTOR_HEAP_TYPE descriptor_type;
		//��������Ӧ����Դָ��
		std::vector<VirtualResourcePointer> resource_data;
		CommonDescriptorPointer()
		{
			//��ʼ����ͨ������ָ��
			if_multi_buffer = false;
			descriptor_type = D3D12_DESCRIPTOR_HEAP_TYPE::D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		}
	};
	//��󶨵�������ҳ
	struct BindlessResourceViewPointer
	{
		//������ҳ����ʼ��ַ
		pancy_object_id resource_view_offset;
		//������ҳ������������������
		pancy_object_id resource_view_num;
		//ÿһ���������ĸ�ʽ����
		std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> SRV_desc;
		//ÿһ����������ָ�����Դ
		std::vector<VirtualResourcePointer> describe_memory_data;
		BindlessResourceViewPointer()
		{
			resource_view_offset = 0;
			resource_view_num = 0;
		}
	};
	//��������
	class BindlessResourceViewSegmental
	{
		ID3D12DescriptorHeap *descriptor_heap_data;//�������ѵ���ʵ��ַ
		pancy_object_id per_descriptor_size;       //ÿ���������Ĵ�С
		pancy_object_id segmental_offset_position; //��ǰ����������ȫ�ֵ�ƫ��
		pancy_object_id max_descriptor_num;        //��ǰ��������������ɵ�����������
		pancy_object_id now_pointer_offset;        //��ǰ����������ʹ�����ݵ�ƫ��
		pancy_object_id now_pointer_refresh;       //��ǰ�������������Ҫһ�������������������ʼ����λ��
		pancy_object_id now_descriptor_pack_id_self_add;//��ǰ��������Ϊÿ��������ҳ�����ID������������
		std::queue<pancy_object_id> now_descriptor_pack_id_reuse;//֮ǰ���ͷŵ������ڿ�������ʹ�õ�������ID

		std::unordered_map<pancy_resource_id, BindlessResourceViewPointer> descriptor_data;//ÿ�������������������ҳ
	public:
		BindlessResourceViewSegmental(
			const pancy_object_id &max_descriptor_num,
			const pancy_object_id &segmental_offset_position_in,
			const pancy_object_id &per_descriptor_size_in,
			const ComPtr<ID3D12DescriptorHeap> descriptor_heap_data_in
		);
		inline pancy_object_id GetEmptyDescriptorNum()
		{
			return max_descriptor_num - now_pointer_offset;
		}
		//�����������￪��һ��bindless������ҳ
		PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPack(
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
			const std::vector<VirtualResourcePointer> &describe_memory_data,
			const pancy_object_id &SRV_pack_size,
			pancy_object_id &SRV_pack_id
		);
		//������������ɾ��һ��bindless������ҳ
		PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPack(const pancy_object_id &SRV_pack_id);
		//Ϊ�������ν���һ����������Ƭ���������
		PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewPack();
		//������������ɾ��һ��bindless������ҳ��ִ��һ���������
		PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPackAndRefresh(const pancy_object_id &SRV_pack_id);
		//��ȡһ��������ҳ�Ļ���ƫ��
		const BindlessResourceViewPointer GetDescriptorPageOffset(const pancy_object_id &descriptor_page_id);
	private:
		//����������ҳ��ָ����Ϣ�����������ѿ���������
		PancystarEngine::EngineFailReason BuildShaderResourceView(const BindlessResourceViewPointer &resource_view_pointer);
	};

	//���������ҳ��id��
	struct BindlessDescriptorID
	{
		//ȫ��ID
		pancy_object_id bindless_id;
		//������Դ������
		pancy_object_id empty_resource_size;
		//����С�������
		bool operator<(const BindlessDescriptorID& other)  const;
	};
	//����������ε�id��
	struct BindlessResourceViewID
	{
		pancy_object_id segmental_id;
		pancy_object_id page_id;
	};

	//���ڴ�����������
	class PancyDescriptorHeap
	{
		D3D12_DESCRIPTOR_HEAP_DESC descriptor_desc;                                        //�������ѵĸ�ʽ
		std::string descriptor_heap_name;                                                  //�������ѵ�����
		pancy_object_id per_descriptor_size;                                               //ÿ���������Ĵ�С
		ComPtr<ID3D12DescriptorHeap> descriptor_heap_data;                                 //�������ѵ���ʵ����

		//�������ֲ�ͬ��������(ȫ����������bindless�������Լ���ͨ�İ�������)
		std::unordered_map<std::string, pancy_object_id> descriptor_globel_map;//�������ѵ�ȫ�������������ݼ���

		pancy_object_id bind_descriptor_num;                                               //�����������֧�ֵİ�����������
		std::queue<pancy_object_id> bind_descriptor_offset_reuse;                          //���������Ļ������õ�ID��
		std::unordered_map<pancy_object_id, CommonDescriptorPointer> descriptor_bind_map;  //�������ѵİ������������ݼ���

		pancy_object_id bindless_descriptor_num;                                                 //�����������֧�ֵ�bindless����������
		pancy_object_id per_segmental_size;                                                      //ÿһ���������εĴ�С
		std::unordered_map<pancy_object_id, BindlessDescriptorID> bindless_descriptor_id_map;    //�������ѵ�����bindless�������ε�id����
		std::map<BindlessDescriptorID, BindlessResourceViewSegmental*> descriptor_segmental_map; //�������ѵ�����bindless�������ε����ݼ���

	public:
		PancyDescriptorHeap();
		~PancyDescriptorHeap();
		PancystarEngine::EngineFailReason Create(
			const D3D12_DESCRIPTOR_HEAP_DESC &descriptor_heap_desc,
			const std::string &descriptor_heap_name_in,
			const pancy_object_id &bind_descriptor_num_in,
			const pancy_object_id &bindless_descriptor_num_in,
			const pancy_object_id &per_segmental_size_in
		);
		inline PancystarEngine::EngineFailReason GetDescriptorHeapData(ID3D12DescriptorHeap **descriptor_heap_out)
		{
			*descriptor_heap_out = descriptor_heap_data.Get();
			return PancystarEngine::succeed;
		}
		//����ȫ��������
		PancystarEngine::EngineFailReason BuildGlobelDescriptor(
			const std::string &globel_name,
			const std::vector<BasicDescriptorDesc> &SRV_desc,
			const std::vector <VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer
		);
		PancystarEngine::EngineFailReason DeleteGlobelDescriptor(const std::string &globel_name);
		PancystarEngine::EngineFailReason GetGlobelDesciptorID(const std::string &globel_name, pancy_object_id &descriptor_id);
		PancystarEngine::EngineFailReason BindGlobelDescriptor(
			const std::string &globel_name,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		//����˽�еİ�������
		PancystarEngine::EngineFailReason BuildBindDescriptor(
			const std::vector<BasicDescriptorDesc> &descriptor_desc,
			const std::vector<VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer,
			pancy_object_id &descriptor_id
		);
		PancystarEngine::EngineFailReason DeleteBindDescriptor(const pancy_object_id &descriptor_id);
		PancystarEngine::EngineFailReason BindCommonDescriptor(
			const pancy_object_id &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		PancystarEngine::EngineFailReason GetCommonDescriptorCpuOffset(const pancy_object_id &descriptor_id, CD3DX12_CPU_DESCRIPTOR_HANDLE &Cpu_Handle);
		//����˽�е�bindless������ҳ
		PancystarEngine::EngineFailReason BuildBindlessShaderResourceViewPage(
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
			const std::vector<VirtualResourcePointer> &describe_memory_data,
			const pancy_object_id &SRV_pack_size,
			BindlessResourceViewID &descriptor_id
		);
		//ɾ��˽�е�bindless������ҳ(����ָ���Ƿ�ɾ����Ϻ��ҳ��Ƭ��������)
		PancystarEngine::EngineFailReason DeleteBindlessShaderResourceViewPage(
			const BindlessResourceViewID &descriptor_id,
			bool is_refresh_segmental = true
		);
		//�������е��������Σ����Ľϴ����л���ͼ��Դ��ʱ����ϲ�������Ƭ��ɾ��ʹ��
		PancystarEngine::EngineFailReason RefreshBindlessShaderResourceViewSegmental();
		//���������������rootsignature
		PancystarEngine::EngineFailReason BindBindlessDescriptor(
			const BindlessResourceViewID &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
	private:
		//����ˢ�½����Դ�������εĴ�С��������������ɾ��ĵ�ʱ�򱻵���
		PancystarEngine::EngineFailReason RefreshBindlessResourcesegmentalSize(const pancy_object_id &resourc_id);
		//Ԥ��������������
		PancystarEngine::EngineFailReason PreBuildBindDescriptor(
			const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_type,
			const bool if_build_multi_buffer,
			std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> &descriptor_cpu_handle,
			CommonDescriptorPointer &new_descriptor_data
		);
		//��ȡ�������Ѹ�ʽ
		D3D12_DESCRIPTOR_HEAP_TYPE GetDescriptorHeapTypeOfDescriptor(const BasicDescriptorDesc &descriptor_desc);
	};

	//��������������ָ��
	struct BindDescriptorPointer
	{
		//��������ID
		pancy_resource_id descriptor_heap_id;
		//������ID
		pancy_object_id descriptor_id;
	};
	//���������������ָ��
	struct BindlessDescriptorPointer
	{
		//��������ID
		pancy_resource_id descriptor_heap_id;
		//������ID
		BindlessResourceViewID descriptor_pack_id;
	};
	//���ڹ������е���������
	class PancyDescriptorHeapControl
	{
		pancy_resource_id descriptor_heap_id_self_add;
		std::queue<pancy_resource_id> descriptor_heap_id_reuse;
		pancy_resource_id common_descriptor_heap_shader_resource;
		pancy_resource_id common_descriptor_heap_render_target;
		pancy_resource_id common_descriptor_heap_depth_stencil;
		std::unordered_map<pancy_resource_id, PancyDescriptorHeap*> descriptor_heap_map;
		PancyDescriptorHeapControl();
	public:
		~PancyDescriptorHeapControl();
		static PancyDescriptorHeapControl* GetInstance()
		{
			static PancyDescriptorHeapControl* this_instance;
			if (this_instance == NULL)
			{
				this_instance = new PancyDescriptorHeapControl();
			}
			return this_instance;
		}
		//��ȡ��������������
		PancystarEngine::EngineFailReason GetBasicDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE &descriptor_desc, ID3D12DescriptorHeap **descriptor_heap_out);
		//ȫ��������
		PancystarEngine::EngineFailReason BuildCommonGlobelDescriptor(
			const std::string &globel_srv_name,
			const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
			const std::vector<VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer
		);
		PancystarEngine::EngineFailReason GetCommonGlobelDescriptorID(
			PancyDescriptorType basic_descriptor_type,
			const std::string &globel_srv_name,
			BindDescriptorPointer &descriptor_id
		);
		PancystarEngine::EngineFailReason BindCommonGlobelDescriptor(
			PancyDescriptorType basic_descriptor_type,
			const std::string &globel_name,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		PancystarEngine::EngineFailReason BindCommonRenderTargetUncontiguous(
			const std::vector<pancy_object_id> rendertarget_list,
			const pancy_object_id depthstencil_descriptor,
			PancyRenderCommandList *m_commandList,
			const bool &if_have_rendertarget = true,
			const bool &if_have_depthstencil = true
		);
		//todo:Ŀǰ����RTV�ڽ�������ȡ�����޷�������ʱ����ȡ��depthstencil�ķ������ڲ��ԣ�����Դ������������Ҫɾ��
		PancystarEngine::EngineFailReason GetCommonDepthStencilBufferOffset(
			const pancy_object_id depthstencil_descriptor,
			CD3DX12_CPU_DESCRIPTOR_HANDLE &dsvHandle
		);
		//��������
		PancystarEngine::EngineFailReason BuildCommonDescriptor(
			const std::vector<BasicDescriptorDesc> &now_descriptor_desc_in,
			const std::vector<VirtualResourcePointer>& memory_data,
			const bool if_build_multi_buffer,
			BindDescriptorPointer &descriptor_id
		);
		PancystarEngine::EngineFailReason BindCommonDescriptor(
			const BindDescriptorPointer &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		//���������
		PancystarEngine::EngineFailReason BuildCommonBindlessShaderResourceView(
			const std::vector<D3D12_SHADER_RESOURCE_VIEW_DESC> &SRV_desc,
			const std::vector<VirtualResourcePointer> &describe_memory_data,
			const pancy_object_id &SRV_pack_size,
			BindlessDescriptorPointer &descriptor_id
		);
		PancystarEngine::EngineFailReason BindBindlessDescriptor(
			const BindlessDescriptorPointer &descriptor_id,
			const D3D12_COMMAND_LIST_TYPE &render_param_type,
			PancyRenderCommandList *m_commandList,
			const pancy_object_id &root_signature_offset
		);
		PancystarEngine::EngineFailReason ClearRenderTarget();
		//�����ɾ��һ����������
		PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_name, const Json::Value &root_value, pancy_resource_id &descriptor_heap_id);
		PancystarEngine::EngineFailReason BuildNewDescriptorHeapFromJson(const std::string &json_file_name, pancy_resource_id &descriptor_heap_id);
		PancystarEngine::EngineFailReason DeleteDescriptorHeap(const pancy_resource_id &descriptor_heap_id);
	private:
		pancy_resource_id GetCommonDescriptorHeapID(const BasicDescriptorDesc &descriptor_desc);
		pancy_resource_id GetCommonDescriptorHeapID(const PancyDescriptorType &descriptor_type);
	};

	

}
