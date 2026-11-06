// Checks the threading KiCad leans on at start-up, step by step, so a hang
// shows which step it is: std::thread, a condition variable, std::async and
// KiCad's BS::priority_thread_pool (as GetKiCadThreadPool makes it).
//   pooltest
// Copyright (c) 2026 AmiCradle Team. MIT.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <future>
#include <mutex>
#include <thread>
#include <bs_thread_pool.hpp>

static void step( const char* s )
{
    std::printf( "%s\n", s );
    std::fflush( stdout );
}

int main()
{
    std::printf( "hardware_concurrency %u\n", std::thread::hardware_concurrency() );
    step( "1 thread start" );
    {
        std::thread t( [] { step( "  in thread" ); } );
        t.join();
    }
    step( "1 joined" );

    step( "2 condition variable" );
    {
        std::mutex m;
        std::condition_variable cv;
        bool ready = false;
        std::thread t( [&] {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            std::lock_guard<std::mutex> l( m );
            ready = true;
            cv.notify_one();
        } );
        std::unique_lock<std::mutex> l( m );
        bool ok = cv.wait_for( l, std::chrono::seconds( 5 ), [&] { return ready; } );
        l.unlock();
        t.join();
        std::printf( "2 %s\n", ok ? "notified" : "TIMED OUT" );
        std::fflush( stdout );
    }

    step( "3 async" );
    {
        auto f = std::async( std::launch::async, [] { return 42; } );
        auto st = f.wait_for( std::chrono::seconds( 5 ) );
        std::printf( "3 %s\n", st == std::future_status::ready ? "ready" : "TIMED OUT" );
        std::fflush( stdout );
    }

    step( "4 thread pool" );
    {
        unsigned n = std::max( 2u, std::thread::hardware_concurrency() );
        BS::priority_thread_pool pool( n );
        std::printf( "4 pool of %u threads\n", (unsigned) pool.get_thread_count() );
        std::fflush( stdout );
        std::atomic<int> done{ 0 };
        std::vector<std::future<void>> fs;
        for( int i = 0; i < 16; i++ )
            fs.push_back( pool.submit_task( [&] { done++; } ) );
        int ready = 0;
        for( auto& f : fs )
            if( f.wait_for( std::chrono::seconds( 5 ) ) == std::future_status::ready )
                ready++;
        std::printf( "4 %d of 16 tasks ready, %d ran\n", ready, done.load() );
        std::fflush( stdout );
        pool.wait();
        step( "4 pool waited" );
    }
    step( "4 pool destroyed" );
    step( "all done" );
    return 0;
}
