[英文](README.md) | 🇨🇳中文

## 📖 简介

TimerManager 是一个基于C11 实现的跨平台异步定时器管理器, 内部调度采用时间轮方式.接口简单,易于移植.只需周期调用OnTick()即可驱动时间轮运转



## 接口：

```cpp
//单例接口
static TimerManager& getInstance(); 

// 设置定时器
TimerPtr SetTimer(uint32 msDelay, uint32 msPeriod,TimerCallback timer_cb, void* arg);

//关闭定时器 
void KillTimer(const TimerPtr& tmr);

//返回当前 timebase tick
uint64 Now();

//定时timebase 步进函数, 需周期调用本函数完成定时器的调度管理,可采用select/sleep/或其他定时器等完成
void OnTick();

```




## 使用


``` cpp
#include "timerwheel.h"
#include <time.h>

using namespace pandaknight::TimerWheel;
using namespace std;

TimerManager* tmr;
TimerPtr timer;

bool stop = false;

int cnt = 0;

void cb(const TimerPtr& timer){
    cout << cnt++ << endl;
    if(stop) {
        cout << cnt++ << endl;
        tmr->KillTimer(timer);
    }
}


int main() {
    
    tmr = &TimerManager::getInstance();     // 获取TimerManager singleton
      
    std::thread([]{
        
        timer = tmr->SetTimer(0, 10, cb, &timer);               //启动异步定时器 period = 10 * time_base
        auto t0 = chrono::steady_clock::now(); 
        this_thread::sleep_for(std::chrono::seconds(1));
        stop = true;
    }).detach();
   
   
   while(!stop){
       this_thread::sleep_for(std::chrono::microseconds(100));    // time_base = 1ms
       tmr->OnTick();                                             //驱动定时器时钟步进
   }

}

```


## 📄 证书

源码允许用户在遵循 [MIT 开源证书](/LICENSE) 规则的前提下使用。

