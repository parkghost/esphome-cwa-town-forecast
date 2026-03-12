#pragma once

#include <string>
#include <cstdio>

namespace esphome {
namespace cwa_town_forecast {

/// RFC 3986 URL encoding
inline std::string url_encode(const std::string &value) {
  std::string encoded;
  encoded.reserve(value.size() * 3);  // worst case
  for (unsigned char c : value) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      encoded += static_cast<char>(c);
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      encoded += buf;
    }
  }
  return encoded;
}

}  // namespace cwa_town_forecast
}  // namespace esphome
