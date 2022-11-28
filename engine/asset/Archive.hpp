#include "core/Core.hpp"

// Engine Data
// - ArchiveHeader :: Information, Section Offsets
// - Blob[]        :: Format-dependent binary blobs
// - FileEntry[]   :: File lookup information

// Editor Data
// - TypeMapping[] :: Describes a serialized type, support loading if definition changes

namespace Asset {

constexpr u32 make4CC(char const code[4]) {
  return code[0] | (code[1] << 8) | (code[2] << 16) | (code[3] << 24);
}

constexpr u32 archiveSignature = make4CC("JANK");

#pragma pack(1)
struct ArchiveHeader {
  enum Flags : u8 {
    PreloadDependencies = 1 << 0,
    PreloadAllFiles     = 1 << 1
  };

  u32   signature;
  u16   version;
  u8    locale;
  Flags flags;
  u32   numFiles;
  u32   numDependencies;
  u64   archiveSize;
  u64   fileEntriesOffset;
};
static_assert(sizeof(ArchiveHeader) == 32);

struct FileEntry {
  enum Flags : u32 {
    Compressed = 1 << 0
  };

  u16 version;
  u16 numDependencies;
  u32 size;
  u64 offset;
  u32 typeId;
  Flags flags;
};
static_assert(sizeof(FileEntry) == 24);

struct Dependency {
};

#if BUILD_EDITOR
struct TypeMapping {
};

struct FileName {
};
#endif
#pragma pack()


/// 
class Archive : NonCopyable {

};


// SERIALIZATION
// - Don't want to reimport on different computers
// - Don't want an asset server
// - Don't want to manually import
// - Don't want to merge one large metadata DB file

} // namespace Asset
