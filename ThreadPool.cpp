#include "ThreadPool.h"
#include <iostream>
#include <string>
#include <unistd.h>
//mingw-w64提供的gcc编译器(posix版本)提供的pthread实现(简称WIN_PTHREADS)与pthread for win32提供的pthread(简称PTW32)实现是有差别的。
//PTW32中pthread_t定义是一个结构，而WIN_PTHREADS则与linux版本的pthread定义一样，是个整数类型.
//因为pthread.h的这个版本区别,所以在写跨平台的代码时要获取线程id,就要区别对待，如下：
static inline unsigned int pthread_id()
{
#ifdef PTW32_VERSION
    return pthread_getw32threadid_np(pthread_self());
#else
    return (unsigned int)pthread_self();
#endif
}


ThreadPool::ThreadPool(int minNum, int maxNum) {
    this->m_taskQ = new TaskQueue;
    do{
        //初始化线程池
        this->m_minNum = minNum;
        this->m_maxNum = maxNum;
        this->m_busyNum = 0;
        this->m_aliveNum = minNum;


        //根据线程最大上限给线程数组分配内存
        this->m_threadIDs = new pthread_t[this->m_maxNum];
        if(this->m_threadIDs== nullptr){
            std::cout << "new thread_t failed" <<std::endl;
            break;
        }

        //初始化
        memset(this->m_threadIDs,0,sizeof(pthread_t)*this->m_maxNum);

        //初始化互斥量，条件变量
        if(pthread_mutex_init(&this->m_lock, nullptr)!=0||pthread_cond_init(&this->m_notEmpty, nullptr)!=0){
            std::cout << "init mutex or condition fail" << std::endl;
            break;
        }

        //////////////////创建线程//////////////////
        //根据最小线程个数，创建线程
        for(int i = 0;i<this->m_minNum;i++){
            pthread_create(&this->m_threadIDs[i], nullptr,worker,this); //this是指针
            std::cout << "创建子线程" << std::to_string(pthread_id()) <<std::endl;
        }
        pthread_create(&this->m_managerID, nullptr,manager,this);
    }while(0);
}

ThreadPool::~ThreadPool() {
    m_shutdown = 1;
    //销毁管理者线程
    pthread_join(this->m_managerID, nullptr);
    //唤醒所有消费者线程
    for(int i = 0 ; i<this->m_aliveNum;i++){
        pthread_cond_signal(&this->m_notEmpty);
    }

    if(this->m_taskQ){
        delete this->m_taskQ;
    }
    if(this->m_threadIDs){
        delete [] this->m_threadIDs;
    }
    //摧毁信号量
    pthread_mutex_destroy(&this->m_lock);
    pthread_cond_destroy(&this->m_notEmpty);
}

void ThreadPool::addTask(Task task) {
    if(this->m_shutdown){
        return;
    }
    //添加任务，不需要加锁，因为Task中有锁
    this->m_taskQ->addTask(task);
    //唤醒工作中的进程
    pthread_cond_signal(&this->m_notEmpty);
}

int ThreadPool::getAliveNumber() {
    int getAliveNum = 0;
    pthread_mutex_lock(&this->m_lock);
    getAliveNum = this->m_aliveNum;
    pthread_mutex_unlock(&this->m_lock);
    return getAliveNum;
}

int ThreadPool::getBusyNumber() {
    int getBusyNum = 0;
    pthread_mutex_lock(&this->m_lock);
    getBusyNum = this->m_busyNum;
    pthread_mutex_lock(&this->m_lock);
    return getBusyNum;
}

//工作线程函数
 void * worker(void * arg){
    ThreadPool * pool = static_cast<ThreadPool * >(arg);
    //一直不停工作
    while(true){
        //访问任务队列，共享资源加锁
        pthread_mutex_lock(&pool->m_lock);
        while(pool->m_taskQ->taskNumber()==0&&!pool->m_shutdown){
            std::cout << "thread " << pthread_id() <<" waiting" <<std::endl;
            //阻塞线程
            pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);
            //解除阻塞后，判断是否要销毁线程
            if(pool->m_exitNum>0){
                pool->m_exitNum--;
                if(pool->m_aliveNum>pool->m_minNum){
                    pool->m_aliveNum--;
                    pthread_mutex_unlock(&pool->m_lock);
                    pool->threadExit();
                }
            }
        }
        //判断是否关闭线程池
        if(pool->m_shutdown){
            pthread_mutex_unlock(&pool->m_lock);
            pool->threadExit();
        }

        //从任务队列中取出一个任务
        Task task;
        //工作的线程+1
        pool->addTask(task);
        pool->m_busyNum++;
        //线程池解锁
        pthread_mutex_unlock(&pool->m_lock);
        //执行任务
        std::cout << "thread " <<std::to_string(pthread_id()) << " start working" << std::endl;
        task.function(task.arg);
        delete task.arg;
        task.arg = nullptr;

        //任务处理结束
        std::cout << "thread " <<std::to_string(pthread_id()) << " end working" << std::endl;
        pthread_mutex_lock(&pool->m_lock);
        pool->m_busyNum--;
        pthread_mutex_unlock(&pool->m_lock);
    }
    return nullptr;
}

//管理者线程任务函数
void * manager(void *arg) {
    ThreadPool * pool = static_cast<ThreadPool*>(arg);
    //如果线程池没有关闭，就一直检测
    while(!pool->m_shutdown){
        //每隔5s检测一次
        sleep(5);
        //取出线程池中的任务数和线程数量
        //取出工作的线程池数量
        pthread_mutex_lock(&pool->m_lock);
        int queueSize = pool->m_taskQ->taskNumber();
        int liveNum = pool->m_aliveNum;
        int busyNum = pool->m_busyNum;
        pthread_mutex_unlock(&pool->m_lock);

        //创建线程
        const int NUMBER = 2;
        if(queueSize>liveNum&&liveNum<pool->m_maxNum){
            //线程池加锁
            pthread_mutex_lock(&pool->m_lock);
            int num = 0;
            for(int i = 0;i<pool->m_maxNum && num<NUMBER&&pool->m_aliveNum<pool->m_maxNum;i++){
                if(pool->m_threadIDs[i]==0){
                    pthread_create(&pool->m_threadIDs[i], nullptr,worker,pool);
                    num++;
                    pool->m_aliveNum++;
                }
            }
            pthread_mutex_unlock(&pool->m_lock);
        }
        //销毁多余线程
        //忙线程*2 < 存活的线程数目  && 存活的线程数大于最小的线程数量
        if(busyNum*2 < liveNum && liveNum>pool->m_minNum){
            pthread_mutex_lock(&pool->m_lock);
            pool->m_exitNum = NUMBER;
            pthread_mutex_unlock(&pool->m_lock);
            for(int i  = 0;i<NUMBER;i++){
                pthread_cond_signal(&pool->m_notEmpty);
            }
        }
    }
    return nullptr;
}

void ThreadPool::threadExit(){
    pthread_t tid = pthread_self();
    for(int i = 0;i<this->m_maxNum;i++){
        if(this->m_threadIDs[i]==tid){
            std::cout << "threadExit() function: thread "
                 << pthread_id() << " exiting..." << std::endl;
            m_threadIDs[i] = 0;
            break;
        }
    }
    pthread_exit(nullptr);
}

