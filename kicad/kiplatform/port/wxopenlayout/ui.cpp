/*
 * KiCad platform layer for the GFOpenWX Toolkit (wxWidgets on GFOpenLayout,
 * AmigaOS 3). wx's own calls do the work; Intuition needs no workarounds.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <kiplatform/ui.h>

#include <wx/cursor.h>
#include <wx/dialog.h>
#include <wx/nonownedwnd.h>
#include <wx/settings.h>
#include <wx/toplevel.h>
#include <wx/utils.h>
#include <wx/window.h>

bool KIPLATFORM::UI::IsDarkTheme()
{
    wxColour bg = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    double   brightness = ( bg.Red() / 255.0 ) * 0.299 + ( bg.Green() / 255.0 ) * 0.587
                        + ( bg.Blue() / 255.0 ) * 0.117;
    return brightness < 0.5;
}

wxColour KIPLATFORM::UI::GetDialogBGColour()
{
    return wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE );
}

void KIPLATFORM::UI::GetInfoBarColours( wxColour& aFGColour, wxColour& aBGColour )
{
    aBGColour = wxSystemSettings::GetColour( wxSYS_COLOUR_INFOBK );
    aFGColour = wxSystemSettings::GetColour( wxSYS_COLOUR_INFOTEXT );
}

void KIPLATFORM::UI::ForceFocus( wxWindow* aWindow )
{
    aWindow->SetFocus();
}

bool KIPLATFORM::UI::IsWindowActive( wxWindow* aWindow )
{
    if( !aWindow )
        return false;

    wxTopLevelWindow* tlw = dynamic_cast<wxTopLevelWindow*>( wxGetTopLevelParent( aWindow ) );
    return tlw && tlw->IsActive();
}

void KIPLATFORM::UI::EnsureVisible( wxWindow* ) {}
void KIPLATFORM::UI::ReparentModal( wxNonOwnedWindow* ) {}
void KIPLATFORM::UI::ReparentWindow( wxNonOwnedWindow*, wxTopLevelWindow* ) {}
void KIPLATFORM::UI::FixupCancelButtonCmdKeyCollision( wxWindow* ) {}

bool KIPLATFORM::UI::IsStockCursorOk( wxStockCursor aCursor )
{
    switch( aCursor )
    {
    case wxCURSOR_ARROW:
    case wxCURSOR_WAIT:
        return true;
    default:
        return false;
    }
}

void KIPLATFORM::UI::LargeChoiceBoxHack( wxChoice* ) {}
void KIPLATFORM::UI::EllipsizeChoiceBox( wxChoice* ) {}

double KIPLATFORM::UI::GetPixelScaleFactor( const wxWindow* aWindow )
{
    return aWindow->GetContentScaleFactor();
}

double KIPLATFORM::UI::GetContentScaleFactor( const wxWindow* aWindow )
{
    return aWindow->GetDPIScaleFactor();
}

wxSize KIPLATFORM::UI::GetUnobscuredSize( const wxWindow* aWindow )
{
    return aWindow->GetClientSize();
}

void KIPLATFORM::UI::SetOverlayScrolling( const wxWindow*, bool ) {}

bool KIPLATFORM::UI::AllowIconsInMenus()
{
    // Intuition menus show text only
    return false;
}

bool KIPLATFORM::UI::SupportsKeyState()
{
    return true;
}

wxPoint KIPLATFORM::UI::GetMousePosition()
{
    return wxGetMousePosition();
}

bool KIPLATFORM::UI::WarpPointer( wxWindow* aWindow, int aX, int aY )
{
    aWindow->WarpPointer( aX, aY );
    return true;
}

void KIPLATFORM::UI::ImmControl( wxWindow*, bool ) {}
void KIPLATFORM::UI::ImeNotifyCancelComposition( wxWindow* ) {}

bool KIPLATFORM::UI::InfiniteDragPrepareWindow( wxWindow* )
{
    return true;
}

void KIPLATFORM::UI::InfiniteDragReleaseWindow() {}
void KIPLATFORM::UI::SetFloatLevel( wxWindow* ) {}
void KIPLATFORM::UI::ReleaseChildWindow( wxNonOwnedWindow* ) {}
void KIPLATFORM::UI::AllowNetworkFileSystems( wxDialog* ) {}
void KIPLATFORM::UI::CancelPendingScroll( wxDataViewCtrl* ) {}
