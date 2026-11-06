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
