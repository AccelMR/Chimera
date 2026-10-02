/************************************************************************/
/**
 * @file chSTDThreading.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2026/10/01
 * @brief Engine names for the standard threading types.
 *
 * Kept out of chSTDHeaders.h because <mutex> and <thread> pull in <chrono> and
 * are heavy, so only the files that need threads pay for them.
 *
 * @bug No bug known.
 */
/************************************************************************/
#pragma once

#include <atomic>
#include <mutex>
#include <thread>

namespace chEngineSDK {

using Mutex = std::mutex;
using RecursiveMutex = std::recursive_mutex;
using RecursiveLock = std::unique_lock<RecursiveMutex>;

template<typename M>
using LockGuard = std::lock_guard<M>;

template<typename T>
using Atomic = std::atomic<T>;

using Thread = std::thread;

} // namespace chEngineSDK
