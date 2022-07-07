#include "BoxApp.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		BoxApp theApp(hInstance);
		if (!theApp.Initialize())
		{
			return 0;
		}

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR failed", MB_OK);
		return 0;
	}
}

BoxApp::BoxApp(HINSTANCE hInstance) : D3DApp(hInstance)
{} //do nothing


BoxApp::~BoxApp(){}

bool BoxApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	BuildDescriptorHeap();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShaderAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	ThrowIfFailed(mCommandList->Close());

	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	FlushCommandQueue();

	return true;
}

void BoxApp::OnResize()
{
	D3DApp::OnResize();

	//â�� ũ�Ⱑ �ٲ�����Ƿ� ��ȸ�� �����ϰ� ��������� �ٽ� ����Ѵ�.
	XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, AspectRatio(), 1.0f, 1000.0f);
	// ù��° �Ķ���ʹ� �þ߰�(�����þ߰��̸� ���� �����̴� ���� 45���� �þ߰��� ������ �ȴ�, ����þ߰��� ���� �ʴ´�. �̴� ȭ������� ���� ����ϱ� ����)
	// �ι�°�� ��Ⱦ��( �ʺ� / ����) 
	// ����°�� near clipping plane
	// �׹�°�� far clipping plane
	XMStoreFloat4x4(&mProj, P);
}

void BoxApp::Update(const GameTimer& gt)
{
	//���� ��ǥ�� ��ī��Ʈ ��ǥ�� ��ȯ�Ѵ�.  
	/*
		������ǥ : 3���� ���������� ����ǥ��
		������ǥ�� ��ī��Ʈ�� ��ȯ��  ������ * sin(p) * cos(t) �� x
		
	*/
	float x = mRadius * sinf(mPhi) * cosf(mTheta);    //sin45 * cos270
	float z = mRadius * sinf(mPhi) * sinf(mTheta);
	float y = mRadius * cosf(mPhi);

	//�þ� �࿭�� �����Ѵ�.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	//�þ� �࿭�� ������ �Լ� 
	// ī�޶�� ��ġ, ���� �ִ� �����, ������ up �𷺼� �� �޾� �þ��࿭�� �����.
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	//������� �࿭�� ��������� float4x4�� �����Ѵ�.
	XMStoreFloat4x4(&mView, view);

	XMMATRIX world = XMLoadFloat4x4(&mWorld);
	XMMATRIX proj = XMLoadFloat4x4(&mProj);
	XMMATRIX worldViewProj = world * view * proj;

	//�ֽ� worldViewProj�� ��� ���۸� �����Ѵ�.
	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WorldViewProj, XMMatrixTranspose(worldViewProj));
	mObjectCB->CopyData(0, objConstants);

}

void BoxApp::Draw(const GameTimer& gt)
{
	//��� ��Ͽ� ���õ� �޸��� ��Ȱ���� ���� ��� �Ҵ��ڸ� �缳���Ѵ�.
	//�缳���� GPU�� ���� �����ϵ��� ��� ó���Ѵ����� �Ͼ��.
	ThrowIfFailed(mDirectCmdListAlloc->Reset());

	//��ɸ���� ExecuteCommandList�� ���ؼ� ��� ��⿭�� �߰��ߴٸ�
	//��� ����� �缳�� �� �� �ִ�. ��� ����� �缳���ϸ� �޸𸮰� ��Ȱ��ȴ�.
	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), mPSO.Get()));
	
	//RS = �����Ͷ����� ��������
	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	//�ڿ� �뵵�� ���õ� ���� ���̸� Direct3D�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	//�ĸ� ���ۿ� ���� ���۸� �����.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), Colors::LightBlue, 0, nullptr); //��ü �����
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);


	//�������� ����� ��ϵ� ���� ��� ���۵��� �����Ѵ�.
	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
	//Bind one or more render targets atomically and the depth-stencil buffer to the output-merger stage.

	ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	mCommandList->IASetVertexBuffers(0, 1, &mBoxGeo->VertexBufferView());
	//Bind an array of vertex buffers to the input - assembler stage.
	mCommandList->IASetIndexBuffer(&mBoxGeo->IndexBufferView());
	//Bind an index buffer to the input-assembler stage.
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//�ﰢ������ �̷��� ���������� �ɼ� ����

	mCommandList->SetGraphicsRootDescriptorTable(0, mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	//Sets a descriptor table into the graphics root signature.

	//string, submeshgeometry �� ���� submesh�� �����ͼ� indexcount ��ƴ ��ο�
	mCommandList->DrawIndexedInstanced(mBoxGeo->DrawArgs["box"].IndexCount, 1, 0, 0, 0);

	//�� �׷ȴٸ� �뵵�� PRESENT�� �����Ѵ�.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	//��ɵ��� ��� ��ġ��
	ThrowIfFailed(mCommandList->Close());

	//��� ������ ���� ��� ����� ��� ��⿭�� �߰��Ѵ�.
	ID3D12CommandList* cmdLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdLists), cmdLists);

	//�ĸ� ���ۿ� ���� ���� ��ü
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	FlushCommandQueue();
}

void BoxApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	//���콺 ���� �ۿ����� Mousemove�̺�Ʈ�� ������� �ϴ°�(�������� ��츸)
	SetCapture(mhMainWnd);
}

void BoxApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();  //ĸó ����
}

void BoxApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		//���콺�� 1�ȼ��� 1/4 degree�� �����Ѵ�.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		//���콺 �Է¿� �����ؼ� ������ �����Ѵ�. �̿� ���� ī�޶� ���ڸ� �߽����� �����Ѵ�.
		mTheta += dx;
		mPhi += dy;

		//mPhi ������ �����Ѵ�.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, MathHelper::Pi - 0.1f); //1~ 179 �� ������ ����

	}
	else if ((btnState & MK_RBUTTON) != 0) { //��Ŭ�� ��
		//���ȼ��� ����� 0.005������ ����
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// ���콺 �Է¿� �����ؼ� ī�޶� �������� ����
		mRadius += (dx - dy);

		//������ ����
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}


//�ʱ�ȭ�� ����Ǵ� �Լ��� -> ������ ���� �����Ѵ�.
void BoxApp::BuildDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
	cbvHeapDesc.NumDescriptors = 1;
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbvHeapDesc.NodeMask = 0;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

//�̺κ��� �ٽ� �����ض� ������Ʈ �ܽ�źƮ�� �������� ���� �ٽ�!!!
/*
  �߿�!!!
*/
void BoxApp::BuildConstantBuffers()
{
	//ObjectConstant ���ε� ����
	mObjectCB = make_unique<UploadBuffer<ObjectConstants>>(md3dDevice.Get(), 1, true);

	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = mObjectCB->Resource()->GetGPUVirtualAddress();

	//���ۿ��� i��° ��ü�� ��� ������ �������� ��´�.
	// i = 0���� ����

	int boxCBufferIndex = 0;
	cbAddress += boxCBufferIndex * objCBByteSize; //0��° ������Ʈ ���

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	cbvDesc.BufferLocation = cbAddress;
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	md3dDevice->CreateConstantBufferView(&cbvDesc, mCbvHeap->GetCPUDescriptorHandleForHeapStart());
}

void BoxApp::BuildRootSignature()
{
	// �Ϲ������� ���̴� ���α׷��� Ư�� �ڿ����� �Էµȴٰ� ����Ѵ�, 
	//��Ʈ ������ ���̴� ���α׷��� ����ϴ� �ڿ����� �����Ѵ�. 
	//���̴� ���α׷��� ���������� �ϳ��� �Լ��̰� 
	//���̴��� �ԷµǴ� �ڿ����� �Լ��� �Ű������鿡 �ش��ϹǷ�, 
	//��Ʈ ������ �� �Լ� ������ �����ϴ� �����̶� �� �� �ִ�.

	//��Ʈ �Ű������� ������ ���̺��̰ų� ��Ʈ ������ �Ǵ� ��Ʈ ����̴�.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	//CBV �ϳ��� ��� ������ ���̺��� ����
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	//��Ʈ ������ ��Ʈ �Ű��������� �迭�̴�. 
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	//��� ���� �ϳ��� ������ ������ ������ ����Ű�� ���� 
	//�ϳ��� �̷���� ��Ʈ ������ �����Ѵ�.
	ComPtr<ID3DBlob> serializeRootsig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc, 
		D3D_ROOT_SIGNATURE_VERSION_1, 
		serializeRootsig.GetAddressOf(), 
		errorBlob.GetAddressOf());

	if (errorBlob != nullptr) //������ �߻��ߴٸ�
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}

	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0, 
		serializeRootsig->GetBufferPointer(), 
		serializeRootsig->GetBufferSize(), 
		IID_PPV_ARGS(&mRootSignature)));

}

void BoxApp::BuildShaderAndInputLayout()
{
	HRESULT hr = S_OK;

	mvsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	mpsByteCode = d3dUtil::CompileShader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");

	mInputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
	};
}

void BoxApp::BuildBoxGeometry()
{

	array<Vertex, 8> vertices =
	{
		Vertex({XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White)}),
		Vertex({XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black)}),
		Vertex({XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red)}),
		Vertex({XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green)}),
		Vertex({XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue)}),
		Vertex({XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow)}),
		Vertex({XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan)}),
		Vertex({XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta)})
	};

	//�ﰢ�� 12���� ��ǥ���� ���� 36��
	array<uint16_t, 36> indices = {
		0, 1, 2,
		0, 2, 3,

		4, 6, 5,
		4, 7, 6,

		4, 5, 1,
		4, 1, 0,

		3, 2, 6,
		3, 6, 7,

		1, 5, 6,
		1, 6, 2,

		4, 0, 3,
		4, 3, 7
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	mBoxGeo = make_unique<MeshGeometry>();
	mBoxGeo->Name = "boxGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &mBoxGeo->VertexBufferCPU));
	CopyMemory(mBoxGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &mBoxGeo->IndexBufferCPU));
	CopyMemory(mBoxGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	mBoxGeo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize, mBoxGeo->VertexBufferUploader);
	mBoxGeo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(md3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize, mBoxGeo->IndexBufferUploader);

	mBoxGeo->VertexByteStride = sizeof(Vertex);
	mBoxGeo->VertexBufferByteSize = vbByteSize;
	mBoxGeo->IndexFormat = DXGI_FORMAT_R16_UINT;
	mBoxGeo->IndexBufferByteSize = ibByteSize;

	//�� ����?
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	mBoxGeo->DrawArgs["box"] = submesh;
}

void BoxApp::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	ZeroMemory(&psoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	psoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	psoDesc.pRootSignature = mRootSignature.Get();
	
	//���ؽ� ���̴�
	psoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mvsByteCode->GetBufferPointer()),
		mvsByteCode->GetBufferSize()
	};
	//�ȼ� ���̴� ����
	psoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mpsByteCode->GetBufferPointer()),
		mpsByteCode->GetBufferSize()
	};
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = mBackBufferFormat;
	psoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	psoDesc.DSVFormat = mDepthStencilFormat;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO)));

}

