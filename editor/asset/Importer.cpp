#include "asset/Importer.hpp"

#include "core/CoreDebug.hpp"

namespace Asset {

// Type
// ----------------------------------------------------------------------------

ASSET_FORMAT(Generic, jank);

static Type folderType { "Folder"sv };
static Type genericType{ "Generic"sv };

Type::Type(StringView name) : name(name) {}


// Importer
// ----------------------------------------------------------------------------

bool Importer::canImport() const { return true; }

bool Importer::process() { ASSERT(0); UNREACHABLE; }

void Importer::clear() {}


// Folder Importer
// ----------------------------------------------------------------------------

void FolderImporter::insert(ImporterPtr& child) {
  ASSERT_EX(std::find(children.cbegin(), children.cend(), child) == children.cend());
  children.push_back(child);
}

void FolderImporter::remove(ImporterPtr& child) {
  auto it{ std::find(children.cbegin(), children.cend(), child) };
  ASSERT_EX(it != children.cend());
  children.erase(it);
}

bool FolderImporter::canImport() const { return false; }

void FolderImporter::clear() { children.clear(); }

Type const& FolderImporter::getType() const { return folderType; }


// Generic Importer
// ----------------------------------------------------------------------------

bool GenericImporter::process() {
  ASSERT(0, "TODO");
  return true;
}

Type const& GenericImporter::getType() const { return genericType; }

} // namespace Asset
