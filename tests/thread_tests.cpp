#include <vms/core/thread_worker.h>

#include <atomic>
#include <cstdint>
#include <chrono>
#include <iostream>
#include <sched.h>
#include <thread>
#include <vector>

namespace
{
    using TestClock = std::chrono::steady_clock;

    template <typename Predicate>
    bool wait_for_condition(Predicate&& predicate, std::chrono::milliseconds timeout)
    {
        const auto deadline = TestClock::now() + timeout;

        while (!predicate())
        {
            if (TestClock::now() >= deadline)
            {
                return false;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return true;
    }

    class LifecycleThread : public vms::core::Thread
    {
    public:
        explicit LifecycleThread(int target_iterations)
            : target_iterations_(target_iterations)
        {
        }

        bool init() override
        {
            init_calls_.fetch_add(1, std::memory_order_relaxed);
            return true;
        }

        void uninit() override
        {
            uninit_calls_.fetch_add(1, std::memory_order_relaxed);
        }

        void pre_run() override
        {
            pre_calls_.fetch_add(1, std::memory_order_relaxed);
        }

        void run() override
        {
            run_calls_.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            if (run_calls_.load(std::memory_order_acquire) >= target_iterations_)
            {
                stop(false);
            }
        }

        void post_run() override
        {
            post_calls_.fetch_add(1, std::memory_order_relaxed);
        }

        int init_calls() const { return init_calls_.load(std::memory_order_relaxed); }
        int uninit_calls() const { return uninit_calls_.load(std::memory_order_relaxed); }
        int pre_calls() const { return pre_calls_.load(std::memory_order_relaxed); }
        int post_calls() const { return post_calls_.load(std::memory_order_relaxed); }
        int run_calls() const { return run_calls_.load(std::memory_order_relaxed); }

    private:
        const int target_iterations_;

        std::atomic<int> init_calls_{0};
        std::atomic<int> uninit_calls_{0};
        std::atomic<int> pre_calls_{0};
        std::atomic<int> post_calls_{0};
        std::atomic<int> run_calls_{0};
    };

    class RecordingTimedThread : public vms::core::TimedThread
    {
    public:
        RecordingTimedThread(int32_t microseconds, size_t target_iterations)
            : vms::core::TimedThread(microseconds)
            , target_iterations_(target_iterations)
        {
            timestamps_.reserve(target_iterations);
        }

        void run() override
        {
            timestamps_.push_back(TestClock::now());
            std::this_thread::sleep_for(std::chrono::microseconds(200));

            if (timestamps_.size() >= target_iterations_)
            {
                done_.store(true, std::memory_order_release);
                stop(false);
            }
        }

        bool finished() const { return done_.load(std::memory_order_acquire); }
        const std::vector<TestClock::time_point>& timestamps() const { return timestamps_; }

    private:
        const size_t target_iterations_;
        std::vector<TestClock::time_point> timestamps_;
        std::atomic<bool> done_{false};
    };

    class FailingInitThread : public vms::core::Thread
    {
    public:
        bool init() override
        {
            init_called_.store(true, std::memory_order_release);
            return false;
        }

        void run() override
        {
            run_called_.store(true, std::memory_order_release);
        }

        bool init_called() const { return init_called_.load(std::memory_order_acquire); }
        bool run_called() const { return run_called_.load(std::memory_order_acquire); }

    private:
        std::atomic<bool> init_called_{false};
        std::atomic<bool> run_called_{false};
    };
    class RecordingHiResThread : public vms::core::HiResTimedThread
    {
    public:
        RecordingHiResThread(int32_t microseconds, size_t target_iterations)
            : vms::core::HiResTimedThread(microseconds)
            , target_iterations_(target_iterations)
        {
            timestamps_.reserve(target_iterations);
        }

        void run() override
        {
            timestamps_.push_back(TestClock::now());
            std::this_thread::sleep_for(std::chrono::microseconds(500));

            if (timestamps_.size() >= target_iterations_)
            {
                done_.store(true, std::memory_order_release);
                stop(false);
            }
        }

        bool finished() const { return done_.load(std::memory_order_acquire); }
        const std::vector<TestClock::time_point>& timestamps() const { return timestamps_; }

    private:
        const size_t target_iterations_;
        std::vector<TestClock::time_point> timestamps_;
        std::atomic<bool> done_{false};
    };

    bool test_thread_lifecycle()
    {
        LifecycleThread worker(5);

        if (!worker.start())
        {
            std::cerr << "[Thread] Unable to start worker\n";
            return false;
        }

        if (worker.start())
        {
            std::cerr << "[Thread] Should not start twice while running\n";
            worker.stop();
            return false;
        }

        const bool reached_target = wait_for_condition(
            [&]() { return worker.run_calls() >= 5; }, std::chrono::milliseconds(500));

        worker.stop();

        if (!reached_target)
        {
            std::cerr << "[Thread] Run loop did not reach target iterations\n";
            return false;
        }

        if (worker.pre_calls() != worker.post_calls())
        {
            std::cerr << "[Thread] pre_run/post_run call count mismatch: "
                      << worker.pre_calls() << " vs " << worker.post_calls() << '\n';
            return false;
        }

        if (worker.init_calls() != 1 || worker.uninit_calls() != 1)
        {
            std::cerr << "[Thread] init/uninit calls mismatch: "
                      << worker.init_calls() << " vs " << worker.uninit_calls() << '\n';
            return false;
        }

        if (!worker.start())
        {
            std::cerr << "[Thread] Failed to restart worker\n";
            return false;
        }

        worker.stop();

        if (worker.init_calls() != 2 || worker.uninit_calls() != 2)
        {
            std::cerr << "[Thread] Restart cycle did not trigger init/uninit\n";
            return false;
        }

        return true;
    }

    bool test_thread_init_failure()
    {
        FailingInitThread worker;

        if (!worker.start())
        {
            std::cerr << "[ThreadInitFail] Unable to start worker\n";
            return false;
        }

        const bool init_called = wait_for_condition(
            [&]() { return worker.init_called(); }, std::chrono::milliseconds(100));

        worker.stop();

        if (!init_called)
        {
            std::cerr << "[ThreadInitFail] init() was never invoked\n";
            return false;
        }

        if (worker.run_called())
        {
            std::cerr << "[ThreadInitFail] run() should not execute when init fails\n";
            return false;
        }

        return true;
    }

    bool test_set_process_priority()
    {
        const int invalid_priority = sched_get_priority_max(SCHED_FIFO) + 1;
        const bool result = vms::core::Thread::set_process_priority(
            invalid_priority, vms::core::ThreadSchedulingPolicy::FIFO);

        if (result)
        {
            std::cerr << "[SetProcessPriority] Expected failure for invalid priority\n";
            return false;
        }

        return true;
    }

    bool test_timed_thread_interval()
    {
        constexpr int32_t period_us = 2000; // 2ms sleep per iteration
        constexpr auto expected = std::chrono::microseconds(period_us);
        constexpr auto tolerance = std::chrono::microseconds(500);

        RecordingTimedThread worker(period_us, 5);

        if (!worker.start())
        {
            std::cerr << "[TimedThread] Unable to start worker\n";
            return false;
        }

        const bool finished = wait_for_condition(
            [&]() { return worker.finished(); }, std::chrono::milliseconds(500));

        worker.stop();

        if (!finished)
        {
            std::cerr << "[TimedThread] Worker did not complete in time\n";
            return false;
        }

        const auto& timestamps = worker.timestamps();
        if (timestamps.size() != 5)
        {
            std::cerr << "[TimedThread] Unexpected number of iterations recorded: "
                      << timestamps.size() << '\n';
            return false;
        }

        for (size_t i = 1; i < timestamps.size(); ++i)
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                timestamps[i] - timestamps[i - 1]);

            if (elapsed + tolerance < expected)
            {
                std::cerr << "[TimedThread] Interval too short: " << elapsed.count()
                          << "us (expected at least " << expected.count() << "us)\n";
                return false;
            }
        }

        return true;
    }

    bool test_hires_timed_thread_interval()
    {
        constexpr int32_t period_us = 5000; // 5ms loop period
        constexpr auto expected = std::chrono::microseconds(period_us);
        constexpr auto tolerance = std::chrono::microseconds(2000);

        RecordingHiResThread worker(period_us, 6);

        if (!worker.start())
        {
            std::cerr << "[HiResTimedThread] Unable to start worker\n";
            return false;
        }

        const bool finished = wait_for_condition(
            [&]() { return worker.finished(); }, std::chrono::milliseconds(1000));

        worker.stop();

        if (!finished)
        {
            std::cerr << "[HiResTimedThread] Worker did not complete in time\n";
            return false;
        }

        const auto& timestamps = worker.timestamps();
        if (timestamps.size() != 6)
        {
            std::cerr << "[HiResTimedThread] Unexpected iteration count: "
                      << timestamps.size() << '\n';
            return false;
        }

        for (size_t i = 1; i < timestamps.size(); ++i)
        {
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                timestamps[i] - timestamps[i - 1]);

            const auto delta = (elapsed > expected) ? (elapsed - expected) : (expected - elapsed);

            if (delta > tolerance)
            {
                std::cerr << "[HiResTimedThread] Interval deviation too large: "
                          << elapsed.count() << "us (expected " << expected.count()
                          << "us)\n";
                return false;
            }
        }

        return true;
    }
}

int main()
{
    struct TestEntry
    {
        const char* name;
        bool (*func)();
    };

    const TestEntry tests[] = {
        {"Thread lifecycle", &test_thread_lifecycle},
        {"Thread init failure", &test_thread_init_failure},
        {"Thread set process priority", &test_set_process_priority},
        {"TimedThread interval", &test_timed_thread_interval},
        {"HiResTimedThread interval", &test_hires_timed_thread_interval},
    };

    bool all_passed = true;

    for (const auto& test : tests)
    {
        if (!test.func())
        {
            std::cerr << "Test FAILED: " << test.name << '\n';
            all_passed = false;
        }
        else
        {
            std::cout << "Test passed: " << test.name << '\n';
        }
    }

    return all_passed ? 0 : 1;
}
