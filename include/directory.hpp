#ifndef DIRECTORY_HPP
#define DIRECTORY_HPP

#include <memory>
#include <algorithm>
#include <regex>
#include <stdexcept>
#include <cerrno>
#include <dirent.h>

auto dircloser = [](DIR* dir) { return closedir(dir); };
using directory = std::unique_ptr<DIR, decltype(dircloser)>;

inline
directory open_dir(std::string const& path)
{
  auto dir = opendir(path.c_str());

  if (dir == nullptr) {
    auto const e = errno;
    throw std::runtime_error(strerror(e));
  }

  return directory(dir, dircloser);
}

inline
std::vector<std::string> ls(directory& dir)
{
  rewinddir(dir.get());
  dirent const* element = nullptr;

  std::vector<std::string> ret;

  while ((element = readdir(dir.get()))) {
    ret.push_back(element->d_name);
  }

  std::sort(ret.begin(), ret.end());

  return ret;
}

inline
std::vector<std::string> matching_items(directory& dir, std::string pattern)
{
  auto const list = ls(dir);

  std::regex const regex { pattern };

  std::vector<std::string> ret;

  std::copy_if(
      begin(list)
    , end(list)
    , std::back_inserter(ret)
    , [&regex](std::string const& item) {
        return std::regex_match(std::begin(item), std::end(item), regex);
      }
  );

  return ret;
}

#endif // DIRECTORY_HPP
