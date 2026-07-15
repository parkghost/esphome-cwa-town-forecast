#pragma once

#include <cstdint>
#include <ctime>
#include <limits>

namespace esphome {
namespace cwa_town_forecast {

// Stores a civil (wall-clock) date/time as seconds since 1970-01-01 00:00:00
// of that same wall clock, replacing the former heap-allocated PsramTime
// (one ~44-byte std::tm allocation per field). Conversion to epoch uses pure
// calendar arithmetic (Hinnant's days_from_civil) and deliberately avoids
// mktime(): CWA payload times and the component's query times are all Taiwan
// local wall times, so one fixed TZ-independent convention on both sides
// compares correctly and round-trips losslessly — matching the previous
// mktime-on-both-sides behavior where any TZ offset cancelled out.
class TimeField {
 public:
  TimeField() = default;
  TimeField(const std::tm &tm) { *this = tm; }
  explicit TimeField(time_t wall_epoch) : epoch_(wall_epoch) {}

  TimeField &operator=(const std::tm &tm) {
    this->epoch_ = to_wall_epoch(tm);
    return *this;
  }

  // Civil date/time -> seconds since 1970-01-01 of the same wall clock.
  // Out-of-range fields (tm_mon, tm_mday, tm_hour, ...) normalize
  // arithmetically, matching what callers previously got from std::mktime.
  // tm_isdst is deliberately ignored: CWA forecast times carry no DST and the
  // component compares device wall clock against forecast wall clock, so both
  // sides use the same DST-free convention.
  static time_t to_wall_epoch(const std::tm &tm) {
    // Carry out-of-range months into the year
    int64_t mon = tm.tm_mon;
    int64_t y = static_cast<int64_t>(tm.tm_year) + 1900 + (mon >= 0 ? mon / 12 : (mon - 11) / 12);
    mon = ((mon % 12) + 12) % 12;
    const unsigned m = static_cast<unsigned>(mon + 1);
    // days_from_civil for the 1st of the month (proleptic Gregorian), then
    // add day/time offsets so out-of-range tm_mday/hour/min/sec normalize
    y -= m <= 2;
    const int64_t era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int64_t days = era * 146097 + static_cast<int64_t>(doe) - 719468 + (tm.tm_mday - 1);
    return static_cast<time_t>(days * 86400 + tm.tm_hour * 3600LL + tm.tm_min * 60LL + tm.tm_sec);
  }

  bool is_valid() const { return this->epoch_ != INVALID_EPOCH; }
  operator bool() const { return this->is_valid(); }

  // Wall-clock seconds; only meaningful for comparison/difference against
  // values produced by to_wall_epoch() with the same convention.
  time_t epoch() const { return this->epoch_; }

  std::tm to_tm() const {
    std::tm tm{};
    if (this->is_valid()) {
      // gmtime_r is TZ-independent, so it inverts the wall-epoch convention
      // exactly (and fills tm_wday/tm_yday correctly)
      time_t e = this->epoch_;
      gmtime_r(&e, &tm);
    }
    return tm;
  }

  operator std::tm() const { return this->to_tm(); }

  void reset() { this->epoch_ = INVALID_EPOCH; }

 private:
  static constexpr time_t INVALID_EPOCH = std::numeric_limits<time_t>::min();
  time_t epoch_{INVALID_EPOCH};
};

}  // namespace cwa_town_forecast
}  // namespace esphome
