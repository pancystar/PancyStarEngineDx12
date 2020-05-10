#pragma once
#include"..\\PancystarEngineDx12\\PancySceneDesign.h"

#ifdef _DEBUG
#pragma comment(lib,"..\\x64\\Debug\\PancystarEngineDx12.lib")
#else
#pragma comment(lib,"..\\x64\\Release\\PancystarEngineDx12.lib")
#endif

class scene_test_simple : public SceneRoot
{
	
	bool if_have_previous_frame;
	//����״̬
	ComPtr<ID3D12PipelineState> m_pipelineState;
	std::vector<PancyThreadIdGPU> renderlist_ID;
	//��Ļ�ռ�ģ��
	PancystarEngine::GeometryBasic *test_model;
	//ģ�Ͳ���
	PancystarEngine::PancyBasicModel *test_model_common;
	PancystarEngine::PancyBasicModel *test_model_pointmesh;
	PancystarEngine::PancyBasicModel *test_model_skinmesh;
	//�ӿ�
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT view_rect;
	//֡�ȴ�fence����
	PancyFenceIdGPU last_broken_fence_id;
	PancyFenceIdGPU broken_fence_id;
	//ģ��ID��
	PancystarEngine::PancyBasicModel model_common, model_skinmesh, model_pointmesh;
	PancystarEngine::BindlessDescriptorPointer model_skinmesh_descriptor_id;
	PancystarEngine::PancyRenderParamID render_param_id_skin_mesh_draw;
	PancystarEngine::PancyRenderParamID render_param_id_skin_mesh_compute;
	//pbr����
	PancystarEngine::VirtualResourcePointer tex_brdf_id;
	PancystarEngine::BindDescriptorPointer brdf_rtv_id;
	PancystarEngine::VirtualResourcePointer tex_ibl_spec_id;
	PancystarEngine::VirtualResourcePointer tex_ibl_diffuse_id;
	//�հ�����
	PancystarEngine::VirtualResourcePointer tex_empty_id;
	//���Բ���
	PancystarEngine::VirtualResourcePointer test_material;
	//psoID
	//pancy_object_id PSO_test;
	pancy_object_id PSO_pbr;
	//��������������
	pancy_object_id skinmesh_descriptor;
	pancy_object_id skinmesh_compute_descriptor;
public:
	scene_test_simple()
	{
		using namespace PancystarEngine;
		if_have_previous_frame = false;
		renderlist_ID.clear();
		JSON_REFLECT_INIT_ENUM(PbrType_MetallicRoughness);
		JSON_REFLECT_INIT_ENUM(PbrType_SpecularSmoothness);
	}
	~scene_test_simple();
	
	void Display();
	void DisplayNopost() {};
	void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix);
	void Update(float delta_time);
private:
	PancystarEngine::EngineFailReason BuildGlobelTextureSRV(const std::string &shader_resource, PancystarEngine::VirtualResourcePointer &tex_res_id);
	PancystarEngine::EngineFailReason ShowFloor();
	PancystarEngine::EngineFailReason ShowModel();
	PancystarEngine::EngineFailReason ShowSkinModel();
	PancystarEngine::EngineFailReason Init();
	PancystarEngine::EngineFailReason BuildSkinmeshDescriptor();
	PancystarEngine::EngineFailReason BuildSkinmeshComputeDescriptor();
	PancystarEngine::EngineFailReason ScreenChange();
	void PopulateCommandListSky();
	void PopulateCommandListModelDeal();
	PancystarEngine::EngineFailReason PretreatBrdf();
	PancystarEngine::EngineFailReason PretreatPbrDescriptor();
	void ClearScreen();
	void WaitForPreviousFrame();
	void updateinput(float delta_time);
	inline int ComputeIntersectionArea(int ax1, int ay1, int ax2, int ay2, int bx1, int by1, int bx2, int by2)
	{
		return max(0, min(ax2, bx2) - max(ax1, bx1)) * max(0, min(ay2, by2) - max(ay1, by1));
	}
};