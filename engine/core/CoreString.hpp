#pragma once

#include "core/CoreTypes.hpp"

#include <string>

using namespace std::literals;

using String     = std::string;
using StringView = std::string_view;

using UString     = std::basic_string<uchar>;
using UStringView = std::basic_string_view<uchar>;

/**
 * An interned string.
 */
class Symbol {
  u32 id;

public:
  Symbol(StringView s);

  Symbol() : id(0) {}
  ~Symbol() = default;

  Symbol(Symbol const& s) : id(s.id) {}
  Symbol& operator=(Symbol const& s) {
    id = s.id;
    return *this;
  }

  operator bool() const { return id != 0; }
  bool operator!() const { return id == 0; }

  bool operator==(Symbol const& s) const { return id == s.id; }
  bool operator!=(Symbol const& s) const { return id != s.id; }

  StringView operator*() const;
};

