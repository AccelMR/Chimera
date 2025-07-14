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
const String UUID::CH_UUID_STR = "f81d4fae-7dec-11d0-a765-00a0c91e6bf6";
uuids::uuid_name_generator UUID::UUID_NAME_GENERATOR(uuids::uuid::from_string(CH_UUID_STR).value());

/*
*/
// UUID::UUID(const String& str)
//   : m_uuid(UUID_NAME_GENERATOR(str)) {}

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
    return UUID(UUID_NAME_GENERATOR(name));
  }
  else {
    uuids::uuid_name_generator generator(namespace_uuid.m_uuid);
    return UUID(generator(name));
  }
}
} // namespace chEngineSDK
