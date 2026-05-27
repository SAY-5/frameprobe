#include "pipeline/queue.h"

#include <gtest/gtest.h>

#include <atomic>
#include <set>
#include <thread>
#include <vector>

using namespace frameprobe;

TEST(BoundedQueue, FifoSingleThread) {
    BoundedQueue<int> q(4);
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_EQ(q.pop().value(), 1);
    EXPECT_EQ(q.pop().value(), 2);
}

TEST(BoundedQueue, CloseDrainsThenReturnsNullopt) {
    BoundedQueue<int> q(4);
    q.push(10);
    q.push(20);
    q.close();
    EXPECT_EQ(q.pop().value(), 10);
    EXPECT_EQ(q.pop().value(), 20);
    EXPECT_FALSE(q.pop().has_value());
}

TEST(BoundedQueue, DropPolicyCountsDrops) {
    BoundedQueue<int> q(2, Overflow::kDrop);
    EXPECT_TRUE(q.push(1));
    EXPECT_TRUE(q.push(2));
    EXPECT_FALSE(q.push(3));  // full -> dropped
    EXPECT_FALSE(q.push(4));  // full -> dropped
    EXPECT_EQ(q.drop_count(), 2u);
    EXPECT_EQ(q.size(), 2u);
}

TEST(BoundedQueue, BlockPolicyNeverLosesFramesUnderBackpressure) {
    constexpr int kItems = 2000;
    BoundedQueue<int> q(8, Overflow::kBlock);
    std::set<int> received;
    std::thread consumer([&] {
        while (auto v = q.pop()) {
            received.insert(*v);
        }
    });
    for (int i = 0; i < kItems; ++i) {
        ASSERT_TRUE(q.push(i));
    }
    q.close();
    consumer.join();
    EXPECT_EQ(received.size(), static_cast<std::size_t>(kItems));
    EXPECT_EQ(q.drop_count(), 0u);
}

TEST(BoundedQueue, MultiProducerMultiConsumerNoLoss) {
    constexpr int kProducers = 4;
    constexpr int kPerProducer = 1000;
    BoundedQueue<int> q(16);
    std::atomic<int> consumed{0};
    std::vector<std::thread> consumers;
    for (int c = 0; c < 3; ++c) {
        consumers.emplace_back([&] {
            while (q.pop())
                consumed.fetch_add(1, std::memory_order_relaxed);
        });
    }
    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&] {
            for (int i = 0; i < kPerProducer; ++i)
                q.push(i);
        });
    }
    for (auto& t : producers)
        t.join();
    q.close();
    for (auto& t : consumers)
        t.join();
    EXPECT_EQ(consumed.load(), kProducers * kPerProducer);
}
