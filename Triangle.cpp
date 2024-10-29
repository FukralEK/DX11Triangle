#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")

IDXGISwapChain* swapChain;
ID3D11Device* device;
ID3D11DeviceContext* deviceContext;
ID3D11RenderTargetView* renderTargetView;
ID3D11Buffer* vertexBuffer;
ID3D11InputLayout* inputLayout;
ID3D11VertexShader* vertexShader;
ID3D11PixelShader* pixelShader;

struct Vertex {
	float x, y, z;
	float r, g, b;
};

const char* vertexShaderSource = R"(
    struct VS_INPUT {
        float3 position : POSITION;
        float3 color : COLOR;
    };
    struct PS_INPUT {
        float4 position : SV_POSITION;
        float3 color : COLOR;
    };
    PS_INPUT main(VS_INPUT input) {
        PS_INPUT output;
        output.position = float4(input.position, 1.0);
        output.color = input.color;
        return output;
    }
)";
const char* pixelShaderSource = R"(
    struct PS_INPUT {
        float4 position : SV_POSITION;
        float3 color : COLOR;
    };
    float4 main(PS_INPUT input) : SV_TARGET {
        return float4(1.0, 0.0, 1.0, 1.0);
    }
)";

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hprev, LPSTR cmdLine, int cmdShow)
{
	WNDCLASSEX wc = {};
	wc.lpfnWndProc = WindowProc;
	wc.hInstance = hi;
	wc.lpszClassName = L"TheWindowsWindow";
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.cbSize = sizeof(WNDCLASSEX);
	RegisterClassEx(&wc);

	HWND hWnd = CreateWindowEx(0, L"TheWindowsWindow", L"DX11 Triangle", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720, NULL, NULL, hi, NULL);
	ShowWindow(hWnd, cmdShow);

	DXGI_SWAP_CHAIN_DESC scd = {};
	scd.BufferCount = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = hWnd;
	scd.SampleDesc.Count = 1;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0, NULL, 0, D3D11_SDK_VERSION, &scd, &swapChain, &device, NULL, &deviceContext);

	ID3D11Texture2D* backBuffer = nullptr;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
	device->CreateRenderTargetView(backBuffer, NULL, &renderTargetView);
	backBuffer->Release();

	deviceContext->OMSetRenderTargets(1, &renderTargetView, NULL);

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = 1280;
	viewport.Height = 720;
	deviceContext->RSSetViewports(1, &viewport);

	ID3DBlob* vsBlob;
	ID3DBlob* psBlob;

	D3DCompile(vertexShaderSource, strlen(vertexShaderSource), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &vsBlob, NULL);
	D3DCompile(pixelShaderSource, strlen(pixelShaderSource), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &psBlob, NULL);

	device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vertexShader);
	device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pixelShader);

	deviceContext->VSSetShader(vertexShader, 0, 0);
	deviceContext->PSSetShader(pixelShader, 0, 0);

	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{
			"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,0 , D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{
			"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
			D3D11_INPUT_PER_VERTEX_DATA, 0
		}
	};

	device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
	deviceContext->IASetInputLayout(inputLayout);

	vsBlob->Release();
	psBlob->Release();

	Vertex vertices[] = {
		{ 0.0f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f }, // Top (Red)
		{ 0.45f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f }, // Right (Green)
		{ -0.45f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f }  // Left (Blue)
	};

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(vertices);
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertices;

	device->CreateBuffer(&bufferDesc, &initData, &vertexBuffer);

	MSG msg = {};
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		float bgColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
		deviceContext->ClearRenderTargetView(renderTargetView, bgColor);

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
		deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		deviceContext->Draw(3, 0);

		swapChain->Present(1, 0);
	}

	swapChain->Release();
	device->Release();
	deviceContext->Release();
	renderTargetView->Release();
	vertexBuffer->Release();
	inputLayout->Release();
	vertexShader->Release();
	pixelShader->Release();

	return msg.wParam;
}