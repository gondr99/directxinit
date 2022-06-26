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
	ThrowIfFailed(mDirectCmdListAlloc->Reset()); //명령 얼로케이터를 모두 리셋

	//명령 목록을 ExecuteCommandList를 통해서 명령 대기열에 추가했다면
	//명령 목록을 재성절 할 수 있다. 명령 목록을 재설정하면 메모리가 재활용된다.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	//자원 용도에 관련된 상태 전이를 Direct3D에 통지한다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	//대기상태에서 렌더 상태로 전이 되도록

	//뷰포트와 가위 직사각형 설정
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//후면 버퍼와 깊이 버퍼를 지운다.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), 
							D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
							//지우고자 하는 요소를 선택한다. 이렇게하면 전부 지움
							1.0f, 0, 0, nullptr);

	//렌더링 결과가 기록될 렌더 대상 버퍼들을 지정한다. 렌더타겟과 깊이스텐실 버퍼를 동시에 지정한다.
	// 렌더링에 사용될 렌더타겟과 깊이 스텐실 버퍼를 파이프라인에 묶는다. 
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	//자원은 리소스고, 그걸 사용하기 위해선 서술자(View, Descriptor)가 필요함
	//자원이 다 쓰여졌다면 해당 자원의 상태를 다시 PRESENT로 바꾸는 커맨드를 날려놓는다.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//명령 기록을 마무리
	ThrowIfFailed(mCommandList->Close());

	//커맨드 리스트에 있던걸 전부 꺼내서 명령대기열에 추가시켜서 실질적인 실행이 되게 만든다.

	ID3D12CommandList* cmdLists[] = { mCommandList.Get() }; //배열객체 초기화라서 중괄호 써줘야 함.
	//stdlib에 있는 countof 헬퍼를 이용해서 배열의 갯수를 셀 수 있다.
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	//작업기록이 완료되었다면 전면버퍼와 후면퍼버를 교환한다.
	ThrowIfFailed(mSwapChain->Present(0, 0)); //렌더링된 이미지를 유저에게 
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;


	FlushCommandQueue(); //이번 프레임의 모든 명령이 처리되길 기다린다.(이 방식은 비효율적이다. 나중에 더 좋은거 알려준데)

}
