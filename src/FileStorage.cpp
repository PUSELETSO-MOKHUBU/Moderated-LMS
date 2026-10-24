#include "FileStorage.h"

#include <openssl/rand.h>

#include <array>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

std::string randomFileId() {
    std::array<unsigned char, 16> bytes{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) {
        throw std::runtime_error("Unable to generate a safe upload filename.");
    }
    static const char digits[] = "0123456789abcdef";
    std::string value;
    value.reserve(bytes.size() * 2);
    for (unsigned char byte : bytes) {
        value.push_back(digits[byte >> 4]);
        value.push_back(digits[byte & 0x0f]);
    }
    return value;
}

std::string lowerExtension(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    for (char& character : extension) {
        character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
    }
    return extension;
}

}  // namespace

FileStorage::FileStorage(std::filesystem::path root) : root_(std::move(root)) {}

bool FileStorage::isSupportedDocument(const std::filesystem::path& path) {
    static const std::array<std::string, 12> supported = {
        ".pdf", ".doc", ".docx", ".odt", ".rtf", ".txt",
        ".ppt", ".pptx", ".xls", ".xlsx", ".csv", ".md"
    };
    const std::string extension = lowerExtension(path);
    for (const auto& candidate : supported) {
        if (extension == candidate) return true;
    }
    return false;
}

std::string FileStorage::storeDocument(const std::string& sourcePath, UploadKind kind) const {
    if (sourcePath.empty() || sourcePath.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument("Select a document file.");
    }
    const std::filesystem::path source = std::filesystem::u8path(sourcePath);
    std::error_code error;
    const auto sourceStatus = std::filesystem::status(source, error);
    if (error || !std::filesystem::is_regular_file(sourceStatus)) {
        throw std::invalid_argument("The selected document does not exist or is not a regular file.");
    }
    if (!isSupportedDocument(source)) {
        throw std::invalid_argument(
            "Unsupported document format. Allowed: PDF, DOC/DOCX, ODT, RTF, TXT/MD, PPT/PPTX, XLS/XLSX, CSV.");
    }
    const std::uintmax_t size = std::filesystem::file_size(source, error);
    if (error) throw std::runtime_error("Unable to read the selected document size.");
    if (size == 0) throw std::invalid_argument("The selected document is empty.");
    if (size > maximumFileBytes()) throw std::invalid_argument("Documents must not exceed 25 MB.");

    const std::filesystem::path directory =
        root_ / (kind == UploadKind::Assignment ? "assignments" : "submissions");
    std::filesystem::create_directories(directory, error);
    if (error) throw std::runtime_error("Unable to create the upload storage directory: " + error.message());

    for (int attempt = 0; attempt < 5; ++attempt) {
        const std::filesystem::path destination =
            directory / (randomFileId() + lowerExtension(source));
        error.clear();
        if (std::filesystem::copy_file(source, destination, std::filesystem::copy_options::none, error)) {
            return destination.generic_string();
        }
        if (error != std::errc::file_exists) {
            throw std::runtime_error("Unable to store the document: " + error.message());
        }
    }
    throw std::runtime_error("Unable to create a unique document filename.");
}

void FileStorage::openDocument(const std::string& storedPath) const {
    if (storedPath.empty()) throw std::invalid_argument("No document is attached to this record.");
    std::filesystem::path path = std::filesystem::u8path(storedPath);
    if (path.is_relative()) path = std::filesystem::current_path() / path;
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error) || error) {
        throw std::runtime_error("Document file is missing or inaccessible: " + storedPath);
    }
#ifdef _WIN32
    const std::wstring nativePath = path.wstring();
    const HINSTANCE result = ShellExecuteW(nullptr, L"open", nativePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if (reinterpret_cast<std::intptr_t>(result) <= 32) {
        throw std::runtime_error("Windows could not open the document with its registered application.");
    }
#else
    throw std::runtime_error("Opening documents is currently supported only on Windows.");
#endif
}

void FileStorage::removeStoredFile(const std::string& storedPath) const {
    if (storedPath.empty()) return;
    const std::filesystem::path path = std::filesystem::u8path(storedPath);
    const std::filesystem::path normalized = path.lexically_normal();
    const std::filesystem::path relative = normalized.lexically_relative(root_.lexically_normal());
    if (relative.empty() || relative.is_absolute() || *relative.begin() == "..") {
        throw std::invalid_argument("The stored upload path is outside the upload directory.");
    }
    std::error_code error;
    const bool removed = std::filesystem::remove(normalized, error);
    if (error) throw std::runtime_error("Unable to remove an uncommitted upload: " + error.message());
    if (!removed && std::filesystem::exists(normalized)) {
        throw std::runtime_error("Unable to remove an uncommitted upload.");
    }
}
