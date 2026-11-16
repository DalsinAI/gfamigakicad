/*
 * No-op wxGLCanvas / wxGLContext for this NDK stove, which has no
 * GFOpenGPU developer kit and therefore no libwx_openlayoutu_gl.
 * KiCad's 3D / OpenGL canvas is compiled against the toolkit headers
 * (setup.h sets wxUSE_GLCANVAS) but must not try to draw. IsDisplaySupported
 * is false; SetCurrent and SwapBuffers fail; the Cairo canvas is used.
 * Copyright (c) 2026 AmiCradle Team. MIT.
 */
#include "wx/wxprec.h"

#if wxUSE_GLCANVAS

#include "wx/glcanvas.h"
#include "wx/log.h"

static void Put(wxGLAttribsBase& a, int what, int value)
{
    a.AddAttribute(what);
    a.AddAttribute(value);
}

void wxGLAttribsBase::AddAttribBits(int searchVal, int combineVal)
{
    wxVector<int>::iterator it = m_GLValues.begin();
    while ( it != m_GLValues.end() && *it != searchVal )
        ++it;
    if ( it != m_GLValues.end() )
    {
        if ( ++it == m_GLValues.end() )
            m_GLValues.push_back(combineVal);
        else
            *it |= combineVal;
    }
    else
    {
        m_GLValues.push_back(searchVal);
        m_GLValues.push_back(combineVal);
    }
}

wxGLContextAttrs& wxGLContextAttrs::CoreProfile() { Put(*this, WX_GL_CORE_PROFILE, 1); return *this; }
wxGLContextAttrs& wxGLContextAttrs::MajorVersion(int val) { if ( val > 0 ) Put(*this, WX_GL_MAJOR_VERSION, val); return *this; }
wxGLContextAttrs& wxGLContextAttrs::MinorVersion(int val) { if ( val >= 0 ) Put(*this, WX_GL_MINOR_VERSION, val); return *this; }
wxGLContextAttrs& wxGLContextAttrs::CompatibilityProfile() { Put(*this, WX_GL_COMPAT_PROFILE, 1); return *this; }
wxGLContextAttrs& wxGLContextAttrs::ForwardCompatible() { return *this; }
wxGLContextAttrs& wxGLContextAttrs::ES2() { Put(*this, WX_GL_ES2, 1); return *this; }
wxGLContextAttrs& wxGLContextAttrs::DebugCtx() { return *this; }
wxGLContextAttrs& wxGLContextAttrs::Robust() { return *this; }
wxGLContextAttrs& wxGLContextAttrs::NoResetNotify() { return *this; }
wxGLContextAttrs& wxGLContextAttrs::LoseOnReset() { return *this; }
wxGLContextAttrs& wxGLContextAttrs::ResetIsolation() { return *this; }
wxGLContextAttrs& wxGLContextAttrs::ReleaseFlush(int WXUNUSED(val)) { return *this; }
wxGLContextAttrs& wxGLContextAttrs::PlatformDefaults() { renderTypeRGBA = true; x11Direct = true; return *this; }
void wxGLContextAttrs::EndList() { AddAttribute(0); }

wxGLAttributes& wxGLAttributes::RGBA() { return *this; }
wxGLAttributes& wxGLAttributes::BufferSize(int WXUNUSED(val)) { return *this; }
wxGLAttributes& wxGLAttributes::Level(int WXUNUSED(val)) { return *this; }
wxGLAttributes& wxGLAttributes::DoubleBuffer() { Put(*this, WX_GL_DOUBLEBUFFER, 1); return *this; }
wxGLAttributes& wxGLAttributes::Stereo() { return *this; }
wxGLAttributes& wxGLAttributes::AuxBuffers(int WXUNUSED(val)) { return *this; }
wxGLAttributes& wxGLAttributes::MinRGBA(int WXUNUSED(mRed), int WXUNUSED(mGreen), int WXUNUSED(mBlue), int mAlpha)
{
    if ( mAlpha > 0 )
        Put(*this, WX_GL_MIN_ALPHA, mAlpha);
    return *this;
}
wxGLAttributes& wxGLAttributes::Depth(int val) { if ( val >= 0 ) Put(*this, WX_GL_DEPTH_SIZE, val); return *this; }
wxGLAttributes& wxGLAttributes::Stencil(int val) { if ( val >= 0 ) Put(*this, WX_GL_STENCIL_SIZE, val); return *this; }
wxGLAttributes& wxGLAttributes::MinAcumRGBA(int WXUNUSED(mRed), int WXUNUSED(mGreen), int WXUNUSED(mBlue),
                                            int WXUNUSED(mAlpha)) { return *this; }
wxGLAttributes& wxGLAttributes::SampleBuffers(int WXUNUSED(val)) { return *this; }
wxGLAttributes& wxGLAttributes::Samplers(int WXUNUSED(val)) { return *this; }
wxGLAttributes& wxGLAttributes::FrameBuffersRGB() { return *this; }
void wxGLAttributes::EndList() { AddAttribute(0); }
wxGLAttributes& wxGLAttributes::PlatformDefaults() { return *this; }
wxGLAttributes& wxGLAttributes::Defaults()
{
    RGBA().DoubleBuffer().Depth(16);
    return *this;
}
void wxGLAttributes::AddDefaultsForWXBefore31()
{
    Defaults();
}

wxGLCanvasBase::wxGLCanvasBase()
{
#if WXWIN_COMPATIBILITY_2_8
    m_glContext = NULL;
#endif
    SetBackgroundStyle(wxBG_STYLE_PAINT);
}

wxGLCanvasBase::~wxGLCanvasBase() {}

bool wxGLCanvasBase::SetCurrent(const wxGLContext& context) const
{
    return context.SetCurrent(*static_cast<const wxGLCanvas*>(this));
}

bool wxGLCanvasBase::SetColour(const wxString& WXUNUSED(colour))
{
    return false;
}

/* static */
bool wxGLCanvasBase::IsExtensionInList(const char * WXUNUSED(list), const char * WXUNUSED(extension))
{
    return false;
}

/* static */
bool wxGLCanvasBase::ParseAttribList(const int *attribList,
                                     wxGLAttributes& dispAttrs,
                                     wxGLContextAttrs* ctxAttrs)
{
    dispAttrs.PlatformDefaults();
    if ( ctxAttrs )
        ctxAttrs->PlatformDefaults();
    if ( !attribList )
    {
        dispAttrs.AddDefaultsForWXBefore31();
        dispAttrs.EndList();
        if ( ctxAttrs )
            ctxAttrs->EndList();
        return true;
    }
    return false;
}

/* static */
bool wxGLCanvasBase::IsDisplaySupported(const wxGLAttributes& WXUNUSED(dispAttrs))
{
    return false;
}

/* static */
bool wxGLCanvasBase::IsDisplaySupported(const int * WXUNUSED(attribList))
{
    return false;
}

/* static */
bool wxGLCanvasBase::IsExtensionSupported(const char * WXUNUSED(extension))
{
    return false;
}

wxIMPLEMENT_CLASS(wxGLContext, wxObject);

wxGLContext::wxGLContext(wxGLCanvas * WXUNUSED(win),
                         const wxGLContext * WXUNUSED(other),
                         const wxGLContextAttrs * WXUNUSED(ctxAttrs))
    : m_gla(NULL)
{
    m_isOk = false;
}

wxGLContext::~wxGLContext() {}

bool wxGLContext::SetCurrent(const wxGLCanvas& WXUNUSED(win)) const
{
    return false;
}

wxIMPLEMENT_CLASS(wxGLCanvas, wxWindow);

void wxGLCanvas::Init()
{
    m_olBuffer = NULL;
    m_olTarget = NULL;
    m_olContext = NULL;
    m_olW = m_olH = 0;
    m_olDouble = true;
    m_olDepth = 16;
    m_olStencil = 0;
    m_olAlpha = 0;
    m_olSeen = false;
}

wxGLCanvas::wxGLCanvas(wxWindow *parent, const wxGLAttributes& dispAttrs, wxWindowID id, const wxPoint& pos,
                       const wxSize& size, long style, const wxString& name, const wxPalette& palette)
{
    Init();
    Create(parent, dispAttrs, id, pos, size, style, name, palette);
}

wxGLCanvas::wxGLCanvas(wxWindow *parent, wxWindowID id, const int *attribList, const wxPoint& pos,
                       const wxSize& size, long style, const wxString& name, const wxPalette& palette)
{
    Init();
    Create(parent, id, pos, size, style, name, attribList, palette);
}

bool wxGLCanvas::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
                        const wxString& name, const int *attribList, const wxPalette& palette)
{
    wxGLAttributes dispAttrs;
    if ( !ParseAttribList(attribList, dispAttrs, &m_GLCTXAttrs) )
        return false;
    return Create(parent, dispAttrs, id, pos, size, style, name, palette);
}

bool wxGLCanvas::Create(wxWindow *parent, const wxGLAttributes& WXUNUSED(dispAttrs), wxWindowID id, const wxPoint& pos,
                        const wxSize& size, long style, const wxString& name,
                        const wxPalette& WXUNUSED(palette))
{
    return wxWindow::Create(parent, id, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name);
}

void wxGLCanvas::OLTakeAttributes(const int * WXUNUSED(attrs)) {}

wxGLCanvas::~wxGLCanvas() {}

struct gla_buffer *wxGLCanvas::OLGetBuffer()
{
    return NULL;
}

void wxGLCanvas::OLAim() {}

void wxGLCanvas::OLDrawOwn(int WXUNUSED(x), int WXUNUSED(y), int WXUNUSED(w), int WXUNUSED(h)) {}

bool wxGLCanvas::SwapBuffers()
{
    return false;
}

wxIMPLEMENT_CLASS(wxGLApp, wxApp);

bool wxGLApp::InitGLVisual(const int * WXUNUSED(attribList))
{
    return false;
}

#endif // wxUSE_GLCANVAS
