#pragma once

#include "core/CoreTypes.hpp"

#include <string>

using namespace std::literals;

using String      = std::string;
using StringView  = std::string_view;

using WString     = std::wstring;
using WStringView = std::wstring_view;

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


// Conversion Functions
// ----------------------------------------------------------------------------

 String toUtf8 (WString const& s);
WString toUtf16( String const& s);

 StringView toUtf8 (wchar_t const* from, i32 fromLength,  char  * to, i32 toLength);
WStringView toUtf16( char   const* from, i32 fromLength, wchar_t* to, i32 toLength);

inline StringView toUtf8(wchar_t const* from, char* to, i32 toLength) {
  return toUtf8(from, -1, to, toLength);
}
inline WStringView toUtf16(char const* from, wchar_t* to, i32 toLength) {
  return toUtf16(from, -1, to, toLength);
}

inline  String& toUtf8 ( String& s) { return s; }
inline WString& toUtf16(WString& s) { return s; }

