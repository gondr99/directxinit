#include "D3dApp.h"



#include "d3dApp.h"
#include <WindowsX.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;

LRESULT CALLBACK
MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	// Forward hwnd on because we can get messages (e.g., WM_CREATE)
	// before CreateWindow returns, and thus before mhMainWnd is valid.
	return D3DApp::GetApp()->MsgProc(hwnd, msg, wParam, lParam);
}

D3DApp* D3DApp::mApp = nullptr;
D3DApp* D3DApp::GetApp()
{
	return mApp;
}
//�����ڿ����� instance�Ҵ�� ���ø����̼� �����Ϳ� �־��ִ� �ϸ� ��.
D3DApp::D3DApp(HINSTANCE hInstance)
	: mhAppInst(hInstance)
{
	// assert�� false�϶� �ٷ� ���� ����. ����� ����϶��� ������.
	assert(mApp == nullptr);
	mApp = this;
}

D3DApp::~D3DApp()
{
	if (md3dDevice != nullptr)
		FlushCommandQueue();
}

//�ν��Ͻ��� ���������� �ڵ��� ���� 
HINSTANCE D3DApp::AppInst()const
{
	return mhAppInst;
}

HWND D3DApp::MainWnd()const
{
	return mhMainWnd;
}

float D3DApp::AspectRatio()const
{
	return static_cast<float>(mClientWidth) / mClientHeight;
}

bool D3DApp::Get4xMsaaState()const
{
	return m4xMsaaState;
}

void D3DApp::Set4xMsaaState(bool value)
{
	if (m4xMsaaState != value)
	{
		m4xMsaaState = value;

		// Recreate the swapchain and buffers with new multisample settings.
		CreateSwapChain();
		OnResize();
	}
}

int D3DApp::Run()
{
	MSG msg = { 0 };

	mTimer.Reset();

	while (msg.message != WM_QUIT)
	{
		// If there are Window messages then process them.
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		// Otherwise, do animation/game stuff.
		else
		{
			mTimer.Tick();

			if (!mAppPaused)
			{
				CalculateFrameStats();
				Update(mTimer);
				Draw(mTimer);
			}
			else
			{
				Sleep(100);
			}
		}
	}

	return (int)msg.wParam;
}

//�ʱ�ȭ ����.
bool D3DApp::Initialize()
{
	//win32 �ʱ�ȭ
	if (!InitMainWindow())
		return false;

	//���̷�Ʈ X�ʱ�ȭ
	if (!InitDirect3D())
		return false;

	// Do the initial resize code.
	OnResize();

	return true;
}

void D3DApp::OnResize()
{
	assert(md3dDevice);
	assert(mSwapChain);
	assert(mDirectCmdListAlloc);
	//���� 3������ �ϳ��� �ȵǾ����� ����

	//Ŀ�ǵ� ť ����
	FlushCommandQueue();

	//Ŀ�ǵ�ť�� ����, ������������ ������ nullptr
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	// Release the previous resources we will be recreating.
	for (int i = 0; i < SwapChainBufferCount; ++i)
		mSwapChainBuffer[i].Reset();
	mDepthStencilBuffer.Reset();

	// Resize the swap chain.
	ThrowIfFailed(mSwapChain->ResizeBuffers(
		SwapChainBufferCount,
		mClientWidth, mClientHeight,
		mBackBufferFormat,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	mCurrBackBuffer = 0;

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SwapChainBufferCount; i++)
	{
		//����ü���� i��° ���۸� ��´�. 
		ThrowIfFailed(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
		//�ش� ���ۿ� ���� RTV�� �����Ѵ�.
		md3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
		//���� ���� �׸����� �Ѿ��.
		rtvHeapHandle.Offset(1, mRtvDescriptorSize);
	}

	// ����/���ٽ� ���� ��ũ���� ����
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;  //unkown, buffer, texture1d, 2d, 3d 
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = mClientWidth;				//�ؽ��� �ʺ�
	depthStencilDesc.Height = mClientHeight;			//�ؽ��� ����
	depthStencilDesc.DepthOrArraySize = 1;				//3���� �ؽ�ó�� ����, 1���� �� 2���� �ؽ�ó �迭 ũ��
	depthStencilDesc.MipLevels = 1;						//�Ӹ� ���� ����
	depthStencilDesc.Format = mDepthStencilFormat;		//D24_UNORM_S8_UINT
	depthStencilDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	depthStencilDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = mDepthStencilFormat;
	optClear.DepthStencil.Depth = 1.0f;
	optClear.DepthStencil.Stencil = 0;
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(mDepthStencilBuffer.GetAddressOf())));

	// ��ü �ڿ��� �Ӹ� ���� 0�� ���� �����ڸ� �ش� �ڿ��� �ȼ� ������ �����ؼ� �����Ѵ�.
	md3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, DepthStencilView());

	// ���ٽǹ��� �ڿ��� �ʱ� ���� ����� �� �ִ� ���·� �����Ѵ�. ���̽� �ٸ� ���� ���ǵ帮�� �踮� ����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));

	//Ŀ�ǵ帮��Ʈ �ڿ��� ������.
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// ResizeĿ�ǵ尡 �۵��� ���������� ���
	FlushCommandQueue();

	// Update the viewport transform to cover the client area.
	mScreenViewport.TopLeftX = 0;
	mScreenViewport.TopLeftY = 0;
	mScreenViewport.Width = static_cast<float>(mClientWidth);
	mScreenViewport.Height = static_cast<float>(mClientHeight);
	mScreenViewport.MinDepth = 0.0f;
	mScreenViewport.MaxDepth = 1.0f;

	mScissorRect = { 0, 0, mClientWidth, mClientHeight }; //������Ʈ�� �� �ۿ� �ִ� �༮���� �����Ͷ����� ���� �ʵ��� �Ѵ�.(����ȭ)
}

LRESULT D3DApp::MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		// WM_ACTIVATE is sent when the window is activated or deactivated.  
		// We pause the game when the window is deactivated and unpause it 
		// when it becomes active.  
	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			mAppPaused = true;
			mTimer.Stop();
		}
		else
		{
			mAppPaused = false;
			mTimer.Start();
		}
		return 0;

		// WM_SIZE is sent when the user resizes the window.  
	case WM_SIZE:
		// Save the new client area dimensions.
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

				// Restoring from minimized state?
				if (mMinimized)
				{
					mAppPaused = false;
					mMinimized = false;
					OnResize();
				}

				// Restoring from maximized state?
				else if (mMaximized)
				{
					mAppPaused = false;
					mMaximized = false;
					OnResize();
				}
				else if (mResizing)
				{
					// If user is dragging the resize bars, we do not resize 
					// the buffers here because as the user continuously 
					// drags the resize bars, a stream of WM_SIZE messages are
					// sent to the window, and it would be pointless (and slow)
					// to resize for each WM_SIZE message received from dragging
					// the resize bars.  So instead, we reset after the user is 
					// done resizing the window and releases the resize bars, which 
					// sends a WM_EXITSIZEMOVE message.
				}
				else // API call such as SetWindowPos or mSwapChain->SetFullscreenState.
				{
					OnResize();
				}
			}
		}
		return 0;

		// WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
	case WM_ENTERSIZEMOVE:
		mAppPaused = true;
		mResizing = true;
		mTimer.Stop();
		return 0;

		// WM_EXITSIZEMOVE is sent when the user releases the resize bars.
		// Here we reset everything based on the new window dimensions.
	case WM_EXITSIZEMOVE:
		mAppPaused = false;
		mResizing = false;
		mTimer.Start();
		OnResize();
		return 0;

		// WM_DESTROY is sent when the window is being destroyed.
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

		// The WM_MENUCHAR message is sent when a menu is active and the user presses 
		// a key that does not correspond to any mnemonic or accelerator key. 
	case WM_MENUCHAR:
		// Don't beep when we alt-enter.
		return MAKELRESULT(0, MNC_CLOSE);

		// Catch this message so to prevent the window from becoming too small.
	case WM_GETMINMAXINFO:
		((MINMAXINFO*)lParam)->ptMinTrackSize.x = 200;
		((MINMAXINFO*)lParam)->ptMinTrackSize.y = 200;
		return 0;

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
	WNDCLASS wc;
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = MainWndProc;   //�ݹ� ���
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = mhAppInst;
	wc.hIcon = LoadIcon(0, IDI_APPLICATION);
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszMenuName = 0;
	wc.lpszClassName = L"MainWnd";

	if (!RegisterClass(&wc))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return false;
	}

	// �����ص� Ŭ���̾�Ʈ ũ���� 800 X 600���� â�� �����Ѵ�.
	RECT R = { 0, 0, mClientWidth, mClientHeight };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	int width = R.right - R.left;
	int height = R.bottom - R.top;

	//�����츦 ����� ���� ������ �ڵ鿡 �̸� �����Ѵ�.
	mhMainWnd = CreateWindow(L"MainWnd", mMainWndCaption.c_str(),
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, mhAppInst, 0);
	if (!mhMainWnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return false;
	}

	//win32api �� â�� ����.
	ShowWindow(mhMainWnd, SW_SHOW);
	UpdateWindow(mhMainWnd);

	return true;
}

bool D3DApp::InitDirect3D()
{
	//����� ������ ��� D12�� ����� ���̾��� Ȱ��ȭ��Ų��.
#if defined(DEBUG) || defined(_DEBUG) 
	{
		ComPtr<ID3D12Debug> debugController;
		ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
		debugController->EnableDebugLayer();
	}
#endif

	//DirectX Factory�� �����Ͽ� mdxgiFactory ������ �ִ´�.
	//IID_PPV_ARGS �� �־��� ��ü�� ����ġ�� ���� GUID �� ������ �μ��� �������̽� �Ļ������� �˻��Ͽ� �ֽ��ϴ�.
	//GUID �� �۷ι� ����ũ ���̵��� ���ڷ� ����Ʈ����� ������ �ĺ��� ������ �ο��ϴ°�
	//	IID_PPV_ARGS_Helper �� ���� ȣ�� �Ұ��� ComPtr Ÿ���� ������ �޾Ƽ� Ȯ���� (���� �ƴ϶�� ������ ����)
	// �ش� comptr�κ��� ���� ����̽��� ** �� �����ɴϴ�. 
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&mdxgiFactory)));

	// �ϵ���� ����͸� ��Ÿ���� ��ġ �ڵ��� �����Ѵ�.
	HRESULT hardwareResult = D3D12CreateDevice(
		nullptr,             // �⺻ ����ͷ� �����´�. 
		D3D_FEATURE_LEVEL_11_0, //�ּҷ���
		IID_PPV_ARGS(&md3dDevice));

	// ���� �ϵ���� ����� ��ġ�ڵ��� �����ϴµ� �����ߴٸ� WARP ����͸� ��Ÿ���� ��ġ�� Ȱ��ȭ �Ѵ�.
	// WARP�� ����Ʈ���� ������� = > window advanced rasterization platform 
	if (FAILED(hardwareResult))
	{
		ComPtr<IDXGIAdapter> pWarpAdapter;
		ThrowIfFailed(mdxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pWarpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			pWarpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&md3dDevice)));
	}

	//CPU, GPU�� �۾� ������ ���� �潺�� ��ġ�Ѵ�.
	ThrowIfFailed(md3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE,
		IID_PPV_ARGS(&mFence)));

	//�� �������� ũ��� GPU���� �ٸ��� ������ ������ �� �̸� ũ�⸦ ��������� �����صξ�� �Ѵ�.
	//���� Ÿ�� ��(��ũ����) => �׷��ִ� �ڿ�
	mRtvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//�X�� ���ٽ� �� => ���̿� ���ٽ� �ڿ�
	mDsvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	// �ܽ�ź�� ���� �� => �������
	mCbvSrvUavDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 4X MSAA �� üũ�Ͽ� �����ϴ����� ����.(Multi Sampling Anti-Aliasing.)
	// Check 4X MSAA quality support for our back buffer format.
	// All Direct3D 11 capable devices support 4X MSAA for all render 
	// target formats, so we only need to check quality support.

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = mBackBufferFormat;
	msQualityLevels.SampleCount = 4;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(md3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

	m4xMsaaQuality = msQualityLevels.NumQualityLevels;
	assert(m4xMsaaQuality > 0 && "Unexpected MSAA quality level.");

#ifdef _DEBUG
	//LogAdapters();
#endif

	CreateCommandObjects();
	CreateSwapChain();
	CreateRtvAndDsvDescriptorHeaps();

	return true;
}

void D3DApp::CreateCommandObjects()
{
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	//mCommandQueue�� ����̽��κ��� Ŀ�ǵ� ť�� �޾Ƽ� �����Ѵ�.
	ThrowIfFailed(md3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	ThrowIfFailed(md3dDevice->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(mDirectCmdListAlloc.GetAddressOf())));

	ThrowIfFailed(md3dDevice->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mDirectCmdListAlloc.Get(), // Associated command allocator
		nullptr,                   // Initial PipelineStateObject , �ƹ��͵� �׸��� ������ �ʱ� ������������ �η� ����
		IID_PPV_ARGS(mCommandList.GetAddressOf())));

	// ���� ���·� �����Ѵ�. ���� ���� ����� ó�� ������ �� Reset�� �ϰ� �Ǵµ�
	// 	�����ϱ� ���ؼ��� �ݵ�� closed ���¿��� �ϱ� �����̴�.
	
	mCommandList->Close();
}

//��ȯ�罽 ����
void D3DApp::CreateSwapChain()
{
	// Release the previous swapchain we will be recreating.
	mSwapChain.Reset();

	//����ü�� ��ũ���� ����ü�� �ʿ��� �������� �Ҵ��Ѵ�.
	DXGI_SWAP_CHAIN_DESC sd;
	//BufferDesc �ĸ� �۹� �Ӽ��� ����
	// �ʺ�, ����, ����(R8G8B8A8_UNORM)
	sd.BufferDesc.Width = mClientWidth;
	sd.BufferDesc.Height = mClientHeight;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferDesc.Format = mBackBufferFormat;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	//���� ǥ��ȭ ������ ǰ�� ������ �����ϴ� ����ü, ���� ǥ��ȭ�� �Ϸ��� ǰ�� ������ 0, ������ 1�� �����ϸ� ��.
	sd.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	sd.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	//�ĸ� ���ۿ� ������ �Ұ��̴� ����Ÿ�� �ƿ�ǲ�� �����Ͽ� ���뵵�� ���Ѵ�.
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//�ĸ��۹��� ������ ����(2)
	sd.BufferCount = SwapChainBufferCount;
	sd.OutputWindow = mhMainWnd;
	sd.Windowed = true;
	//�۹��� �����Ҷ� �� �ൿ�� �����ϴ� ��.
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	//��üȭ�� ��ȯ�� â�� ����ũ�⿡ �߸´� �ػ󵵷� ��ȯ, ������� ����ũž �⺻ �ػ󵵷�
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// ����ü���� Ŀ�ǵ�ť�� �̿��ؼ� flush�� �����ϰ� �ȴ�. ���� Ŀ�ǵ�ť�� ����̽��� �޴´�.
	ThrowIfFailed(mdxgiFactory->CreateSwapChain(
		mCommandQueue.Get(),
		&sd,
		mSwapChain.GetAddressOf()));
}

//CreateDescriptorHeap �� �̿��ؼ� ������ ���� �����.
void D3DApp::CreateRtvAndDsvDescriptorHeaps()
{
	//����ü�ο��� ����ϴ� RTV �� ������ ��

	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount; //����ü���� ��ϵ� ���۹� ������ŭ ����
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(mRtvHeap.GetAddressOf())));


	//���� ���ٽ� ������ ������ ��
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(mDsvHeap.GetAddressOf())));
}

//�潺�� �̿��ؼ� CPU�� GPU�� Ŀ�ǵ� ť�� �� ��� ������ ��ٸ���.
void D3DApp::FlushCommandQueue()
{
	// ���� �潺�� ���� �ѹ��� �ϳ� ������Ų��.
	mCurrentFence++;

	// ���ο� ������ Ŀ��� ť�� �߰���Ų��.(���ο� �潺 ������ ����)
	// GPU�� Ÿ�Ӷ��ο����� ���� ���ο� �潺�� ���õǾ� ���� �ʱ� �����̴�.
	// ���� GPU�� ť�� �ִ� ��� ������ �����Ű�� �ñ׳� ������ �����Ű�� �Ǹ� �潺 ������ �Ϸ�ȴ�.
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), mCurrentFence));


	// ���� currentFence���� ��� ó���� �� �Ȼ��¸� �׳� �Ѿ�� �ƴϸ� ���⼭ �����ȴ�.
	if (mFence->GetCompletedValue() < mCurrentFence)
	{
		//����� ���� �̺�Ʈ�� ����� win32 �Լ�. 
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

		// Fire event when GPU hits current fence.  
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));

		// �潺 �̺�Ʈ�� fire�Ǳ������� ���⼭ ���Ѵ����°� �ȴ�.
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

ID3D12Resource* D3DApp::CurrentBackBuffer()const
{
	//����ü�� ���ۿ��� ���� ���۸� ����
	return mSwapChainBuffer[mCurrBackBuffer].Get();
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::CurrentBackBufferView()const
{
	//�� == ��ũ���� 
	//��ũ���������� ��ũ���͸� �̾ƿ��°�
	//ȭ�� ��ũ���ʹ� ��ũ������ ����� �˾ƾ��ϱ� ������ mRtvDescriptorSize�� �����ص� ���̴�.
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(
		mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
		mCurrBackBuffer,
		mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3DApp::DepthStencilView()const
{
	return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void D3DApp::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
	// average time it takes to render one frame.  These stats 
	// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((mTimer.TotalTime() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		wstring fpsStr = to_wstring(fps);
		wstring mspfStr = to_wstring(mspf);

		wstring windowText = mMainWndCaption +
			L"    fps: " + fpsStr +
			L"   mspf: " + mspfStr;

		SetWindowText(mhMainWnd, windowText.c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

//�Ʒ� 3���� �׳� ����׿� �ڵ���. ��� ��.
//�ý��ۿ� �ִ� ��� ����͸� �����Ѵ�.
void D3DApp::LogAdapters()
{
	UINT i = 0;
	IDXGIAdapter* adapter = nullptr;
	std::vector<IDXGIAdapter*> adapterList;
	//���丮�� ����͵��� ����Ʈ�� ������ �ִ�.
	while (mdxgiFactory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;  //����� ��ũ���� ����ü
		adapter->GetDesc(&desc);

		//���⼭���ʹ� ����� �ڵ�
		std::wstring text = L"***Adapter: ";
		text += desc.Description;
		text += L"\n";

		//C_str�� C��Ÿ�� ���ڿ��� ��ȯ���ش�(�����ͷ� �̷���� ĳ���Ϳ� �������� null�� ������)
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
	UINT i = 0;
	IDXGIOutput* output = nullptr;
	//����ũ�� ����Ʈ Basic render Driver�� ���� ����Ʈ���� ���÷��� ������̴�. ���� ����� ���÷��̰� ���� ������ ���⼭ ���� ��µ��� �ʴ´�.
	//DXGI_ADAPTER_DESC desc;  //����� ��ũ���� ����ü
	//adapter->GetDesc(&desc);
	//std::wstring text = L"***���� �����: ";
	//text += desc.Description;
	//text += L"\n";

	//OutputDebugString(text.c_str());

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

//���÷��� �ػ󵵸� ����ϴ� �Լ�
void D3DApp::LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format)
{
	UINT count = 0;
	UINT flags = 0;

	// Call with nullptr to get list count.
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