#include "core/CoreString.hpp"

#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

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
