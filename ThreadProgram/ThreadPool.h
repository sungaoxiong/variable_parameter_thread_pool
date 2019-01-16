#pragma once

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace std
{
#define MAX_THREAD_NUM 256

    class  threadpool
    {
    private:
        using Task = std::function<void()>;

        // 线程池
        std::vector<std::thread> pool;

        // 任务队列
        std::queue<Task> tasks;

        //同步
        std::mutex m_lock;

        //条件阻塞
        std::condition_variable cv_task;

        // 是否关闭
        std::atomic<bool> stopped;

        std::atomic<int> idleThreadNum;

    public:
        inline threadpool(unsigned short size = 4) : stopped{ false }
        {
            idleThreadNum = size < 1 ? 1 : size;

            for (size = 0; size < idleThreadNum; size++)
            {
                pool.emplace_back(
                    [this]
                {
                    //工作线程函数
                    while (!this->stopped)
                    {
                        std::function<void()> task;
                        {
                            // 获取一个待执行的task
                            std::unique_lock<std::mutex> lock{ this->m_lock };
                            this->cv_task.wait(lock,
                                [this] {
                                return this->stopped.load() || !this->tasks.empty();
                            }
                            );

                            if (this->stopped && this->tasks.empty())
                            {
                                return;
                            }
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }

                        idleThreadNum--;
                        task();
                        idleThreadNum++;
                    }
                }
                );
            }

        }

        ~threadpool()
        {
            stopped.store(true);
            cv_task.notify_all();

            for (std::thread & thread : pool)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
        }

        // 提交一个任务
        // 调用.get()获取返回值会等待任务执行完成，获取返回值
        template<typename F, typename... Args>
        auto commit(F &&f, Args&&...args)->std::future<decltype(f(args...))>
        {
            if (stopped.load())
            {
                throw std::runtime_error("commit on ThreadPool is stopped.");
            }

            using RetType = decltype(f(args...));
            auto task = std::make_shared<std::packaged_task<RetType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
                );

            std::future<RetType> future = task->get_future();
            {
                std::lock_guard<std::mutex> lock{ m_lock };
                tasks.emplace(
                    [task]()
                {
                    (*task)();
                }
                );
            }

            cv_task.notify_one();

            return future;
        }

        int idleCount() { return idleThreadNum; }

    };


}


#endif // THREAD_POOL_H
