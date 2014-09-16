#include "Common.h"
#ifdef OVR_OS_WIN32

#define OVR_D3D_VERSION 11
#include <../../../LibOVR/Src/Kernel/OVR_Math.h>
#include <../../../LibOVR/Src/Kernel/OVR_Array.h>
#include <../../../LibOVR/Src/Kernel/OVR_String.h>
#include <../../../LibOVR/Src/Kernel/OVR_Color.h>
#include <../../../LibOVR/Src/Kernel/OVR_Log.h>
#include <d3d11.h>
#include <OVR_CAPI_D3D.h>
#include <GL/wglew.h>
#undef new



class GlStubWindow : public GlfwApp {
public:
  GlStubWindow() {
    createRenderingTarget();
    if (!window) {
      FAIL("Unable to create OpenGL window");
    }
  }

  ~GlStubWindow() {
    glfwDestroyWindow(window);
  }

  virtual void createRenderingTarget() {
    glfwWindowHint(GLFW_VISIBLE, 0);
    createWindow(glm::uvec2(320, 180), glm::ivec2(100, 100));
  }

  //virtual void finishFrame() {
  //  /*
  //  * The parent class calls glfwSwapBuffers in finishFrame,
  //  * but with the Oculus SDK, the SDK it responsible for buffer
  //  * swapping, so we have to override the method and ensure it
  //  * does nothing, otherwise the dual buffer swaps will
  //  * cause a constant flickering of the display.
  //  */
  //}
};



namespace OVR {
  namespace RenderTiny {
    class RenderDevice;

    enum MapFlags
    {
      Map_Discard = 1,
      Map_Read = 2, // do not use
      Map_Unsynchronized = 4, // like D3D11_MAP_NO_OVERWRITE
    };

    enum TextureFormat
    {
      Texture_RGBA = 0x0100,
      Texture_Depth = 0x8000,
      Texture_TypeMask = 0xff00,
      Texture_SamplesMask = 0x00ff,
      Texture_RenderTarget = 0x10000,
      Texture_GenMipmaps = 0x20000,
    };

    // Texture sampling modes.
    enum SampleMode
    {
      Sample_Linear = 0,
      Sample_Nearest = 1,
      Sample_Anisotropic = 2,
      Sample_FilterMask = 3,

      Sample_Repeat = 0,
      Sample_Clamp = 4,
      Sample_ClampBorder = 8, // If unsupported Clamp is used instead.
      Sample_AddressMask = 12,

      Sample_Count = 13,
    };

    class Texture : public RefCountBase<Texture>
    {
    public:
      RenderDevice*                   Ren;
      Ptr<ID3D11Texture2D>            Tex;
      Ptr<ID3D11ShaderResourceView>   TexSv;
      Ptr<ID3D11RenderTargetView>     TexRtv;
      Ptr<ID3D11DepthStencilView>     TexDsv;
      mutable Ptr<ID3D11SamplerState> Sampler;
      int                             Width, Height;
      int                             Samples;

      Texture(RenderDevice* r, int fmt, int w, int h);
      virtual ~Texture();

      virtual int GetWidth() const    { return Width; }
      virtual int GetHeight() const   { return Height; }
      virtual int GetSamples() const  { return Samples; }

      virtual void SetSampleMode(int sm);

      // Updates texture to point to specified resources
      //  - used for slave rendering.
      void UpdatePlaceholderTexture(ID3D11Texture2D* texture, ID3D11ShaderResourceView* psrv)
      {
        Tex = texture;
        TexSv = psrv;
        TexRtv.Clear();
        TexDsv.Clear();

        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);
        Width = desc.Width;
        Height = desc.Height;
      }
    };

    //-----------------------------------------------------------------------------------

    enum DisplayMode
    {
      Display_Window = 0,
      Display_Fullscreen = 1
    };


    // Rendering parameters used by RenderDevice::CreateDevice.
    struct RendererParams
    {
      int     Multisample;
      int     Fullscreen;
      // Resolution of the rendering buffer used during creation.
      // Allows buffer of different size then the widow if not zero.
      Sizei   Resolution;

      // Windows - Monitor name for fullscreen mode.
      String  MonitorName;
      // MacOS
      long    DisplayId;

      RendererParams(int ms = 1) : Multisample(ms), Fullscreen(0), Resolution(0) {}

      bool IsDisplaySet() const
      {
        return MonitorName.GetLength() || DisplayId;
      }
    };

    class RenderDevice : public RefCountBase<RenderDevice>
    {


    protected:
      int             WindowWidth, WindowHeight;
      RendererParams  Params;
      Matrix4f        Proj;

    public:
      enum CompareFunc
      {
        Compare_Always = 0,
        Compare_Less = 1,
        Compare_Greater = 2,
        Compare_Count
      };

      Ptr<IDXGIFactory>           DXGIFactory;
      HWND                        Window;

      Ptr<ID3D11Device>           Device;
      Ptr<ID3D11DeviceContext>    Context;
      Ptr<IDXGISwapChain>         SwapChain;
      Ptr<IDXGIAdapter>           Adapter;
      Ptr<IDXGIOutput>            FullscreenOutput;
      int                         FSDesktopX, FSDesktopY;
      Ptr<ID3D11Texture2D>        BackBuffer;
      Ptr<ID3D11RenderTargetView> BackBufferRT;
      Ptr<Texture>                CurRenderTarget;
      Ptr<Texture>                CurDepthBuffer;
      Ptr<ID3D11RasterizerState>  Rasterizer;
      Ptr<ID3D11BlendState>       BlendState;
      D3D11_VIEWPORT              D3DViewport;

      Ptr<ID3D11DepthStencilState> DepthStates[1 + 2 * Compare_Count];
      Ptr<ID3D11DepthStencilState> CurDepthState;
      Ptr<ID3D11InputLayout>      ModelVertexIL;

      Ptr<ID3D11SamplerState>     SamplerStates[Sample_Count];
      Array<Ptr<Texture> >        DepthBuffers;

    public:

      RenderDevice(const RendererParams& p, HWND window);
      virtual ~RenderDevice();

      // Implement static initializer function to create this class.
      // Creates a new rendering device
      static RenderDevice* CreateDevice(const RendererParams& rp, void* oswnd);

      // Constructor helper
      void  initShadersAndStates();
      void        UpdateMonitorOutputs();

      void         SetViewport(int x, int y, int w, int h) { SetViewport(Recti(x, y, w, h)); }
      // Set viewport ignoring any adjustments used for the stereo mode.
      virtual void SetViewport(const Recti& vp);
      virtual void SetFullViewport();

      virtual bool SetParams(const RendererParams& newParams);
      const RendererParams& GetParams() const { return Params; }

      virtual void Present(bool vsyncEnabled);

      // Waits for rendering to complete; important for reducing latency.
      virtual void WaitUntilGpuIdle();

      // Don't call these directly, use App/Platform instead
      virtual bool SetFullscreen(DisplayMode fullscreen);

      virtual void Clear(float r = 0, float g = 0, float b = 0, float a = 1, float depth = 1);

      // Resources
      virtual Texture* CreateTexture(int format, int width, int height, const void* data, int mipcount = 1);

      // Placeholder texture to come in externally
      virtual Texture* CreatePlaceholderTexture(int format);

      Texture* GetDepthBuffer(int w, int h, int ms);

      // Begin drawing directly to the currently selected render target, no post-processing.
      virtual void BeginRendering();

      // Begin drawing the primary scene, starting up whatever post-processing may be needed.
      virtual void BeginScene();
      virtual void FinishScene();

      // Texture must have been created with Texture_RenderTarget. Use NULL for the default render target.
      // NULL depth buffer means use an internal, temporary one.
      virtual void SetRenderTarget(Texture* color,
        Texture* depth = NULL,
        Texture* stencil = NULL);
      void SetDefaultRenderTarget() { SetRenderTarget(NULL, NULL); }
      virtual void SetDepthMode(bool enable, bool write, CompareFunc func = Compare_Less);

      virtual Matrix4f GetProjection() const { return Proj; }
      bool                RecreateSwapChain();
      ID3D11SamplerState* GetSamplerState(int sm);
    };

    int GetNumMipLevels(int w, int h);

    // Filter an rgba image with a 2x2 box filter, for mipmaps.
    // Image size must be a power of 2.
    void FilterRgba2x2(const uint8_t* src, int w, int h, uint8_t* dest);
  }
}

//Anything including this file, uses these
using namespace OVR;
using namespace OVR::RenderTiny;


#include <d3dcompiler.h>

namespace OVR {
  namespace RenderTiny {

    //-------------------------------------------------------------------------------------
    // ***** Texture
    // 
    Texture::Texture(RenderDevice* ren, int fmt, int w, int h)
      : Ren(ren), Tex(NULL), TexSv(NULL), TexRtv(NULL), TexDsv(NULL), Width(w), Height(h)
    {
      OVR_UNUSED(fmt);
      Sampler = Ren->GetSamplerState(0);
    }

    Texture::~Texture()
    {
    }

    void Texture::SetSampleMode(int sm)
    {
      Sampler = Ren->GetSamplerState(sm);
    }

    //-------------------------------------------------------------------------------------
    // ***** Render Device

    RenderDevice::RenderDevice(const RendererParams& p, HWND window)
    {
      RECT rc;
      if (p.Resolution == Sizei(0))
      {
        GetClientRect(window, &rc);
        WindowWidth = rc.right - rc.left;
        WindowHeight = rc.bottom - rc.top;
      }
      else
      {
        // TBD: Rename from WindowHeight or use Resolution from params for surface
        WindowWidth = p.Resolution.w;
        WindowHeight = p.Resolution.h;
      }

      Window = window;
      Params = p;

      DXGIFactory = NULL;
      HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&DXGIFactory.GetRawRef()));
      if (FAILED(hr))
        return;

      // Find the adapter & output (monitor) to use for fullscreen, based on the reported name of the HMD's monitor.
      if (Params.MonitorName.GetLength() > 0)
      {
        for (UINT AdapterIndex = 0;; AdapterIndex++)
        {
          Adapter = NULL;
          HRESULT hr = DXGIFactory->EnumAdapters(AdapterIndex, &Adapter.GetRawRef());
          if (hr == DXGI_ERROR_NOT_FOUND)
            break;

          DXGI_ADAPTER_DESC Desc;
          Adapter->GetDesc(&Desc);

          UpdateMonitorOutputs();

          if (FullscreenOutput)
            break;
        }

        if (!FullscreenOutput)
          Adapter = NULL;
      }

      if (!Adapter)
      {
        DXGIFactory->EnumAdapters(0, &Adapter.GetRawRef());
      }

      int flags = 0;
      //int flags =  D3D11_CREATE_DEVICE_DEBUG;    

      Device = NULL;
      Context = NULL;
      hr = D3D11CreateDevice(Adapter, Adapter ? D3D_DRIVER_TYPE_UNKNOWN : D3D_DRIVER_TYPE_HARDWARE,
        NULL, flags, NULL, 0, D3D11_SDK_VERSION,
        &Device.GetRawRef(), NULL, &Context.GetRawRef());

      if (FAILED(hr))
        return;

      if (!RecreateSwapChain())
        return;

      if (Params.Fullscreen)
        SwapChain->SetFullscreenState(1, FullscreenOutput);

      initShadersAndStates();
    }

    // Constructor helper
    void  RenderDevice::initShadersAndStates()
    {
      CurRenderTarget = NULL;
      D3D11_BLEND_DESC bm;
      memset(&bm, 0, sizeof(bm));
      bm.RenderTarget[0].BlendEnable = true;
      bm.RenderTarget[0].BlendOp = bm.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
      bm.RenderTarget[0].SrcBlend = bm.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
      bm.RenderTarget[0].DestBlend = bm.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
      bm.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
      BlendState = NULL;
      Device->CreateBlendState(&bm, &BlendState.GetRawRef());

      D3D11_RASTERIZER_DESC rs;
      memset(&rs, 0, sizeof(rs));
      rs.AntialiasedLineEnable = true;
      rs.CullMode = D3D11_CULL_BACK;
      rs.DepthClipEnable = true;
      rs.FillMode = D3D11_FILL_SOLID;
      Rasterizer = NULL;
      Device->CreateRasterizerState(&rs, &Rasterizer.GetRawRef());
      SetDepthMode(0, 0);
    }

    RenderDevice::~RenderDevice()
    {
      if (SwapChain && Params.Fullscreen)
      {
        SwapChain->SetFullscreenState(false, NULL);
      }
    }

    // Implement static initializer function to create this class.
    RenderDevice* RenderDevice::CreateDevice(const RendererParams& rp, void* oswnd)
    {
      RenderDevice* p = new RenderDevice(rp, (HWND)oswnd);
      if (p)
      {
        if (!p->Device)
        {
          p->Release();
          p = 0;
        }
      }
      return p;
    }

    // Fallback monitor enumeration in case newly plugged in monitor wasn't detected.
    // Added originally for the FactoryTest app.
    // New Outputs don't seem to be detected unless adapter is re-created, but that would also
    // require us to re-initialize D3D11 (recreating objects, etc). This bypasses that for "fake"
    // fullscreen modes.
    BOOL CALLBACK MonitorEnumFunc(HMONITOR hMonitor, HDC, LPRECT, LPARAM dwData)
    {
      RenderDevice* renderer = (RenderDevice*)dwData;

      MONITORINFOEX monitor;
      monitor.cbSize = sizeof(monitor);

      if (::GetMonitorInfo(hMonitor, &monitor) && monitor.szDevice[0])
      {
        DISPLAY_DEVICE dispDev;
        memset(&dispDev, 0, sizeof(dispDev));
        dispDev.cb = sizeof(dispDev);

        if (::EnumDisplayDevices(monitor.szDevice, 0, &dispDev, 0))
        {
          if (strstr(String(dispDev.DeviceName).ToCStr(), renderer->GetParams().MonitorName.ToCStr()))
          {
            renderer->FSDesktopX = monitor.rcMonitor.left;
            renderer->FSDesktopY = monitor.rcMonitor.top;
            return FALSE;
          }
        }
      }

      return TRUE;
    }


    void RenderDevice::UpdateMonitorOutputs()
    {
      HRESULT hr;

      bool deviceNameFound = false;

      for (UINT OutputIndex = 0;; OutputIndex++)
      {
        Ptr<IDXGIOutput> Output;
        hr = Adapter->EnumOutputs(OutputIndex, &Output.GetRawRef());
        if (hr == DXGI_ERROR_NOT_FOUND)
        {
          break;
        }

        DXGI_OUTPUT_DESC OutDesc;
        Output->GetDesc(&OutDesc);

        MONITORINFOEX monitor;
        monitor.cbSize = sizeof(monitor);
        if (::GetMonitorInfo(OutDesc.Monitor, &monitor) && monitor.szDevice[0])
        {
          DISPLAY_DEVICE dispDev;
          memset(&dispDev, 0, sizeof(dispDev));
          dispDev.cb = sizeof(dispDev);

          if (::EnumDisplayDevices(monitor.szDevice, 0, &dispDev, 0))
          {
            if (strstr(String(dispDev.DeviceName).ToCStr(), Params.MonitorName.ToCStr()))
            {
              deviceNameFound = true;
              FullscreenOutput = Output;
              FSDesktopX = monitor.rcMonitor.left;
              FSDesktopY = monitor.rcMonitor.top;
              break;
            }
          }
        }
      }

      if (!deviceNameFound && !Params.MonitorName.IsEmpty())
      {
        EnumDisplayMonitors(0, 0, MonitorEnumFunc, (LPARAM)this);
      }
    }

    bool RenderDevice::RecreateSwapChain()
    {
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
      scDesc.SampleDesc.Count = Params.Multisample;
      scDesc.SampleDesc.Quality = 0;
      scDesc.Windowed = (Params.Fullscreen != Display_Fullscreen);
      scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
      scDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;

      if (SwapChain)
      {
        SwapChain->SetFullscreenState(FALSE, NULL);
        SwapChain->Release();
        SwapChain = NULL;
      }

      Ptr<IDXGISwapChain> newSC;
      if (FAILED(DXGIFactory->CreateSwapChain(Device, &scDesc, &newSC.GetRawRef())))
        return false;
      SwapChain = newSC;

      BackBuffer = NULL;
      BackBufferRT = NULL;
      HRESULT hr = SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&BackBuffer.GetRawRef());
      if (FAILED(hr))
        return false;

      hr = Device->CreateRenderTargetView(BackBuffer, NULL, &BackBufferRT.GetRawRef());
      if (FAILED(hr))
        return false;

      Texture* depthBuffer = GetDepthBuffer(WindowWidth, WindowHeight, Params.Multisample);
      CurDepthBuffer = depthBuffer;
      if (CurRenderTarget == NULL)
      {
        Context->OMSetRenderTargets(1, &BackBufferRT.GetRawRef(), depthBuffer->TexDsv);
      }
      return true;
    }

    bool RenderDevice::SetParams(const RendererParams& newParams)
    {
      String oldMonitor = Params.MonitorName;

      Params = newParams;
      if (newParams.MonitorName != oldMonitor)
      {
        UpdateMonitorOutputs();
      }

      return RecreateSwapChain();
    }


    bool RenderDevice::SetFullscreen(DisplayMode fullscreen)
    {
      if (fullscreen == Params.Fullscreen)
        return true;

      HRESULT hr = SwapChain->SetFullscreenState(fullscreen, fullscreen ? FullscreenOutput : NULL);
      if (FAILED(hr))
      {
        return false;
      }

      Params.Fullscreen = fullscreen;
      return true;
    }

    void RenderDevice::SetViewport(const Recti& vp)
    {
      D3DViewport.Width = (float)vp.w;
      D3DViewport.Height = (float)vp.h;
      D3DViewport.MinDepth = 0;
      D3DViewport.MaxDepth = 1;
      D3DViewport.TopLeftX = (float)vp.x;
      D3DViewport.TopLeftY = (float)vp.y;
      Context->RSSetViewports(1, &D3DViewport);
    }

    void RenderDevice::SetFullViewport()
    {
      D3DViewport.Width = (float)WindowWidth;
      D3DViewport.Height = (float)WindowHeight;
      D3DViewport.MinDepth = 0;
      D3DViewport.MaxDepth = 1;
      D3DViewport.TopLeftX = 0;
      D3DViewport.TopLeftY = 0;
      Context->RSSetViewports(1, &D3DViewport);
    }

    static int GetDepthStateIndex(bool enable, bool write, RenderDevice::CompareFunc func)
    {
      if (!enable)
        return 0;
      return 1 + int(func) * 2 + write;
    }

    void RenderDevice::SetDepthMode(bool enable, bool write, CompareFunc func)
    {
      int index = GetDepthStateIndex(enable, write, func);
      if (DepthStates[index])
      {
        CurDepthState = DepthStates[index];
        Context->OMSetDepthStencilState(DepthStates[index], 0);
        return;
      }

      D3D11_DEPTH_STENCIL_DESC dss;
      memset(&dss, 0, sizeof(dss));
      dss.DepthEnable = enable;
      switch (func)
      {
      case Compare_Always:  dss.DepthFunc = D3D11_COMPARISON_ALWAYS;  break;
      case Compare_Less:    dss.DepthFunc = D3D11_COMPARISON_LESS;    break;
      case Compare_Greater: dss.DepthFunc = D3D11_COMPARISON_GREATER; break;
      default:
        OVR_ASSERT(0);
      }
      dss.DepthWriteMask = write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
      Device->CreateDepthStencilState(&dss, &DepthStates[index].GetRawRef());
      Context->OMSetDepthStencilState(DepthStates[index], 0);
      CurDepthState = DepthStates[index];
    }

    Texture* RenderDevice::GetDepthBuffer(int w, int h, int ms)
    {
      for (unsigned i = 0; i < DepthBuffers.GetSize(); i++)
      {
        if (w == DepthBuffers[i]->Width && h == DepthBuffers[i]->Height &&
          ms == DepthBuffers[i]->Samples)
          return DepthBuffers[i];
      }

      Ptr<Texture> newDepth = *CreateTexture(Texture_Depth | Texture_RenderTarget | ms, w, h, NULL);
      if (newDepth == NULL)
      {
        OVR_DEBUG_LOG(("Failed to get depth buffer."));
        return NULL;
      }

      DepthBuffers.PushBack(newDepth);
      return newDepth.GetPtr();
    }

    void RenderDevice::Clear(float r, float g, float b, float a, float depth)
    {
      const float color[] = { r, g, b, a };

      // Needed for each eye to do its own clear, since ClearRenderTargetView doesn't honor viewport.    

      // Save state that is affected by clearing this way
      ID3D11DepthStencilState* oldDepthState = CurDepthState;
      SetDepthMode(true, true, Compare_Always);
      Context->IASetInputLayout(ModelVertexIL);
      Context->GSSetShader(NULL, NULL, 0);

      ID3D11ShaderResourceView* sv[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
      // Clear Viewport   
      Context->OMSetBlendState(NULL, NULL, 0xffffffff);
      Context->Draw(4, 0);

      // reset
      CurDepthState = oldDepthState;
      Context->OMSetDepthStencilState(CurDepthState, 0);
    }


    ID3D11SamplerState* RenderDevice::GetSamplerState(int sm)
    {
      if (SamplerStates[sm])
        return SamplerStates[sm];

      D3D11_SAMPLER_DESC ss;
      memset(&ss, 0, sizeof(ss));
      if (sm & Sample_Clamp)
        ss.AddressU = ss.AddressV = ss.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
      else if (sm & Sample_ClampBorder)
        ss.AddressU = ss.AddressV = ss.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
      else
        ss.AddressU = ss.AddressV = ss.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;

      if (sm & Sample_Nearest)
      {
        ss.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
      }
      else if (sm & Sample_Anisotropic)
      {
        ss.Filter = D3D11_FILTER_ANISOTROPIC;
        ss.MaxAnisotropy = 8;
      }
      else
      {
        ss.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
      }
      ss.MaxLOD = 15;
      Device->CreateSamplerState(&ss, &SamplerStates[sm].GetRawRef());
      return SamplerStates[sm];
    }

    // Placeholder texture to come in externally in slave rendering mode
    Texture* RenderDevice::CreatePlaceholderTexture(int format)
    {
      Texture* newTex = new Texture(this, format, 0, 0);
      newTex->Samples = 1;

      return newTex;
    }


    Texture* RenderDevice::CreateTexture(int format, int width, int height, const void* data, int mipcount)
    {
      OVR_UNUSED(mipcount);

      DXGI_FORMAT d3dformat;
      int         bpp;
      switch (format & Texture_TypeMask)
      {
      case Texture_RGBA:
        bpp = 4;
        d3dformat = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
      case Texture_Depth:
        bpp = 0;
        d3dformat = DXGI_FORMAT_D32_FLOAT;
        break;
      default:
        return NULL;
      }

      int samples = (format & Texture_SamplesMask);
      if (samples < 1)
      {
        samples = 1;
      }

      Texture* NewTex = new Texture(this, format, width, height);
      NewTex->Samples = samples;

      D3D11_TEXTURE2D_DESC dsDesc;
      dsDesc.Width = width;
      dsDesc.Height = height;
      dsDesc.MipLevels = (format == (Texture_RGBA | Texture_GenMipmaps) && data) ? GetNumMipLevels(width, height) : 1;
      dsDesc.ArraySize = 1;
      dsDesc.Format = d3dformat;
      dsDesc.SampleDesc.Count = samples;
      dsDesc.SampleDesc.Quality = 0;
      dsDesc.Usage = D3D11_USAGE_DEFAULT;
      dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
      dsDesc.CPUAccessFlags = 0;
      dsDesc.MiscFlags = 0;

      if (format & Texture_RenderTarget)
      {
        if ((format & Texture_TypeMask) == Texture_Depth)
        { // We don't use depth textures, and creating them in d3d10 requires different options.
          dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        }
        else
        {
          dsDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
        }
      }

      HRESULT hr = Device->CreateTexture2D(&dsDesc, NULL, &NewTex->Tex.GetRawRef());
      if (FAILED(hr))
      {
        OVR_DEBUG_LOG_TEXT(("Failed to create 2D D3D texture."));
        NewTex->Release();
        return NULL;
      }
      if (dsDesc.BindFlags & D3D11_BIND_SHADER_RESOURCE)
      {
        Device->CreateShaderResourceView(NewTex->Tex, NULL, &NewTex->TexSv.GetRawRef());
      }

      if (data)
      {
        Context->UpdateSubresource(NewTex->Tex, 0, NULL, data, width * bpp, width * height * bpp);
        if (format == (Texture_RGBA | Texture_GenMipmaps))
        {
          int srcw = width, srch = height;
          int level = 0;
          uint8_t* mipmaps = NULL;
          do
          {
            level++;
            int mipw = srcw >> 1;
            if (mipw < 1)
            {
              mipw = 1;
            }
            int miph = srch >> 1;
            if (miph < 1)
            {
              miph = 1;
            }
            if (mipmaps == NULL)
            {
              mipmaps = (uint8_t*)OVR_ALLOC(mipw * miph * 4);
            }
            FilterRgba2x2(level == 1 ? (const uint8_t*)data : mipmaps, srcw, srch, mipmaps);
            Context->UpdateSubresource(NewTex->Tex, level, NULL, mipmaps, mipw * bpp, miph * bpp);
            srcw = mipw;
            srch = miph;
          } while (srcw > 1 || srch > 1);

          if (mipmaps != NULL)
          {
            OVR_FREE(mipmaps);
          }
        }
      }

      if (format & Texture_RenderTarget)
      {
        if ((format & Texture_TypeMask) == Texture_Depth)
        {
          Device->CreateDepthStencilView(NewTex->Tex, NULL, &NewTex->TexDsv.GetRawRef());
        }
        else
        {
          Device->CreateRenderTargetView(NewTex->Tex, NULL, &NewTex->TexRtv.GetRawRef());
        }
      }

      return NewTex;
    }

    // Rendering
    void RenderDevice::BeginRendering()
    {
      Context->RSSetState(Rasterizer);
    }

    void RenderDevice::BeginScene()
    {
      BeginRendering();
    }

    void RenderDevice::FinishScene()
    {
      SetRenderTarget(0);
    }

    void RenderDevice::SetRenderTarget(Texture* colorTex,
      Texture* depth,
      Texture* stencil)
    {
      OVR_UNUSED(stencil);

      CurRenderTarget = (Texture*)colorTex;
      if (colorTex == NULL)
      {
        Texture* newDepthBuffer = GetDepthBuffer(WindowWidth, WindowHeight, Params.Multisample);
        if (newDepthBuffer == NULL)
        {
          OVR_DEBUG_LOG(("New depth buffer creation failed."));
        }
        if (newDepthBuffer != NULL)
        {
          CurDepthBuffer = GetDepthBuffer(WindowWidth, WindowHeight, Params.Multisample);
          Context->OMSetRenderTargets(1, &BackBufferRT.GetRawRef(), CurDepthBuffer->TexDsv);
        }
        return;
      }
      if (depth == NULL)
      {
        depth = GetDepthBuffer(colorTex->GetWidth(), colorTex->GetHeight(), CurRenderTarget->Samples);
      }

      CurDepthBuffer = (Texture*)depth;
      Context->OMSetRenderTargets(1, &((Texture*)colorTex)->TexRtv.GetRawRef(), ((Texture*)depth)->TexDsv);
    }


    void RenderDevice::Present(bool vsyncEnabled)
    {
      SwapChain->Present(vsyncEnabled ? 1 : 0, 0);
    }

    void RenderDevice::WaitUntilGpuIdle()
    {
      // Flush and Stall CPU while waiting for GPU to complete rendering all of the queued draw calls
      D3D11_QUERY_DESC queryDesc = { D3D11_QUERY_EVENT, 0 };
      Ptr<ID3D11Query> query;
      BOOL             done = FALSE;

      if (Device->CreateQuery(&queryDesc, &query.GetRawRef()) == S_OK)
      {
        Context->End(query);
        do {} while (!done && !FAILED(Context->GetData(query, &done, sizeof(BOOL), 0)));
      }
    }

    int GetNumMipLevels(int w, int h)
    {
      int n = 1;
      while (w > 1 || h > 1)
      {
        w >>= 1;
        h >>= 1;
        n++;
      }
      return n;
    }

    void FilterRgba2x2(const uint8_t* src, int w, int h, uint8_t* dest)
    {
      for (int j = 0; j < (h & ~1); j += 2)
      {
        const uint8_t* psrc = src + (w * j * 4);
        uint8_t*       pdest = dest + ((w >> 1) * (j >> 1) * 4);

        for (int i = 0; i < w >> 1; i++, psrc += 8, pdest += 4)
        {
          pdest[0] = (((int)psrc[0]) + psrc[4] + psrc[w * 4 + 0] + psrc[w * 4 + 4]) >> 2;
          pdest[1] = (((int)psrc[1]) + psrc[5] + psrc[w * 4 + 1] + psrc[w * 4 + 5]) >> 2;
          pdest[2] = (((int)psrc[2]) + psrc[6] + psrc[w * 4 + 2] + psrc[w * 4 + 6]) >> 2;
          pdest[3] = (((int)psrc[3]) + psrc[7] + psrc[w * 4 + 3] + psrc[w * 4 + 7]) >> 2;
        }
      }
    }

  }
}


template<typename T, typename F>
void populateSharedPtr(std::shared_ptr<T> & ptr, F function) {
  T * rawPtr;
  function(&rawPtr);
  ptr = std::shared_ptr<T>(rawPtr);
}


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

HWND                hWnd = NULL;
HINSTANCE           hInstance = NULL;
POINT               WindowCenter;

// User inputs
bool                Quit = 0;

LRESULT CALLBACK systemWindowProc(HWND arg_hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
  switch (msg)
  {
  case(WM_NCCREATE) : hWnd = arg_hwnd; break;

  case WM_MOUSEMOVE:	{
    // Convert mouse motion to be relative
    // (report the offset and re-center).
    POINT newPos = { LOWORD(lp), HIWORD(lp) };
    ::ClientToScreen(hWnd, &newPos);
    if ((newPos.x == WindowCenter.x) && (newPos.y == WindowCenter.y))
      break;
    ::SetCursorPos(WindowCenter.x, WindowCenter.y);
    break;
  }

  case WM_MOVE:		RECT r;
    GetClientRect(hWnd, &r);
    WindowCenter.x = r.right / 2;
    WindowCenter.y = r.bottom / 2;
    ::ClientToScreen(hWnd, &WindowCenter);
    break;

  case WM_CREATE:     SetTimer(hWnd, 0, 100, NULL); break;
  case WM_TIMER:      KillTimer(hWnd, 0);

  case WM_SETFOCUS:
    SetCursorPos(WindowCenter.x, WindowCenter.y);
    SetCapture(hWnd);
    ShowCursor(FALSE);
    break;
  case WM_KILLFOCUS:
    ReleaseCapture();
    ShowCursor(TRUE);
    break;

  case WM_QUIT:
  case WM_CLOSE:      Quit = true;
    return 0;
  }

  return DefWindowProc(hWnd, msg, wp, lp);
}


HWND Util_InitWindowAndGraphics(Recti vp, int fullscreen, int multiSampleCount, bool UseAppWindowFrame, RenderDevice ** returnedDevice)
{
  RendererParams  renderParams;

  // Window
  WNDCLASS wc;
  memset(&wc, 0, sizeof(wc));
  wc.lpszClassName = L"OVRAppWindow";
  wc.style = CS_OWNDC;
  wc.lpfnWndProc = systemWindowProc;
  wc.cbWndExtra = NULL;
  RegisterClass(&wc);

  DWORD wsStyle = WS_POPUP;
  DWORD sizeDivisor = 1;

  if (UseAppWindowFrame)
  {
    // If using our driver, displaya window frame with a smaller window.
    // Original HMD resolution is still passed into the renderer for proper swap chain.
    wsStyle |= WS_OVERLAPPEDWINDOW;
    renderParams.Resolution = vp.GetSize();
    sizeDivisor = 2;
  }

  RECT winSize = { 0, 0, vp.w / sizeDivisor, vp.h / sizeDivisor };
  AdjustWindowRect(&winSize, wsStyle, false);
  hWnd = CreateWindowA("OVRAppWindow", "OculusRoomTiny",
    wsStyle | WS_VISIBLE,
    vp.x, vp.y,
    winSize.right - winSize.left, winSize.bottom - winSize.top,
    NULL, NULL, hInstance, NULL);

  POINT center = { vp.w / 2 / sizeDivisor, vp.h / 2 / sizeDivisor };
  ::ClientToScreen(hWnd, &center);
  WindowCenter = center;

  if (!hWnd) return(NULL);

  // Graphics
  renderParams.Multisample = multiSampleCount;
  renderParams.Fullscreen = fullscreen;
  Allocator::setInstance(new DefaultAllocator());
  Allocator * pAllocator = Allocator::GetInstance();
  *returnedDevice = RenderDevice::CreateDevice(renderParams, (void*)hWnd);

  return(hWnd);
}


struct EyeArgs {
  glm::mat4               projection;
  glm::mat4               viewOffset;
  gl::FrameBufferWrapper  framebuffer;
};

class HelloRift : public GlStubWindow, RiftStubApp {
protected:
  HWND              dxWnd;
  EyeArgs           perEyeArgs[2];
  ovrTexture        textures[2];
  float             eyeHeight{ OVR_DEFAULT_EYE_HEIGHT };
  float             ipd{ OVR_DEFAULT_IPD };
  glm::mat4         player;
  HANDLE            gl_handleD3D;
  HANDLE            gl_handles[2];
  Texture *         pTextures[2];
  RenderDevice*     pRender;

public:


  HelloRift() {
    dxWnd = Util_InitWindowAndGraphics(Recti(0, 0, 1920, 1080), 1, 1, true, &pRender);
    gl_handleD3D = wglDXOpenDeviceNV(pRender->Device);
    ovrHmd_AttachToWindow(hmd, dxWnd, nullptr, nullptr);

    for_each_eye([&](ovrEyeType eye){
      EyeArgs & eyeArgs = perEyeArgs[eye];
      memset(textures + eye, 0, sizeof(ovrTexture));

      ovrTextureHeader & eyeTextureHeader = textures[eye].Header;
      eyeTextureHeader.TextureSize = ovrHmd_GetFovTextureSize(hmd, eye, hmd->DefaultEyeFov[eye], 1.0f);
      eyeTextureHeader.RenderViewport.Size = eyeTextureHeader.TextureSize;
      eyeTextureHeader.RenderViewport.Pos.x = 0;
      eyeTextureHeader.RenderViewport.Pos.y = 0;
      eyeTextureHeader.API = ovrRenderAPI_D3D11;
      ovrD3D11Texture & d3dTexture = (ovrD3D11Texture&)textures[eye];
      pTextures[eye] = pRender->CreateTexture(Texture_RGBA, eyeTextureHeader.TextureSize.w, eyeTextureHeader.TextureSize.h, nullptr);
      d3dTexture.D3D11.pTexture = pTextures[eye]->Tex;
      d3dTexture.D3D11.pSRView = pTextures[eye]->TexSv;
      eyeArgs.framebuffer.color = gl::FrameBufferWrapper::ColorTexturePtr(new gl::Texture2d());
      ID3D11Texture2D * texHandle = d3dTexture.D3D11.pTexture;
      gl_handles[eye] = wglDXRegisterObjectNV(gl_handleD3D, texHandle, eyeArgs.framebuffer.color->texture,
        GL_TEXTURE_2D,
        WGL_ACCESS_READ_WRITE_NV);
    });
    BOOL success = wglDXLockObjectsNV(gl_handleD3D, 2, gl_handles);
    if (!success) {
      FAIL("Could not lock objects");
    }
    for_each_eye([&](ovrEyeType eye){
      EyeArgs & eyeArgs = perEyeArgs[eye];
      ovrTextureHeader & eyeTextureHeader = textures[eye].Header;
      eyeArgs.framebuffer.init(Rift::fromOvr(eyeTextureHeader.TextureSize));
    });
    success = wglDXUnlockObjectsNV(gl_handleD3D, 2, gl_handles);
    if (!success) {
      FAIL("Could not unlock objects");
    }

    ovrD3D11Config cfg;
    memset(&cfg, 0, sizeof(ovrGLConfig));
    cfg.D3D11.Header.API = ovrRenderAPI_D3D11;
    cfg.D3D11.Header.RTSize = hmd->Resolution;
    cfg.D3D11.Header.Multisample = 1;
    cfg.D3D11.pDevice = pRender->Device;
    cfg.D3D11.pSwapChain = pRender->SwapChain;
    cfg.D3D11.pDeviceContext = pRender->Context;
    cfg.D3D11.pBackBufferRT = pRender->BackBufferRT;

    int distortionCaps =
      ovrDistortionCap_TimeWarp |
      ovrDistortionCap_Chromatic |
      ovrDistortionCap_Vignette |
      ovrDistortionCap_Overdrive;

    ovrEyeRenderDesc              eyeRenderDescs[2];
    int configResult = ovrHmd_ConfigureRendering(hmd, &cfg.Config,
      distortionCaps, hmd->DefaultEyeFov, eyeRenderDescs);

    for_each_eye([&](ovrEyeType eye){
      EyeArgs & eyeArgs = perEyeArgs[eye];
      eyeArgs.projection = Rift::fromOvr(
        ovrMatrix4f_Projection(hmd->DefaultEyeFov[eye], 0.01, 100, true));
      eyeArgs.viewOffset = glm::translate(glm::mat4(),
        Rift::fromOvr(eyeRenderDescs[eye].ViewAdjust));
    });

    ovrHmd_ConfigureTracking(hmd,
      ovrTrackingCap_Orientation |
      ovrTrackingCap_Position, 0);
    resetPosition();
  }

  ~HelloRift() {
    ovrHmd_Destroy(hmd);
    hmd = 0;
  }

  virtual void resetPosition() {
    static const glm::vec3 EYE = glm::vec3(0, eyeHeight, ipd * 5);
    static const glm::vec3 LOOKAT = glm::vec3(0, eyeHeight, 0);
    player = glm::inverse(glm::lookAt(EYE, LOOKAT, GlUtils::UP));
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
      }
      else {
        ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_NoVSync);
      }
      break;

    case GLFW_KEY_P:
      if (caps & ovrHmdCap_LowPersistence) {
        ovrHmd_SetEnabledCaps(hmd, caps & ~ovrHmdCap_LowPersistence);
      }
      else {
        ovrHmd_SetEnabledCaps(hmd, caps | ovrHmdCap_LowPersistence);
      }
      break;

    case GLFW_KEY_R:
      resetPosition();
      break;

    default:
      GlfwApp::onKey(key, scancode, action, mods);
      break;
    }
  }

  virtual void update() {
    CameraControl::instance().applyInteraction(player);
    gl::Stacks::modelview().top() = glm::inverse(player);
  }


  void draw() {
    static int frameIndex = 0;
    static ovrPosef eyePoses[2];
    gl::MatrixStack & mv = gl::Stacks::modelview();
    ovrHmd_BeginFrame(hmd, frameIndex++);
    pRender->BeginScene();
    pRender->Clear();

    glEnable(GL_DEPTH_TEST);
    if (!wglDXLockObjectsNV(gl_handleD3D, 2, gl_handles)) {
      FAIL("Could not lock objects");
    }
    for (int i = 0; i < 2; ++i) {
      ovrEyeType eye = hmd->EyeRenderOrder[i];
      EyeArgs & eyeArgs = perEyeArgs[eye];
      gl::Stacks::projection().top() = eyeArgs.projection;

      eyePoses[eye] = ovrHmd_GetEyePose(hmd, eye);
      eyeArgs.framebuffer.activate();
      mv.withPush([&]{
        // Apply the per-eye offset & the head pose
        mv.top() = eyeArgs.viewOffset * glm::inverse(Rift::fromOvr(eyePoses[eye])) * mv.top();
        renderScene();
      });
      eyeArgs.framebuffer.deactivate();
    };
    if (!wglDXUnlockObjectsNV(gl_handleD3D, 2, gl_handles)) {
      FAIL("Could not unlock objects");
    }
    pRender->FinishScene();
    ovrHmd_EndFrame(hmd, eyePoses, textures);
  }

  virtual void renderScene() {
    glClearColor(0, 1, 1, 1);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    GlUtils::renderSkybox(Resource::IMAGES_SKY_CITY_XNEG_PNG);
    GlUtils::renderFloor();

    gl::MatrixStack & mv = gl::Stacks::modelview();
    mv.with_push([&]{
      mv.translate(glm::vec3(0, 0, ipd * -5));
      GlUtils::renderManikin();
    });

    mv.with_push([&]{
      mv.translate(glm::vec3(0, eyeHeight, 0));
      mv.scale(ipd);
      GlUtils::drawColorCube();
    });
  }

  int run() {
    // Processes messages and calls OnIdle() to do rendering.
    while (!Quit)
    {
      glfwPollEvents();
      MSG msg;
      if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
      else
      {
        update();
        draw();

        // Keep sleeping when we're minimized.
        if (IsIconic(hWnd)) Sleep(10);
      }
    }

    return 0;
  }
};

int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
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