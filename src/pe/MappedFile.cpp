#include "MappedFile.h"

#include <windows.h>

namespace pe {

MappedFile::~MappedFile() { close(); }

MappedFile::MappedFile(MappedFile&& o) noexcept
    : m_file_handle(o.m_file_handle),
      m_mapping_handle(o.m_mapping_handle),
      m_data(o.m_data),
      m_size(o.m_size) {
    o.m_file_handle = o.m_mapping_handle = nullptr;
    o.m_data = nullptr;
    o.m_size = 0;
}

MappedFile& MappedFile::operator=(MappedFile&& o) noexcept {
    if (this != &o) {
        close();
        m_file_handle    = o.m_file_handle;
        m_mapping_handle = o.m_mapping_handle;
        m_data           = o.m_data;
        m_size           = o.m_size;
        o.m_file_handle = o.m_mapping_handle = nullptr;
        o.m_data = nullptr;
        o.m_size = 0;
    }
    return *this;
}

bool MappedFile::open(const std::wstring& path) {
    close();

    HANDLE file = ::CreateFileW(
        path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    LARGE_INTEGER size_li{};
    if (!::GetFileSizeEx(file, &size_li) || size_li.QuadPart == 0) {
        ::CloseHandle(file);
        return false;
    }

    HANDLE mapping = ::CreateFileMappingW(
        file, nullptr, PAGE_READONLY, 0, 0, nullptr);
    if (!mapping) {
        ::CloseHandle(file);
        return false;
    }

    void* view = ::MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    if (!view) {
        ::CloseHandle(mapping);
        ::CloseHandle(file);
        return false;
    }

    m_file_handle    = file;
    m_mapping_handle = mapping;
    m_data           = static_cast<const uint8_t*>(view);
    m_size           = static_cast<size_t>(size_li.QuadPart);
    return true;
}

void MappedFile::close() {
    if (m_data) { ::UnmapViewOfFile(m_data); m_data = nullptr; }
    if (m_mapping_handle) { ::CloseHandle(m_mapping_handle); m_mapping_handle = nullptr; }
    if (m_file_handle) { ::CloseHandle(m_file_handle); m_file_handle = nullptr; }
    m_size = 0;
}

} // namespace pe
