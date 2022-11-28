#include "asset/Archive.hpp"

#include <lz4.h>

#include <cstddef>
#include <memory>
#include <new>
#include <string>

/**
 * Like std::unique_ptr but with a few differences:
 * - It cannot be set to the null pointer.
 * - The default constructor allocates uninitialized memory.
 */
template<typename T>
class UniqueMem {
  T* ptr;

public:
  UniqueMem() : ptr(new T) {}
  UniqueMem(T* mem) : ptr(mem) {}
  ~UniqueMem() { delete ptr; }

  T* get() const { return ptr; }

  T* operator->() const { return ptr; }
  T& operator*() { return *ptr; }
};

#define LAUNDER(T, name) \
  alignas(T) std::byte mem_##name[sizeof(T)]; \
  auto name{ std::launder(reinterpret_cast<T*>(&mem_##name)) }

namespace Asset {

bool loadArchive() {
  LAUNDER(ArchiveHeader, header);

  // fread(...)

  if (header->signature != archiveSignature) {
    // TODO error
  }

  if (header->version != 0) {
    // TODO error
  }

  if (header->numFiles == 0) {
    // TODO error
  }
#if 0
  if (header->numFileRefs != 0) {
    std::unique_ptr<char[]> refs{ new char[header->fileRefsSize] };
    auto end{ refs.get() + header->fileRefsSize };

    // fseek to header->fileRefsOffset
    // fread header->fileRefsSize

    for (auto ptr{ refs.get() }; ptr < end;) {
      auto size{ static_cast<usize>(*ptr++) };
      std::string_view name{ ptr, size };
      // loadArchive(name)
      ptr += size;
    }
  }
#endif

  std::unique_ptr<u64[]> fileOffsets{ new u64[header->numFiles] };
  // fseek to header->fileIndexOffset
  // fread

  for (auto n{ 0 }; n < header->numFiles; n++) {
    // 
  }

  return true;
}

// NESTING
// - Asset archive

// FORMAT
// - Header
// - File blobs
// - Reference strings
// - File Index

// TODO
// - Archives within archives?
// - Don't load all files automatically (ie pack everything into single archive for release)

} // namespace Asset
