#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <functional>
#include <vector>
#include <atomic>
#include <optional>
#include <string>
#include <queue>
 
namespace sync {
    template <typename T>
    class BlockingQueue {
        public:
            BlockingQueue() = default;
 
            void Put(T data) {
                std::lock_guard lk(m_);
                queue_.emplace_back(std::move(data));
                next_cv_.notify_all();
            }
 
            std::optional<T> Pop() {
                std::unique_lock lk(m_);
                if (queue_.empty()) {
                    while (!closed_ && queue_.empty()) {
                        next_cv_.wait(lk);
                    }
                }
                if (closed_ && queue_.empty()) {
                    return std::nullopt;
                } else {
                    auto last = std::move(queue_.front());
                    queue_.pop_front();
                    return std::move(last);
                }
            }
 
            bool Empty() {
                std::lock_guard lk(m_);
                return queue_.empty();
            }
            void Close() {
                std::lock_guard lk(m_);
                closed_ = true;
                next_cv_.notify_all();
            }
 
        private:
            std::deque<T> queue_;
            bool closed_{false};
 
            std::mutex m_;
            std::condition_variable next_cv_;
    };
}
 
 
namespace tp {
    class ThreadPool;
 
    using Task = std::function<void()>;
    thread_local ThreadPool* thread_pool = nullptr;
 
    ThreadPool* GetCurrentThreadPool() {
        return thread_pool;
    }
 
    class ThreadPool {
        public:
            ThreadPool(size_t threads_cnt) {
                for (size_t i = 0; i < threads_cnt; ++i) {
                    workers_.emplace_back([&](){
                        thread_pool = this;
                        std::optional<Task> task;
                        do {
                            if (task_queue_.Empty()) {
                                task_queue_.Close();
                            }
                            task = task_queue_.Pop();
 
                            if (task) {
                                task.value()();
                            } else if (joined_) {
                                task_queue_.Close();
                            }
 
                        } while (!joined_ || task);
                    });
                }
            }
 
            void Submit(Task task) {
                task_queue_.Put(task);
            }
 
            void Join() {
                joined_ = true;
                if (task_queue_.Empty()) {
                    task_queue_.Close();
                }
                for (auto&& worker : workers_) {
                    worker.join();
                }
            }
 
        private:
            std::atomic<bool> joined_{false};
            std::vector<std::thread> workers_;
            sync::BlockingQueue<Task> task_queue_;
    };
}
 
 
int main() {
 
    std::cout << "lol1";
    tp::ThreadPool pool(4);
    pool.Submit([]() {
        std::cout << "kek";
    });
    std::cout << "lol2";
 
    pool.Submit([]() {
        std::cout << "kek";
    });
 
    pool.Submit([]() {
        std::cout << "kek";
    });
 
    pool.Submit([]() {
        std::cout << "kek";
    });
 
    pool.Submit([]() {
        std::cout << "kek";
        tp::GetCurrentThreadPool()->Submit([]{
            std::cout << "Continue1";
            tp::GetCurrentThreadPool()->Submit([]{
                std::cout << "Continue2";
                tp::GetCurrentThreadPool()->Submit([]{
                    std::cout << "Continue3";
                });
            });
        });
    });
    std::cout << "lol";
    pool.Join();
 
    return 0;
}
