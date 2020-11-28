/*
*  Direct3DӦ�ó�����
*  d3dApp.h
*  ���ߣ� Frank Luna (C)
*/

#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// ���������d3d12��
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

class D3DApp
{
protected:

	D3DApp(HINSTANCE hInstance);
	D3DApp(const D3DApp& rhs) = delete;
	D3DApp& operator=(const D3DApp& rhs) = delete;
	virtual ~D3DApp();

public:

	static D3DApp* GetApp();

	HINSTANCE AppInst()const;
	HWND      MainWnd()const;
	float     AspectRatio()const;

	bool Get4xMsaaState()const;
	void Set4xMsaaState(bool value);

	int Run();

	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

protected:

	virtual void CreateRtvAndDsvDescriptorHeaps();
	virtual void OnResize();

	// ����ÿһ֡ʱ���ø÷���������ʱ�����Ƹ���3DӦ�ó���
	// ����ֶ������ƶ������������ײ����Լ�����û������
	virtual void Update(const GameTimer& gt) = 0;

	// ����ÿһ֡ʱ���ø÷������ڸ÷����з�����Ⱦ����
	// ����ǰ֡�����ػ��Ƶ���̨�������У����ƺ��ٵ���IDXGISwapChain::Present����
	// ����̨�����������ݳ�������Ļ��
	virtual void Draw(const GameTimer& gt) = 0;

	// ���ڴ������İ��¡��ͷź��ƶ��¼��ṩ����д������������дMsgProc����
	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:

	bool InitMainWindow();
	bool InitDirect3D();
	void CreateCommandObjects();
	void CreateSwapChain();

	void FlushCommandQueue();

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	void CalculateFrameStats();

	void LogAdapters();
	void LogAdapterOutputs(IDXGIAdapter* adapter);
	void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:

	static D3DApp* mApp;  // Ӧ�ó���

	HINSTANCE mhAppInst = nullptr;		 // Ӧ�ó���ʵ�����
	HWND      mhMainWnd = nullptr;		 // �����ھ��
	bool      mAppPaused = false;		 // Ӧ�ó����Ƿ���ͣ
	bool      mMinimized = false;		 // Ӧ�ó����Ƿ���С��
	bool      mMaximized = false;		 // Ӧ�ó����Ƿ����
	bool      mResizing = false;		 // ��С�������Ƿ��ܵ���ק
	bool      mFullscreenState = false;  // �Ƿ���ȫ��ģʽ

	// ��ѯ�Ƿ����ʹ��4X MSAA��Ĭ��Ϊ����ʹ��
	bool      m4xMsaaState = false;    
	UINT      m4xMsaaQuality = 0;  // 4X MSAA�����ȼ�

	// ���ڼ�¼"delta-time"��֮֡���ʱ����������Ϸ��ʱ��
	GameTimer mTimer;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;  // ������DXGI����
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;   // �����Ľ�����
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;     // ������D3D12�豸

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;  // ����Χ��
	UINT64 mCurrentFence = 0;					 // ��ǰΧ��ֵ

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;            // �������
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;  // ����������
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;		 // �����б�

	static const int SwapChainBufferCount = 2;  // ����˫����
	int mCurrBackBuffer = 0;					// ��¼��ǰ��̨����������
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];  // ������������
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;						// ���ģ�建����

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;  // RTV��
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;  // DSV��

	D3D12_VIEWPORT mScreenViewport;  // ��Ļ�ӿ�
	D3D12_RECT mScissorRect;		 // �ü�����

	UINT mRtvDescriptorSize = 0;		// ��ȾĿ�껺������������С
	UINT mDsvDescriptorSize = 0;		// ���ģ�建������������С
	UINT mCbvSrvUavDescriptorSize = 0;  // ��������ɫ����Դ��������ʻ�������������С

	// ����������������캯�����Զ�����Щ��ʼֵ
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE; 
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;  // �����ʽ
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};