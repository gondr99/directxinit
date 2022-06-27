#include "InitDirect3DApp.h"
#include <DirectXColors.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	

	try {
		InitDirect3DApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	} 
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}

}

InitDirect3DApp::InitDirect3DApp(HINSTANCE hInstance) : D3DApp(hInstance)
{

}

InitDirect3DApp::~InitDirect3DApp()
{
	
}

bool InitDirect3DApp::Initialize()
{
	if (D3DApp::Initialize() == false) {
		return false;
	}
	return true;
}

void InitDirect3DApp::OnResize()
{
	D3DApp::OnResize();
}

void InitDirect3DApp::Update(const GameTimer& gt)
{
}

void InitDirect3DApp::Draw(const GameTimer& gt)
{
	ThrowIfFailed(mDirectCmdListAlloc->Reset()); //��� ��������͸� ��� ����

	//��� ����� ExecuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�
	//��� ����� �缺�� �� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	//�ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//�����¿��� ���� ���·� ���� �ǵ���

	//����Ʈ�� ���� ���簢�� ����
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//�ĸ� ���ۿ� ���� ���۸� �����.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), 
							D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
							//������� �ϴ� ��Ҹ� �����Ѵ�. �̷����ϸ� ���� ����
							1.0f, 0, 0, nullptr);

	//������ ����� ��ϵ� ���� ��� ���۵��� �����Ѵ�. ����Ÿ�ٰ� ���̽��ٽ� ���۸� ���ÿ� �����Ѵ�.
	// �������� ���� ����Ÿ�ٰ� ���� ���ٽ� ���۸� ���������ο� ���´�. 
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	//�ڿ��� ���ҽ���, �װ� ����ϱ� ���ؼ� ������(View, Descriptor)�� �ʿ���
	//�ڿ��� �� �������ٸ� �ش� �ڿ��� ���¸� �ٽ� PRESENT�� �ٲٴ� Ŀ�ǵ带 �������´�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//��� ����� ������
	ThrowIfFailed(mCommandList->Close());

	//Ŀ�ǵ� ����Ʈ�� �ִ��� ���� ������ ��ɴ�⿭�� �߰����Ѽ� �������� ������ �ǰ� �����.

	ID3D12CommandList* cmdLists[] = { mCommandList.Get() }; //�迭��ü �ʱ�ȭ�� �߰�ȣ ����� ��.
	//stdlib�� �ִ� countof ���۸� �̿��ؼ� �迭�� ������ �� �� �ִ�.
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	//�۾������ �Ϸ�Ǿ��ٸ� ������ۿ� �ĸ��۹��� ��ȯ�Ѵ�.
	ThrowIfFailed(mSwapChain->Present(0, 0)); //�������� �̹����� �������� 
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;


	FlushCommandQueue(); //�̹� �������� ��� ����� ó���Ǳ� ��ٸ���.(�� ����� ��ȿ�����̴�. ���߿� �� ������ �˷��ص�)

}
