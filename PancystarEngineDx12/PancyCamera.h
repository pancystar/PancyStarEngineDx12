#pragma once
#include"PancystarEngineBasicDx12.h"
#define CameraForFPS 0
#define CameraForFly 1
class PancyCamera
{
	DirectX::XMFLOAT3 camera_right;    //����������ҷ�������
	DirectX::XMFLOAT3 camera_look;     //������Ĺ۲췽������
	DirectX::XMFLOAT3 camera_up;       //����������Ϸ�������
	DirectX::XMFLOAT3 camera_position; //�����������λ������
private:
	PancyCamera();
public:
	static PancyCamera* GetInstance()
	{
		static PancyCamera* this_instance;
		if (this_instance == NULL)
		{
			this_instance = new PancyCamera();
		}
		return this_instance;
	}
	void RotationRight(float angle);                    //��������������ת
	void RotationUp(float angle);                       //��������������ת
	void RotationLook(float angle);                     //���Ź۲�������ת

	void RotationX(float angle);                    //����x��������ת
	void RotationY(float angle);                    //����y��������ת
	void RotationZ(float angle);                    //����z��������ת

	void WalkFront(float distance);                     //�������ǰƽ��
	void WalkRight(float distance);                     //���������ƽ��
	void WalkUp(float distance);                        //���������ƽ��
	void CountViewMatrix(DirectX::XMFLOAT4X4* view_matrix);     //����ȡ������
	void CountViewMatrix(DirectX::XMFLOAT3 rec_look, DirectX::XMFLOAT3 rec_up, DirectX::XMFLOAT3 rec_pos, DirectX::XMFLOAT4X4 *matrix);
	void CountInvviewMatrix(DirectX::XMFLOAT4X4* inv_view_matrix);  //����ȡ�����������
	void CountInvviewMatrix(DirectX::XMFLOAT3 rec_look, DirectX::XMFLOAT3 rec_up, DirectX::XMFLOAT3 rec_pos, DirectX::XMFLOAT4X4* inv_view_matrix);  //����ȡ�����������
	
	void GetViewPosition(DirectX::XMFLOAT4 *view_pos);
	void GetViewDirect(DirectX::XMFLOAT3 *view_direct);
	void GetRightDirect(DirectX::XMFLOAT3 *right_direct);
	void SetCamera(DirectX::XMFLOAT3 rec_look, DirectX::XMFLOAT3 rec_up, DirectX::XMFLOAT3 rec_pos);
	void ResetCamera();
private:
};