#include "TaskQueue.h"
#include <pthread.h>
TaskQueue::TaskQueue() {
    pthread_mutex_init(&this->m_mutex, nullptr);
}

TaskQueue::~TaskQueue() {
    pthread_mutex_destroy(&this->m_mutex);
}

void TaskQueue::addTask(Task &task) {
    pthread_mutex_lock(&this->m_mutex);
    this->m_queue.push(task);
    pthread_mutex_unlock(&this->m_mutex);
}

void TaskQueue::addTask(callback func, void *arg) {
    pthread_mutex_lock(&this->m_mutex);
    Task task(func,arg);
    this->m_queue.push(task);
    pthread_mutex_unlock(&this->m_mutex);
}

Task TaskQueue::takeTask() {
    Task t;
    pthread_mutex_lock(&this->m_mutex);
    if(this->m_queue.size()>0){
        t = this->m_queue.front();
        this->m_queue.pop();
    }
    pthread_mutex_unlock(&this->m_mutex);
    return t;
}
