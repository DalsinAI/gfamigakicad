/* KiCad's web view cookies on the GFOpenWX Toolkit: it has no web view yet
 * (amiga_wx_compat.h's stand-in), so there are no cookies to keep.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad. */
#include <kiplatform/webview.h>

bool KIPLATFORM::WEBVIEW::SaveCookies( wxWebView*, const wxString& )
{
    return false;
}

bool KIPLATFORM::WEBVIEW::LoadCookies( wxWebView*, const wxString& )
{
    return false;
}

bool KIPLATFORM::WEBVIEW::DeleteCookies( wxWebView* )
{
    return false;
}
