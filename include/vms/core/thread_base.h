/*
    Library Utilities - Copyright (C) 2025 Manuel Virgilio
    This file is part of a project licensed under the terms
    of the LGPLv3 + Attribution. See LICENSE for details.
*/

#pragma once

#include <thread>
#include <cstdint>
#include <pthread.h>
#include <sched.h>
#include <atomic>
#include <mutex>

namespace vms::core
{
    enum class ThreadSchedulingPolicy : int
    {
        OTHER = SCHED_OTHER,
        RR = SCHED_RR,
        FIFO = SCHED_FIFO,
        BATCH = SCHED_BATCH,
        IDLE = SCHED_IDLE
    };

    /**
     * @brief Thread object providing a basic loop + lifecycle management.
     */
    class Thread
    {
    public:
        /** @brief Construct an idle thread object (no worker started yet). */
        Thread();

        /** @brief Ensure the worker is stopped and joined before destruction. */
        virtual ~Thread();

        /**
         * @brief Start the worker loop by spawning a new std::thread.
         * 
         * @return true thread starts successfully
         * @return false thread already running
         */
        bool start ();

        /**
         * @brief Request the worker loop to stop and optionally join the thread.
         *
         * @param bWaitJoin join the internal thread before returning
         */
        void stop (bool bWaitJoin = true);

        static bool set_process_priority (int priority, ThreadSchedulingPolicy policy);

    protected:
        /** @brief Called before the loop starts; returning false aborts the run. */
        virtual bool init();

        /** @brief Called after the loop exits to release resources. */
        virtual void uninit();

        /** @brief Actual work for the thread body, must be implemented. */
        virtual void run() = 0;

        /** @brief Hook invoked before each run() iteration. */
        virtual void pre_run();

        /** @brief Hook invoked after each run() iteration. */
        virtual void post_run();

    private:
        /**
         * @brief execution loop, the one that calls run() and check exit conditions
         * 
         */
        void loop ();

        /** @brief Underlying std::thread handle. */
        std::thread thread_;

        /** @brief Stop flag toggled by start()/stop(). */
        std::atomic<bool> stop_flag_;

        /** @brief Protects thread_ and state transitions. */
        mutable std::mutex state_mutex_;
    };
}
