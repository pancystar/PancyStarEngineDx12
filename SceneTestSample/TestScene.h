#pragma once
#include"..\\PancystarEngineDx12\\PancySceneDesign.h"
#include <assimp/Importer.hpp>      // �������ڸ�ͷ�ļ��ж���
#include <assimp/scene.h>           // ��ȡ����ģ�����ݶ�����scene��
#include <assimp/postprocess.h>     // ��ͷ�ļ��а�������ı�־λ����
#include <assimp/matrix4x4.h>
#include <assimp/matrix3x3.h>
#ifdef _DEBUG
#pragma comment(lib,"..\\x64\\Debug\\PancystarEngineDx12.lib")
#else
#pragma comment(lib,"..\\x64\\Release\\PancystarEngineDx12.lib")
#endif
enum TexType
{
	tex_diffuse = 0,
	tex_normal,
	tex_metallic_roughness,
	tex_specular,
	tex_ambient
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
		model_mesh = new PancystarEngine::GeometryCommonModel<T>(vertex_need, index_need, vert_num, index_num);
		auto check_error = model_mesh->Create();
		if (!check_error.CheckIfSucceed())
		{
			return check_error;
		}
		material_use = material_id;
		return PancystarEngine::succeed;
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
class PancyModelBasic : public PancystarEngine::PancyBasicVirtualResource
{
	std::vector<PancySubModel*> model_resource_list;     //ģ�͵�ÿ���Ӳ���
	std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> material_list;
	std::vector<pancy_object_id> texture_list;
protected:
	std::string model_root_path;
public:
	PancyModelBasic(const std::string &desc_file_in);
	void GetRenderMesh(std::vector<PancySubModel*> &render_mesh);
	inline pancy_object_id GetSubModelNum()
	{
		return model_resource_list.size();
	};
	inline pancy_object_id GetSubModelTexture(pancy_object_id submodel_id, TexType texture_type) 
	{
		if (submodel_id > model_resource_list.size()) 
		{
			PancystarEngine::EngineFailReason error_message(E_FAIL,"submodel id:"+std::to_string(submodel_id)+" bigger than the submodel num:"+std::to_string(model_resource_list.size())+" of model: " +resource_name);
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Find submodel from model ", error_message);
			return 0;
		}
		pancy_object_id mat_use = model_resource_list[submodel_id]->GetMaterial();
		auto material_data = material_list.find(mat_use);
		auto texture_data = material_data->second.find(texture_type)->second;
		return texture_data;
	}
	virtual ~PancyModelBasic();
private:
	
	PancystarEngine::EngineFailReason InitResource(const std::string &resource_desc_file);
	virtual PancystarEngine::EngineFailReason LoadModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	) = 0;
	void GetRootPath(const std::string &desc_file_in);
};
class PancyModelJson : public PancyModelBasic
{
public:
	PancyModelJson(const std::string &desc_file_in);

};
class PancyModelAssimp : public PancyModelBasic
{
	//��ʱ��Ⱦ����(ģ�ʹ������в���view��������ʽʹ�û���view������)
	std::string pso_use;                  //pso
	std::vector<SubMemoryPointer> cbuffer;//����������
	std::vector<ResourceViewPointer> table_offset;//ÿ��shader�ⲿ������λ��
	//ģ�ͼ��ر���
	Assimp::Importer importer;
	const aiScene *model_need;//assimpģ�ͱ���
public:
	PancyModelAssimp(const std::string &desc_file_in, const std::string &pso_in);
	inline PancyPiplineStateObjectGraph* GetPso() 
	{
		return PancyEffectGraphic::GetInstance()->GetPSO(pso_use);
	}
	inline std::vector<ResourceViewPointer> GetDescriptorHeap() 
	{
		return table_offset;
	}
	void update(DirectX::XMFLOAT4X4 world_matrix, DirectX::XMFLOAT4X4 uv_matrix,float delta_time);
private:
	
	PancystarEngine::EngineFailReason LoadModel(
		const std::string &resource_desc_file,
		std::vector<PancySubModel*> &model_resource,
		std::unordered_map<pancy_object_id, std::unordered_map<TexType, pancy_object_id>> &material_list,
		std::vector<pancy_object_id> &texture_use
	);
};
class scene_test_simple : public SceneRoot
{
	//����״̬
	ComPtr<ID3D12PipelineState> m_pipelineState;
	std::vector<PancyThreadIdGPU> renderlist_ID;
	//ģ�Ͳ���
	PancystarEngine::GeometryBasic *test_model;
	//�ӿ�
	CD3DX12_VIEWPORT view_port;
	CD3DX12_RECT view_rect;
	//֡�ȴ�fence����
	PancyFenceIdGPU broken_fence_id;
	//��Դ��(��պ�)
	ResourceViewPointer table_offset[3];
	SubMemoryPointer cbuffer[2];
	//��Դ��(������ģ��)
	ResourceViewPointer table_offset_model[4];
	/*
	cbuffer_per_object
	cbuffer_per_view
	diffuse
	normal
	metallic/specular
	roughness/smoothness
	*/
	SubMemoryPointer cbuffer_model[2];
	//ģ����Դ
	PancyModelBasic *model_sky;
	PancyModelBasic *model_cube;
	PancyModelBasic *model_deal;
	//pbr����
	pancy_object_id tex_brdf_id;
	pancy_object_id tex_metallic_id;
	pancy_object_id tex_roughness_id;
	pancy_object_id tex_ibl_spec_id;
	pancy_object_id tex_ibl_diffuse_id;
public:
	scene_test_simple()
	{
		renderlist_ID.clear();
	}
	~scene_test_simple();
	
	void Display();
	void DisplayNopost() {};
	void DisplayEnvironment(DirectX::XMFLOAT4X4 view_matrix, DirectX::XMFLOAT4X4 proj_matrix);
	void Update(float delta_time);
private:
	PancystarEngine::EngineFailReason Init();
	PancystarEngine::EngineFailReason ScreenChange();
	void PopulateCommandList(PancyModelBasic *now_res);
	void PopulateCommandListSky();
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