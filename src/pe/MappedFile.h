#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace pe {

class MappedFile {
public:
    MappedFile() = default;
    ~MappedFile();

    MappedFile(const MappedFile&)            = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& o) noexcept;
    MappedFile& operator=(MappedFile&& o) noexcept;

    bool open(const std::wstring& path);
    void close();

    const uint8_t* data() const { return m_data; }
    size_t         size() const { return m_size; }
    bool           valid() const { return m_data != nullptr; }

private:
    void* m_file_handle    = nullptr; // HANDLE
    void* m_mapping_handle = nullptr; // HANDLE
    const uint8_t* m_data  = nullptr;
    size_t m_size          = 0;
};

} // namespace pe
