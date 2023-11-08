//
// Created by 34885 on 2023/3/30.
//

#ifndef THREADPOOLFINAL_TASKQUEUE_H
#define THREADPOOLFINAL_TASKQUEUE_H
#pragma once
#include <pthread.h>
#include <queue>
using callback = void(*)(void *);
struct Task{
    Task(){
        this->function = nullptr;
        this->arg = nullptr;
    }
    Task(callback f,void * arg){
        this->function = f;
        this->arg = arg;
    }
    callback function;
    void * arg;
};
//其中 Task 是任务类，里边有两个成员，分别是两个指针 void(*)(void*) 和 void*
//
//另外一个类 TaskQueue 是任务队列，提供了添加任务、取出任务、存储任务、获取任务个数、线程同步的功能。

class TaskQueue{
public:
    TaskQueue();
    ~TaskQueue();

    //添加任务
    void addTask(Task & task);
    void addTask(callback func,void * arg);

    //取出一个任务
    Task takeTask();

    //获取当前队列中的任务个数
    inline int taskNumber(){
        return this->m_queue.size();
    }

private:
    pthread_mutex_t m_mutex; //互斥锁
    std::queue<Task> m_queue; //任务队列

};
#endif //THREADPOOLFINAL_TASKQUEUE_H
