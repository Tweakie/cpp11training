#include "stdafx.h"

#include <gtest/gtest.h>

#include <thread>
#include <future>
#include <algorithm>

#include "perftest.h"

std::chrono::milliseconds operator"" _ms(unsigned long long count) {
    return std::chrono::milliseconds(count);
}

class AsyncTest : public ::testing::Test {
public:
    AsyncTest()
    {}

    class Events {
    public:
        struct Event {
            std::string source;
            std::string message;
            bool operator == (const Event &other) const {
                return source == other.source && message == other.message;
            }
        };
        void push(Event s) {
            std::lock_guard<std::mutex> guard(mutex);
            events.emplace_back(std::move(s));
        }

        int index(const Event &e) {
            std::lock_guard<std::mutex> guard(mutex);
            auto it = std::find(std::begin(events), std::end(events), e);
            return distance(std::begin(events), it);
        }

        std::mutex mutex;
    private:
        std::vector<Event> events;
    };

    Events events;

    auto get(std::string url)
    {
        events.push({"get: "+url, "entry"});
        std::this_thread::sleep_for(1000_ms);
        events.push({"get: " + url, "returning" });
        return "results for " + url;
    }

    template<class ...Args>
    auto post(std::string url, const Args... args)
    {
        events.push({ "post: " + url, "entry" });
        std::this_thread::sleep_for(1000_ms);
        events.push({ "post: " + url, "exit" });
        return "posted to " + url;
    }

    template<class Urls>
    auto get_parallel(const Urls &urls)
    {
        events.push({ "get_parallel: ", "entry" });

        std::vector<std::future<std::string>> futures;
        for (const auto &url : urls)
        {
            futures.emplace_back(std::async([=] { return get(url); }));
        }
        events.push({ "get_parallel: ", "exit" });
        return futures;
    }

    const std::vector<std::string> urls{
        "http://google.com", "http://yahoo.com", "http://sioux.eu"
    };
};

TEST_F(AsyncTest, we_can_delegate_stuff)
{
    EXPECT_GT(2 * 1000_ms, duration([=] {get_parallel(urls); }, 1));
}

TEST_F(AsyncTest, DISABLED_we_can_wait_for_delegated_stuff)
{
    auto google = get("http://google.com");
    auto correct = post("http://spell_checker.com", google);

    //EXPECT_TRUE(correct);
    EXPECT_LT(
        events.index({ "get: http://google.com", "exit" }),
        events.index({ "post: http://spell_checker.com", "entry" }));
}

TEST_F(AsyncTest, we_can_delay_execution_till_input_is_known)
{
    const auto task = [=](std::future<int> &n_future) {
        const auto n = n_future.get();
        events.push({ "task: n received: " + std::to_string(n), "" });
        for (int i = 0; i != n; ++i) {
            std::this_thread::sleep_for(100_ms);
        }
        events.push({ "task returns " + std::to_string(n), "" });
        return n;
    };

    std::promise<int> input_promise;
    auto fut = input_promise.get_future();
    auto result_fut = std::async(std::launch::async,
        task, std::ref(fut));

    std::async(std::launch::async, [&] {
        events.push({ "input defined", "" });
        input_promise.set_value(10);
    });

    const auto result = result_fut.get();
    events.push({ "{return value known: " + std::to_string(result), "" });

    EXPECT_EQ(10, result);
    EXPECT_LT(events.index({ "input defined", "" }), events.index({ "task: n received: 10", "" }));
    EXPECT_LT(events.index({ "task returns 10", "" }), events.index({ "{return value known: 10", "" }));
}
