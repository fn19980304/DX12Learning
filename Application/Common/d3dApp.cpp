/*
*  Direct3D应用程序类实现
*  d3dApp.cpp
*  作者： Frank Luna (C)
*/

#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 窗口过程，在处理消息时候自动调用
	// 实际的窗口过程处理函数为MsgProc方法

	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
	// 获得一个D3D应用程序实例

	return mApp;
}

D3DApp::D3DApp(HINSTANCE hInstance) :mhAppInst(hInstance)
{
	// 简单地将数据成员初始化为默认值

	// mhAppInst = hInstance
	// 只允许构建一个D3DApp
	assert(mApp == nullptr);
	mApp = this;
}

D3DApp::~D3DApp()
{
	// 释放D3DApp中所用的COM接口对象并刷新命令队列

	if (md3dDevice != nullptr) {
		FlushCommandQueue();
	}
}

HINSTANCE D3DApp::AppInst()const
{
	// 返回应用程序实例句柄

	return mhAppInst;
}

HWND D3DApp::MainWnd()const
{
	// 返回主窗口句柄

	return mhMainWnd;
}

float D3DApp::AspectRatio()const
{
	// 纵横比（其实是后台缓冲区宽度/高度）

	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState()const
{
	// 获取4X MSAA开启情况

	return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
	// 开启或禁用4X MSAA

	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;

		// 重置交换链和缓冲区
		CreateSwapChain();
		OnResize();
	}
}

int D3DApp::Run()
{
	// 运行窗口
	// 没有窗口消息到来时就会处理我们的游戏逻辑部分

	// 消息循环
	// 定义消息结构体
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT) {
		// 如果有窗口消息就进行处理
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);  // 键盘按键的虚拟键消息转换为字符消息
			DispatchMessage(&msg);   // 消息分派
		}
		// 否则就执行动画与游戏的相关逻辑
		else {
			mTimer.Tick();

			if (!mAppPaused) {
				CalculateFrameStats();
				Update(mTimer);
				Draw(mTimer);
			}
			else {
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

bool D3DApp::Initialize()
{
	// 初始化方法

	// 初始化窗口
	if (!InitMainWindow())
		return false;

	// 初始化D3D
	if (!InitDirect3D())
		return false;

	// 调整大小
	OnResize();

	return true;
}

void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	// 创建描述符堆

	// RTV堆创建
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;  // RTV针对的是前、后台缓冲区
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));

	// DSV堆创建
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;						// 只创建一个DSV堆（DSV针对后台缓冲区）
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

void D3DApp::OnResize()
{
	// 创建目标渲染视图以及窗口工作区调整后调整大小

	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);

	// 在改变任何资源以前刷新刷新命令队列，再重置命令列表
	FlushCommandQueue();
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// 释放之前的资源
	for (int i = 0; i < SwapChainBufferCount; ++i) {
		mSwapChainBuffer[i].Reset();
	}
	mDepthStencilBuffer.Reset();

	// 修改交换链
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	// 为交换链中的每一个缓冲区创建一个RTV
	// 首先创建一个用来记录当前创建RTV在堆中所处位置的句柄
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; ++i) {
		// 获得存于交换链中的第i个缓冲区资源
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));

		// 为该资源创建一个RTV
		md3dDevice->CreateRenderTargetView(
			mSwapChainBuffer[i].Get(),  // 指定用作渲染目标的资源（后台缓冲区） 
			nullptr,					// 资源中元素的数据格式 
			rtvHeapHandle);

		// 偏移到描述符堆的下一个描述符
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// 创建深度模板缓冲区及其视图
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;				// 资源维度（使用2D纹理）
	depthStencilDesc.Alignment = 0;													// 对齐
	depthStencilDesc.Width = mClientWidth;											// 以纹素为单位的纹理宽度
	depthStencilDesc.Height = mClientHeight;										// 以纹素为单位的纹理高度
	depthStencilDesc.DepthOrArraySize = 1;											// 以纹素为单位的纹理深度（2D纹理数组的大小）
	depthStencilDesc.MipLevels = 1;													// mipmap层级
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;							// 纹素格式
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;						// 多重采样次数
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;  // 多重采样质量级别
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;                         // 纹理布局
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;														// 清除资源的优化值
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),			  // 堆的属性
		D3D12_HEAP_FLAG_NONE,										  // 堆的额外标志
		&depthStencilDesc,											  // 待建资源
		D3D12_RESOURCE_STATE_COMMON,								  // 设置资源状态为初始状态
		&optClear,													  // 清除资源的优化值
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// 利用该资源格式，为整个资源的第0mip层创建描述符
	md3dDevice->CreateDepthStencilView(
		mDepthStencilBuffer.Get(),  // 指定深度模板缓冲区资源
		nullptr,					// 资源中元素的数据格式（书中为nullptr，官方代码中使用dsvDesc）
		DepthStencilView());

	// 将资源从初始状态转换为深度缓冲区
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	// 执行调整大小命令
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 调整大小结束前让CPU等待
	FlushCommandQueue();

	// 更新视口
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight };

	// 在之后使用该框架编程时可以调用
	// RSSetViewports及RSSetScissorRects方法设置视口和裁剪矩形
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// 窗口过程函数

	switch (msg)
	{
		// 当一个程序激活（activate）或未激活（deactivate）时便会发送WM_ACTIVATE消息
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			// 暂停游戏（deactivated），停止计时
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			// 启动游戏（actived），开始计时
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

		// 当我们改变窗口大小的时候就会产生WM_SIZE消息
	case WM_SIZE:
		// 记录新的窗口大小
		mClientWidth = LOWORD(lParam);
		mClientHeight = HIWORD(lParam);
		if (md3dDevice)
		{
			if (wParam == SIZE_MINIMIZED)
			{
				mAppPaused = true;
				mMinimized = true;
				mMaximized = false;
			}
			else if (wParam == SIZE_MAXIMIZED)
			{
				mAppPaused = false;
				mMinimized = false;
				mMaximized = true;
				OnResize();
			}
			else if (wParam == SIZE_RESTORED)
			{

				// 是否从最小化恢复
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// 是否从最大化恢复
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// 当用户拖动调整栏时，会连续发出WM_SIZE，此时我们不应该随之
					// 连续调整缓冲区大小。所以，此时应什么都不做（除了暂停程序），
					// 直到用户调整操作结束后，再执行OnResize方法
				}
				else
				{
					// 如SetWindowPos或mSwapChain->SetFullscreenState这样的API调用
					OnResize();
				}
			}
		}
		return 0;

		// 当用户抓取调整栏时会发送WM_EXITSIZEMOVE消息
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		// 当用户释放调整栏时会发送WM_EXITSIZEMOVE消息
		// 此时根据新的窗口大小重置相关对象（缓冲区、视图等）
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		// 当窗口被销毁时发送WM_DESTROY消息
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// 当某一菜单处于激活状态且用户按下的既不是助记键又不是加速键时
		// 发送WM_MENUCHAR消息
	case WM_MENUCHAR:
		// 当按下组合键alt-enter时不会发出beep蜂鸣声
		return MAKELRESULT(0, MNC_CLOSE);

		// 捕获此消息以防窗口变得过小
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

		// 按以下方式处理与鼠标有关的消息
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		OnMouseDown(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		OnMouseUp(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_MOUSEMOVE:
		OnMouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		return 0;
	case WM_KEYUP:
		if (wParam == VK_ESCAPE)
		{
			PostQuitMessage(0);
		}
		else if ((int)wParam == VK_F2)
			Set4xMsaaState(!m4xMsaaState);

		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool D3DApp::InitMainWindow()
{
	// 初始化Windows窗口

	// Step1:填写窗口初始化描述结构体(WNDCLASS)
	WNDCLASS wc;

	wc.style = CS_HREDRAW | CS_VREDRAW;				         // 工作区宽高改变重绘窗口
	wc.lpfnWndProc = MainWndProc;							 // 绑定窗口过程
	wc.cbClsExtra = 0;										 // 以上两个字段为当前应用分配额外的内存空间
	wc.cbWndExtra = 0;										 // 不分配时=0
	wc.hInstance = mhAppInst;								 // 应用程序实例
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);				 // 应用程序图标：默认
	wc.hCursor = LoadCursor(0, IDC_ARROW);					 // 鼠标指针样式：默认
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);  // 背景颜色：白色
	wc.lpszMenuName = 0;									 // 不设置菜单栏
	wc.lpszClassName = L"MainWnd";							 // 窗口名

	if (!RegisterClass(&wc)) {
		// 注册窗口
		// 如果失败报错
		MessageBox(0, L"RegisterClass FAILED", 0, 0);
		return false;
	}

	// Step2:根据请求的用户区域尺寸计算窗口矩形尺寸
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	// Step3:注册成功并计算尺寸后创建窗口
	//       该方法返回一个HWND类型的窗口句柄，创建失败时返回0
	mhMainWnd = CreateWindow(
		L"MainWnd",				  // 使用前面注册的WNDCLASS实例，必须与wc.lpszClassName一致
		mMainWndCaption.c_str(),  // 窗口标题
		WS_OVERLAPPEDWINDOW,	  // 窗口样式示例
		CW_USEDEFAULT,			  // x坐标
		CW_USEDEFAULT,			  // y坐标
		width,				      // 窗口宽
		height,				      // 窗口高
		0,						  // 父窗口
		0,						  // 菜单
		mhAppInst,				  // 应用程序实例
		0
	);
	if (mhMainWnd == 0) {
		// 创建失败报错
		MessageBox(0, L"CreateWindow FAILED", 0, 0);
		return false;
	}

	// Step4:展示窗口并更新
	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
	// 初始化Direct3D
#if defined(DEBUG) || defined(_DEBUG) 
	// 启用D3D12的调试层
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	// Step1:用D3D12CreateDevice函数创建ID3D12Device接口实例

	// 首先创建DXGI工厂接口
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// 在DXGI工厂接口之上创建D3D12设备
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,				   // 默认适配器
		D3D_FEATURE_LEVEL_11_0,    // 支持D3D11特性
		IID_PPV_ARGS(&md3dDevice)  // 创建ID3D12Device的COMID
	);

	// 回退至WARP设备
	if (FAILED(hardwareResult)) {
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}


	// Step2:创建一个ID3D12Fence对象，并查询描述符的大小

	// 创建围栏
	ThrowIfFailed(md3dDevice->CreateFence(
		0,  // 标记围栏点，初值为0
		D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	// 查询描述符大小，方便在描述符堆中找到相应描述符
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV
	);
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	);
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
	);

	// Step3:检测用户设备对4X MSAA质量级别的支持情况

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;       // 采用4X MSAA
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;  // 质量级别
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));  // 第二参数既是输入又是输出，查询后填写NumQualityLevels

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");  // 若支持MSAA应返回>0，否则就报错

#ifdef _DEBUG
	LogAdapters();
#endif

	// Step4:依次创建命令队列、名列列表分配器和主命令列表

	CreateCommandObjects();

	// Step5:描述并创建交换链

	CreateSwapChain();

	// Step6:创建应用所需的描述符堆

	CreateRtvAndDsvDescriptorHeaps();

	// Step7:调整后台缓冲区大小，并为它创建渲染目标视图
	// Step8:设置视口和裁剪矩形

	// OnResize();

	return true;
}

void D3DApp::CreateCommandObjects()
{
	// 创建命令队列和命令列表

	// 命令队列
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;  // 存储的命令是GPU可直接执行的
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// 命令适配器
	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	// 命令列表
	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(),  // 关联命令分配器
		nullptr,                    // 初始化流水线状态对象
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// 在第一次引用命令列表前要重置，而在调用重置方法前要先关闭
	mCommandList->Close();
}

void D3DApp::CreateSwapChain()
{
	// 描述并创建交换链

	// 首先应当释放之前的交换链，再重建
	mSwapChain.Reset();

	// 填写DXGI_SWAP_CHAIN_DESC结构体
	DXGI_SWAP_CHAIN_DESC sd;
	sd.BufferDesc.Width = mClientWidth;								        // 缓冲区分辨率宽度
	sd.BufferDesc.Height = mClientHeight;							        // 缓冲区分辨率高度
	sd.BufferDesc.RefreshRate.Numerator = 60;						        // 刷新率分母
	sd.BufferDesc.RefreshRate.Denominator = 1;						        // 刷新率分子
	sd.BufferDesc.Format = mBackBufferFormat;							    // 缓冲区显示格式
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;  // 逐行 vs. 隔行扫描（未指定） 
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;					// 图像如何拉伸（未指定）
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;								// 多重采样次数
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;		// 多重采样质量级别
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;                       // 将后台缓冲区作为渲染目标
	sd.BufferCount = SwapChainBufferCount;                                  // 交换链中的缓冲区数量（双缓冲）
	sd.OutputWindow = mhMainWnd;											// 渲染窗口
	sd.Windowed = true;														// 窗口模式
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;						// 切换全屏自适应

	// 通过命令队列刷新交换链
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

void D3DApp::FlushCommandQueue()
{
	// 强制CPU等待GPU，以解决同步问题，在CPU可能产生同步问题前都要调用该方法刷新命令队列

	// 增加围栏值，接下来将命令标记到此围栏点
	mCurrentFence++;

	// CPU告诉GPU向命令队列中添加一条用来设置新围栏点的命令
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));

	// 在CPU端等待GPU，直到后者执行完这个围栏点之前的所有命令
	if (mFence->GetCompletedValue() < mCurrentFence) {
		// 创建用来实现CPU等待的事件
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

		// 若GPU命中当前的围栏（即执行到Signal()指令，修改了围栏值），则激发预定事件
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// 等待GPU命中围栏激发事件，若事件处于未激发（is not signaled）CPU一直处于等待状态
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	// 返回交换链中当前后台缓冲区的ID3D12Resource

	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	// 返回当前后台缓冲区的RTV句柄

	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),  // 堆中的首个句柄
		mCurrBackBuffer,                                 // 偏移至后台缓冲区描述符句柄的索引
		mRtvDescriptorSize);                             // 描述符所占字节的大小  
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	// 返回主深度模板模板缓冲区的DSV句柄

	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
	// 计算每秒的平均帧数以及每帧的平均渲染时间
	// 这些统计信息都会被附加到窗口的标题栏中

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// 以1秒为统计周期来计算平均帧数以及每帧的平均渲染时间
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f) {
		float fps = (float)frameCnt;  // fps = frameCnt / 1
		float mspf = 1000.0f / fps;


		// 显示文本内容到窗口
		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// 为计算下一组平均值重置
		frameCnt = 0;
		timeElapsed += 1.0f;
	}

}

void D3DApp::LogAdapters()
{
	// 枚举系统的所有显示适配器（例如独立显卡）

	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		adapter->GetDesc(&desc);

		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		OutputDebugString(text.c_str());

		adapterList.push_back(adapter);

		++i;
	}

	for (size_t i = 0; i < adapterList.size(); ++i)
	{
		LogAdapterOutputs(adapterList[i]);
		ReleaseCom(adapterList[i]);
	}
}

void D3DApp::LogAdapterOutputs(IDXGIAdapter* adapter)
{
	// 枚举与某块适配器关联的所有显示输出（例如一块显卡与它相关联的显示器）

	UINT i = 0;
	IDXGIOutput* output = nullptr;
	while (adapter->EnumOutputs(i, &output) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_OUTPUT_DESC desc;
		output->GetDesc(&desc);

		std::wstring text = L"***Output: ";
		text += desc.DeviceName;
		text += L"\n";
		OutputDebugString(text.c_str());

		LogOutputDisplayModes(output, mBackBufferFormat);

		ReleaseCom(output);

		++i;
	}
}

void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	// 获取某个显示输出对具体格式所支持的全部显示模式

	UINT count = 0;
	UINT flags = 0;

	// 以nullptr作为参数调用此函数来获取符合条件的显示模式的个数
	output->GetDisplayModeList(format, flags, &count, nullptr);

	std::vector<DXGI_MODE_DESC> modeList(count);
	output->GetDisplayModeList(format, flags, &count, &modeList[0]);

	for (auto& x : modeList)
	{
		UINT n = x.RefreshRate.Numerator;
		UINT d = x.RefreshRate.Denominator;
		std::wstring text =
			L"Width = " + std::to_wstring(x.Width) + L" " +
			L"Height = " + std::to_wstring(x.Height) + L" " +
			L"Refresh = " + std::to_wstring(n) + L"/" + std::to_wstring(d) +
			L"\n";

		::OutputDebugString(text.c_str());
	}
}