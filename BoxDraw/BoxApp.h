#pragma once

#include "../Common/D3dApp.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace std;

struct Vertex
{
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

struct ObjectConstants
{
	XMFLOAT4X4 WorldViewProj = MathHelper::Identity4x4(); //4X4�׵���ķ� ����
};

class BoxApp : public D3DApp
{
public:
	BoxApp(HINSTANCE hInstance);
	BoxApp(const BoxApp& rhs) = delete;
	BoxApp& operator=(const BoxApp& rhs) = delete;
	~BoxApp();

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;
	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y) override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y) override;

	void BuildDescriptorHeap();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShaderAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:
	ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;

	unique_ptr<MeshGeometry> mBoxGeo = nullptr;

	ComPtr<ID3DBlob> mvsByteCode = nullptr;  //Vertex shader�ڵ� blob
	ComPtr<ID3DBlob> mpsByteCode = nullptr;  //Pixel shader�ڵ� blob

	vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout; //�ԷµǴ� �������� �����ϴ� ���̾ƿ� ��ü

	ComPtr<ID3D12PipelineState> mPSO = nullptr;  //���������� ������Ʈ ��ũ����  

	XMFLOAT4X4 mWorld = MathHelper::Identity4x4();
	XMFLOAT4X4 mView = MathHelper::Identity4x4();
	XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 0;  //x��ǥ , 1.5f * XM_PI
	float mPhi = XM_PIDIV4;  //45��   , y
	float mRadius = 5.0f;

	POINT mLastMousePos;
};

