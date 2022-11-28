#include "core/CoreString.hpp"
#include "core/CoreDebug.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

#if PLATFORM_WINDOWS
# pragma warning(push, 0)
# include <Windows.h>
# pragma warning(pop)
#endif


// Symbols
// ----------------------------------------------------------------------------

/**
 * Static array of symbol strings.
 *
 * Prevents the strings from moving, guaranteeing the views stay valid.
 * This is important because a string using the small string optimization
 * also moves its storage. Accessing string views referencing it after a
 * move would then yield an use-after-free bug.
 */
struct Bucket {
  static constexpr u32 size = 128;

  String strings[size];
};

struct Node {
  u32 bucket;
  u32 index;
};

static inline Node getNode(u32 id) {
  return {
    id / Bucket::size,
    id % Bucket::size
  };
}

/// Synchronize accesses to nextId, symbols and symbolData.
static std::mutex mutex;

/// Counter for new Symbol::id values.
static u32 nextId;

/// Lookup for existing symbols.
static std::unordered_map<StringView, Symbol> symbols;

/// Storage for symbol strings.
static std::vector<std::unique_ptr<Bucket>> buckets;

Symbol::Symbol(StringView s) {
  if (s.size() == 0) {
    id = 0;
  }
  else {
    std::scoped_lock _(mutex);
    if (auto it{ symbols.find(s) }; it != symbols.end()) {
      id = it->second.id;
    }
    else {
      id = ++nextId; // Pre-increment to keep id 0 for the empty string.

      auto node{ getNode(id) };
      if (node.index == 0 || buckets.empty()) {
        buckets.push_back(std::make_unique<Bucket>());
      }

      auto bucket{ buckets[node.bucket].get() };
      bucket->strings[node.index] = s;

      symbols.insert({ StringView{ bucket->strings[node.index] }, *this });
    }
  }
}

StringView Symbol::operator*() const {
  if (id == 0) return ""sv;

  auto node{ getNode(id) };

  std::scoped_lock _(mutex);
  return buckets[node.bucket]->strings[node.index];
}


// Conversion Functions
// ----------------------------------------------------------------------------

#if PLATFORM_WINDOWS
String toUtf8(WString const& s) {
  auto in{ static_cast<i32>(s.size()) };
  auto sz{ okWin(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                     s.data(), in, nullptr, 0, nullptr, nullptr)) };
 
  String out;
  out.resize(static_cast<usize>(sz));

  okWin(WideCharToMultiByte(CP_UTF8, 0, s.data(), in, out.data(), sz, nullptr, nullptr));
  return out;
}

WString toUtf16(String const& s) {
  auto in{ static_cast<i32>(s.size()) };
  auto sz{ okWin(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                     s.data(), in, nullptr, 0)) };

  WString out;
  out.resize(static_cast<usize>(sz));

  okWin(MultiByteToWideChar(CP_UTF8, 0, s.data(), in, out.data(), sz));
  return out;
}

StringView toUtf8(wchar_t const* from, i32 fromLength, char* to, i32 toLength) {
  auto sz{ okWin(WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                     from, fromLength, to, toLength, nullptr, nullptr)) };
  return { to, static_cast<usize>(sz) };
}

WStringView toUtf16(char const* from, i32 fromLength, wchar_t* to, i32 toLength) {
  auto sz{ okWin(MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                     from, fromLength, to, toLength)) };
  return { to, static_cast<usize>(sz) };
}
#else
# error TODO
#endif
