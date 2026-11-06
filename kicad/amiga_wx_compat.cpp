/*
 * The parts of amiga_wx_compat.h that need code.
 * Copyright (c) 2026 AmiCradle Team
 * Designed, tested and delivered by The AmigaChrome Team
 */
#include <amiga_wx_compat.h>

#if !wxUSE_SNGLINST_CHECKER
#include <wx/app.h>
#include <wx/utils.h>
#include <exec/ports.h>
#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>

namespace
{
struct NAMED_PORT
{
    struct MsgPort port;
    char           name[64];
};
}


bool wxSingleInstanceChecker::Create( const wxString& aName, const wxString& )
{
    if( m_made )
        return true;

    m_made = true;

    // public port names are global: keep the name short and prefixed
    wxString name = wxS( "wxSingleInstance." ) + aName;
    wxCharBuffer utf = name.utf8_str();

    NAMED_PORT* np = static_cast<NAMED_PORT*>(
            AllocMem( sizeof( NAMED_PORT ), MEMF_PUBLIC | MEMF_CLEAR ) );

    if( !np )
        return false;

    strncpy( np->name, utf.data(), sizeof( np->name ) - 1 );
    np->port.mp_Node.ln_Type = NT_MSGPORT;
    np->port.mp_Node.ln_Name = np->name;
    np->port.mp_Flags = PA_IGNORE;
    // an empty list (what amiga.lib's NewList() does)
    struct List* l = &np->port.mp_MsgList;
    l->lh_Head = (struct Node*) &l->lh_Tail;
    l->lh_Tail = nullptr;
    l->lh_TailPred = (struct Node*) &l->lh_Head;

    Forbid();
    m_other = FindPort( (CONST_STRPTR) np->name ) != nullptr;

    if( !m_other )
        AddPort( &np->port );

    Permit();

    if( m_other )
        FreeMem( np, sizeof( NAMED_PORT ) );
    else
        m_port = np;

    return true;
}


bool wxSingleInstanceChecker::CreateDefault()
{
    if( !wxTheApp )
        return false;

    return Create( wxTheApp->GetAppName() + '-' + wxGetUserId() );
}


wxSingleInstanceChecker::~wxSingleInstanceChecker()
{
    if( m_port )
    {
        RemPort( &static_cast<NAMED_PORT*>( m_port )->port );
        FreeMem( m_port, sizeof( NAMED_PORT ) );
    }
}
#endif
