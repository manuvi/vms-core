/*
    Library Utilities - Copyright (C) 2025 Manuel Virgilio
    This file is part of a project licensed under the terms
    of the LGPLv3 + Attribution. See LICENSE for details.
*/

#pragma once

#include <chrono>

#include <vms/core/thread_base.h>

namespace vms::core
{
    /**
     * @brief Periodically sleeps before each iteration of the worker loop.
     *
     * The class simply enforces a fixed delay (in microseconds) right before
     * @ref Thread::run is invoked. It is useful when the actual run()
     * implementation represents a burst of work that must be throttled.
     */
    class TimedThread : public Thread
    {
    public:
        /**
         * @brief Construct a timed thread using the provided delay.
         *
         * @param micro_sec Delay expressed in microseconds. Values below zero
         *                  are treated as zero (no sleep).
         */
        explicit TimedThread(int32_t micro_sec);
        ~TimedThread() override = default;

    protected:
        void pre_run() override;

    private:
        std::chrono::microseconds sleep_duration_;
    };

    /**
     * @brief Worker that attempts to keep a high precision, fixed-rate loop.
     *
     * After each iteration the class compensates for the work time in order
     * to maintain the requested period. High precision is achieved by using
     * @c std::chrono::steady_clock and @c sleep_until.
     */
    class HiResTimedThread : public Thread
    {
    public:
        /**
         * @brief Construct a high-resolution timed thread.
         *
         * @param micro_sec Loop period expressed in microseconds. Non-positive
         *                  values disable the extra sleeping logic.
         */
        explicit HiResTimedThread(int32_t micro_sec);
        ~HiResTimedThread() override = default;

    protected:
        /** @brief Capture the new deadline at the beginning of each loop. */
        void pre_run() override;
        /** @brief Sleep until the next deadline, compensating for work duration. */
        void post_run() override;
        /** @brief Reset timing state whenever the worker stops. */
        void uninit() override;

    private:
        using Clock = std::chrono::steady_clock;

        std::chrono::microseconds loop_interval_;
        Clock::time_point next_deadline_;
        bool first_iteration_;
    };
}
