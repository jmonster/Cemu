// Exercise the production SDK header without Cemu's precompiled header.
#include "HostFile.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
using Switch2Kit::HostFileResult;
using Switch2Kit::readHostFile;

static std::string utf8(const fs::path& path) {
    const auto bytes = path.u8string();
    return std::string(bytes.begin(), bytes.end());
}

static void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

static void reject(const std::string& path, size_t limit,
                   HostFileResult expected = HostFileResult::Invalid) {
    std::string output = "unchanged";
    require(readHostFile(path, limit, output) == expected, "unexpected failure result");
    require(output == "unchanged", "failure changed the caller's output");
}

static void checkFile(const fs::path& path, const std::string& bytes, size_t limit) {
    {
        std::ofstream file(path, std::ios::binary);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        require(file.good(), "could not create fixture");
    }
    std::string output = "old contents";
    require(readHostFile(utf8(path), limit, output) == HostFileResult::OK, "regular file rejected");
    require(output == bytes, "file bytes changed or were truncated");
    if (bytes.size() > 1) reject(utf8(path), bytes.size() - 1);
}

int main() {
    try {
        const auto root = fs::current_path() / "host-file-fixtures";
        require(fs::create_directory(root), "fixture directory already exists");
        const auto path = root / "profile.bin";
        checkFile(path, "", 1);
        checkFile(path, std::string("a\r\nb\x1a\0\xff", 7), 7); // No CRT text translation.
        checkFile(path, std::string(524288, 'x'), 524288);
        checkFile(root / fs::path(u8"profile-\u03c0-\U0001f3ae.json"), "unicode path", 12);
        reject(utf8(path), 0);
        reject(utf8(path), 524289);
        reject("", 1024);
        reject(std::string(4097, 'x'), 1024);
        reject("bad\npath", 1024);
        reject("bad\x7fpath", 1024);
        reject(std::string("bad\0path", 8), 1024);
        reject(utf8(root), 1024);
        reject(utf8(root / "missing"), 1024, HostFileResult::Missing);
        reject(utf8(root / "absent" / "missing"), 1024, HostFileResult::Missing);
#if defined(_WIN32)
        reject("NUL", 1024);
        reject(std::string("\xc0\xaf", 2), 1024); // Invalid UTF-8, not an ANSI path.
        const auto name = L"\\\\.\\pipe\\switch2kit-host-file-" + std::to_wstring(::GetCurrentProcessId());
        const HANDLE pipe = ::CreateNamedPipeW(name.c_str(), PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 4096, 4096, 0, nullptr);
        require(pipe != INVALID_HANDLE_VALUE, "could not create named-pipe fixture");
        struct Close { HANDLE value; ~Close() { ::CloseHandle(value); } } close{pipe};
        reject("\\\\.\\pipe\\switch2kit-host-file-" + std::to_string(::GetCurrentProcessId()), 1024);
#else
        reject("/dev/null", 1024);
        const auto fifo = root / "fifo";
        require(::mkfifo(fifo.c_str(), 0600) == 0, "could not create FIFO fixture");
        reject(utf8(fifo), 1024); // Must not block waiting for a writer.
#endif
        fs::remove_all(root);
        std::cout << "Host-file regressions passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
