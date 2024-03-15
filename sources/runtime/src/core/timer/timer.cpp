#include "timer.h"

namespace Goonya {

Timer::TimePoint Timer::init;
Timer::TimePoint Timer::last;
float Timer::delta_time;
float Timer::total_time;

}