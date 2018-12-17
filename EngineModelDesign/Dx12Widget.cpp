#include "Dx12Widget.h"
D3d12RenderWidget::D3d12RenderWidget(QWidget *parent) : QWidget(parent)
{

	//���ô������ԣ��ؼ����裬����D3D���Ƴ�����
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_NativeWindow, true);
	//�������ʼ��D3D�ͳ���
}

D3d12RenderWidget::~D3d12RenderWidget()
{
	//�������ͷŽӿ�
	//CleanUp();
	delete new_scene;

	PancystarEngine::EngineFailLog::GetInstance()->PrintLogToconsole();
	delete PancystarEngine::EngineFailLog::GetInstance();
	delete ThreadPoolGPUControl::GetInstance();
	delete PancyShaderControl::GetInstance();
	delete PancyRootSignatureControl::GetInstance();
	delete PancyEffectGraphic::GetInstance();
	delete PancyJsonTool::GetInstance();
	delete MemoryHeapGpuControl::GetInstance();
	delete PancyDescriptorHeapControl::GetInstance();
	delete PancystarEngine::PancyTextureControl::GetInstance();
	delete SubresourceControl::GetInstance();
	delete PancystarEngine::FileBuildRepeatCheck::GetInstance();
	delete PancyInput::GetInstance();
	delete PancyCamera::GetInstance();
	delete PancyDx12DeviceBasic::GetInstance();
}

PancystarEngine::EngineFailReason D3d12RenderWidget::Create(SceneRoot *new_scene_in)
{
	//����directx�豸
	PancystarEngine::EngineFailReason check_error;
	check_error = PancyDx12DeviceBasic::SingleCreate((HWND)winId(), width(), height());
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build D3d12 Device");
		return error_message;
	}
	//ע�ᵥ��
	PancyShaderControl::GetInstance();
	PancyRootSignatureControl::GetInstance();
	PancyEffectGraphic::GetInstance();
	PancyJsonTool::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	PancyDescriptorHeapControl::GetInstance();
	MemoryHeapGpuControl::GetInstance();
	SubresourceControl::GetInstance();
	PancyInput::SingleCreate((HWND)winId(), GetModuleHandle(0));
	PancyCamera::GetInstance();
	//�����̳߳ع���
	check_error = ThreadPoolGPUControl::SingleCreate();
	if (!check_error.CheckIfSucceed())
	{
		PancystarEngine::EngineFailReason error_message(E_FAIL, "Could not Build d3d12 GPU pool Device");
		return error_message;
	}
	//��������
	new_scene = new_scene_in;
	//new_scene = new scene_test_simple();

	check_error = new_scene->Create(width(), height());
	if (!check_error.CheckIfSucceed())
	{
		return check_error;
	}
	return PancystarEngine::succeed;
}
void D3d12RenderWidget::paintEvent(QPaintEvent *event)
{
	//����fps
	//frameCount++;
	//if (getTime() > 1.0f)
	//{
	//	fps = frameCount;
	//	frameCount = 0;
	//	startFPStimer();
		//���ø����ڱ�����ʾfpsֵ
	//	parentWidget()->setWindowTitle("FPS: " + QString::number(fps));
	//}
	//frameTime = getFrameTime();
	//���³�������Ⱦ����
	//UpdateScene(frameTime);
	//RenderScene();
	//��֤�˺�����ÿһ֡������
	static int check = 0;
	if (check == 0) 
	{
		new_scene->Update(0);
		new_scene->Display();
	}
	
	update();
}