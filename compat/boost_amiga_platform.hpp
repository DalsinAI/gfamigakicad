// Boost's platform file for AmigaOS 3 with libnix and libpthread, used in
// place of Boost's own amigaos.hpp (which turns threads off) through
// -DBOOST_PLATFORM_CONFIG. Copyright (c) 2026 AmiCradle Team
#pragma once
#define BOOST_PLATFORM "AmigaOS"
#define BOOST_HAS_UNISTD_H
#define BOOST_HAS_PTHREADS
#define BOOST_HAS_GETTIMEOFDAY
#define BOOST_HAS_SCHED_YIELD
#define BOOST_HAS_THREADS
#define BOOST_NO_CWCHAR_IS_FINE
#define BOOST_HAS_NANOSLEEP_IS_MISSING 1
// posix_features.hpp is not included: it includes <unistd.h> first, and
// libpthread's cancellation-point macros (read, close) then break <istream>.
