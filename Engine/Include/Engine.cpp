#include "Engine.h"
#include "Logging/Logger.h"

static void EnsureCOM() 
{
    static bool once = [] 
        {
        HRESULT hr = CoInitializeEx(nullptr, COINITBASE_MULTITHREADED);
        if (hr == RPC_E_CHANGED_MODE) return true;
        return SUCCEEDED(hr);
        }();

    (void)once;
}

CEngine::~CEngine()
{
}

bool CEngine::LoadTestTextures()
{
    const wchar_t* kPNG = L"D:\\MapleStory_UnrealSource\\Client\\Bin\\Sprites\\swingD2.png";

    m_DemoTextures.clear();
    m_DemoTextures.reserve(4);

    // PNG (sRGB on)
    {
        DemoTex t{};
        t.Path = kPNG; t.bSRGB = true;
        if (m_TexMgr.LoadTextureFromFile(kPNG, t.SRV, &t.W, &t.H, /*srgb*/true, /*mips*/true))
        {
            m_DemoTextures.push_back(std::move(t));
        }
    }

    // PNG (sRGB off �񱳿�)
    {
        DemoTex t{};
        t.Path = kPNG; t.bSRGB = false;
        if (m_TexMgr.LoadTextureFromFile(kPNG, t.SRV, &t.W, &t.H, /*srgb*/false, /*mips*/true)) 
        {
            m_DemoTextures.push_back(std::move(t));
        }
    }

    // �ƹ� �͵� �� �÷ȴٸ�, üũ����� �ϳ� �ø�
    if (m_DemoTextures.empty()) 
    {
        DemoTex t{};
        t.Path = L"[checker]";
        t.W = t.H = 128; t.bSRGB = true;
        if (m_TexMgr.CreateCheckerTexture(128, 128, 16, 0xFFFFFFFF, 0xFFCCCCCC, t.SRV)) 
        {
            m_DemoTextures.push_back(std::move(t));
        }

        else 
        {
            return false;
        }
    }

    // �α�
    for (const auto& dt : m_DemoTextures) 
    {
        wchar_t buf[512];
        swprintf_s(buf, L"[CEngine] Loaded: %s (sRGB=%d, %ux%u)\n", dt.Path ? dt.Path : L"(null)", dt.bSRGB ? 1 : 0, dt.W, dt.H);
        OutputDebugStringW(buf);
    }

    return true;
}

void CEngine::DrawTestSprites()
{
    const auto& vp = m_Device.GetViewport();
    FInt2 screen = { (int)vp.Width, (int)vp.Height };

    // ��ġ ��ġ: �»�ܺ��� ���η� �þ����, �� �� �Ѿ�� ���� ��
    int x = 32;
    int y = 32;
    const int pad = 24;

    for (size_t i = 0; i < m_DemoTextures.size(); ++i)
    {
        const DemoTex& dt = m_DemoTextures[i];
        if (!dt.SRV) 
        {
            continue;
        }

        // 1) ���� ũ��
        {
            FInt2 pos = { x, y };
            FInt2 size = { (int)dt.W, (int)dt.H };
            float color[4] = { 1, 1, 1, 1 };    // no tint
            float uv[4] = { 0, 0, 1, 1 };
            m_Sprite.Draw(screen, pos, size, dt.SRV.Get(), color, uv);
            x += size.X + pad;
        }

        // 2) ���(���� Ȯ��)
        {
            FInt2 size2 = { (int)dt.W / 3, (int)dt.H / 3 };
            FInt2 pos2 = { x, y + (int)dt.H / 2 - size2.Y / 2 };
            float color2[4] = { 1, 1, 1, 1 };
            float uv2[4] = { 0, 0, 1, 1 };
            m_Sprite.Draw(screen, pos2, size2, dt.SRV.Get(), color2, uv2);
            x += size2.X + pad;
        }

        // 3) UV ũ�� (��� 80%��)
        {
            FInt2 size3 = { (int)dt.W / 2, (int)dt.H / 2 };
            FInt2 pos3 = { x, y + (int)dt.H / 2 - size3.Y / 2 };
            float color3[4] = { 1, 1, 1, 1 };
            float uv3[4] = { 0.1f, 0.1f, 0.9f, 0.9f };
            m_Sprite.Draw(screen, pos3, size3, dt.SRV.Get(), color3, uv3);
            x += size3.X + pad;
        }

        // 4) ���� ƾƮ (���� 70%/���� 70%)
        {
            FInt2 size4 = { (int)dt.W / 2, (int)dt.H / 2 };
            FInt2 pos4 = { x, y + (int)dt.H / 2 - size4.Y / 2 };
            float color4[4] = { 1.0f, 0.3f, 0.3f, 0.7f };
            float uv4[4] = { 0, 0, 1, 1 };
            m_Sprite.Draw(screen, pos4, size4, dt.SRV.Get(), color4, uv4);
            x += size4.X + pad;
        }

        // ���� �ٷ� ����
        x = 32;
        y += (int)dt.H + 2 * pad;
        if (y > (int)vp.Height - 64) 
        {
            y = 32; // ȭ���� ������ ���� ����
        }
    }
}

CEngine& CEngine::Get()
{
    static CEngine Instance;
    return Instance;
}

bool CEngine::Init(HWND hwnd, int width, int height, bool vsync, bool windowed)
{
    if (!m_Device.Init(hwnd, width, height, vsync, windowed))
    {
        return false;
    }

    if (!m_ShaderMgr.Init(m_Device.GetDevice()))
    {
        return false;
    }

    if (!m_TexMgr.Init(m_Device.GetDevice(), m_Device.GetContext()))
    {
        return false;
    }

   if (!m_Sprite.Init(&m_Device, &m_ShaderMgr))
    {
        return false;
    }

   EnsureCOM();

    // ���� �ؽ�ó (üĿ���� or �ܻ�)
    // üĿ: ���� ȸ��/����
   /*if (!LoadTestTextures()) 
   {
       OutputDebugStringW(L"[CEngine] LoadTestTextures failed. Using checker fallback.\n");
   }*/

    return true;
}

void CEngine::Shutdown()
{
    m_DemoTextures.clear();
    m_Sprite.Shutdown();
    m_TexMgr.Shutdown();
    m_ShaderMgr.Shutdown();
    m_Device.Shutdown();
}

void CEngine::Render2D()
{
    const auto vp = m_Device.GetViewport();
    FInt2 screen{ (int)vp.Width, (int)vp.Height };
    m_Sprite.RenderAll(screen);
}

void CEngine::BeginFrame()
{
    m_Renderer.BeginFrame(m_ClearColor);

    DrawTestSprites();
}

void CEngine::EndFrame()
{
    m_Renderer.EndFrame();
}

bool CEngine::Resize(int width, int height)
{
    return m_Device.Resize(width, height);
}