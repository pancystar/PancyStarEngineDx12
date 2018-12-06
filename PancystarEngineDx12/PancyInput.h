#pragma once
#include"PancystarEngineBasicDx12.h"
using namespace std;
class PancyInput
{
	ComPtr<IDirectInput8> pancy_dinput;           //DirectInput���豸�ӿ�
	ComPtr<IDirectInputDevice8> dinput_keyboard;  //�����豸�ӿ�
	ComPtr<IDirectInputDevice8> dinput_mouse;     //����豸�ӿ�
	char                       key_buffer[256];   //���̰�����Ϣ�Ļ���
	DIMOUSESTATE               mouse_buffer;      //��������Ϣ�Ļ���
private:
	PancyInput();         //���캯��
public:
	static PancyInput *pancy_input_pInstance;
	static PancystarEngine::EngineFailReason SingleCreate(HWND hwnd, HINSTANCE hinst)
	{
		if (pancy_input_pInstance != NULL)
		{
			
			PancystarEngine::EngineFailReason error_message(E_FAIL, "the d3d input instance have been created before");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create directx input object", error_message);
			return error_message;
		}
		else
		{
			pancy_input_pInstance = new PancyInput();
			PancystarEngine::EngineFailReason check_error = pancy_input_pInstance->Init(hwnd, hinst);
			return check_error;
		}
	}
	static PancyInput * GetInstance()
	{
		return pancy_input_pInstance;
	}
	~PancyInput();
	void GetInput();                                                       //��ȡ��������
	bool CheckKeyboard(int key_value);                                     //�������ϵ�ĳ�����������
	bool CheckMouseDown(int mouse_value);                                  //�������ϵ�ĳ�����������
	float MouseMove_X();                                                     //��ȡ�����x����ƶ���
	float MouseMove_Y();                                                     //��ȡ�����y����ƶ���
	float MouseMove_Z();                                                     //��ȡ�����z����ƶ���
	PancystarEngine::EngineFailReason Init(HWND hwnd, HINSTANCE hinst);
private:
	PancystarEngine::EngineFailReason DinputClear(HWND hwnd,DWORD keyboardCoopFlags, DWORD mouseCoopFlags);//��ʼ������
};