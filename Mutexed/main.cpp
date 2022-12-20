#include <iostream>
#include <mutex>
#include <vector>
#include <thread>
#include <cassert>
#include <sstream>

template<typename T>
class Wrapper {
public:
    Wrapper(std::mutex& mx_, T* object): lk_(mx_) , object_(object) {}
    T& operator *() {
        return *object_;
    }

    T* operator->() {
        return object_;
    };

private:
    T* object_;
    std::lock_guard<std::mutex> lk_;
};

template<typename T>
class Mutexed {
public:

    Mutexed() {}

    Wrapper<T> Lock()  {
        return Wrapper<T>(mx_, &object);
    }

private:
    T object;
    std::mutex mx_;
};

int main() {
    {
        Mutexed<std::vector<int>> vec;
    //    {
    //        auto ref = vec.Lock();
    //        (*ref).push_back(1);
    //        ref->push_back(2);
    //        std::cout << (*ref)[0];
    //        std::cout << (*ref)[1];
    //    }

        std::vector<std::thread> workers;
        for (size_t i = 0; i < 5; ++i) {
            workers.emplace_back([&]() {
                auto ref = vec.Lock();
                for (size_t j = 0; j < 10; ++j) {
                    ref->push_back(j);
                }
            });
        }
        for (size_t i = 0; i < 5; ++i) {
            workers[i].join();
        }
        auto ref = vec.Lock();

        std::stringstream ss;
        for(auto el : *ref) {
            ss << el;
        }
        assert(ss.str() == "01234567890123456789012345678901234567890123456789");
    }

    {
        Mutexed<std::vector<int>> vec;

        std::vector<std::thread> workers;
        for (size_t i = 0; i < 5; ++i) {
            workers.emplace_back([&]() {
                auto ref = vec.Lock();
                for (size_t j = 0; j < 1000000; ++j) {
                    ref->push_back(j);
                }
            });
        }
        for (size_t i = 0; i < 5; ++i) {
            workers[i].join();
        }
        auto ref = vec.Lock();
        assert(ref->size() == 5000000);
    }

    return 0;
}
