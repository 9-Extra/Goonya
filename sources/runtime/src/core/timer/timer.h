#pragma once
#include <chrono>
#include <sys/stat.h>

namespace Goonya {

// 计时器，获取的时间单位为毫秒，使用float类型
class Timer{
private:
    using TimePoint = std::chrono::time_point<std::chrono::high_resolution_clock>;

    static TimePoint init;
	static TimePoint last;

    static float delta_time;
    static float total_time;

public:
    static void initialize(){
        init = TimePoint::clock::now();
        last = init;
    }

    static float delta(){
        return delta_time;
    }

    static float total(){
        return total_time;
    }

    static void tick_update() {
		TimePoint now = std::chrono::high_resolution_clock::now();
		delta_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>((now - last)).count();
		last = now;
        total_time = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>((now - init)).count();
	}

    static void drop() {}
};
}