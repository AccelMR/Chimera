/************************************************************************/
/**
 * @file chStdHeaders.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/10
 * @brief Header for the STD libraries.
 *
 * This file includes the most common and required STL objects.
 * It takes account of the operating system of the build to modify
 * the required object.
 *
 * @bug No bug known.
 */
 /************************************************************************/
#pragma once

/************************************************************************/
/*
 * C type objects
 */
/************************************************************************/
#include <cassert>
#include <cmath>
#include <limits>

/************************************************************************/
/*
 * Types.
 */
/************************************************************************/
#include <type_traits>

/************************************************************************/
/*
 * STL Containers.
 */
/************************************************************************/
#include <array>
#include <vector>
#include <queue>

/************************************************************************/
/**
 * Standard Containers defined as own
 */
 /************************************************************************/
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

 /************************************************************************/
 /*
  * C++ Stream Stuff
  */
 /************************************************************************/
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstring>

#include <functional>

#include <locale>
#include <codecvt>

#include <filesystem>

/************************************************************************/
/*
 * Threading
 */
/************************************************************************/
#include <mutex>
#include <thread>

#include <optional>
#include <variant>
#include <bitset>

#include <algorithm>
#include <chrono>
#include <atomic>


#include <any>

/*****************************************************************************/
/**
 * Windows specifics
 */
/*****************************************************************************/
#if USING(CH_PLATFORM_WIN32)
  //Undefine min & max
# undef min
# undef max

# if !defined(NOMINMAX) && defined(_MSC_VER)
#   define NOMINMAX     //Required to stop windows.h messing up std::min
# endif
# if defined( __MINGW32__ )
#   include <unistd.h>
# endif
#endif //#if USING(CH_PLATFORM_WIN32)


namespace chEngineSDK {
using std::char_traits;
using std::basic_string;
using std::basic_stringstream;
using std::min;
using std::forward;
using std::ios;

/**
 *   Std alias allocator.
 **/
template<typename T>
using Alloc = std::allocator<T>;

/**
*  @brief Fixed-size array sequence container class, holds its elements in a strict linear
*         sequence
*/
template<typename T, SIZE_T size>
using Array = std::array<T, size>;

/*
 *   Vector wrapper to use along the engine.
 **/
template <typename T, class A = Alloc<T>>
using Vector = std::vector<T, A>;

/**
 * @brief An associative container containing an ordered set of elements.
 */
template<typename T, typename P = std::less<T>, typename A = Alloc<T>>
using Set = std::set<T, P, A>;

/**
 * @brief An associative container that contains key-value pairs with unique keys.
 *        Search, insertion, and removal have average constant-time complexity.
 *
 * @tparam K Type of the keys. Must be unique.
 * @tparam T Type of the values.
 * @tparam Hash Optional hash function; by default, a specialisation of std::hash.
 * @tparam Eq Optional function for equality comparison; by default, operator==.
 * @tparam A Optional allocator object for defining storage allocation model; by default, std::allocator.
 */
template<typename K,
  typename T,
  typename Hash = std::hash<K>,
  typename Eq = std::equal_to<K>,
  typename A = std::allocator<std::pair<const K, T>>>
using UnorderedMap = std::unordered_map<K, T, Hash, Eq, A>;

/**
 * @brief An associative container that contains a set of unique objects of type Key.
 *        Search, insertion, and removal operations have average constant-time complexity.
 *
 * @tparam K Type of the keys. Must be unique.
 * @tparam Hash Optional hash function; by default, a specialisation of std::hash.
 * @tparam Eq Optional function for equality comparison; by default, operator==.
 * @tparam A Optional allocator object for defining storage allocation model; by default, std::allocator.
 */
template<typename K,
  typename Hash = std::hash<K>,
  typename Eq = std::equal_to<K>,
  typename A = std::allocator<K>>
  using UnorderedSet = std::unordered_set<K, Hash, Eq, A>;


/**
 * @brief An associative container containing an ordered set of key-value
 *        pairs.
 */
template<typename K,
         typename T,
         typename Compare = std::less<K>,
         typename A = std::allocator<std::pair<const K, T>>>
using Map = std::map<K, T, Compare, A>;

/**
 *   Queue wrapper.
 **/
template< typename T, class Container = std::deque<T>>
using Queue =  std::queue<T, Container>;

/************************************************************************/
/*
 * Smart pointers
 */
/************************************************************************/

/**
 *   Shared pointer that will be used for Chimera.
 **/
template <typename T>
using SPtr = std::shared_ptr<T>;

/**
 *   Weak pointer used along Chimera.
 **/
template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
struct ForwardDeleter {
    void operator()(T* ptr) const {
        delete ptr;
    }
};

/**
 *   Unique pointer used along Chimera.
 **/
template<class T>
using UniquePtr = std::unique_ptr<T, ForwardDeleter<T>>;

/**
 * @brief Create a new shared pointer using a custom allocator category.
 */
template<class T, class... Args>
SPtr<T>
chMakeShared(Args&&... args) {
  return std::allocate_shared<T>(Alloc<T>(),
                                 std::forward<Args>(args)...);
}

/**
 * @brief Create a new shared pointer using a custom allocator category.
 */
template<class T, class... Args>
UniquePtr<T> chMakeUnique(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

/************************************************************************/
/*
 * String related
 */
/************************************************************************/
/**
 * @brief Wide string stream used for primarily for constructing strings
 *        consisting of ASCII text.
 */
using StringStream = std::stringstream;

/**
 * @brief Basic string that uses geEngine memory allocators.
 */
template<typename T>
using BasicString = basic_string<T, char_traits<T>, std::allocator<T>>;

/**
 * @brief Basic string stream that uses geEngine memory allocators.
 */
template<typename T>
using BasicStringStream = basic_stringstream<T, char_traits<T>, std::allocator<T>>;

/**
 * @brief Wide string used primarily for handling Unicode text.
 */
using WString = std::wstring;

/**
 * @brief Narrow string used primarily for handling ASCII text.
 */
using String = std::string;//= BasicString<ANSICHAR>;

/**
 * @brief Wide string used UTF-16 encoded strings.
 */
using U16String = BasicString<char16_t>;

/**
 * @brief Wide string used UTF-32 encoded strings.
 */
using U32String = BasicString<char32_t>;

/************************************************************************/
/*
 * Threading
 */
 /************************************************************************/

/**
 * @brief Wrapper for the C++ std::recursive_mutex.
 */
using RecursiveMutex = std::recursive_mutex;

/**
 * @brief Wrapper for the C++ std::unique_lock<std::recursive_mutex>.
 */
using RecursiveLock = std::unique_lock<RecursiveMutex>;

template<typename T>
using Optional = std::optional<T>;

constexpr auto NullOpt = std::nullopt;

/**
 * @brief Wrapper for the C++ std::variant.
 */
template<typename... Types>
using Variant = std::variant<Types...>;

/**
 * @brief Wrapper for the C++ std::bitset.
 */
template<size_t N>
using BitSet = std::bitset<N>;

/**
 * @brief Wrapper for the C++ mutex.
 */
using Mutex = std::mutex;

/**
 * @brief Wrapper for lock_guard.
 */
template<typename Mutex>
using LockGuard = std::lock_guard<Mutex>;

/**
 * @brief Wrapper for the C++ std::atomic.
 */
template<typename T>
using Atomic = std::atomic<T>;

/**
 * @brief Wrapper for the C++ std::thread.
 */
using Thread = std::thread;

/**
 * @brief Wrapper for the C++ std::pair
*/
template<typename T1, typename T2>
using Pair = std::pair<T1, T2>;

/**
 * @brief Wrapper for the C++ std::any.
 */
using Any = std::any;
} // namespace chEngineSDK
