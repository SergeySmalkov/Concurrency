//
// Created by cheytakker on 18.01.2022.
//

#ifndef THREADPOOL_THREADPOOL_H
#define THREADPOOL_THREADPOOL_H

#include <iostream>
#include <thread>
#include <utility>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <optional>
#include <functional>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <list>
#include <any>

enum status: std::int16_t
{
    PLANNED = 0,
    WAITING_CONTINUATION = 1,
    RUNNING = 2,
    FINISHED = 3,
    WAITING = 4
};

template <typename Task>
class TaskData;

template<typename Task>
class TaskPlanner{
public:
    std::optional<TaskData<Task>> GetNext(){
        std::unique_lock<std::mutex> lk(mt_);
        while(tasks_.empty() && !closed_){
            cv_.wait(lk);
        }
        if(tasks_.empty()){
            return {};
        }

        auto task = std::move(tasks_.front());
        tasks_.pop_front();
        cv_.notify_all();
        return task;
    }

    void Close(bool destroy){
        std::lock_guard<std::mutex> lk(mt_);
        closed_ = true;
        if(destroy){
            tasks_.clear();
        }
        cv_.notify_all();

    }

    void Close(){
        Close(false);
    }

    void Cancel(){
        Close(true);
    }

    size_t PlanTask(const Task &tsk){
        std::lock_guard<std::mutex> lk(mt_);
        if(closed_){
            return 0;
        }
        tasks_.emplace_back(last_id, std::move(tsk));
        cv_.notify_one();
        task_statuses_[last_id] = PLANNED;
        return last_id++;
    }

    bool Empty(){
        std::lock_guard<std::mutex> lk(mt_);
        return tasks_.empty() && linked_tasks_.empty();
    }

    void SetStatus(size_t id, status state){
        std::lock_guard<std::mutex> lk(mt_);
        task_statuses_[id] = state;
    }

    std::int16_t GetStatus(size_t id) {
        std::lock_guard<std::mutex> lk(mt_);
        return task_statuses_[id];
    }

    void PLanContinuations(size_t id){
        std::lock_guard<std::mutex> lk(mt_);
        auto it = linked_tasks_.find(id);
        if (it != linked_tasks_.end()){
            for(auto &&tsk : it->second){
                tasks_.emplace_back(std::move(tsk));
                task_statuses_[tasks_.back().id] = WAITING_CONTINUATION;
            }
            linked_tasks_.erase(it);

            cv_.notify_all();
        }
    }

    size_t SubmitContinuation(size_t id, const std::function<void()> &func){
        std::lock_guard<std::mutex> lk(mt_);
        if(task_statuses_.find(id) == task_statuses_.end()){
            return 0;
        }
        auto it = linked_tasks_.find(id);
        if (it == linked_tasks_.end()){
            linked_tasks_[id] = {};
        }
        task_statuses_[last_id] = WAITING_CONTINUATION;
        linked_tasks_[id].emplace_back(last_id++, func);
        return last_id - 1;
    }

    void Wait(size_t id_previous, size_t id_current){
        std::unique_lock<std::mutex> lk(mt_);
        while(task_statuses_[id_previous] != FINISHED){
            task_statuses_[id_current] = WAITING;
            cv_.wait(lk);
        }
        task_statuses_[id_current] = RUNNING;
    }

    void AwakeAll(){
        cv_.notify_all();
    }

    template<typename T>
    void SetResult(size_t id, T result){
        std::lock_guard<std::mutex> lk(mt_results_);
        task_results_[id] = result;
        cv_results_.notify_all();
    }

    std::any GetResult(size_t id){
        std::unique_lock<std::mutex> lk(mt_results_);
        while(task_results_.find(id) == task_results_.end()){
            cv_results_.wait(lk);
        }
        std::any result = task_results_[id];
        task_results_.erase(id);
        return result;
    }

private:
    std::unordered_map<size_t, std::list<TaskData<Task>>> linked_tasks_;
    std::unordered_map<size_t, std::int16_t> task_statuses_;
    std::unordered_map<size_t, std::any> task_results_;
    size_t last_id{1};
    std::deque<TaskData<Task>> tasks_{};
    bool closed_ {false};
    std::mutex mt_;
    std::mutex mt_results_;
    std::condition_variable cv_;
    std::condition_variable cv_results_;
};

template<typename Task>
class TaskData{
public:
    Task lambda;
    size_t id;
    TaskData(size_t id, std::function<void()> lambda):lambda(std::move(lambda)), id(id){};
    TaskData& operator = (const TaskData & other_task){
        if (this == std::addressof(other_task)) {
            return *this;
        }
        lambda = other_task.lambda;
        id = other_task.id;
        return *this;
    }
    TaskData(const TaskData &other_task) = default;
    TaskData(TaskData && other_task) noexcept = default;
    TaskData& operator = (TaskData&& other_task) noexcept{
        if (this == std::addressof(other_task)) {
            return *this;
        }
        lambda = std::move( other_task.lambda);
        id = other_task.id;
        return *this;
    }

};

class ThreadPool;
thread_local ThreadPool* thread_pool_pointer{nullptr};
thread_local size_t task_id{0};

class ThreadPool{
public:
    ThreadPool(size_t CountWorkers){
        RunWorkers(CountWorkers);
    }

    void Join(){
        joined_ = true;
        for(auto &worker : workers_){
            worker.join();
        }
    }

    size_t Submit(const std::function<void()> &func){
        return taskplanner_.PlanTask(func);
    }

    size_t SubmitContinuation(size_t id, const std::function<void()> &func){
        return taskplanner_.SubmitContinuation(id, func);
    }

    size_t SubmitContinuation(const std::function<void()> &func){
        return SubmitContinuation(task_id, func);
    }

    void Wait (size_t id_previous, size_t id_current){
        taskplanner_.Wait(id_previous, id_current);
    }

    template<typename T>
    void SetResult(size_t id, T result){
        taskplanner_.SetResult(id, result);
    }


    std::any GetResult(size_t id){
        return taskplanner_.GetResult(id);
    }


private:
    void RunWorkers(size_t CountWorkers){
        for(size_t i = 0; i < CountWorkers; ++i){
            workers_.emplace_back([this](){
                thread_pool_pointer = this;
                Worker();
                thread_pool_pointer = nullptr;
            });
        }
    }

    void Worker(){
        decltype(taskplanner_.GetNext()) task;
        do {
            if (taskplanner_.Empty() && joined_) {
                break;
            }

            task = taskplanner_.GetNext();
            if (task) {
                task_id = task->id;
                taskplanner_.SetStatus(task->id, RUNNING);
                task->lambda();
                taskplanner_.SetStatus(task->id, FINISHED);
                taskplanner_.AwakeAll();
                taskplanner_.PLanContinuations(task->id);
            } else if (joined_) {
                taskplanner_.Close();
            }
        } while (task || !joined_);
    }
    TaskPlanner<std::function<void()>> taskplanner_;
    std::vector<std::thread> workers_;
    std::atomic<bool> joined_;
};



ThreadPool* GetCurrentThreadPool(){
    return thread_pool_pointer;
}

size_t SubmitContinuation(const std::function<void()> &func){
    return GetCurrentThreadPool()->SubmitContinuation(func);
}

size_t SubmitContinuation(size_t id, const std::function<void()> &func){
    return GetCurrentThreadPool()->SubmitContinuation(id, func);
}

void Wait(size_t id_previous){
    GetCurrentThreadPool()->Wait(id_previous, task_id);
}

template<typename T>
void SetResult(T result){
    GetCurrentThreadPool()->SetResult(task_id, result);
}

std::any GetResult(size_t id){
    return GetCurrentThreadPool()->GetResult(id);
}

#endif //THREADPOOL_THREADPOOL_H
