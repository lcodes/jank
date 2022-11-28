#pragma once

#include "core/Core.hpp"
#include "core/CoreObject.hpp"
#include "core/CoreString.hpp"

#include <memory>

DECL_LOG_SOURCE(Asset, Info);

namespace App {
class Main;
}

namespace Asset {

// Name / Category / Color

// Context Menu Items

// Preview Thumbnails

class Importer;
class FolderImporter;

using ImporterPtr       = std::shared_ptr<Importer>;
using FolderImporterPtr = std::shared_ptr<FolderImporter>;

class Library {
  friend class App::Main;

  Library() = default;

  static UString assetsPath;
  static UString importPath;

  static void init();
  static void term();

  static void makeDataPath(UString& dataPath, u64 id);

  static void refreshTask();
  static void importTask(ImporterPtr& importer);
  static void importFile(UStringView file,
                         usize nameOffset,
                         FolderImporterPtr& parent);
  static void scanDir(uchar* path,
                      usize length,
                      FolderImporterPtr& parent);
  static ImporterPtr createImporter(UStringView file,
                                    usize nameOffset,
                                    FolderImporterPtr& parent);
  static FolderImporterPtr getChildFolder(UStringView path,
                                          usize nameOffset,
                                          FolderImporterPtr& parent);

public:
  static void refresh();

  static u64 find(UStringView path);

  template<typename T>
  static ObjectPtr<T> load(u64 path) {
    return ObjectPtr<T>(load(path));
  }

  static Object& load(u64 path, ...); // TODO type info

  static void save(UStringView path, Object& object);

  static void save(u64 path, Object const& o);

  static void move(u64 from, UStringView to);

  static u64 copy(u64 from, UStringView to);
};

} // namespace Asset
