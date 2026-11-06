/*
 * KiCad's Python scripting layer for AmigaOS, where there is no Python yet:
 * the same calls, none of which starts an interpreter. Action plugins and the
 * scripting console stay hidden because IsWxAvailable() is false.
 * Copyright (c) 2026 AmiCradle Team
 * Designed, tested and delivered by The AmigaChrome Team
 */
#include <python_scripting.h>

SCRIPTING::SCRIPTING() : m_python_thread_state( nullptr ) {}
SCRIPTING::~SCRIPTING() {}

bool SCRIPTING::IsWxAvailable() { return false; }
bool SCRIPTING::IsModuleLoaded( std::string& ) { return false; }
bool SCRIPTING::scriptingSetup() { return false; }

wxString SCRIPTING::PyScriptingPath( PATH_TYPE ) { return wxEmptyString; }
wxString SCRIPTING::PyPluginsPath( PATH_TYPE ) { return wxEmptyString; }

wxString PyEscapeString( const wxString& aSource ) { return aSource; }
void UpdatePythonEnvVar( const wxString&, const wxString& ) {}
wxString PyStringToWx( PyObject* ) { return wxEmptyString; }
wxArrayString PyArrayStringToWx( PyObject* ) { return wxArrayString(); }
wxString PyErrStringWithTraceback() { return wxEmptyString; }
