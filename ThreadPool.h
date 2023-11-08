//
// Created by 34885 on 2023/3/31.
//

#ifndef THREADPOOLFINAL_THREADPOOL_H
#define THREADPOOLFINAL_THREADPOOL_H
#include "TaskQueue.h"
void * worker(void * arg);
void * manager(void * arg);
class ThreadPool{
    friend void * worker(void * arg);
    friend void * manager(void * arg);
public:
    ThreadPool(int minNum,int maxNum);
    ~ThreadPool();

    //添加任务
    void addTask(Task  task);
    //获取忙线程个数
    int getBusyNumber();
    //获取活着的线程个数
    int getAliveNumber();


    void threadExit();

private:
    pthread_mutex_t m_lock;
    pthread_cond_t m_notEmpty;
    pthread_t * m_threadIDs;
    pthread_t m_managerID;
    TaskQueue * m_taskQ;
    int m_minNum;
    int m_maxNum;
    int m_busyNum;
    int m_aliveNum;
    int m_exitNum;
    bool m_shutdown = false;
};
#endif //THREADPOOLFINAL_THREADPOOL_H
