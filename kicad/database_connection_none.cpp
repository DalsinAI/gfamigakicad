/*
 * KiCad's database libraries on AmigaOS 3: there is no ODBC driver manager
 * on the Amiga, so a connection never opens and says why. Takes the place
 * of common/database/database_connection.cpp (built on nanodbc).
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0-or-later, as KiCad.
 */
#include <database/database_connection.h>

const char* const traceDatabase = "KICAD_DATABASE";

namespace nanodbc
{
class connection
{
};
}

static const char* const s_noOdbc = "Database libraries need ODBC, which AmigaOS does not have.";

DATABASE_CONNECTION::DATABASE_CONNECTION( const std::string& aDataSourceName,
                                          const std::string& aUsername,
                                          const std::string& aPassword, int aTimeoutSeconds,
                                          bool aConnectNow ) :
        m_quoteChar( '"' )
{
    m_dsn = aDataSourceName;
    m_user = aUsername;
    m_pass = aPassword;
    m_timeout = aTimeoutSeconds;
    init();

    if( aConnectNow )
        Connect();
}


DATABASE_CONNECTION::DATABASE_CONNECTION( const std::string& aConnectionString,
                                          int aTimeoutSeconds, bool aConnectNow ) :
        m_quoteChar( '"' )
{
    m_connectionString = aConnectionString;
    m_timeout = aTimeoutSeconds;
    init();

    if( aConnectNow )
        Connect();
}


DATABASE_CONNECTION::~DATABASE_CONNECTION() = default;

void DATABASE_CONNECTION::init()
{
    m_cache = std::make_unique<DB_CACHE_TYPE>( 10, 1 );
}

void DATABASE_CONNECTION::SetCacheParams( int aMaxSize, int aMaxAge )
{
    if( !m_cache )
        return;

    m_cache->SetMaxSize( static_cast<size_t>( std::max( aMaxSize, 0 ) ) );
    m_cache->SetMaxAge( static_cast<time_t>( std::max( aMaxAge, 0 ) ) );
}

bool DATABASE_CONNECTION::Connect()
{
    m_lastError = s_noOdbc;
    return false;
}

bool DATABASE_CONNECTION::Disconnect()
{
    return true;
}

bool DATABASE_CONNECTION::IsConnected() const
{
    return false;
}

bool DATABASE_CONNECTION::CacheTableInfo( const std::string&, const std::set<std::string>&,
                                          const std::set<std::string>& )
{
    m_lastError = s_noOdbc;
    return false;
}

bool DATABASE_CONNECTION::getQuoteChar()
{
    return false;
}

std::string DATABASE_CONNECTION::columnsFor( const std::string& )
{
    return "*";
}

bool DATABASE_CONNECTION::SelectOne( const std::string&, const std::pair<std::string, std::string>&,
                                     ROW& )
{
    m_lastError = s_noOdbc;
    return false;
}

bool DATABASE_CONNECTION::selectAllAndCache( const std::string&, const std::string& )
{
    return false;
}

bool DATABASE_CONNECTION::SelectAll( const std::string&, const std::string&, std::vector<ROW>& )
{
    m_lastError = s_noOdbc;
    return false;
}
