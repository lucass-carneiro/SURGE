#include "files.hpp"

#include "logging.hpp"

// clang-format off
#include <yaml-cpp/yaml.h>
// clang-format on

#include <filesystem>

auto surge::files::validate_path(const char *path) noexcept -> bool {
  using std::printf;

  try {
    const std::filesystem::path fs_opath{path};

    if (!std::filesystem::exists(fs_opath)) {
      log_error("The file %s does not exist.", path);
      return false;
    }

    if (!std::filesystem::is_regular_file(fs_opath)) {
      log_error("The path %s does not point to a regular file.", path);
      return false;
    }

    return true;

  } catch (const std::exception &e) {
    log_error("Error while validating file %s: %s", path, e.what());
    return false;
  }
}