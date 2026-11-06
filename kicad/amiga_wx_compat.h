/*
 * Included first in every C++ file of KiCad's Amiga build (after
 * gfamiga_prefix.h). Fills in wx classes the GFOpenWX Toolkit does not
 * build yet; each part drops out by itself once the toolkit has it.
 * Copyright (c) 2026 AmiCradle Team
 * Designed, tested and delivered by The AmigaChrome Team
 */
#ifndef AMIGA_WX_COMPAT_H
#define AMIGA_WX_COMPAT_H

#include <wx/defs.h>
#include <wx/string.h>
#include <wx/snglinst.h>
#include <wx/log.h>
#include <wx/menu.h>

#if !wxUSE_SNGLINST_CHECKER
/* wxSingleInstanceChecker on a named public message port: the first copy
   adds the port, later copies find it there. The port only marks a name;
   nothing is ever sent to it. */
class wxSingleInstanceChecker
{
public:
    wxSingleInstanceChecker() : m_port( nullptr ), m_other( false ), m_made( false ) {}
    wxSingleInstanceChecker( const wxString& aName, const wxString& aPath = wxEmptyString ) :
            m_port( nullptr ), m_other( false ), m_made( false )
    {
        Create( aName, aPath );
    }
    ~wxSingleInstanceChecker();

    bool Create( const wxString& aName, const wxString& aPath = wxEmptyString );
    bool CreateDefault();
    bool IsAnotherRunning() const
    {
        if( !m_made )
            const_cast<wxSingleInstanceChecker*>( this )->CreateDefault();

        return m_other;
    }

private:
    void* m_port;    // struct MsgPort* this copy added, or nullptr
    bool  m_other;
    bool  m_made;

    wxSingleInstanceChecker( const wxSingleInstanceChecker& ) = delete;
    wxSingleInstanceChecker& operator=( const wxSingleInstanceChecker& ) = delete;
};
#endif

#include <wx/fswatcher.h>

#if !wxUSE_FSWATCHER
/* No file system watcher in the toolkit yet: Add() and AddTree() fail, so
   KiCad's project tree and library frames never wait on change events
   (they still refresh when the user acts). */
#include <wx/event.h>
#include <wx/filename.h>
#include <wx/arrstr.h>

enum
{
    wxFSW_EVENT_CREATE = 0x01,
    wxFSW_EVENT_DELETE = 0x02,
    wxFSW_EVENT_RENAME = 0x04,
    wxFSW_EVENT_MODIFY = 0x08,
    wxFSW_EVENT_ACCESS = 0x10,
    wxFSW_EVENT_ATTRIB = 0x20,
    wxFSW_EVENT_UNMOUNT = 0x2000,
    wxFSW_EVENT_WARNING = 0x40,
    wxFSW_EVENT_ERROR = 0x80,
    wxFSW_EVENT_ALL = 0x20ff
};

class wxFileSystemWatcherEvent : public wxEvent
{
public:
    wxFileSystemWatcherEvent() = default;
    wxEvent* Clone() const override { return new wxFileSystemWatcherEvent( *this ); }
    int GetChangeType() const { return 0; }
    const wxFileName& GetPath() const { return m_path; }
    const wxFileName& GetNewPath() const { return m_path; }
    wxString GetErrorDescription() const { return wxEmptyString; }

private:
    wxFileName m_path;
};

inline const wxEventTypeTag<wxFileSystemWatcherEvent> wxEVT_FSWATCHER( wxNewEventType() );
typedef void ( wxEvtHandler::*wxFileSystemWatcherEventFunction )( wxFileSystemWatcherEvent& );
#define wxFileSystemWatcherEventHandler( func ) \
    wxEVENT_HANDLER_CAST( wxFileSystemWatcherEventFunction, func )

class wxFileSystemWatcher
{
public:
    virtual ~wxFileSystemWatcher() = default;
    bool Add( const wxFileName&, int = wxFSW_EVENT_ALL ) { return false; }
    bool AddTree( const wxFileName&, int = wxFSW_EVENT_ALL, const wxString& = wxEmptyString )
    {
        return false;
    }
    bool Remove( const wxFileName& ) { return false; }
    bool RemoveAll() { return true; }
    int GetWatchedPaths( wxArrayString* aPaths ) const
    {
        if( aPaths )
            aPaths->Clear();

        return 0;
    }
    void SetOwner( wxEvtHandler* ) {}
};
// pcb_base_frame.h and sch_base_frame.h name the watcher class for each
// platform; with no native watcher they pick the polling one
class wxPollingFileSystemWatcher : public wxFileSystemWatcher {};
#endif

#include <wx/glcanvas.h>

#if !wxUSE_GLCANVAS
/* No OpenGL canvas in the toolkit yet. These stand-ins let KiCad's 3D
   viewer compile; IsDisplaySupported() is false and no context is ever OK,
   so nothing draws through them. Drawing panels use the Cairo canvas. */
#include <wx/window.h>
#include <wx/palette.h>
#include <vector>

class wxGLAttribsBase
{
public:
    void AddAttribute( int aAttribute ) { m_values.push_back( aAttribute ); }
    void Reset() { m_values.clear(); }
    const int* GetGLAttrs() const { return m_values.empty() ? nullptr : &m_values[0]; }
    int GetSize() const { return (int) m_values.size(); }

private:
    std::vector<int> m_values;
};

class wxGLAttributes : public wxGLAttribsBase
{
public:
    wxGLAttributes& RGBA() { return *this; }
    wxGLAttributes& BufferSize( int ) { return *this; }
    wxGLAttributes& Level( int ) { return *this; }
    wxGLAttributes& DoubleBuffer() { return *this; }
    wxGLAttributes& Stereo() { return *this; }
    wxGLAttributes& AuxBuffers( int ) { return *this; }
    wxGLAttributes& MinRGBA( int, int, int, int ) { return *this; }
    wxGLAttributes& Depth( int ) { return *this; }
    wxGLAttributes& Stencil( int ) { return *this; }
    wxGLAttributes& MinAcumRGBA( int, int, int, int ) { return *this; }
    wxGLAttributes& PlatformDefaults() { return *this; }
    wxGLAttributes& Defaults() { return *this; }
    wxGLAttributes& SampleBuffers( int ) { return *this; }
    wxGLAttributes& Samplers( int ) { return *this; }
    wxGLAttributes& FrameBuffersRGB() { return *this; }
    void EndList() {}
};

class wxGLContextAttrs : public wxGLAttribsBase
{
public:
    wxGLContextAttrs& CoreProfile() { return *this; }
    wxGLContextAttrs& MajorVersion( int ) { return *this; }
    wxGLContextAttrs& MinorVersion( int ) { return *this; }
    wxGLContextAttrs& OGLVersion( int, int ) { return *this; }
    wxGLContextAttrs& CompatibilityProfile() { return *this; }
    wxGLContextAttrs& ForwardCompatible() { return *this; }
    wxGLContextAttrs& PlatformDefaults() { return *this; }
    void EndList() {}
};

class wxGLContext;

inline constexpr const char wxGLCanvasName[] = "GLCanvas";

class wxGLCanvas : public wxWindow
{
public:
    wxGLCanvas( wxWindow* aParent, const wxGLAttributes&, wxWindowID aId = wxID_ANY,
                const wxPoint& aPos = wxDefaultPosition, const wxSize& aSize = wxDefaultSize,
                long aStyle = 0, const wxString& aName = wxGLCanvasName,
                const wxPalette& = wxNullPalette ) :
            wxWindow( aParent, aId, aPos, aSize, aStyle, aName )
    {
    }

    bool SetCurrent( const wxGLContext& ) const { return false; }
    bool SwapBuffers() { return false; }

    static bool IsDisplaySupported( const wxGLAttributes& ) { return false; }
    static bool IsDisplaySupported( const int* ) { return false; }
    static bool IsExtensionSupported( const char* ) { return false; }
    static bool SetSwapInterval( int ) { return false; }
};

class wxGLContext : public wxObject
{
public:
    explicit wxGLContext( wxGLCanvas*, const wxGLContext* = nullptr,
                          const wxGLContextAttrs* = nullptr ) {}

    bool SetCurrent( const wxGLCanvas& ) const { return false; }
    bool IsOK() const { return false; }
};
#endif

#include <wx/webview.h>

#if !wxUSE_WEBVIEW
/* No web view in the toolkit: the classes KiCad names, with no browser
   behind them. wxWebView::New() gives nullptr, and every KiCad caller
   checks WEBVIEW_PANEL::GetWebView() before using it, so the template
   preview and remote symbol panels stay empty. */
#include <wx/control.h>
#include <wx/sharedptr.h>

class wxWebViewHandler
{
public:
    explicit wxWebViewHandler( const wxString& aScheme ) : m_scheme( aScheme ) {}
    virtual ~wxWebViewHandler() = default;
    const wxString& GetName() const { return m_scheme; }

private:
    wxString m_scheme;
};

class wxWebViewArchiveHandler : public wxWebViewHandler
{
public:
    explicit wxWebViewArchiveHandler( const wxString& aScheme ) : wxWebViewHandler( aScheme ) {}
};

class wxWebViewFSHandler : public wxWebViewHandler
{
public:
    explicit wxWebViewFSHandler( const wxString& aScheme ) : wxWebViewHandler( aScheme ) {}
};

inline constexpr const char wxWebViewBackendDefault[] = "";
inline constexpr const char wxWebViewBackendIE[] = "wxWebViewIE";
inline constexpr const char wxWebViewBackendEdge[] = "wxWebViewEdge";
inline constexpr const char wxWebViewBackendWebKit[] = "wxWebViewWebKit";

enum wxWebViewNavigationActionFlags
{
    wxWEBVIEW_NAV_ACTION_NONE,
    wxWEBVIEW_NAV_ACTION_USER,
    wxWEBVIEW_NAV_ACTION_OTHER
};

class wxWebView : public wxControl
{
public:
    static wxWebView* New( const wxString& = wxWebViewBackendDefault ) { return nullptr; }

    bool Create( wxWindow*, wxWindowID, const wxString& = wxEmptyString,
                 const wxPoint& = wxDefaultPosition, const wxSize& = wxDefaultSize,
                 long = 0, const wxString& = wxEmptyString )
    {
        return false;
    }

    void LoadURL( const wxString& ) {}
    void SetPage( const wxString&, const wxString& ) {}
    bool RunScriptAsync( const wxString&, void* = nullptr ) const { return false; }
    void RegisterHandler( wxSharedPtr<wxWebViewHandler> ) {}
    bool AddScriptMessageHandler( const wxString& ) { return false; }
    bool RemoveScriptMessageHandler( const wxString& ) { return false; }
    bool AddUserScript( const wxString&, int = 0 ) { return false; }
    bool SetUserAgent( const wxString& ) { return false; }
};

class wxWebViewEvent : public wxNotifyEvent
{
public:
    wxWebViewEvent() = default;
    wxEvent* Clone() const override { return new wxWebViewEvent( *this ); }

    const wxString& GetURL() const { return m_url; }
    const wxString& GetTarget() const { return m_target; }
    const wxString& GetMessageHandler() const { return m_handler; }
    wxWebViewNavigationActionFlags GetNavigationAction() const { return wxWEBVIEW_NAV_ACTION_NONE; }
    bool IsError() const { return false; }

private:
    wxString m_url;
    wxString m_target;
    wxString m_handler;
};

inline const wxEventTypeTag<wxWebViewEvent> wxEVT_WEBVIEW_NAVIGATING( wxNewEventType() );
inline const wxEventTypeTag<wxWebViewEvent> wxEVT_WEBVIEW_NAVIGATED( wxNewEventType() );
inline const wxEventTypeTag<wxWebViewEvent> wxEVT_WEBVIEW_LOADED( wxNewEventType() );
inline const wxEventTypeTag<wxWebViewEvent> wxEVT_WEBVIEW_ERROR( wxNewEventType() );
inline const wxEventTypeTag<wxWebViewEvent> wxEVT_WEBVIEW_NEWWINDOW( wxNewEventType() );
inline const wxEventTypeTag<wxWebViewEvent> wxEVT_WEBVIEW_SCRIPT_MESSAGE_RECEIVED( wxNewEventType() );
inline const wxEventTypeTag<wxWebViewEvent> wxEVT_WEBVIEW_SCRIPT_RESULT( wxNewEventType() );
#endif

#endif
