#define threadBlockSize 128
#define MaxBoneNum 100
struct VSInputBone
{
	float3 position     : POSITION;
	float3 normal       : NORMAL;
	float3 tangent      : TANGENT;
	uint4  tex_id       : TEXID;
	float4 tex_uv       : TEXUV;
	uint4  bone_id      : BONEID;
	float4 bone_weight0 : BONEWEIGHTFIR;
	float4 bone_weight1 : BONEWEIGHTSEC;
};
struct mesh_anim
{
	float3 position;
	float3 normal;
	float3 tangent;
};
cbuffer per_object : register(b0)
{
	uint4 data_offset;//{�������ݵĳ�ʼƫ��,�������ݵĳ�ʼƫ��}
	uint4 data_num;//{ģ�͵Ķ�������,ģ�͵Ĺ�������}
}
StructuredBuffer<VSInputBone> vertex_data				: register(t0);	//���ڶ�����Ƥ�Ķ�������
StructuredBuffer<float4x4>    bone_matrix_buffer        : register(t1);	//CPU�˸��µĹ�������
RWStructuredBuffer<mesh_anim> mesh_anim_data	        : register(u0);	//������ϵĶ��㶯������

[numthreads(threadBlockSize, 1, 1)]
void CSMain(uint3 groupId : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
	//����ID�Ų��ҵ���ǰ���账��Ķ���λ��(����ƫ�� + �߳���ƫ�� + �߳�IDƫ��)
	uint now_vertex_index = groupId.x * threadBlockSize + groupIndex;
	//��⵱ǰ��ID���Ƿ����ں���ķ�Χ(ID�ų���ģ�Ͷ����������߳��ÿ�)
	if (now_vertex_index < data_num.x)
	{
		//��ȡԭʼ�Ĺ����任����
		VSInputBone now_vertex = vertex_data[now_vertex_index];
		//��ȡ8����Ӧ�����任�����λ��(����ƫ��+instancingƫ��+����Id��)
		uint bone_id_mask = MaxBoneNum + 100;
		uint bone_start_index = data_offset.x + groupId.y * data_num.y;
		uint bone_id_use0 = bone_start_index + now_vertex.bone_id[0] / bone_id_mask;
		uint bone_id_use1 = bone_start_index + now_vertex.bone_id[1] / bone_id_mask;
		uint bone_id_use2 = bone_start_index + now_vertex.bone_id[2] / bone_id_mask;
		uint bone_id_use3 = bone_start_index + now_vertex.bone_id[3] / bone_id_mask;
		uint bone_id_use4 = bone_start_index + now_vertex.bone_id[0] % bone_id_mask;
		uint bone_id_use5 = bone_start_index + now_vertex.bone_id[1] % bone_id_mask;
		uint bone_id_use6 = bone_start_index + now_vertex.bone_id[2] % bone_id_mask;
		uint bone_id_use7 = bone_start_index + now_vertex.bone_id[3] % bone_id_mask;
		float3 positon_bone = float3(0.0f, 0.0f, 0.0f);
		float3 normal_bone = float3(0.0f, 0.0f, 0.0f);
		float3 tangent_bone = float3(0.0f, 0.0f, 0.0f);
		positon_bone += now_vertex.bone_weight0.x * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use0]).xyz;
		normal_bone += now_vertex.bone_weight0.x * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use0]);
		tangent_bone += now_vertex.bone_weight0.x * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use0]);

		positon_bone += now_vertex.bone_weight0.y * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use1]).xyz;
		normal_bone += now_vertex.bone_weight0.y * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use1]);
		tangent_bone += now_vertex.bone_weight0.y * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use1]);

		positon_bone += now_vertex.bone_weight0.z * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use2]).xyz;
		normal_bone += now_vertex.bone_weight0.z * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use2]);
		tangent_bone += now_vertex.bone_weight0.z * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use2]);

		positon_bone += now_vertex.bone_weight0.w * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use3]).xyz;
		normal_bone += now_vertex.bone_weight0.w * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use3]);
		tangent_bone += now_vertex.bone_weight0.w * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use3]);

		positon_bone += now_vertex.bone_weight1.x * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use4]).xyz;
		normal_bone += now_vertex.bone_weight1.x * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use4]);
		tangent_bone += now_vertex.bone_weight1.x * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use4]);

		positon_bone += now_vertex.bone_weight1.y * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use5]).xyz;
		normal_bone += now_vertex.bone_weight1.y * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use5]);
		tangent_bone += now_vertex.bone_weight1.y * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use5]);

		positon_bone += now_vertex.bone_weight1.z * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use6]).xyz;
		normal_bone += now_vertex.bone_weight1.z * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use6]);
		tangent_bone += now_vertex.bone_weight1.z * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use6]);

		positon_bone += now_vertex.bone_weight1.w * mul(float4(now_vertex.position, 1.0f), bone_matrix_buffer[bone_id_use7]).xyz;
		normal_bone += now_vertex.bone_weight1.w * mul(now_vertex.normal, (float3x3)bone_matrix_buffer[bone_id_use7]);
		tangent_bone += now_vertex.bone_weight1.w * mul(now_vertex.tangent.xyz, (float3x3)bone_matrix_buffer[bone_id_use7]);

		//����instance�Լ���ǰ�Ķ���λ�û�ȡ�洢���㶯�����ݵ�λ��
		uint vertex_result_index = data_offset.y + groupId.y * data_num.x + now_vertex_index;
		mesh_anim_data[vertex_result_index].position = positon_bone;
		mesh_anim_data[vertex_result_index].normal = positon_bone;
		mesh_anim_data[vertex_result_index].tangent = positon_bone;
	}
}