#include "stdafx.h"

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <algorithm>

#include "perftest.h"

TEST(move, speed_this_up)
{
    struct Heavy {
        int * data = nullptr;
        size_t size = 0;
        Heavy() {}
        explicit Heavy(int size)
            : data(new int[size])
            , size(size)
        {}
        Heavy(const Heavy &other)
            : data(new int[other.size])
            , size(other.size)
        {}
        Heavy(Heavy &&other)
        {
            *this = std::move(other);
        }
        Heavy& operator=(Heavy&& other)
        {
            std::swap(data, other.data);
            std::swap(size, other.size);
        }
        ~Heavy() { delete[] data; }
    };

    auto consume = [](std::vector<Heavy> prototype)
    {
        auto copy = std::move(prototype);
    };

    auto d = duration([&]
    {
        std::vector<Heavy> prototype{ 100, Heavy{1000} }; // a 100 copies
        consume(std::move(prototype));
    }, 10'000);

    EXPECT_LE(
        d,
        std::chrono::milliseconds(1000)) << d;
}



TEST(move, there_should_be_only_one_owner)
{
    class Resource {
    public:
        Resource(std::string id)
            : id(std::move(id))
        {}

        std::string id;
    };

    class Pool {
    public:
        Pool()
            : resources{ {Resource{"one"}, Resource{"two"}} }
        {}
        size_t size() const {
            return resources.size();
        }
        
        Resource borrow() {
            Resource r = std::move(resources.front());
            resources.erase(resources.begin());
            return r;
        }
        
        void return_(Resource &&r) {
            resources.emplace_back(std::move(r));
        }

        bool contains(const std::string &id) const
        {
            return std::any_of(begin(resources), end(resources),
                [&](const auto &resource) { return resource.id == id; });
        }
    private:
        std::vector<Resource> resources;
    };


    Pool pool;
    auto r1 = pool.borrow();
    EXPECT_EQ("one", r1.id);
    EXPECT_EQ(1u, pool.size());
    EXPECT_FALSE(pool.contains("one"));
    EXPECT_TRUE(pool.contains("two"));

    pool.return_(std::move(r1));
    EXPECT_EQ(2u, pool.size());
    EXPECT_TRUE(pool.contains("one")); 
    EXPECT_TRUE(pool.contains("two"));
}