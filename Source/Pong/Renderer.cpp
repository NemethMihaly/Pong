#include <new>
#include <fstream>
#include "3rdParty/json.hpp"
#include "Renderer.h"
#include "Debugging/Logger.h"

using namespace DirectX;

#define RELEASE_COM(x) { if (x != nullptr) { x->Release(); x = nullptr; } }

Renderer::Renderer()
{
    m_pd3dDevice			= nullptr;
    m_pd3dDeviceContext		= nullptr;
    m_pDXGISwapChain		= nullptr;
    m_pRenderTargetView		= nullptr;
    ZeroMemory(&m_viewport, sizeof(D3D11_VIEWPORT));

    m_pVertexLayout			= nullptr;
    m_pVertexShader			= nullptr;
    m_pPixelShader			= nullptr;
    
    m_pTextVertexLayout     = nullptr;
    m_pTextVertexShader     = nullptr;
    m_pTextPixelShader      = nullptr;

    m_pVerts                = nullptr;
    m_pIndices              = nullptr;
    m_numVerts              = 0;
    m_numPolys              = 0;
    m_pVertexBuffer         = nullptr;
    m_pIndexBuffer          = nullptr;

    m_pTextVerts            = nullptr;
    m_pTextIndices          = nullptr;
    m_numTextVerts          = 0;
    m_numTextPolys          = 0;
    m_pTextVertexBuffer     = nullptr;
    m_pTextIndexBuffer      = nullptr;
    m_pFontAtlasTextureRV   = nullptr;
    m_pSamplerLinear        = nullptr;

    m_pcbPerFrame           = nullptr;
    m_pcbPerObject          = nullptr;
}

static HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint,
    LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
    HRESULT hr = S_OK;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    // Set the D3DCOMPILE_DEBUG flag to embed debug information in the shaders.
    // Setting this flag improves the shader debugging experience, but still allows 
    // the shaders to be optimized and to run exactly the way they will run in 
    // the release configuration of this program.
    dwShaderFlags |= D3DCOMPILE_DEBUG;

    // Disable optimizations to further improve shader debugging
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ID3DBlob* pErrorBlob = nullptr;
    hr = D3DCompileFromFile(szFileName, nullptr, nullptr, szEntryPoint, szShaderModel,
        dwShaderFlags, 0, ppBlobOut, &pErrorBlob);
    if (FAILED(hr))
    {
        if (pErrorBlob)
        {
            LOG("CompileShaderFromFile", Error, (const char*)pErrorBlob->GetBufferPointer());
            pErrorBlob->Release();
        }

        return hr;
    }

    if (pErrorBlob)
        pErrorBlob->Release();

    return S_OK;
}

bool Renderer::Initialize(HWND hwnd)
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect(hwnd, &rc);
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    DXGI_SWAP_CHAIN_DESC swapChainDesc;
    ZeroMemory(&swapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
    swapChainDesc.BufferDesc.Width = width;
    swapChainDesc.BufferDesc.Height = height;
    swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 1;
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swapChainDesc.Flags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, nullptr, 0, D3D11_SDK_VERSION,
        &swapChainDesc, &m_pDXGISwapChain, &m_pd3dDevice, &featureLevel, &m_pd3dDeviceContext);
    if (FAILED(hr))
        return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = m_pDXGISwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr))
        return false;

    hr = m_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &m_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr))
        return false;

    m_viewport.TopLeftX = 0.0f;
    m_viewport.TopLeftY = 0.0f;
    m_viewport.Width = (FLOAT)width;
    m_viewport.Height = (FLOAT)height;
    m_viewport.MinDepth = 0.0f;
    m_viewport.MaxDepth = 1.0f;

    {
        ID3DBlob* pVSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/Shader.hlsl", "VSMain", "vs_4_0", &pVSBlob);
        if (FAILED(hr))
            return false;

        D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(inputElementDescs);
        hr = m_pd3dDevice->CreateInputLayout(inputElementDescs, numElements, pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(), &m_pVertexLayout);
        if (FAILED(hr))
            return false;

        hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
            nullptr, &m_pVertexShader);
        pVSBlob->Release();
        if (FAILED(hr))
            return false;

        ID3DBlob* pPSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/Shader.hlsl", "PSMain", "ps_4_0", &pPSBlob);
        if (FAILED(hr))
            return false;
        
        hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
            nullptr, &m_pPixelShader);
        pPSBlob->Release();
        if (FAILED(hr))
            return false;
    }

    {
        ID3DBlob* pVSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/TextShader.hlsl", "VSMain", "vs_4_0", &pVSBlob);
        if (FAILED(hr))
            return false;

        D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        UINT numElements = ARRAYSIZE(inputElementDescs);
        hr = m_pd3dDevice->CreateInputLayout(inputElementDescs, numElements, pVSBlob->GetBufferPointer(),
            pVSBlob->GetBufferSize(), &m_pTextVertexLayout);
        if (FAILED(hr))
            return false;

        hr = m_pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(),
            nullptr, &m_pTextVertexShader);
        pVSBlob->Release();
        if (FAILED(hr))
            return false;

        ID3DBlob* pPSBlob = nullptr;
        hr = CompileShaderFromFile(L"Data/TextShader.hlsl", "PSMain", "ps_4_0", &pPSBlob);
        if (FAILED(hr))
            return false;

        hr = m_pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(),
            nullptr, &m_pTextPixelShader);
        pPSBlob->Release();
        if (FAILED(hr))
            return false;
    }

    {
        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numVerts = 4;
        m_pVerts = new (std::nothrow) Vertex[m_numVerts];
        assert(m_pVerts != nullptr && "Out of memory in GameApp::InitDevice()");

        m_pVerts[0] = { XMFLOAT3( 0.5f, 0.5f, 0.0f) };
        m_pVerts[1] = { XMFLOAT3(-0.5f, 0.5f, 0.0f) };
        m_pVerts[2] = { XMFLOAT3( 0.5f,-0.5f, 0.0f) };
        m_pVerts[3] = { XMFLOAT3(-0.5f,-0.5f, 0.0f) };

        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
        bufferDesc.ByteWidth = sizeof(Vertex) * m_numVerts;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA initialData;
        ZeroMemory(&initialData, sizeof(D3D11_SUBRESOURCE_DATA));
        initialData.pSysMem = m_pVerts;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pVertexBuffer);
        if (FAILED(hr))
            return false;

        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numPolys = 2;
        m_pIndices = new (std::nothrow) WORD[m_numPolys * 3];
        assert(m_pIndices != nullptr && "Out of memory in GameApp::InitDevice()");

        // Triangle #1: ACB
        m_pIndices[0] = 0;
        m_pIndices[1] = 2;
        m_pIndices[2] = 1;

        // Triangle #2: BCD
        m_pIndices[3] = 1;
        m_pIndices[4] = 2;
        m_pIndices[5] = 3;

        bufferDesc.ByteWidth = sizeof(WORD) * m_numPolys * 3;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        initialData.pSysMem = m_pIndices;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pIndexBuffer);
        if (FAILED(hr))
            return false;
    }

    {
        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numTextVerts = 4;
        m_pTextVerts = new (std::nothrow) TextVertex[m_numTextVerts];
        assert(m_pTextVerts != nullptr && "Out of memory in GameApp::InitDevice()");

        //m_pTextVerts[0] = { XMFLOAT3( 0.5f, 0.5f, 0.0f), XMFLOAT2(1.0f, 0.0f) };
        //m_pTextVerts[1] = { XMFLOAT3(-0.5f, 0.5f, 0.0f), XMFLOAT2(0.0f, 0.0f) };
        //m_pTextVerts[2] = { XMFLOAT3( 0.5f,-0.5f, 0.0f), XMFLOAT2(1.0f, 1.0f) };
        //m_pTextVerts[3] = { XMFLOAT3(-0.5f,-0.5f, 0.0f), XMFLOAT2(0.0f, 1.0f) };

        D3D11_BUFFER_DESC bufferDesc;
        ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
        bufferDesc.ByteWidth = sizeof(TextVertex) * m_numTextVerts;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        D3D11_SUBRESOURCE_DATA initialData;
        ZeroMemory(&initialData, sizeof(D3D11_SUBRESOURCE_DATA));
        initialData.pSysMem = m_pTextVerts;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pTextVertexBuffer);
        if (FAILED(hr))
            return false;   
            
        // A --- B
        // |   / |
        // | /   | 
        // C --- D

        m_numTextPolys = 2;
        m_pTextIndices = new (std::nothrow) WORD[m_numTextPolys * 3];
        assert(m_pTextIndices != nullptr && "Out of memory in GameApp::InitDevice()");

        // Triangle #1: ACB
        m_pTextIndices[0] = 0;
        m_pTextIndices[1] = 2;
        m_pTextIndices[2] = 1;

        // Triangle #2: BCD
        m_pTextIndices[3] = 1;
        m_pTextIndices[4] = 2;
        m_pTextIndices[5] = 3;

        bufferDesc.ByteWidth = sizeof(WORD) * m_numTextPolys * 3;
        bufferDesc.Usage = D3D11_USAGE_DEFAULT;
        bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bufferDesc.CPUAccessFlags = 0;
        initialData.pSysMem = m_pTextIndices;
        hr = m_pd3dDevice->CreateBuffer(&bufferDesc, &initialData, &m_pTextIndexBuffer);
        if (FAILED(hr))
            return false;
    }

    hr = CreateDDSTextureFromFile(m_pd3dDevice, L"Data/FontAtlas.dds", nullptr, &m_pFontAtlasTextureRV);
    if (FAILED(hr))
        return false;

    D3D11_SAMPLER_DESC samplerDesc;
    ZeroMemory(&samplerDesc, sizeof(D3D11_SAMPLER_DESC));
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = m_pd3dDevice->CreateSamplerState(&samplerDesc, &m_pSamplerLinear);
    if (FAILED(hr))
        return false;

    D3D11_BUFFER_DESC bufferDesc;
    ZeroMemory(&bufferDesc, sizeof(D3D11_BUFFER_DESC));
    bufferDesc.ByteWidth = sizeof(ConstantBuffer_PerFrame);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, &m_pcbPerFrame);
    if (FAILED(hr))
        return false;

    bufferDesc.ByteWidth = sizeof(ConstantBuffer_PerObject);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr = m_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, &m_pcbPerObject);
    if (FAILED(hr))
        return false;

    return true;
}

void Renderer::Uninitialize()
{
    if (m_pd3dDeviceContext != nullptr)
        m_pd3dDeviceContext->ClearState();

    RELEASE_COM(m_pcbPerObject);
    RELEASE_COM(m_pcbPerFrame);

    if (m_pTextVerts != nullptr)
        delete[] m_pTextVerts;
    if (m_pTextIndices != nullptr)
        delete[] m_pTextIndices;
    RELEASE_COM(m_pSamplerLinear);
    RELEASE_COM(m_pFontAtlasTextureRV);
    RELEASE_COM(m_pTextIndexBuffer);
    RELEASE_COM(m_pTextVertexBuffer);

    if (m_pVerts != nullptr)
        delete[] m_pVerts;
    if (m_pIndices != nullptr)
        delete[] m_pIndices;
    RELEASE_COM(m_pIndexBuffer);
    RELEASE_COM(m_pVertexBuffer);

    RELEASE_COM(m_pTextPixelShader);
    RELEASE_COM(m_pTextVertexShader);
    RELEASE_COM(m_pTextVertexLayout);

    RELEASE_COM(m_pPixelShader);
    RELEASE_COM(m_pVertexShader);
    RELEASE_COM(m_pVertexLayout);

    RELEASE_COM(m_pRenderTargetView);
    RELEASE_COM(m_pDXGISwapChain);
    RELEASE_COM(m_pd3dDeviceContext);
    RELEASE_COM(m_pd3dDevice);
}

bool Renderer::LoadFontMetaData()
{
    std::ifstream fs("Data/FontAtlas-meta.json");
    if (!fs.is_open())
        return false;

    nlohmann::json json = nlohmann::json::parse(fs);
    nlohmann::json atlasData = json["atlas"];
    m_fontAtlas.size = atlasData["size"];
    m_fontAtlas.width = atlasData["width"];
    m_fontAtlas.height = atlasData["height"];

    for (const nlohmann::json& glyphData : json["glyphs"])
    {
        Glyph glyph;
        ZeroMemory(&glyph, sizeof(Glyph));

        glyph.unicode = glyphData["unicode"];
        glyph.advance = glyphData["advance"];

        if (glyphData.contains("planeBounds"))
        {
            nlohmann::json planeBounds = glyphData["planeBounds"];
            glyph.planeLeft = planeBounds["left"];
            glyph.planeBottom = planeBounds["bottom"];
            glyph.planeRight = planeBounds["right"];
            glyph.planeTop = planeBounds["top"];
        }

        if (glyphData.contains("atlasBounds"))
        {
            nlohmann::json atlasBounds = glyphData["atlasBounds"];
            glyph.atlasLeft = atlasBounds["left"];
            glyph.atlasBottom = atlasBounds["bottom"];
            glyph.atlasRight = atlasBounds["right"];
            glyph.atlasTop = atlasBounds["top"];
        }

        m_fontAtlas.glyphs[glyph.unicode] = glyph;
    }

    fs.close();

    return true;
}

void Renderer::PreRender()
{
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

        m_pd3dDeviceContext->Map(m_pcbPerFrame, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

        ConstantBuffer_PerFrame* pPerFrame = (ConstantBuffer_PerFrame*)mappedResource.pData;
        XMStoreFloat4x4(&pPerFrame->view, XMMatrixTranspose(XMMatrixIdentity()));
        XMStoreFloat4x4(&pPerFrame->projection,
            XMMatrixTranspose(
                XMMatrixOrthographicOffCenterLH(
                    0.0f, m_viewport.Width, 0.0f, m_viewport.Height, -1.0f, 1.0f
                    )
                )
            );

        m_pd3dDeviceContext->Unmap(m_pcbPerFrame, 0);
    }

    float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    m_pd3dDeviceContext->ClearRenderTargetView(m_pRenderTargetView, clearColor);

    m_pd3dDeviceContext->RSSetViewports(1, &m_viewport);
    m_pd3dDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, nullptr);
}

void Renderer::PostRender()
{
    m_pDXGISwapChain->Present(0, 0);
}

void Renderer::PrepareQuadPass()
{
    m_pd3dDeviceContext->IASetInputLayout(m_pVertexLayout);

    m_pd3dDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);
    m_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pcbPerFrame);
    m_pd3dDeviceContext->VSSetConstantBuffers(1, 1, &m_pcbPerObject);
    m_pd3dDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    m_pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
    m_pd3dDeviceContext->IASetIndexBuffer(m_pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Renderer::PrepareTextPass()
{
    m_pd3dDeviceContext->IASetInputLayout(m_pTextVertexLayout);

    m_pd3dDeviceContext->VSSetShader(m_pTextVertexShader, nullptr, 0);
    m_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &m_pcbPerFrame);
    m_pd3dDeviceContext->VSSetConstantBuffers(1, 1, &m_pcbPerObject);
    m_pd3dDeviceContext->PSSetShader(m_pTextPixelShader, nullptr, 0);
    m_pd3dDeviceContext->PSSetShaderResources(0, 1, &m_pFontAtlasTextureRV);
    m_pd3dDeviceContext->PSSetSamplers(0, 1, &m_pSamplerLinear);

    UINT stride = sizeof(TextVertex);
    UINT offset = 0;
    m_pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pTextVertexBuffer, &stride, &offset);
    m_pd3dDeviceContext->IASetIndexBuffer(m_pTextIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    m_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void Renderer::RenderQuad(const DirectX::XMFLOAT2& pos, const DirectX::XMFLOAT2& scale)
{
    {
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

        m_pd3dDeviceContext->Map(m_pcbPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

        ConstantBuffer_PerObject* pPerObject = (ConstantBuffer_PerObject*)mappedResource.pData;
        XMStoreFloat4x4(&pPerObject->world, XMMatrixTranspose(
                XMMatrixScalingFromVector(XMLoadFloat2(&scale)) *
                XMMatrixTranslationFromVector(XMLoadFloat2(&pos))
            )
        );

        m_pd3dDeviceContext->Unmap(m_pcbPerObject, 0);
    }
    
    m_pd3dDeviceContext->DrawIndexed(m_numPolys * 3, 0, 0);
}

void Renderer::RenderText(const std::string& str, const DirectX::XMFLOAT2& pos, float size)
{
    float offset = 0.0f;

    for (size_t i = 0; i < str.size(); ++i)
    {
        auto findIt = m_fontAtlas.glyphs.find((uint32_t)str[i]);
        if (findIt == m_fontAtlas.glyphs.end())
            continue; 

        const Glyph& glyph = findIt->second;
        //float size = (float)m_fontAtlas.size;

        float u0 = glyph.atlasLeft / (float)m_fontAtlas.width;
        float u1 = glyph.atlasRight / (float)m_fontAtlas.width;
        float v0 = 1.0f - glyph.atlasTop / (float)m_fontAtlas.height;
        float v1 = 1.0f - glyph.atlasBottom / (float)m_fontAtlas.height;

        m_pTextVerts[0] = { XMFLOAT3(glyph.planeRight * size + offset, glyph.planeTop    * size, 0.0f), XMFLOAT2(u1, v0) };
        m_pTextVerts[1] = { XMFLOAT3(glyph.planeLeft  * size + offset, glyph.planeTop    * size, 0.0f), XMFLOAT2(u0, v0) };
        m_pTextVerts[2] = { XMFLOAT3(glyph.planeRight * size + offset, glyph.planeBottom * size, 0.0f), XMFLOAT2(u1, v1) };
        m_pTextVerts[3] = { XMFLOAT3(glyph.planeLeft  * size + offset, glyph.planeBottom * size, 0.0f), XMFLOAT2(u0, v1) };

        m_pd3dDeviceContext->UpdateSubresource(m_pTextVertexBuffer, 0, nullptr, m_pTextVerts, 0, 0);
        
        offset += glyph.advance * size;

        {
            D3D11_MAPPED_SUBRESOURCE mappedResource;
            ZeroMemory(&mappedResource, sizeof(D3D11_MAPPED_SUBRESOURCE));

            m_pd3dDeviceContext->Map(m_pcbPerObject, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);

            ConstantBuffer_PerObject* pPerObject = (ConstantBuffer_PerObject*)mappedResource.pData;
            XMStoreFloat4x4(&pPerObject->world, XMMatrixTranspose(
                    XMMatrixTranslationFromVector(XMLoadFloat2(&pos))
                )
            );

            m_pd3dDeviceContext->Unmap(m_pcbPerObject, 0);
        }

        m_pd3dDeviceContext->DrawIndexed(m_numTextPolys * 3, 0, 0);
    }
}
