#pragma once
#include"PancystarEngineBasicDx12.h"
#include"PancyJsonTool.h"

class PancyDx12DeviceBasic
{
	UINT FrameCount;
	//������Ϣ
	HWND hwnd_window;
	uint32_t width;
	uint32_t height;
	//������֡ʹ����Ϣ
	pancy_object_id current_frame_use;//��ǰ֡��֡��
	pancy_object_id last_frame_use;//��һ֡��֡��
	//dxgi�豸(���ڸ��½�����)
	ComPtr<IDXGIFactory4> dxgi_factory;
	//d3d�豸
	ComPtr<ID3D12Device> m_device;
	//������
	ComPtr<IDXGISwapChain3> dx12_swapchain;
	//��Ⱦ����(direct����,copy���ͣ�compute����)
	ComPtr<ID3D12CommandQueue> command_queue_direct;
	ComPtr<ID3D12CommandQueue> command_queue_copy;
	ComPtr<ID3D12CommandQueue> command_queue_compute;
	//Ĭ�ϵ���ȾĿ��
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	std::vector<ComPtr<ID3D12Resource>> m_renderTargets;
private:
	PancyDx12DeviceBasic(HWND hwnd_window, uint32_t width_in, uint32_t height_in);
public:
	static PancyDx12DeviceBasic* d3dbasic_instance;
	static PancystarEngine::EngineFailReason SingleCreate(HWND hwnd_window_in, UINT wind_width_in, UINT wind_hight_in)
	{
		if (d3dbasic_instance != NULL)
		{
			return PancystarEngine::succeed;
		}
		else
		{
			d3dbasic_instance = new PancyDx12DeviceBasic(hwnd_window_in, wind_width_in, wind_hight_in);
			PancystarEngine::EngineFailReason check_failed = d3dbasic_instance->Init();
			return check_failed;
		}
	}
	static PancyDx12DeviceBasic* GetInstance()
	{
		return d3dbasic_instance;
	}
	PancystarEngine::EngineFailReason Init();
	PancystarEngine::EngineFailReason ResetScreen(uint32_t window_width_in, uint32_t window_height_in);
	//��ȡcom��Դ��Ϣ
	inline ID3D12Device* GetD3dDevice()
	{
		return m_device.Get();
	};
	inline ID3D12CommandQueue* GetCommandQueueDirect()
	{
		return command_queue_direct.Get();
	}
	inline ID3D12CommandQueue* GetCommandQueueCopy()
	{
		return command_queue_copy.Get();
	}
	inline ID3D12CommandQueue* GetCommandQueueCompute()
	{
		return command_queue_compute.Get();
	}
	inline PancystarEngine::EngineFailReason SwapChainPresent(  
		/* [in] */ UINT SyncInterval,
		/* [in] */ UINT Flags)
	{
		HRESULT hr;
		hr = dx12_swapchain->Present(SyncInterval, Flags);
		if (FAILED(hr)) 
		{
			PancystarEngine::EngineFailReason error_message(hr,"swapchain present error");
			PancystarEngine::EngineFailLog::GetInstance()->AddLog("present frame by Swapchain", error_message);
			return error_message;
		}
		last_frame_use = current_frame_use;
		current_frame_use = dx12_swapchain->GetCurrentBackBufferIndex();
		return PancystarEngine::succeed;
	}
	inline ID3D12Resource* GetBackBuffer(CD3DX12_CPU_DESCRIPTOR_HANDLE &heap_handle) 
	{
		auto now_frame_use = dx12_swapchain->GetCurrentBackBufferIndex();
		auto rtv_offset = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart(), now_frame_use, rtv_offset);
		heap_handle = rtvHandle;
		return m_renderTargets[now_frame_use].Get();
	}

	//��ȡ֡����
	inline UINT GetFrameNum()
	{
		return FrameCount;
	}
	inline UINT GetNowFrame()
	{
		return current_frame_use;
	}
	inline UINT GetLastFrame()
	{
		return last_frame_use;
	}
private:
	void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter);
};



