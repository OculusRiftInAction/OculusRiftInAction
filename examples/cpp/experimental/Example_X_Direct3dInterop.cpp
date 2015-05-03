#include "Common.h"

#ifdef OVR_OS_WIN32

#define OVR_D3D_VERSION 11
#include <OVR_CAPI_D3D.h>
#include <GL/wglew.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>

HINSTANCE           hInstance = NULL;

//Anything including this file, uses these

class Texture {
public:
    ID3D11Texture2D*            Tex;
    ID3D11ShaderResourceView*   TexSv;
    int                         Width, Height;

    Texture(int w, int h)
        : Tex(NULL), TexSv(NULL), Width(w), Height(h) {
    }

    virtual ~Texture() {}
};

class RiftStubApp {
protected:
    ovrHmd hmd;

    glm::uvec2 hmdNativeResolution;
    glm::ivec2 hmdDesktopPosition;

public:
    RiftStubApp(ovrHmdType defaultHmdType = ovrHmd_DK1) {
        ovr_Initialize();
        hmd = ovrHmd_Create(0);
        if (NULL == hmd) {
            hmd = ovrHmd_CreateDebug(defaultHmdType);
        }
        hmdNativeResolution = glm::ivec2(hmd->Resolution.w, hmd->Resolution.h);
        hmdDesktopPosition = glm::ivec2(hmd->WindowsPos.x, hmd->WindowsPos.y);
    }

    virtual ~RiftStubApp() {
        ovrHmd_Destroy(hmd);
        hmd = nullptr;
    }
};

template <typename T, typename F>
void populateSharedPointer(std::shared_ptr<T> & ptr, F lambda) {
    T * rawPtr;
    lambda(rawPtr);
    ptr = std::shared_ptr<T>(rawPtr);
}

class DxStubWindow {
protected:
    const uint32_t          WindowWidth, WindowHeight;
    IDXGIFactory*           DXGIFactory{ NULL };
    HWND                    Window{ NULL };
    HWND                    GlWindow;
    ID3D11Device*           Device{ NULL };
    ID3D11DeviceContext*    Context{ NULL };
    IDXGISwapChain*         SwapChain{ NULL };
    ID3D11Texture2D*        BackBuffer;
    ID3D11RenderTargetView* BackBufferRT;
    D3D11_VIEWPORT              D3DViewport;

    static LRESULT CALLBACK systemWindowProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) {
        DxStubWindow * pParent = (DxStubWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        switch (msg) {
        case WM_CREATE:
            pParent = (DxStubWindow*)((LPCREATESTRUCT)lp)->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pParent);
            break;

        case WM_SETFOCUS:
            SetFocus(pParent->GlWindow);
            break;
        }

        return DefWindowProc(hWnd, msg, wp, lp);
    }

public:
    DxStubWindow(HWND glWindow, uint32_t w, uint32_t h) : WindowHeight(h), WindowWidth(w), GlWindow(glWindow) {
        // Register window class
        {
            WNDCLASS wc;
            memset(&wc, 0, sizeof(wc));
            wc.lpszClassName = L"OVRAppWindow";
            wc.style = CS_OWNDC;
            wc.lpfnWndProc = systemWindowProc;
            wc.cbWndExtra = NULL;
            ATOM windowClass = RegisterClass(&wc);
            if (!windowClass) {
                FAIL("Unable to register window class");
            }
        }

        // Create the D3D window (can't be invisible sadly)
        {
            DWORD wsStyle = WS_POPUP | WS_SYSMENU;
            DWORD sizeDivisor = 2;
            RECT winSize = { 0, 0, w / 64, h / 64 };
            AdjustWindowRect(&winSize, wsStyle, false);
            Window = CreateWindowExA(WS_EX_TOOLWINDOW, "OVRAppWindow", "OculusRoomTiny",
                wsStyle, 0, 0, winSize.right - winSize.left, winSize.bottom - winSize.top,
                NULL, NULL, hInstance, this);

            if (!Window) {
                FAIL("Unable to create DX window");
            }
        }

        HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&DXGIFactory));
        if (FAILED(hr)) {
            FAIL("Unable to create DXGI Factory");
        }
        IDXGIAdapter*           Adapter{ NULL };
        DXGIFactory->EnumAdapters(0, &Adapter);

        int flags = 0;
        //int flags =  D3D11_CREATE_DEVICE_DEBUG;    
        hr = D3D11CreateDevice(Adapter, Adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
            NULL, flags, NULL, 0, D3D11_SDK_VERSION,
            &Device, NULL, &Context);
        if (FAILED(hr)) {
            FAIL("Unable to create D3D11 Device & Context");
        }
        Adapter->Release();
        if (!RecreateSwapChain()) {
            FAIL("Unable to create swap chain");
        }
    }

    ~DxStubWindow() {
        if (SwapChain) {
            SwapChain->SetFullscreenState(false, NULL);
        }
    }


    bool RecreateSwapChain() {
        DXGI_SWAP_CHAIN_DESC scDesc;
        memset(&scDesc, 0, sizeof(scDesc));
        scDesc.BufferCount = 2;
        scDesc.BufferDesc.Width = WindowWidth;
        scDesc.BufferDesc.Height = WindowHeight;
        scDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scDesc.BufferDesc.RefreshRate.Numerator = 0;
        scDesc.BufferDesc.RefreshRate.Denominator = 1;
        scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scDesc.OutputWindow = Window;
        scDesc.SampleDesc.Count = 1;
        scDesc.SampleDesc.Quality = 0;
        scDesc.Windowed = FALSE;
        scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        scDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

        if (SwapChain) {
            SwapChain->SetFullscreenState(FALSE, NULL);
            SwapChain->Release();
            SwapChain = NULL;
        }

        IDXGISwapChain* newSC;
        if (FAILED(DXGIFactory->CreateSwapChain(Device, &scDesc, &newSC)))
            return false;
        SwapChain = newSC;

        BackBuffer = NULL;
        BackBufferRT = NULL;
        HRESULT hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer);
        if (FAILED(hr))
            return false;

        hr = Device->CreateRenderTargetView(BackBuffer, NULL, &BackBufferRT);
        if (FAILED(hr))
            return false;
        return true;
    }

    Texture* CreateTexture(int width, int height) {
        Texture* NewTex = new Texture(width, height);
        D3D11_TEXTURE2D_DESC dsDesc;
        dsDesc.Width = width;
        dsDesc.Height = height;
        dsDesc.MipLevels = 1;
        dsDesc.ArraySize = 1;
        dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        dsDesc.SampleDesc.Count = 1;
        dsDesc.SampleDesc.Quality = 0;
        dsDesc.Usage = D3D11_USAGE_DEFAULT;
        dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        dsDesc.CPUAccessFlags = 0;
        dsDesc.MiscFlags = 0;

        HRESULT hr = Device->CreateTexture2D(&dsDesc, NULL, &NewTex->Tex);
        if (FAILED(hr)) {
            //      NewTex->Release();
            return NULL;
        }
        Device->CreateShaderResourceView(NewTex->Tex, NULL, &NewTex->TexSv);
        return NewTex;
    }
};

class GlStubWindow : public GlfwApp {
public:
    GlStubWindow() {
        uvec2 size; ivec2 position;
        preCreate();
        window = createRenderingTarget(size, position);
        if (!getWindow()) {
            FAIL("Unable to create OpenGL window");
        }
        postCreate();
    }

    ~GlStubWindow() {
    }

    virtual GLFWwindow * createRenderingTarget(glm::uvec2 & outSize, glm::ivec2 & outPosition) {
        outSize = uvec2(320, 180);
        outPosition = ivec2(100, 100);
        return glfw::createWindow(outSize, outPosition);
    }
};



struct EyeArgs {
    glm::mat4                 projection;
    FramebufferWrapperPtr     framebuffer;
};

class HelloRift : public GlStubWindow, RiftStubApp, DxStubWindow {
protected:
    EyeArgs           perEyeArgs[2];
    ovrTexture        textures[2];
    ovrVector3f       hmdToEyeOffsets[2];
    float             eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
    float             ipd{ OVR_DEFAULT_IPD };
    glm::mat4         player;
    HANDLE            gl_handleD3D;
    HANDLE            gl_handles[2];
    Texture *         pTextures[2];

public:
    static void GlfwMoveCallback(GLFWwindow* window, int x, int y) {
        HelloRift * instance = (HelloRift *)glfwGetWindowUserPointer(window);
        instance->onWindowMove(x, y);
    }


    HelloRift() : DxStubWindow(glfwGetWin32Window(window), hmd->Resolution.w, hmd->Resolution.h) {
        gl_handleD3D = wglDXOpenDeviceNV(Device);
        glfwSetWindowPosCallback(window, GlfwMoveCallback);
        ovrHmd_AttachToWindow(hmd, Window, nullptr, nullptr);

        for_each_eye([&](ovrEyeType eye) {
            EyeArgs & eyeArgs = perEyeArgs[eye];
            memset(textures + eye, 0, sizeof(ovrTexture));

            ovrTextureHeader & eyeTextureHeader = textures[eye].Header;
            eyeTextureHeader.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmd->DefaultEyeFov[eye], 1.0f);
            eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
            eyeTextureHeader.RenderViewport.Pos.x = 0;
            eyeTextureHeader.RenderViewport.Pos.y = 0;
            eyeTextureHeader.API = ovrRenderAPI_D3D11;
            ovrD3D11Texture & d3dTexture = (ovrD3D11Texture&)textures[eye];
            pTextures[eye] = CreateTexture(eyeTextureHeader.TextureSize.w, eyeTextureHeader.TextureSize.h);
            d3dTexture.D3D11.pTexture = pTextures[eye]->Tex;
            d3dTexture.D3D11.pSRView = pTextures[eye]->TexSv;
            eyeArgs.framebuffer = FramebufferWrapperPtr(new FramebufferWrapper());
            eyeArgs.framebuffer->size = ovr::toGlm(eyeTextureHeader.TextureSize);
            ID3D11Texture2D * texHandle = d3dTexture.D3D11.pTexture;
            gl_handles[eye] = wglDXRegisterObjectNV(gl_handleD3D, texHandle, oglplus::GetName(eyeArgs.framebuffer->color),
                GL_TEXTURE_2D,
                WGL_ACCESS_WRITE_DISCARD_NV);
        });
        BOOL success = wglDXLockObjectsNV(gl_handleD3D, 2, gl_handles);
        if (!success) {
            FAIL("Could not lock objects");
        }
        for_each_eye([&](ovrEyeType eye) {
            FramebufferWrapper & fw = *perEyeArgs[eye].framebuffer;
            fw.initDepth();
            fw.initDone();
        });
        success = wglDXUnlockObjectsNV(gl_handleD3D, 2, gl_handles);
        if (!success) {
            FAIL("Could not unlock objects");
        }
        ovrD3D11Config cfg;
        memset(&cfg, 0, sizeof(ovrGLConfig));
        cfg.D3D11.Header.API = ovrRenderAPI_D3D11;
        cfg.D3D11.Header.BackBufferSize = hmd->Resolution;
        cfg.D3D11.Header.Multisample = 1;
        cfg.D3D11.pDevice = Device;
        cfg.D3D11.pSwapChain = SwapChain;
        cfg.D3D11.pDeviceContext = Context;
        cfg.D3D11.pBackBufferRT = BackBufferRT;

        int distortionCaps =
            ovrDistortionCap_TimeWarp |
            ovrDistortionCap_Vignette |
            ovrDistortionCap_Overdrive;

        ovrEyeRenderDesc              eyeRenderDescs[2];
        int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
            distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);

        for_each_eye([&](ovrEyeType eye) {
            EyeArgs & eyeArgs = perEyeArgs[eye];
            eyeArgs.projection = ovr::toGlm(
                ovrMatrix4f_Projection(hmd->DefaultEyeFov[eye], 0.01, 100,
                ovrProjection_ClipRangeOpenGL | ovrProjection_RightHanded));
            hmdToEyeOffsets[eye] = eyeRenderDescs[eye].HmdToEyeViewOffset;
        });

        ovrHmd_ConfigureTracking(hmd,
            ovrTrackingCap_Orientation |
            ovrTrackingCap_Position, 0);
        resetPosition();
        glCullFace(GL_FRONT);
    }

    ~HelloRift() {
        ovrHmd_Destroy(hmd);
        hmd = 0;
    }

    void onWindowMove(int x, int y) {
        SetWindowPos(Window, HWND_BOTTOM, x + 10, y + 10, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_ASYNCWINDOWPOS);
    }

    virtual void resetPosition() {
        static const glm::vec3 EYE = glm::vec3(0, eyeHeight, ipd * 5);
        static const glm::vec3 LOOKAT = glm::vec3(0, eyeHeight, 0);
        player = glm::inverse(glm::lookAt(EYE, LOOKAT, Vectors::UP));
        ovrHmd_RecenterPose(hmd);
    }


    void onKey(int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            static ovrHSWDisplayState hswDisplayState;
            ovrHmd_GetHSWDisplayState(hmd, &hswDisplayState);
            if (hswDisplayState.Displayed) {
                ovrHmd_DismissHSWDisplay(hmd);
                return;
            }
        }

        if (CameraControl::instance().onKey(key, scancode, action, mods)) {
            return;
        }

        if (GLFW_PRESS != action) {
            GlfwApp::onKey(key, scancode, action, mods);
            return;
        }

        int caps = ovrHmd_GetEnabledCaps(hmd);
        switch (key) {
        case GLFW_KEY_V:
            if (caps & ovrHmdCap_NoVSync) {
                ovrHmd_SetEnabledCaps(hmd, caps & ~ovrHmdCap_NoVSync);
            } else {
                ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_NoVSync);
            }
            break;

        case GLFW_KEY_P:
            if (caps & ovrHmdCap_LowPersistence) {
                ovrHmd_SetEnabledCaps(hmd, caps & ~ovrHmdCap_LowPersistence);
            } else {
                ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_LowPersistence);
            }
            break;

        case GLFW_KEY_R:
            resetPosition();
            break;

        case GLFW_KEY_H:
            if (IsWindowVisible(Window)) {
                ShowWindow(Window, SW_HIDE);
            } else {
                ShowWindow(Window, SW_SHOW);
            }
            break;

        default:
            GlfwApp::onKey(key, scancode, action, mods);
            break;
        }
    }

    virtual void update() {
        CameraControl::instance().applyInteraction(player);
        Stacks::modelview().top() = glm::inverse(player);
    }


    void draw() {
        static int frameIndex = 0;
        static ovrPosef eyePoses[2];
        ++frameIndex;
        MatrixStack & mv = Stacks::modelview();
        ovrHmd_GetEyePoses(hmd, frameIndex, hmdToEyeOffsets, eyePoses, nullptr);
        ovrHmd_BeginFrame(hmd, frameIndex);
        glEnable(GL_DEPTH_TEST);
        if (!wglDXLockObjectsNV(gl_handleD3D, 2, gl_handles)) {
            FAIL("Could not lock objects");
        }
        for (int i = 0; i < 2; ++i) {
            ovrEyeType eye = hmd->EyeRenderOrder[i];
            EyeArgs & eyeArgs = perEyeArgs[eye];
            Stacks::projection().top() = eyeArgs.projection;
            Stacks::projection().scale(glm::vec3(1, -1, 1));

            eyeArgs.framebuffer->Bound([&] {
                mv.withPush([&] {
                    // Apply the per-eye offset & the head pose
                    mv.preMultiply(glm::inverse(ovr::toGlm(eyePoses[eye])));
                    renderScene();
                });
            });
        };
        oglplus::DefaultFramebuffer().Bind(oglplus::Framebuffer::Target::Draw);
        if (!wglDXUnlockObjectsNV(gl_handleD3D, 2, gl_handles)) {
            FAIL("Could not unlock objects");
        }
        ovrHmd_EndFrame(hmd, eyePoses, textures);
    }

    virtual void renderScene() {
        glClearColor(0, 1, 1, 1);
        glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

        oria::renderManikinScene(ipd, eyeHeight);
    }

    int run() {
        MSG msg;
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            } else {
                update();
                draw();
            }
        }
        return 0;
    }
};

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int) {
    hInstance = hinst;
    HelloRift().run();
}

#else

class DoNothing() {
public:
    int run() {
        FAIL("Windows only");
    }
}

RUN_APP(DoNothing);

#endif