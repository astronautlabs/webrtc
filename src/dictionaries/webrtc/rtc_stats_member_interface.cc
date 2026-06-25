#include "src/dictionaries/webrtc/rtc_stats_member_interface.h"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

#include <node-addon-api/napi.h>
#include <webrtc/api/stats/rtc_stats.h>

#include "src/converters.h"
#include "src/converters/napi.h"

namespace node_webrtc {
  TO_NAPI_IMPL(webrtc::Attribute, pair) {
    auto env = pair.first;
    auto attr = pair.second;

    if (attr.holds_alternative<bool>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<bool>()));
    }
    if (attr.holds_alternative<int32_t>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<int32_t>()));
    }
    if (attr.holds_alternative<uint32_t>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<uint32_t>()));
    }
    if (attr.holds_alternative<int64_t>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<int64_t>()));
    }
    if (attr.holds_alternative<uint64_t>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<uint64_t>()));
    }
    if (attr.holds_alternative<double>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<double>()));
    }
    if (attr.holds_alternative<std::string>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::string>()));
    }
    if (attr.holds_alternative<std::vector<bool>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::vector<bool>>()));
    }
    if (attr.holds_alternative<std::vector<int32_t>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::vector<int32_t>>()));
    }
    if (attr.holds_alternative<std::vector<uint32_t>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::vector<uint32_t>>()));
    }
    if (attr.holds_alternative<std::vector<int64_t>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::vector<int64_t>>()));
    }
    if (attr.holds_alternative<std::vector<uint64_t>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::vector<uint64_t>>()));
    }
    if (attr.holds_alternative<std::vector<double>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::vector<double>>()));
    }
    if (attr.holds_alternative<std::vector<std::string>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::vector<std::string>>()));
    }
    if (attr.holds_alternative<std::map<std::string, uint64_t>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::map<std::string, uint64_t>>()));
    }
    if (attr.holds_alternative<std::map<std::string, double>>()) {
      return From<Napi::Value>(std::make_pair(env, attr.get<std::map<std::string, double>>()));
    }

    return From<Napi::Value>(env.Null());
  }
} // namespace node_webrtc
