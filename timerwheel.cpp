#include "timerwheel.h"
#include <time.h>
#include <iostream>
 

namespace pandaknight{
    namespace TimerWheel{

#define OFFSET(N) (TVR_SIZE + (N) * TVN_SIZE)
#define INDEX(V, N) ((V >> (TVR_BITS + (N) * TVN_BITS)) & TVN_MASK)


TimerManager::TimerManager()
{
    _tick = GetCurrentMillisecs();
    std::thread([this]{
            while(1){
                {
                    std::unique_lock<std::mutex> lk(this->mu);
                    this->cv.wait(lk, [this]{return this->_exit || !this->q.empty();});
                }
                if(this->_exit) break;

                while(!this->q.empty()){
                    auto fn = q.front();
                    fn();
                    q.pop();
                }
            }
        }).detach();
}
 
TimerPtr TimerManager::SetTimer(uint32 msDelay, uint32 msPeriod,TimerCallback timer_cb, void* arg)
{
    if(timer_cb == NULL) return nullptr;

    TimerPtr timer = std::make_shared<Timer>();

    uint64 t0 = GetCurrentMillisecs();
    
    if(msDelay > 0){
        timer->expires = t0 + msDelay;
    }else{
        timer->expires = t0 + msPeriod;
    }
    timer->period = msPeriod;
    timer->timer_cb = timer_cb;
    timer->param = arg;
    
    AddTimer(timer);
    
    return timer;
}

void TimerManager::AddTimer(const TimerPtr& timer){
    uint64 t0 = _tick;
    unsigned long long expires = timer->expires;
    unsigned long long dueTime = expires - t0;
    int i = 0;
    TimerList* tl = nullptr;

    if(timer->timer_cb == NULL) return;
   
    if (dueTime < TVR_SIZE)
    {
        i = expires & TVR_MASK;
        tl = _tvr + i;
    }
    else if (dueTime < 1 << (TVR_BITS + TVN_BITS))
    {
        i = INDEX(expires, 0);
        tl = &_tvn[0][0] + i;
    }
    else if (dueTime < 1 << (TVR_BITS + 2 * TVN_BITS))
    {
        i = INDEX(expires, 1);
        tl = &_tvn[1][0] + i;
    }
    else if (dueTime < 1 << (TVR_BITS + 3 * TVN_BITS))
    {
        i = INDEX(expires, 2);
        tl = &_tvn[2][0] + i;
    }
    else if ((long long) dueTime < 0)
    {
        i = t0 & TVR_MASK;
        tl = _tvr + i;
    }
    else
    {   
        if (dueTime > 0xffffffffUL)
        {
            dueTime = 0xffffffffUL;
            expires = dueTime + t0;
        }
        i = INDEX(expires, 3);
        tl = &_tvn[3][0] + i;
    }
 
    tl->push_back(timer);
}


void TimerManager::KillTimer(const TimerPtr& tmr)
{
    if (tmr != nullptr) {
        tmr->timer_cb = NULL;
    }
}


#define TIME_AFTER(a,b) ((long)(b) - (long)(a) < 0)
#define TIME_BEFORE(a,b) TIME_AFTER(b,a)
#define TIME_AFTER_EQ(a,b) ((long)(a) - (long)(b) >= 0)
#define TIME_BEFORE_EQ(a,b) TIME_AFTER_EQ(b,a)


void TimerManager::OnTick()
{
    uint64 now = GetCurrentMillisecs();
    int cnt = 0;
    while (TIME_BEFORE_EQ(_tick, now))
    {
        //当低级时间轮刻度为0时,从上一级时间轮对应槽中遍历定时器并级联到下N级各槽的链表中
    
        int index = _tick & TVR_MASK;
        if (!index &&
            !Cascade(0, INDEX(_tick, 0)) &&
            !Cascade(1, INDEX(_tick, 1)) &&
            !Cascade(2, INDEX(_tick, 2)))
        {
            Cascade(3, INDEX(_tick, 3));
        }
        ++_tick;
 
        TimerList& tlist = _tvr[index];
        TimerList temp;
        temp.splice(temp.end(), tlist);
        for (TimerList::iterator iter = temp.begin(); iter != temp.end(); ++iter)
        {
            TimerPtr& tmr = *iter;
            auto fn = tmr->timer_cb;
            if(fn)
            {
                if(tmr->period > 0){
                    tmr->expires = now + tmr->period;
                    AddTimer(tmr);
                }

                //tmr->timer_cb(tmr);                // sync 
                cnt++;
                std::unique_lock<std::mutex> lock(mu);
                q.emplace([fn,tmr]{
                    fn(tmr);
                });
            }
        }
    }
    if(cnt) cv.notify_one();
}
 
int TimerManager::Cascade(int offset, int index)
{
    TimerList& tlist = _tvn[offset][index];
    TimerList temp;
    temp.splice(temp.end(), tlist);
 
    for (TimerList::iterator iter = temp.begin(); iter != temp.end(); ++iter)
    {
        AddTimer(*iter);
    }
 
    return index;
}

TimerManager::~TimerManager()
{
    {
        std::unique_lock<std::mutex> lk(mu);
        _exit = true;
    }
    cv.notify_one();
    std::cout << "~TimerManager: q.size=" << q.size() << std::endl;
}
 
unsigned long long TimerManager::GetCurrentMillisecs()
{
#ifdef _MSC_VER
    _timeb timebuffer;
    _ftime(&timebuffer);
    unsigned long long ret = timebuffer.time;
    ret = ret * 1000 + timebuffer.millitm;
    return ret;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / (1e6);
#endif
}

}
}