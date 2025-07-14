/************************************************************************/
/**
 * @file chUUID.h
 * @author AccelMR
 * @date 2025/04/26
 * @brief
 *  UUID class for generating and manipulating Universally Unique Identifiers.
 * It uses the stduuid library for UUID generation and manipulation.
 * https://github.com/mariusbancila/stduuid/tree/master
 */
/************************************************************************/
#pragma once

#include "chPrerequisitesUtilities.h"

#define STDUUID_FORCE_IMPLEMENTATION
#define UUID_SYSTEM_GENERATOR
#include "stduuid/uuid.h"

namespace chEngineSDK {

class CH_UTILITY_EXPORT UUID  {
 public:
  UUID() = default;

  //explicit UUID(const String& str);

  explicit UUID(const uuids::uuid& uuid)
    : m_uuid(uuid) {}

  NODISCARD FORCEINLINE bool
  isNull() const { return m_uuid.is_nil(); }

  NODISCARD FORCEINLINE String
  toString() const { return uuids::to_string(m_uuid); }

  NODISCARD FORCEINLINE bool
  operator==(const UUID& other) const { return m_uuid == other.m_uuid; }

  NODISCARD FORCEINLINE bool
  operator!=(const UUID& other) const { return m_uuid != other.m_uuid; }

  NODISCARD FORCEINLINE bool
  operator<(const UUID& other) const { return m_uuid < other.m_uuid; }

  NODISCARD FORCEINLINE SIZE_T
  getHash() const noexcept { return Hash<uuids::uuid>{}(m_uuid); }

  NODISCARD static UUID
  createRandom();

  NODISCARD static UUID
  createFromName(const String& name, const UUID& namespace_uuid = UUID::null());

  NODISCARD FORCEINLINE static const UUID&
  null() { static UUID nullUUID(uuids::uuid{}); return nullUUID; }

 private:
  friend Hash<UUID>;
  uuids::uuid m_uuid;

  static const String CH_UUID_STR;
  static uuids::uuid_name_generator UUID_NAME_GENERATOR;
};

} // namespace chEngineSDK

namespace std {
template <>
struct hash<chEngineSDK::UUID> {
  using argument_type = chEngineSDK::UUID;
  using result_type = chEngineSDK::SIZE_T;

  NODISCARD result_type
  operator()(const argument_type& uuid) const noexcept {
    return uuid.getHash();
  }

};
} // namespace std
