#include <iostream>
#include <future>
#include <thread>
#include <mutex>
#include <queue>
#include <optional>
 
template <class T>
class Promise;
 
template <class T>
class Future {
    friend Promise<T>;
public:
    explicit Future(Promise<T>& promise) {
        promise.associate(this);
        associated_promise_ = std::addressof(promise);
    }
 
    Future(Future&& other)
        : associated_promise_(other.associated_promise_),
          has_result_(other.has_result_),
          result_(std::move(other.result_)) {
        associated_promise_->associate(this);
    }
 
    T get() {
        std::unique_lock lk(m_);
        cv_.wait(lk, [&]() {
           return has_result_;
        });
        return std::move(*result_);
    }
 
private:
 
    void set_value(T value) {
        std::lock_guard lk(m_);
        result_ = value;
        has_result_ = true;
        cv_.notify_one();
    }
 
    std::mutex m_;
    std::condition_variable cv_;
    std::optional<T> result_;
    Promise<T>* associated_promise_;
    bool has_result_{false};
};
 
template <class T>
class Promise {
    friend Future<T>;
public:
    void set_value(T value) {
        associated_future_->set_value(std::move(value));
    }
 
    Future<T> get_future() {
        return Future<T>(*this);
    }
 
private:
    void associate(Future<T>* future_ptr) {
        associated_future_ = future_ptr;
    }
 
    Future<T>* associated_future_;
};
 
template <class T, class PromiseT = std::promise<T>, class FutureT = std::future<T>>
class ResultQueue {
public:
    class iterator {
    public:
        iterator(ResultQueue<T, PromiseT, FutureT>& queue) : queue_(queue) {}
 
        T&& operator * () {
            return std::move(*queue_.GetResult());
        }
 
        iterator operator ++() {
            return *this;
        }
 
        bool operator == (const iterator& other) const {
            return queue_.IsClosed() && other.queue_.IsClosed()
                   && queue_.Empty() && other.queue_.Empty();
        }
 
        bool operator != (const iterator& other) const {
            return !(*this == other);
        }
    private:
        ResultQueue<T, PromiseT, FutureT>& queue_;
    };
 
    ResultQueue() = default;
 
    void PushResult(PromiseT& data) {
        std::lock_guard lk(m_);
        results_.emplace_back(data.get_future());
        cv_.notify_all();
    }
 
    std::optional<T> GetResult() {
        std::unique_lock lk(m_);
 
        cv_.wait(lk, [&](){
            return closed_ || !results_.empty();
        });
        if (!results_.empty()) {
            auto result = results_.front().get();
            results_.pop_front();
            return result;
        }
        return std::nullopt;
    }
 
    void Close() {
        std::lock_guard lk(m_);
        closed_ = true;
    }
 
    bool IsClosed() const {
        return closed_;
    }
 
    bool Empty() const {
        return results_.empty();
    }
 
    iterator begin() {
        return iterator(*this);
    }
 
    iterator end() {
        return iterator(*this);
    }
 
private:
    std::mutex m_;
    std::condition_variable cv_;
    bool closed_{false};
    std::deque<FutureT> results_;
};
 
int main() {
    int res = 0;
    ResultQueue<int, Promise<int>, Future<int>> results;
 
    std::thread th([&]() {
        for (int i = 0; i < 10; ++i) {
            Promise<int> p{};
            res += 1;
            results.PushResult(p);
 
            p.set_value(res);
        }
        results.Close();
 
    });
 
    for (auto&& result : results) {
        std::cout << result << "\n";
    }
 
    th.join();
    return 0;
}
