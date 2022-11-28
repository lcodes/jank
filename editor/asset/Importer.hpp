#pragma once

#include "core/CoreTypes.hpp"
#include "core/CoreString.hpp"

#include "asset/Library.hpp"

#include <atomic>
#include <vector>

namespace Asset {

class Type : NonCopyable {
public:
  StringView name;

  Type(StringView name);
};

class Importer : NonCopyable, public std::enable_shared_from_this<Importer> {
  friend class Library;

  enum State : u8 {
    TaskActive  = 1 << 0,
    NeedsDelete = 1 << 1,
    NeedsImport = 1 << 2,
    NeedsUnload = 1 << 3,
    NeedsLoad   = 1 << 4
  };

  static const State taskFlags{ static_cast<State>(~TaskActive) };

  u64 id;
  UString filePath;
  UString dataPath;
  FolderImporterPtr parent;

  u16 nameOffset{ 0 };
  u16 extOffset { 0 };
  std::atomic<State> stateFlags;

protected:
  Importer() = default;

  virtual bool canImport() const;
  virtual bool process();
  virtual void clear();

public:
  virtual ~Importer() = default;

  virtual Type const& getType() const = 0;

  u64 getId() const { return id; }

  UString const& getFilePath() const { return filePath; }
  UString const& getDataPath() const { return dataPath; }
};

class FolderImporter : public Importer {
  friend class Library;

  std::vector<ImporterPtr> children;

  u16 numSubFolders{ 0 };

  void insert(ImporterPtr& child);
  void remove(ImporterPtr& child);

protected:
  bool canImport() const override;
  void clear() override;

public:
  Type const& getType() const override;

  std::vector<ImporterPtr> const& getChildren() const { return children; }

  bool hasSubFolders() const { return numSubFolders != 0; }
};

class GenericImporter : public Importer {
protected:
  bool process() override;

public:
  Type const& getType() const override;
};

class Format : NonCopyable {
  friend class Library;

protected:
  static void Register(UStringView ext, Format const& format);

  virtual ImporterPtr createImporter() const = 0;
};

template<typename T>
class FormatRegistration : public Format {
  static_assert(std::is_base_of_v<Importer, T>);

protected:
  ImporterPtr createImporter() const override {
    return std::make_shared<T>();
  }

public:
  FormatRegistration(UStringView ext) { Register(ext, *this); }
};

#define ASSET_FORMAT(importer, ext) \
  static ::Asset::FormatRegistration<importer##Importer> _##ext(USTR(#ext##sv))

} // namespace Asset
