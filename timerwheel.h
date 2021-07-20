#pragma once
#include <list>
#include <algorithm>    // std::make_heap, std::pop_heap, std::push_heap, std::sort_heap
#include <vector>       // std::vector
#include <queue>        // priority_queue
#include <functional>     // std::greater
#include <memory>       // shared_ptr
#include <chrono>
#include <thread>       // std::
#include <mutex>
#include <condition_variable>
#include <iostream>


namespace pandaknight{
    namespace TimerWheel{

typedef char int8;
typedef unsigned char uint8;
typedef uint8 byte;
typedef short int16;
typedef unsigned short uint16;
typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;


#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)
#define MAX_TVAL ((unsigned long)((1ULL << (TVR_BITS + 4*TVN_BITS)) - 1))


class Timer;
using TimerPtr = std::shared_ptr<Timer>;
typedef std::list<TimerPtr> TimerList;
using TimerCallback = void (*)(const TimerPtr& timer);

struct Timer{
    uint64 expires;
    uint32 period;
    TimerCallback timer_cb;
    void *param;
    
    ~Timer(){
        std::cout << "~Timer: " << period << std::endl;
    }
};


class TimerManager{
public:
    TimerManager(TimerManager const&)    = delete;
    void operator=(TimerManager const&)  = delete;


    static TimerManager& getInstance()
    {
        static TimerManager instance; // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }
 

    TimerPtr SetTimer(uint32 msDelay, uint32 msPeriod, TimerCallback cb, void* arg);
    void OnTick();

    void KillTimer(const TimerPtr& tmr);
   
    static uint64 Now(){
        return GetCurrentMillisecs();
    }

    ~TimerManager();

private:
    TimerManager();
    static unsigned long long GetCurrentMillisecs();
    int Cascade(int offset, int index);
    void AddTimer(const TimerPtr& tmr);

  
    
    uint64 _tick;
    TimerList _tvr[TVR_SIZE];
    TimerList _tvn[4][TVN_SIZE];

    std::mutex mu;
    std::condition_variable cv;
    std::queue<std::function<void()> > q;
    bool _exit;
};

    }

}


 