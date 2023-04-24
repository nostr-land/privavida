//
//  timer.cpp
//  privavida-core
//
//  Created by Bartholomew Joyce on 2023-04-22.
//

#include "timer.hpp"
#include <app.hpp>
#include <chrono>
#include <vector>

static int schedule_timer(std::function<void()> callback, long duration_in_ms, bool repeat);
static void clear_timer(int timer_id);

int timer::set_timeout(std::function<void()> callback, long time_in_ms) {
    return schedule_timer(std::move(callback), time_in_ms, false);
}

void timer::clear_timeout(int timeout_id) {
    clear_timer(timeout_id);
}

int timer::set_interval(std::function<void()> callback, long time_in_ms) {
    return schedule_timer(std::move(callback), time_in_ms, true);
}

void timer::clear_interval(int interval_id) {
    clear_timer(interval_id);
}



struct Timer {
    int id;
    std::function<void()> callback;
    std::chrono::high_resolution_clock::duration duration;
    std::chrono::high_resolution_clock::time_point due_time;
    bool repeat;
    bool erase;
};


static std::vector<Timer> timers;
static int next_timer_id;

void timer::init() {
    next_timer_id = 1;
}

void timer::update() {
    auto current_time = std::chrono::high_resolution_clock::now();

    // Call all callbacks that are due
    for (auto& timer : timers) {
        if (timer.due_time <= current_time) {
            if (timer.repeat) {
                app::set_immediate(timer.callback);
                timer.due_time = timer.due_time + timer.duration;
            } else {
                app::set_immediate(std::move(timer.callback));
                timer.erase = true;
            }
        }
    }

    // Erase timeouts
    for (int i = (int)timers.size() - 1; i >= 0; --i) {
        if (timers[i].erase) {
            timers.erase(timers.begin() + i);
        }
    }

}

int schedule_timer(std::function<void()> callback, long duration_in_ms, bool repeat) {
    bool thread_is_idle = timers.empty();

    Timer timer;
    timer.id = next_timer_id++;
    timer.callback = std::move(callback);
    timer.duration = std::chrono::milliseconds(duration_in_ms);
    timer.due_time = std::chrono::high_resolution_clock::now() + timer.duration;
    timer.repeat = repeat;
    timer.erase = false;
    timers.push_back(std::move(timer));
    
    return timers.back().id;
}

void clear_timer(int timer_id) {
    for (int i = 0; i < timers.size(); ++i) {
        if (timers[i].id == timer_id) {
            timers.erase(timers.begin() + i);
            break;
        }
    }
}
