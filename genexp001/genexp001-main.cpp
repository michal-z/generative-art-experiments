#include "genexp001-external.h"

static ID2D1DeviceContext6* s_Graphics;
static ID2D1Device6* s_GraphicsDevice;
static ID2D1Factory7* s_GraphicsFactory;

#include "genexp001.cpp"

static double
GetTime()
{
    static LARGE_INTEGER StartCounter;
    static LARGE_INTEGER Frequency;
    if (StartCounter.QuadPart == 0)
    {
        QueryPerformanceFrequency(&Frequency);
        QueryPerformanceCounter(&StartCounter);
    }
    LARGE_INTEGER Counter;
    QueryPerformanceCounter(&Counter);
    return (Counter.QuadPart - StartCounter.QuadPart) / (double)Frequency.QuadPart;
}

static void
UpdateFrameStats(HWND Window, const char* Name, double& OutTime, float& OutDeltaTime)
{
    static double PreviousTime = -1.0;
    static double HeaderRefreshTime = 0.0;
    static unsigned FrameCount = 0;

    if (PreviousTime < 0.0)
    {
        PreviousTime = GetTime();
        HeaderRefreshTime = PreviousTime;
    }

    OutTime = GetTime();
    OutDeltaTime = (float)(OutTime - PreviousTime);
    PreviousTime = OutTime;

    if ((OutTime - HeaderRefreshTime) >= 1.0)
    {
        const double FramesPerSecond = FrameCount / (OutTime - HeaderRefreshTime);
        const double MilliSeconds = (1.0 / FramesPerSecond) * 1000.0;
        char Header[256];
        snprintf(Header, sizeof(Header), "[%.1f fps  %.3f ms] %s", FramesPerSecond, MilliSeconds, Name);
        SetWindowText(Window, Header);
        HeaderRefreshTime = OutTime;
        FrameCount = 0;
    }
    FrameCount++;
}

static LRESULT CALLBACK
ProcessWindowMessage(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYDOWN:
        if (wparam == VK_ESCAPE)
        {
            PostQuitMessage(0);
            return 0;
        }
        break;
    }
    return DefWindowProc(window, message, wparam, lparam);
}

static HWND
InitializeWindow(const char* Name, unsigned Width, unsigned Height)
{
    WNDCLASS WinClass = {};
    WinClass.lpfnWndProc = ProcessWindowMessage;
    WinClass.hInstance = GetModuleHandle(nullptr);
    WinClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    WinClass.lpszClassName = Name;
    if (!RegisterClass(&WinClass))
        assert(0);

    RECT Rect = { 0, 0, (LONG)Width, (LONG)Height };
    if (!AdjustWindowRect(&Rect, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX, 0))
        assert(0);

    HWND Window = CreateWindowEx(
        0, Name, Name, WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        Rect.right - Rect.left, Rect.bottom - Rect.top,
        nullptr, nullptr, nullptr, 0);
    assert(Window);
    return Window;
}

static IDXGISwapChain1*
InitializeGraphics(HWND Window)
{
    D2D1_FACTORY_OPTIONS FactoryOptions = {};
#ifdef _DEBUG
    FactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
#else
    FactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_NONE;
#endif
    VHR(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(s_GraphicsFactory), &FactoryOptions,
                          (void**)&s_GraphicsFactory));

    ID3D11Device* Direct3D11Device;
    ID3D11DeviceContext* Direct3D11Context;
    D3D_FEATURE_LEVEL FeatureLevel = D3D_FEATURE_LEVEL_11_1;
    VHR(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                          &FeatureLevel, 1, D3D11_SDK_VERSION, &Direct3D11Device, nullptr, &Direct3D11Context));

    IDXGIDevice* DxgiDevice;
    VHR(Direct3D11Device->QueryInterface(&DxgiDevice));

    VHR(s_GraphicsFactory->CreateDevice(DxgiDevice, &s_GraphicsDevice));
    VHR(s_GraphicsDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &s_Graphics));

    DXGI_SWAP_CHAIN_DESC1 SwapChainDesc = {};
    SwapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    SwapChainDesc.SampleDesc.Count = 1;
    SwapChainDesc.SampleDesc.Quality = 0;
    SwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    SwapChainDesc.BufferCount = 2;
    SwapChainDesc.Scaling = DXGI_SCALING_NONE;
    SwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

    IDXGIAdapter* DxgiAdapter;
    IDXGIFactory2* DxgiFactory;
    VHR(DxgiDevice->GetAdapter(&DxgiAdapter));
    VHR(DxgiAdapter->GetParent(IID_PPV_ARGS(&DxgiFactory)));

    IDXGISwapChain1* SwapChain;
    VHR(DxgiFactory->CreateSwapChainForHwnd(Direct3D11Device, Window, &SwapChainDesc, nullptr, nullptr, &SwapChain));

    D2D1_BITMAP_PROPERTIES1 BitmapProps = BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 96.0f, 96.0f);

    IDXGISurface* DxgiBackBuffer;
    VHR(SwapChain->GetBuffer(0, IID_PPV_ARGS(&DxgiBackBuffer)));

    ID2D1Bitmap1* TargetBitmap;
    VHR(s_Graphics->CreateBitmapFromDxgiSurface(DxgiBackBuffer, &BitmapProps, &TargetBitmap));
    s_Graphics->SetTarget(TargetBitmap);

    CRELEASE(TargetBitmap);
    CRELEASE(Direct3D11Device);
    CRELEASE(Direct3D11Context);
    CRELEASE(DxgiBackBuffer);
    CRELEASE(DxgiDevice);
    CRELEASE(DxgiAdapter);
    CRELEASE(DxgiFactory);

    return SwapChain;
}

int CALLBACK
WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    const char* k_Name = "genexp001";

    HWND Window = InitializeWindow(k_Name, k_ResolutionX, k_ResolutionY);
    IDXGISwapChain1* SwapChain = InitializeGraphics(Window);

    Initialize();

    for (;;)
    {
        MSG Message = {};
        if (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
        {
            DispatchMessage(&Message);
            if (Message.message == WM_QUIT)
                break;
        }
        else
        {
            double Time;
            float DeltaTime;
            UpdateFrameStats(Window, k_Name, Time, DeltaTime);
            Update();
            VHR(SwapChain->Present(0, 0));
        }
    }

    Shutdown();

    ID2D1Image* RenderTarget;
    s_Graphics->GetTarget(&RenderTarget);

    CRELEASE(RenderTarget);
    CRELEASE(s_GraphicsFactory);
    CRELEASE(s_Graphics);
    CRELEASE(s_GraphicsDevice);
    CRELEASE(SwapChain);

    return 0;
}
// vim: set ts=4 sw=4 expandtab:
