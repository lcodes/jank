#include "asset/Library.hpp"
#include "asset/Importer.hpp"

#include "asset/Archive.hpp"

#include "core/CoreTask.hpp"

#include <fstream>
#include <mutex>
#include <unordered_map>

#if PLATFORM_WINDOWS
# pragma warning(push, 0)
# include <Windows.h>
# include <ShlObj_core.h>
# pragma warning(pop)
constexpr uchar pathSeparator = L'\\';
#else
constexpr uchar pathSeparator = '/';
#endif

namespace Asset {

std::hash<UStringView> hasher;

constexpr u32 maxPathLength = 1024;

constexpr uchar impExt[]{ USTR(".oi") };
constexpr usize impExtLength{ countof(impExt) - 1 };

UString Library::assetsPath;
UString Library::importPath;

static std::atomic<u32> numRunningTasks;

static std::mutex mutex;

static FolderImporterPtr rootFolder;

static std::unordered_map<UStringView, ImporterPtr> db;
static std::unordered_map<u64, ImporterPtr> dbById;

static std::unordered_map<UStringView, Format const&>* formats;

void Format::Register(UStringView ext, Format const& format) {
  if (!formats) {
    formats = new std::unordered_map<UStringView, Format const&>();
  }
  auto it{ formats->emplace(ext, format) };
  ASSERT_EX(it.second, "Asset extension already registered: " UFMT, USTR_ARGS(ext));
}

#if PLATFORM_POSIX
static bool checkDirectoryExists(char const* path, bool& isDir) {
  struct stat s;
  if (stat(path, &s) == 0) {
    if (S_ISDIR(s.st_mode)) {
      isDir = true;
    }
    else {
      LOG(Asset, Error, "Failed to create directory '%s': Not a directory", path);
      isDir = false;
    }
    return true;
  }

  return false;
}

static bool createDirectory(char const* path) {
  auto result{ mkdir(path) };
  if (result) {
    char buf[1024];
    auto len{ getErrorC(buf, countof(buf), errno) };
    LOG(Asset, Error, "Failed to create directory '%s': %.*s", path, len, buf);
    return false;
  }
  return true;
}
#endif

static bool createDirectory(UString const& path) {
#if PLATFORM_WINDOWS
  auto result{ SHCreateDirectory(nullptr, path.data()) };
  switch (result) {
  default: {
    uchar buf[1024];
    auto len{ getErrorWin(buf, countof(buf), static_cast<DWORD>(result)) };
    LOG(Asset, Error, "Failed to create directory '%.*S': %.*S",
        USTR_ARGS(path), len, buf);
    return false;
  }
  case ERROR_SUCCESS:
  case ERROR_ALREADY_EXISTS:
    return true;
  }
#else
  char buf[maxPathLength];
  ASSERT(path.size() + 1 < countof(buf));
  ASSERT(path[0] == pathSeparator, "createDirectory expects absolute path");
  ASSERT(path[path.size() - 1] != pathSeparator);

  bool isDir;
  if (checkDirectoryExists(path.c_str(), isDir)) {
    return isDir;
  }

  auto str{ path.data() + 1 };
  auto ptr{ buf };
  while (true) {
    auto end{ strchr(str, pathSeparator) };
    if (!end) break;

    auto len{ end - str };
    memcpy(ptr, str, len);

    str = end + 1;

    ptr[len] = '\0';

    if (checkDirectoryExists(buf, isDir)) {
      if (!isDir) {
        return false;
      }
    }
    else if (!createDirectory(buf)) {
      return false;
    }

    ptr[len] = pathSeparator;
    ptr += len + 1;
  }

  return createDirectory(path.c_str());
#endif
}

static UStringView getParent(UStringView path) {
  auto n{ path.size() - 1 };
  while (n > 0 && path[n] != pathSeparator) n--;
  return path.substr(0, n);
}

static UStringView getExt(UStringView path) {
  auto n{ path.size() - 1 };
  while (n > 0) {
    switch (path[n]) {
    case USTR('.'):     return path.substr(n + 1);
    case pathSeparator: return USTR(""sv);
    default: n--;
    }
  }
  ASSERT(0); UNREACHABLE;
}

void Library::init() {
  ASSERT(formats);

  uchar path[maxPathLength];

#if PLATFORM_WINDOWS
  auto length{ static_cast<usize>(okWin(GetCurrentDirectoryW(countof(path), path))) };
  if (length + 10 >= countof(path)) {
    errorMessageBox(USTR("Error"), USTR("Invalid working directory"));
    exit(EXIT_FAILURE);
  }
#else
  okC(getcwd(path, countof(path)));
  auto length{ strlen(path) };
#endif

  path[length++] = pathSeparator;

  constexpr uchar const assetsName[]{ USTR("assets") };
  constexpr uchar const importName[]{ USTR("import") };

  memcpy(path + length, assetsName, sizeof(assetsName));
  assetsPath.assign(path, length + countof(assetsName) - 1);

  memcpy(path + length, importName, sizeof(importName));
  importPath.assign(path, length + countof(importName) - 1);

  if (!createDirectory(assetsPath) || !createDirectory(importPath)) {
    errorMessageBox(USTR("Error"), USTR("Failed to create project directories"));
    exit(EXIT_FAILURE);
  }

  rootFolder = std::make_shared<FolderImporter>();
  rootFolder->filePath = assetsPath;
  rootFolder->nameOffset = static_cast<u16>(getParent(assetsPath).size() + 1);
  db.emplace(assetsPath, rootFolder);

  // TODO load db cache

  refresh();
}

void Library::term() {
  // TODO save db cache

  rootFolder.reset();

  delete formats;
}

void Library::refresh() {
  auto expected{ 0u };
  if (numRunningTasks.compare_exchange_strong(expected, 1u, std::memory_order_relaxed)) {
    Task::run(refreshTask);
  }
}

void Library::refreshTask() {
  std::scoped_lock _(mutex);

  uchar path[maxPathLength];
  memcpy(path, assetsPath.data(), assetsPath.size() * sizeof(uchar));
  scanDir(path, assetsPath.size(), rootFolder);
}

void Library::scanDir(uchar* path, usize length, FolderImporterPtr& parent) {
#if PLATFORM_WINDOWS
  path[length++] = L'\\';
  path[length + 0] = L'*';
  path[length + 1] = L'\0';

  WIN32_FIND_DATAW data;
  auto find{ FindFirstFileW(path, &data) };
  if (find == INVALID_HANDLE_VALUE) failWin();

  do {
    if (data.cFileName[0] == L'.') {
      continue;
    }

    // Ignore metadata files.
    auto nameLength{ wcslen(data.cFileName) };
    if (wcsncmp(data.cFileName + nameLength - impExtLength, impExt, impExtLength) == 0) {
      continue;
    }

    // Update the path to point to the current file.
    memcpy(path + length, data.cFileName, (nameLength + 1) * sizeof(uchar));

    auto fileLength{ length + nameLength };
    UStringView file{ path, fileLength };
    if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      auto folder{ getChildFolder(file, length, parent) };
      scanDir(path, fileLength, folder);
    }
    else {
      importFile(file, length, parent);
    }
  } while (FindNextFileW(find, &data));

  if (auto e{ GetLastError() }; e != ERROR_NO_MORE_FILES) {
    failWin(e);
  }

  okWin(FindClose(find));
#else
  path[length] = '\0';

  auto dir{ okC(opendir(path)) };
  path[length++] = '/';

  while (auto entry{ readdir(dir) }) {
    if (entry->d_name[0] == '.') {
      continue;
    }

    // Ignore metadata files.
    auto nameLength{ strlen(entry->d_name) };
    if (strncmp(entry->d_name + nameLength - impExtLength, impExt, impExtLength) == 0) {
      continue;
    }

    // Update the path to point to the current file.
    memcpy(path + length, entry->d_name, nameLength + 1);

    auto fileLength{ length + nameLength };
    UStringView file{ path, fileLength };
    if (entry->d_type == DT_DIR) {
      auto folder{ getChildFolder(file, length, parent) };
      scanDir(path, fileLength, folder);
    }
    else {
      importFile(file, length, parent);
    }
  }

  okC(closedir(dir));
#endif
}

FolderImporterPtr Library::getChildFolder(UStringView path,
                                          usize nameOffset,
                                          FolderImporterPtr& parent)
{
  if (auto it{ db.find(path) }; it != db.end()) {
    // TODO check if really a folder
    if (it->second->parent != parent) {
      // TODO handle
    }
    return std::static_pointer_cast<FolderImporter>(it->second);
  }

  auto folder{ std::make_shared<FolderImporter>() };
  folder->filePath   = path;
  folder->nameOffset = static_cast<u16>(nameOffset);
  folder->parent     = parent;

  db.emplace(path, folder);

  auto imp{ std::static_pointer_cast<Importer>(folder) };
  parent->insert(imp);
  parent->numSubFolders++;

  return folder;
}

static Type unknownType{ "Unknown" };

class NullImporter : public Importer {
protected:
  bool canImport() const override { return false; }

public:
  Type const& getType() const { return unknownType; }
};

void Library::makeDataPath(UString& dataPath, u64 id) {
  uchar buf[17];
  auto len{ swprintf_s(buf, countof(buf), L"%016llx", id) };
  ASSERT_EX(len == countof(buf) - 1);

  dataPath.reserve(importPath.size() + 1 + len);
  dataPath = importPath;
  dataPath += pathSeparator;
  dataPath += UStringView{ buf, static_cast<usize>(len) };
}

ImporterPtr Library::createImporter(UStringView file, usize nameOffset, FolderImporterPtr& parent) {
  ImporterPtr imp;
  auto ext{ getExt(file) };
  if (!ext.empty()) {
    auto it{ formats->find(ext) };
    if (it != formats->end()) {
      imp            = it->second.createImporter();
      imp->filePath  = file;
      imp->extOffset = static_cast<u16>(ext.data() - file.data() - 1);

      // TODO load meta

      // TODO match ID
      imp->id = hasher(file);

      // TODO insert into lookups, ensure ID is unique

      makeDataPath(imp->dataPath, imp->id);

      //imp->data = importPath + pathSeparator + ; TODO id->hex_string
    }
    else {
      goto createDefault;
    }
  }
  else {
  createDefault:
    imp = std::make_shared<NullImporter>();
    imp->filePath = file;
  }

  imp->parent     = parent;
  imp->nameOffset = static_cast<u16>(nameOffset);

  db.emplace(imp->filePath, imp);

  parent->insert(imp);
  return imp;
}

void Library::importFile(UStringView file, usize nameOffset, FolderImporterPtr& parent) {
  auto existing{ db.find(file) };
  auto importer{ existing != db.end()
    ? existing->second
    : createImporter(file, nameOffset, parent) };

  if (importer->canImport()) {
  #if 0
    auto time{ getLastUpdate(imp->data) };
    if (time == 0 || time < getLastUpdate(imp->file)) {
      imp->stateFlags |= Importer::NeedsImport;
    }
    if (imp->stateFlags != 0) {
      Task::run([imp]() { importTask(imp) });
    }
  #else
    Task::run([&importer]() { importTask(importer); });
  #endif
  }
}

void Library::importTask(ImporterPtr& importer) {
  importer->process();
}

u64 Library::find(UStringView path) {
  return 0u;
}

Object& Library::load(u64 path, ...) {
  UNREACHABLE; // TODO
}

static void saveImpl(ImporterPtr const& importer, Object const& o) {
  // Save Generic Asset ?
  // Save Importer

  // Save Imported Binary Asset
  std::ofstream f{ importer->getDataPath(), std::ios::out | std::ios::binary };

  // write ArchiveHeader

  o.save(f);
}

void Library::save(UStringView path, Object& o) {
  std::scoped_lock _(mutex);

  if (auto it{ db.find(path) }; it != db.end()) {
    saveImpl(it->second, o);
  }
  else {
    auto importer{ std::make_shared<GenericImporter>() };
    importer->filePath.reserve(assetsPath.size() + 1 + path.size());
    importer->filePath = assetsPath;
    importer->filePath += pathSeparator;
    importer->filePath += path;
    importer->id = hasher(importer->filePath);

    makeDataPath(importer->dataPath, importer->id);

    o.setId(importer->id);

    saveImpl(importer, o);
  }
}

void Library::save(u64 id, Object const& o) {
  std::scoped_lock _(mutex);

  if (auto it{ dbById.find(id) }; it != dbById.end()) {
    saveImpl(it->second, o);
  }
  else {
    ASSERT(0, "Invalid asset ID: %lld", id);
  }
}

void Library::move(u64 id, UStringView to) {
  std::scoped_lock _(mutex);
  // TODO update lookups to new path
  // TODO fs move
}

u64 Library::copy(u64 id, UStringView to) {
  std::scoped_lock _(mutex);
  // TODO new ID
  // copy asset data
  // add to lookups
  // fs save
  return 0;
}

} // namespace Asset
