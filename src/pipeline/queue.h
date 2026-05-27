#ifndef FRAMEPROBE_PIPELINE_QUEUE_H
#define FRAMEPROBE_PIPELINE_QUEUE_H

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>

namespace frameprobe {

enum class Overflow { kBlock, kDrop };

// A bounded multi-producer/multi-consumer queue.
//
//   kBlock: push() blocks until space is available (back-pressure). No frame is
//           ever lost.
//   kDrop:  push() drops the new item when full and increments drop_count.
//
// close() wakes all blocked threads; after close, pop() drains remaining items
// then returns nullopt so consumers can exit cleanly.
template <typename T>
class BoundedQueue {
  public:
    explicit BoundedQueue(std::size_t capacity, Overflow policy = Overflow::kBlock)
        : capacity_(capacity == 0 ? 1 : capacity), policy_(policy) {
    }

    // Returns true if the item was enqueued, false if dropped or the queue is
    // closed.
    bool push(T value) {
        std::unique_lock<std::mutex> lk(mu_);
        if (policy_ == Overflow::kDrop) {
            if (q_.size() >= capacity_) {
                drop_count_.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
        } else {
            not_full_.wait(lk, [&] { return q_.size() < capacity_ || closed_; });
            if (closed_)
                return false;
        }
        q_.push(std::move(value));
        lk.unlock();
        not_empty_.notify_one();
        return true;
    }

    // Blocks until an item is available or the queue is closed and drained.
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lk(mu_);
        not_empty_.wait(lk, [&] { return !q_.empty() || closed_; });
        if (q_.empty())
            return std::nullopt;
        T value = std::move(q_.front());
        q_.pop();
        lk.unlock();
        not_full_.notify_one();
        return value;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lk(mu_);
            closed_ = true;
        }
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return q_.size();
    }
    std::size_t capacity() const {
        return capacity_;
    }
    std::uint64_t drop_count() const {
        return drop_count_.load(std::memory_order_relaxed);
    }

  private:
    mutable std::mutex mu_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::queue<T> q_;
    const std::size_t capacity_;
    const Overflow policy_;
    bool closed_ = false;
    std::atomic<std::uint64_t> drop_count_{0};
};

}  // namespace frameprobe

#endif  // FRAMEPROBE_PIPELINE_QUEUE_H
