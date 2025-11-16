/*
    Library Utilities - Copyright (C) 2025 Manuel Virgilio
    This file is part of a project licensed under the terms
    of the LGPLv3 + Attribution. See LICENSE for details.
*/

#include <vms/core/thread_base.h>

#include <utility>

namespace vms::core
{
    // Base Thread Implementation

    Thread::Thread()
        : stop_flag_(true)
    {}

    Thread::~Thread()
    {
        stop(true);
    }

    bool Thread::start ()
    {
        std::lock_guard<std::mutex> lock(state_mutex_);

        if (thread_.joinable())
        {
            return false;
        }

        stop_flag_.store(false, std::memory_order_release);

        try
        {
            thread_ = std::thread(&Thread::loop, this);
        }
        catch (...)
        {
            stop_flag_.store(true, std::memory_order_release);
            throw;
        }

        return true;
    }

    void Thread::stop (bool wait_join /*= true*/)
    {
        std::thread join_handle;
        bool should_join = wait_join;

        {
            std::lock_guard<std::mutex> lock(state_mutex_);

            stop_flag_.store(true, std::memory_order_release);

            if (!thread_.joinable())
            {
                return;
            }

            if (thread_.get_id() == std::this_thread::get_id())
            {
                should_join = false;
            }

            if (should_join)
            {
                join_handle = std::move(thread_);
            }
        }

        if (should_join && join_handle.joinable())
        {
            join_handle.join();
        }
    }

    bool Thread::init()
    {
        return true;
    }

    void Thread::uninit()
    {
    }

    void Thread::pre_run()
    {
    }

    void Thread::post_run()
    {
    }

    void Thread::loop()
    {
        if (!init())
        {
            stop_flag_.store(true, std::memory_order_release);
            return;
        }
        
        while  (!stop_flag_.load(std::memory_order_acquire))
        {
            pre_run();
            run();
            post_run();
        }

        uninit();
    }

    bool Thread::set_process_priority(int priority, ThreadSchedulingPolicy policy)
    {
        struct sched_param schedParam;
        schedParam.sched_priority = priority;

        const int sched_policy = static_cast<int>(policy);

        if (sched_setscheduler(0, sched_policy, &schedParam) == -1) {
            return false;
        }
        
        return true;
    }

}
