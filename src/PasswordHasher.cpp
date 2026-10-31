#include "PasswordHasher.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <string>

namespace {
constexpr int kIterations = 200000;
constexpr int kSaltBytes = 16;
constexpr int kHashBytes = 32;

std::string toHex(const unsigned char* bytes, std::size_t length) {
    static const char digits[] = "0123456789abcdef";
    std::string output;
    output.reserve(length * 2);
    for (std::size_t index = 0; index < length; ++index) {
        output.push_back(digits[bytes[index] >> 4]);
        output.push_back(digits[bytes[index] & 0x0f]);
    }
    return output;
}

bool fromHex(const std::string& value, unsigned char* output, std::size_t length) {
    if (value.size() != length * 2) return false;
    for (std::size_t index = 0; index < length; ++index) {
        if (!std::isxdigit(static_cast<unsigned char>(value[index * 2])) ||
            !std::isxdigit(static_cast<unsigned char>(value[index * 2 + 1]))) {
            return false;
        }
        unsigned int byte = 0;
        try {
            byte = static_cast<unsigned int>(std::stoul(value.substr(index * 2, 2), nullptr, 16));
        } catch (const std::exception&) {
            return false;
        }
        output[index] = static_cast<unsigned char>(byte);
    }
    return true;
}
}

std::string PasswordHasher::hash(const std::string& password) {
    if (password.empty() || password.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::invalid_argument("Password must contain a valid number of characters.");
    }
    std::array<unsigned char, kSaltBytes> salt{};
    std::array<unsigned char, kHashBytes> derived{};
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1 ||
        PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()), salt.data(),
                          static_cast<int>(salt.size()), kIterations, EVP_sha256(),
                          static_cast<int>(derived.size()), derived.data()) != 1) {
        throw std::runtime_error("OpenSSL could not derive a password hash.");
    }
    return "pbkdf2_sha256$" + std::to_string(kIterations) + "$" +
           toHex(salt.data(), salt.size()) + "$" + toHex(derived.data(), derived.size());
}

bool PasswordHasher::verify(const std::string& password, const std::string& encodedHash) {
    if (password.empty() || password.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()) ||
        !isEncodedHash(encodedHash)) return false;
    const std::size_t iterationsEnd = encodedHash.find('$', 14);
    const std::size_t saltEnd = encodedHash.find('$', iterationsEnd + 1);
    if (iterationsEnd == std::string::npos || saltEnd == std::string::npos) return false;
    int iterations = 0;
    try {
        const std::string iterationText = encodedHash.substr(14, iterationsEnd - 14);
        std::size_t consumed = 0;
        iterations = std::stoi(iterationText, &consumed);
        if (consumed != iterationText.size()) return false;
    } catch (const std::exception&) {
        return false;
    }
    if (iterations < 100000 || iterations > 2000000) return false;
    std::array<unsigned char, kSaltBytes> salt{};
    std::array<unsigned char, kHashBytes> expected{};
    if (!fromHex(encodedHash.substr(iterationsEnd + 1, saltEnd - iterationsEnd - 1),
                 salt.data(), salt.size()) ||
        !fromHex(encodedHash.substr(saltEnd + 1), expected.data(), expected.size())) {
        return false;
    }
    std::array<unsigned char, kHashBytes> actual{};
    if (PKCS5_PBKDF2_HMAC(password.data(), static_cast<int>(password.size()), salt.data(),
                          static_cast<int>(salt.size()), iterations, EVP_sha256(),
                          static_cast<int>(actual.size()), actual.data()) != 1) {
        throw std::runtime_error("OpenSSL could not verify a password hash.");
    }
    return CRYPTO_memcmp(actual.data(), expected.data(), actual.size()) == 0;
}

bool PasswordHasher::isEncodedHash(const std::string& value) {
    if (value.rfind("pbkdf2_sha256$", 0) != 0) return false;
    const std::size_t iterationsEnd = value.find('$', 14);
    const std::size_t saltEnd = value.find('$', iterationsEnd + 1);
    if (iterationsEnd == std::string::npos || saltEnd == std::string::npos) return false;
    try {
        const std::string iterationText = value.substr(14, iterationsEnd - 14);
        std::size_t consumed = 0;
        const int iterations = std::stoi(iterationText, &consumed);
        std::array<unsigned char, kSaltBytes> salt{};
        std::array<unsigned char, kHashBytes> digest{};
        return consumed == iterationText.size() && iterations >= 100000 && iterations <= 2000000 &&
               fromHex(value.substr(iterationsEnd + 1, saltEnd - iterationsEnd - 1),
                       salt.data(), salt.size()) &&
               fromHex(value.substr(saltEnd + 1), digest.data(), digest.size());
    } catch (const std::exception&) {
        return false;
    }
}
