#include"PancyInput.h"
PancyInput *PancyInput::pancy_input_pInstance = NULL;
PancyInput::PancyInput()
{
}
PancystarEngine::EngineFailReason PancyInput::Init(HWND hwnd, HINSTANCE hinst)
{
	HRESULT hr = DirectInput8Create(hinst, DIRECTINPUT_HEADER_VERSION, IID_IDirectInput8, (void**)&pancy_dinput, NULL);//��ȡDirectInput�豸
	if (FAILED(hr)) 
	{
		PancystarEngine::EngineFailReason error_message(hr, "init directinput error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create directx input object", error_message);
		return error_message;
	}
	PancystarEngine::EngineFailReason check_error = DinputClear(hwnd, (DISCL_FOREGROUND | DISCL_NONEXCLUSIVE), (DISCL_FOREGROUND | DISCL_NONEXCLUSIVE));//�������̼����
	if (!check_error.CheckIfSucceed()) 
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
PancyInput::~PancyInput()
{
}
PancystarEngine::EngineFailReason PancyInput::DinputClear(HWND hwnd,DWORD keyboardCoopFlags, DWORD mouseCoopFlags)
{
	//���������豸
	HRESULT hr = pancy_dinput->CreateDevice(GUID_SysKeyboard,&dinput_keyboard,NULL);
	if (FAILED(hr)) 
	{
		PancystarEngine::EngineFailReason error_message(hr, "init directinput device error");
		PancystarEngine::EngineFailLog::GetInstance()->AddLog("Create directx input object", error_message);
		return error_message;
	}
	dinput_keyboard->SetDataFormat(&c_dfDIKeyboard);//�����豸�����ݸ�ʽ
	dinput_keyboard->SetCooperativeLevel(hwnd,keyboardCoopFlags);//�����豸�Ķ�ռ�ȼ�
	dinput_keyboard->Acquire();//��ȡ�豸�Ŀ���Ȩ
	dinput_keyboard->Poll();//������ѯ
	//��������豸
	pancy_dinput->CreateDevice(GUID_SysMouse,&dinput_mouse,NULL);
	dinput_mouse->SetDataFormat(&c_dfDIMouse);//�����豸�����ݸ�ʽ
	dinput_mouse->SetCooperativeLevel(hwnd,mouseCoopFlags);//�����豸�Ķ�ռ�ȼ�
	dinput_mouse->Acquire();//��ȡ�豸�Ŀ���Ȩ
	dinput_mouse->Poll();//������ѯ
	return PancystarEngine::succeed;
}
void PancyInput::GetInput()
{
	//��ȡ�����Ϣ
	ZeroMemory(&mouse_buffer,sizeof(mouse_buffer));
	while(true)
	{
		dinput_mouse->Poll();
		dinput_mouse->Acquire();
		HRESULT hr;
		if(SUCCEEDED(dinput_mouse->GetDeviceState(sizeof(mouse_buffer),(LPVOID)&mouse_buffer)))
		{
			break;
		}
		else
		{
			hr = dinput_mouse->GetDeviceState(sizeof(mouse_buffer),(LPVOID)&mouse_buffer);
		}
		if (hr != DIERR_INPUTLOST || hr != DIERR_NOTACQUIRED)
		{
			break;
		};
		if (FAILED(dinput_mouse->Acquire()))
		{
			break;
		};
	}
	//��ȡ������Ϣ
	ZeroMemory(&key_buffer,sizeof(key_buffer));
	while(true)
	{
		dinput_keyboard->Poll();
		dinput_keyboard->Acquire();
		HRESULT hr;
		if(SUCCEEDED(dinput_keyboard->GetDeviceState(sizeof(key_buffer),(LPVOID)&key_buffer)))
		{
			break;
		}
		else
		{
			hr = dinput_keyboard->GetDeviceState(sizeof(key_buffer),(LPVOID)&key_buffer);
		}
		if (hr != DIERR_INPUTLOST || hr != DIERR_NOTACQUIRED)
		{
			break;
		};
		if (FAILED(dinput_keyboard->Acquire()))
		{
			break;
		};
	}
	
}
bool PancyInput::CheckKeyboard(int key_value)
{
	if(key_buffer[key_value] & 0x80)
	{
		return true;
	}
	return false;
}
bool PancyInput::CheckMouseDown(int mouse_value)
{
	if((mouse_buffer.rgbButtons[mouse_value]&0x80) != 0)
	{
		return true;
	}
	return false;
}
float PancyInput::MouseMove_X()
{
	return (float)mouse_buffer.lX;
}
float PancyInput::MouseMove_Y()
{
	return (float)mouse_buffer.lY;
}
float PancyInput::MouseMove_Z()
{
	return (float)mouse_buffer.lZ;
}