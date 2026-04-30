#pragma once

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstdio>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>
#include <vector>

// Tests use this when exercising process-global environment knobs. Restoring in
// the destructor keeps one test's MOOS-IvP path settings from leaking onward.
class ScopedEnv {
 public:
  ScopedEnv(const std::string& name, const std::string& value) : m_name(name)
  {
    const char* old_value = std::getenv(name.c_str());
    if(old_value != nullptr) {
      m_had_original = true;
      m_original = old_value;
    }
    setenv(m_name.c_str(), value.c_str(), 1);
  }

  ScopedEnv(const ScopedEnv&) = delete;
  ScopedEnv& operator=(const ScopedEnv&) = delete;

  ~ScopedEnv()
  {
    if(m_had_original)
      setenv(m_name.c_str(), m_original.c_str(), 1);
    else
      unsetenv(m_name.c_str());
  }

 private:
  std::string m_name;
  bool m_had_original = false;
  std::string m_original;
};

// Unique temp roots let CTest run library tests in parallel without shared
// split/log/cache directories colliding.
class TempDir {
 public:
  explicit TempDir(const std::string& stem)
  {
    std::string tmpl = ::testing::TempDir() + "/" + stem + "_XXXXXX";
    std::vector<char> buffer(tmpl.begin(), tmpl.end());
    buffer.push_back('\0');
    char* result = mkdtemp(buffer.data());
    EXPECT_NE(result, nullptr);
    if(result != nullptr)
      m_path = result;
  }

  TempDir(const TempDir&) = delete;
  TempDir& operator=(const TempDir&) = delete;

  TempDir(TempDir&& other) noexcept : m_path(std::move(other.m_path))
  {
    other.m_path.clear();
  }

  TempDir& operator=(TempDir&& other) noexcept
  {
    if(this != &other) {
      if(!m_path.empty())
        removeTree(m_path);
      m_path = std::move(other.m_path);
      other.m_path.clear();
    }
    return *this;
  }

  ~TempDir()
  {
    if(!m_path.empty())
      removeTree(m_path);
  }

  const std::string& path() const { return m_path; }

  std::string filePath(const std::string& name) const
  {
    return m_path + "/" + name;
  }

  std::string writeFile(const std::string& name, const std::string& contents) const
  {
    std::string out_path = filePath(name);
    std::ofstream out(out_path.c_str());
    EXPECT_TRUE(out.is_open());
    out << contents;
    return out_path;
  }

 private:
  // Test-only cleanup: remove anything the test wrote under its private root.
  static void removeTree(const std::string& path)
  {
    DIR* dir = opendir(path.c_str());
    if(dir == nullptr) {
      std::remove(path.c_str());
      return;
    }

    dirent* entry = nullptr;
    while((entry = readdir(dir)) != nullptr) {
      std::string name = entry->d_name;
      if(name == "." || name == "..")
        continue;
      std::string child = path + "/" + name;
      struct stat st;
      if(lstat(child.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
        removeTree(child);
      else
        std::remove(child.c_str());
    }
    closedir(dir);
    rmdir(path.c_str());
  }

  std::string m_path;
};

// Many MOOS-IvP readers care about file suffixes, so this helper preserves
// extensions like .alog, .moos, .zaic, and .ipp while still making unique paths.
class TempFile {
 public:
  TempFile(const std::string& stem,
           const std::string& contents,
           const std::string& suffix = "")
  {
    std::string tmpl = ::testing::TempDir() + "/" + stem + "_XXXXXX" + suffix;
    std::vector<char> buffer(tmpl.begin(), tmpl.end());
    buffer.push_back('\0');
    int fd = suffix.empty() ? mkstemp(buffer.data())
                            : mkstemps(buffer.data(),
                                       static_cast<int>(suffix.size()));
    EXPECT_NE(fd, -1);
    if(fd != -1) {
      close(fd);
      m_path = buffer.data();
      std::ofstream out(m_path.c_str());
      EXPECT_TRUE(out.is_open());
      out << contents;
    }
  }

  TempFile(const TempFile&) = delete;
  TempFile& operator=(const TempFile&) = delete;

  TempFile(TempFile&& other) noexcept : m_path(std::move(other.m_path))
  {
    other.m_path.clear();
  }

  TempFile& operator=(TempFile&& other) noexcept
  {
    if(this != &other) {
      if(!m_path.empty())
        std::remove(m_path.c_str());
      m_path = std::move(other.m_path);
      other.m_path.clear();
    }
    return *this;
  }

  ~TempFile()
  {
    if(!m_path.empty())
      std::remove(m_path.c_str());
  }

  const std::string& path() const { return m_path; }

 private:
  std::string m_path;
};
