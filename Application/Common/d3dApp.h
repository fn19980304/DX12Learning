/*
*  Direct3D应用程序类
*  d3dApp.h
*  作者： Frank Luna (C)
*/

#pragma once

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include "d3dUtil.h"
#include "GameTimer.h"

// 链接所需的d3d12库
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

	// 绘制每一帧时调用该方法，随着时间推移更新3D应用程序
	// 如呈现动画、移动摄像机、做碰撞检测以及检查用户输入等
	virtual void Update(const GameTimer& gt) = 0;

	// 绘制每一帧时调用该方法，在该方法中发出渲染命令
	// 将当前帧真正地绘制到后台缓冲区中，绘制后再调用IDXGISwapChain::Present方法
	// 将后台缓冲区的内容呈现在屏幕上
	virtual void Draw(const GameTimer& gt) = 0;

	// 便于处理鼠标的按下、释放和移动事件提供的重写方法而不必重写MsgProc方法
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

	static D3DApp* mApp;  // 应用程序

	HINSTANCE mhAppInst = nullptr;		 // 应用程序实例句柄
	HWND      mhMainWnd = nullptr;		 // 主窗口句柄
	bool      mAppPaused = false;		 // 应用程序是否暂停
	bool      mMinimized = false;		 // 应用程序是否最小化
	bool      mMaximized = false;		 // 应用程序是否最大化
	bool      mResizing = false;		 // 大小调整栏是否受到拖拽
	bool      mFullscreenState = false;  // 是否开启全屏模式

	// 查询是否可以使用4X MSAA，默认为不能使用
	bool      m4xMsaaState = false;    
	UINT      m4xMsaaQuality = 0;  // 4X MSAA质量等级

	// 用于记录"delta-time"（帧之间的时间间隔）和游戏总时间
	GameTimer mTimer;

	Microsoft::WRL::ComPtr<IDXGIFactory4> mdxgiFactory;  // 创建的DXGI工厂
	Microsoft::WRL::ComPtr<IDXGISwapChain> mSwapChain;   // 创建的交换链
	Microsoft::WRL::ComPtr<ID3D12Device> md3dDevice;     // 创建的D3D12设备

	Microsoft::WRL::ComPtr<ID3D12Fence> mFence;  // 创建围栏
	UINT64 mCurrentFence = 0;					 // 当前围栏值

	Microsoft::WRL::ComPtr<ID3D12CommandQueue> mCommandQueue;            // 命令队列
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;  // 命令适配器
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList;		 // 命令列表

	static const int SwapChainBufferCount = 2;  // 采用双缓冲
	int mCurrBackBuffer = 0;					// 记录当前后台缓冲区索引
	Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];  // 交换链缓冲区
	Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;						// 深度模板缓冲区

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mRtvHeap;  // RTV堆
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mDsvHeap;  // DSV堆

	D3D12_VIEWPORT mScreenViewport;  // 屏幕视口
	D3D12_RECT mScissorRect;		 // 裁剪矩阵

	UINT mRtvDescriptorSize = 0;		// 渲染目标缓冲区描述符大小
	UINT mDsvDescriptorSize = 0;		// 深度模板缓冲区描述符大小
	UINT mCbvSrvUavDescriptorSize = 0;  // 常量、着色器资源、随机访问缓冲区描述符大小

	// 在派生类的派生构造函数中自定义这些初始值
	std::wstring mMainWndCaption = L"d3d App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE; 
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;  // 纹理格式
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};