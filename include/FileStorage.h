#pragma once

#include <filesystem>
#include <string>

enum class UploadKind {
    Assignment,
    Submission
};

class FileStorage {
public:
    explicit FileStorage(std::filesystem::path root = "uploads");

    std::string storeDocument(const std::string& sourcePath, UploadKind kind) const;
    void removeStoredFile(const std::string& storedPath) const;

    static bool isSupportedDocument(const std::filesystem::path& path);
    static constexpr std::uintmax_t maximumFileBytes() noexcept {
        return 25U * 1024U * 1024U;
    }

private:
    std::filesystem::path root_;
};
