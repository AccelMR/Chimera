/************************************************************************/
/**
 * @file chUUID.cpp
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  UUID class for generating and manipulating Universally Unique Identifiers.
 * It uses the stduuid library for UUID generation and manipulation.
 * https://github.com/mariusbancila/stduuid/tree/master
 */
/************************************************************************/

#include "chUUID.h"

namespace chEngineSDK {
namespace {
constexpr ANSICHAR kNameNamespace[] = "f81d4fae-7dec-11d0-a765-00a0c91e6bf6";

// Built on first use: type ids are made from names while other modules run their global
// constructors, which may come before the globals of this file.
uuids::uuid_name_generator&
getNameGenerator()
{
  static uuids::uuid_name_generator generator(uuids::uuid::from_string(kNameNamespace).value());
  return generator;
}
} // namespace

/*
*/
UUID
UUID::createRandom() {
  static thread_local std::mt19937 mt_rand(std::random_device{}());
  uuids::uuid_random_generator gen(mt_rand);
  return UUID(gen());
}

/*
*/
UUID
UUID::createFromName(const String& name, const UUID& namespace_uuid) {
  if (namespace_uuid.isNull()) {
    return UUID(getNameGenerator()(name));
  }
  else {
    uuids::uuid_name_generator generator(namespace_uuid.m_uuid);
    return UUID(generator(name));
  }
}
} // namespace chEngineSDK
