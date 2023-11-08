cmake_minimum_required(VERSION 3.20)
project(threadPoolFinal)

set(CMAKE_CXX_STANDARD 14)

add_executable(threadPoolFinal main.cpp TaskQueue.cpp TaskQueue.h ThreadPool.h ThreadPool.cpp)
