/*
    Library Utilities - Copyright (C) 2025 Manuel Virgilio
    This file is part of a project licensed under the terms
    of the LGPLv3 + Attribution. See LICENSE for details.
*/

#include <vms/core/thread_worker.h>

#include <thread>

namespace
{
    constexpr std::chrono::microseconds make_non_negative_duration(int32_t microseconds) noexcept
    {
        return std::chrono::microseconds{microseconds < 0 ? 0 : microseconds};
    }
}

namespace vms::core
{
    // --------------------------------------------------------------------- TimedThread

    TimedThread::TimedThread(int32_t micro_sec)
        : sleep_duration_(make_non_negative_duration(micro_sec))
    {
    }

    void TimedThread::pre_run()
    {
        if (sleep_duration_.count() > 0)
        {
            std::this_thread::sleep_for(sleep_duration_);
        }
    }

    // ------------------------------------------------------------- HiResTimedThread

    HiResTimedThread::HiResTimedThread(int32_t micro_sec)
        : loop_interval_(make_non_negative_duration(micro_sec))
        , next_deadline_{}
        , first_iteration_(true)
    {
    }

    void HiResTimedThread::pre_run()
    {
        if (loop_interval_.count() == 0)
        {
            return;
        }

        if (first_iteration_)
        {
            next_deadline_ = Clock::now() + loop_interval_;
            first_iteration_ = false;
        }
    }

    void HiResTimedThread::post_run()
    {
        if (loop_interval_.count() == 0)
        {
            return;
        }

        const auto now = Clock::now();

        if (now < next_deadline_)
        {
            std::this_thread::sleep_until(next_deadline_);
            next_deadline_ += loop_interval_;
        }
        else
        {
            next_deadline_ = now + loop_interval_;
        }
    }

    void HiResTimedThread::uninit()
    {
        first_iteration_ = true;
        next_deadline_ = Clock::time_point{};
        Thread::uninit();
    }
}
