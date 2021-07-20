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
