/************************************************************************/
/**
 * @file chSTDHeaders.h
 * @author AccelMR <accel.mr@gmail.com>
 * @date 2021/09/10
 * @brief Engine names for the standard library types used everywhere.
 *
 * Every file in the engine includes this one through the prerequisites, so it
 * only holds light headers that most of the engine needs. Heavy or rarely used
 * headers (streams, files, threads, time, math) are included by the files that
 * use them. Threads live in chSTDThreading.h and string streams in
 * chSTDStreams.h.
 *
 * When an engine type replaces one of these aliases, remove the alias from
 * here and give the new type its own file.
 *
 * @bug No bug known.
 */
/************************************************************************/
#pragma once

#include <any>
#include <array>
#include <bitset>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace chEngineSDK {

/************************************************************************/
/*
 * Containers
 */
/************************************************************************/
template<typename T>
using Alloc = std::allocator<T>;

template<typename T, SIZE_T size>
using Array = std::array<T, size>;

template<typename T, typename A = Alloc<T>>
using Vector = std::vector<T, A>;

template<typename T, typename A = Alloc<T>>
using Deque = std::deque<T, A>;

template<typename T, typename Container = Deque<T>>
using Queue = std::queue<T, Container>;

template<typename T, typename P = std::less<T>, typename A = Alloc<T>>
using Set = std::set<T, P, A>;

template<typename K,
         typename T,
         typename Compare = std::less<K>,
         typename A = Alloc<std::pair<const K, T>>>
using Map = std::map<K, T, Compare, A>;

template<typename K>
using Hash = std::hash<K>;

template<typename K,
         typename H = Hash<K>,
         typename Eq = std::equal_to<K>,
         typename A = Alloc<K>>
using UnorderedSet = std::unordered_set<K, H, Eq, A>;

template<typename K,
         typename T,
         typename H = Hash<K>,
         typename Eq = std::equal_to<K>,
         typename A = Alloc<std::pair<const K, T>>>
using UnorderedMap = std::unordered_map<K, T, H, Eq, A>;

template<SIZE_T N>
using BitSet = std::bitset<N>;

template<typename T1, typename T2>
using Pair = std::pair<T1, T2>;

/************************************************************************/
/*
 * Smart pointers
 */
/************************************************************************/
template<typename T>
using SPtr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
struct ForwardDeleter
{
  void
  operator()(T* ptr) const
  {
    delete ptr;
  }
};

template<typename T>
using UniquePtr = std::unique_ptr<T, ForwardDeleter<T>>;

template<typename T, typename... Args>
SPtr<T>
chMakeShared(Args&&... args)
{
  return std::allocate_shared<T>(Alloc<T>(), std::forward<Args>(args)...);
}

template<typename T, typename... Args>
UniquePtr<T>
chMakeUnique(Args&&... args)
{
  return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

/************************************************************************/
/*
 * Strings
 */
/************************************************************************/
template<typename T>
using BasicString = std::basic_string<T, std::char_traits<T>, Alloc<T>>;

using String = std::string;
using WString = std::wstring;
using U16String = BasicString<char16_t>;
using U32String = BasicString<char32_t>;

using StringView = std::string_view;

/************************************************************************/
/*
 * Value wrappers
 */
/************************************************************************/
template<typename T>
using Optional = std::optional<T>;

constexpr auto NullOpt = std::nullopt;

template<typename... Types>
using Variant = std::variant<Types...>;

template<typename Signature>
using Function = std::function<Signature>;

using Any = std::any;

namespace AnyUtils {
template<typename T>
concept AnyCompatible = requires(const Any& any) { std::any_cast<T>(any); };

template<AnyCompatible T>
FORCEINLINE bool
hasType(const Any& any) noexcept
{
  return any.type() == typeid(T);
}

template<AnyCompatible T>
FORCEINLINE bool
tryGetValue(const Any& any, T& output) noexcept
{
  const T* value = std::any_cast<T>(&any);
  if (value == nullptr) {
    return false;
  }

  output = *value;
  return true;
}
} // namespace AnyUtils

} // namespace chEngineSDK
