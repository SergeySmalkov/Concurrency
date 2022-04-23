#include <iostream>
#include <future>
#include <optional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include <functional>
#include <exception>
#include <stdexcept>
#include <string>

namespace sync{

    enum FutureStatus: std::int16_t{
        DEFERRED,
        TIMEOUT,
        READY,
        INVALID
    };

    std::ostream& operator << (std::ostream& out, FutureStatus fstat){
        switch(fstat){
            case DEFERRED :
                std::cout << "DEFERRED";
                break;
            case TIMEOUT :
                std::cout << "TIMEOUT";
                break;
            case READY :
                std::cout << "READY";
                break;
            case INVALID :
                std::cout << "INVALID";
                break;
            }
        return out;
    }

    class ThreadExitExecutor{
    public:
        ThreadExitExecutor() = default;
        ~ThreadExitExecutor(){
            for(auto &lambda : tasks_){
                lambda();
            }
        }
        void Submit(const std::function<void()> &func){
            tasks_.emplace_back(func);
        }
    private:
        std::vector<std::function<void()>> tasks_;
    };

    thread_local ThreadExitExecutor thread_exit_executor_{};

    template<typename T>
    class SharedState{
    public:
        std::mutex mt;
        std::condition_variable cv_;
        std::optional<T> value;
        std::exception_ptr e;
        FutureStatus fstate_;
    };

    template<typename T>
    class Future{
    public:
        explicit Future(std::shared_ptr<SharedState<T>> sptr_):shared_state_ptr_(sptr_){}

        T Get(){
            std::unique_lock<std::mutex> lk(shared_state_ptr_->mt);
            while(shared_state_ptr_->fstate_ != READY){
                shared_state_ptr_->cv_.wait(lk);
            }
            if(shared_state_ptr_->e){
                std::rethrow_exception(shared_state_ptr_->e);
            }
            shared_state_ptr_->fstate_ = INVALID;
            return *shared_state_ptr_->value;

        }

        void Wait(){
            std::unique_lock<std::mutex> lk(shared_state_ptr_->mt);
            while(shared_state_ptr_->fstate_ != READY){
                shared_state_ptr_->cv_.wait(lk);
            }
        }

        bool Valid(){
            std::lock_guard<std::mutex> lk(shared_state_ptr_->mt);
            return shared_state_ptr_->fstate_ != INVALID;
        }

        FutureStatus WaitFor(std::chrono::seconds s){
            std::unique_lock<std::mutex> lk(shared_state_ptr_->mt);

            shared_state_ptr_->cv_.wait_for(lk, s, [this](){
                return shared_state_ptr_->fstate_ == READY;
            });
            if(shared_state_ptr_->fstate_ != READY){
                return TIMEOUT;
            }
            return shared_state_ptr_->fstate_;
        }

    private:
        std::shared_ptr<SharedState<T>> shared_state_ptr_;
        FutureStatus state_;
    };

    template<typename T>
    class Promise{
    public:
        Promise():shared_state_ptr_(new SharedState<T>){}
        ~Promise() = default;

        Future<T> GetFuture(){
            Future<T> current_future (shared_state_ptr_);
            return current_future;
        }

        void SetValue(const T& value){
            std::lock_guard<std::mutex> lk(shared_state_ptr_->mt);
            shared_state_ptr_->value = value;
            shared_state_ptr_->fstate_ = READY;
            shared_state_ptr_->cv_.notify_one();
        }

        void SetValueAtThreadExit(const T& value){
            thread_exit_executor_.Submit([this, value](){
                SetValue(value);
            });
        }

        void SetException(const std::exception_ptr &p){
            std::lock_guard<std::mutex> lk(shared_state_ptr_->mt);
            shared_state_ptr_->e = p;
            shared_state_ptr_->fstate_ = READY;
            shared_state_ptr_->cv_.notify_one();
        }

        void SetExceptionAtThreadExit(std::exception_ptr p){
            thread_exit_executor_.Submit([this, p = std::move(p)](){
                SetException(p);
            });

        }
    private:
        std::shared_ptr<SharedState<T>> shared_state_ptr_;
    };
}




int main() {
    sync::Promise<int> p;
    sync::Future<int> f = p.GetFuture();
    std::thread t1 ([&p, &f](){
        size_t j = 0;
        try{
            for(size_t i = 0; i < 100; ++i){
                ++j;
            }

            throw std::runtime_error("Run");
        } catch (...){
                p.SetExceptionAtThreadExit(std::current_exception());
            }
        //p.SetValue(j);
        sync::FutureStatus state = f.WaitFor(std::chrono::seconds(2));
        std::cout << state << " ";
        //p.SetValueAtThreadExit(j);

    });


    std::cout << f.Get();

    t1.join();

    return 0;
}
