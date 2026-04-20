#include "PEImage.h"

#include <windows.h>
#include <cstring>

namespace pe {

namespace {

template <typename T>
const T* read_at(const uint8_t* base, size_t size, size_t offset) {
    if (offset + sizeof(T) > size) return nullptr;
    return reinterpret_cast<const T*>(base + offset);
}

std::string read_cstr(const uint8_t* base, size_t size, size_t offset, size_t max = 0x1000) {
    if (offset >= size) return {};
    const size_t end = (std::min)(size, offset + max);
    size_t i = offset;
    while (i < end && base[i] != 0) ++i;
    return std::string(reinterpret_cast<const char*>(base + offset), i - offset);
}

// CodeView record layout for "RSDS" (PDB 7.0)
#pragma pack(push, 1)
struct CvInfoPdb70 {
    uint32_t cv_signature; // 'SDSR'
    uint8_t  guid[16];
    uint32_t age;
    // followed by NUL-terminated pdb path (ASCII/UTF-8)
};
#pragma pack(pop)

} // namespace

// -----------------------------------------------------------------------------
// Section flags
bool Section::executable() const { return (characteristics & IMAGE_SCN_MEM_EXECUTE) != 0; }
bool Section::readable()   const { return (characteristics & IMAGE_SCN_MEM_READ)    != 0; }
bool Section::writable()   const { return (characteristics & IMAGE_SCN_MEM_WRITE)   != 0; }

// -----------------------------------------------------------------------------
bool PEImage::load(const std::wstring& path) {
    m_loaded = false;
    m_sections.clear();
    m_imports.clear();
    m_exports.clear();
    m_data_dirs.clear();
    m_pdb_path.clear();
    m_path = path;

    if (!m_file.open(path)) return false;
    if (!parse_headers()) return false;
    if (!parse_sections()) return false;
    parse_imports();   // best-effort
    parse_exports();   // best-effort
    parse_debug();     // best-effort
    m_loaded = true;
    return true;
}

// -----------------------------------------------------------------------------
bool PEImage::parse_headers() {
    const uint8_t* buf = m_file.data();
    const size_t   sz  = m_file.size();

    auto dos = read_at<IMAGE_DOS_HEADER>(buf, sz, 0);
    if (!dos || dos->e_magic != IMAGE_DOS_SIGNATURE) return false;

    m_nt_headers_off = static_cast<uint32_t>(dos->e_lfanew);
    auto sig = read_at<uint32_t>(buf, sz, m_nt_headers_off);
    if (!sig || *sig != IMAGE_NT_SIGNATURE) return false;

    auto file_hdr = read_at<IMAGE_FILE_HEADER>(buf, sz, m_nt_headers_off + 4);
    if (!file_hdr) return false;

    m_machine = file_hdr->Machine;

    // optional header — branch on magic
    const size_t opt_off = m_nt_headers_off + 4 + sizeof(IMAGE_FILE_HEADER);
    auto magic = read_at<uint16_t>(buf, sz, opt_off);
    if (!magic) return false;

    if (*magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        auto oh = read_at<IMAGE_OPTIONAL_HEADER64>(buf, sz, opt_off);
        if (!oh) return false;
        m_is64              = true;
        m_image_base        = oh->ImageBase;
        m_image_size        = oh->SizeOfImage;
        m_entry_rva         = oh->AddressOfEntryPoint;
        m_section_alignment = oh->SectionAlignment;
        m_file_alignment    = oh->FileAlignment;
        m_num_data_dirs     = oh->NumberOfRvaAndSizes;
        m_data_dirs.reserve(m_num_data_dirs);
        for (uint32_t i = 0; i < m_num_data_dirs; ++i) {
            m_data_dirs.push_back({ oh->DataDirectory[i].VirtualAddress,
                                    oh->DataDirectory[i].Size });
        }
    } else if (*magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        auto oh = read_at<IMAGE_OPTIONAL_HEADER32>(buf, sz, opt_off);
        if (!oh) return false;
        m_is64              = false;
        m_image_base        = oh->ImageBase;
        m_image_size        = oh->SizeOfImage;
        m_entry_rva         = oh->AddressOfEntryPoint;
        m_section_alignment = oh->SectionAlignment;
        m_file_alignment    = oh->FileAlignment;
        m_num_data_dirs     = oh->NumberOfRvaAndSizes;
        m_data_dirs.reserve(m_num_data_dirs);
        for (uint32_t i = 0; i < m_num_data_dirs; ++i) {
            m_data_dirs.push_back({ oh->DataDirectory[i].VirtualAddress,
                                    oh->DataDirectory[i].Size });
        }
    } else {
        return false;
    }

    return true;
}

// -----------------------------------------------------------------------------
bool PEImage::parse_sections() {
    const uint8_t* buf = m_file.data();
    const size_t   sz  = m_file.size();

    auto file_hdr = read_at<IMAGE_FILE_HEADER>(buf, sz, m_nt_headers_off + 4);
    if (!file_hdr) return false;

    const size_t opt_size = file_hdr->SizeOfOptionalHeader;
    const size_t sect_off = m_nt_headers_off + 4 + sizeof(IMAGE_FILE_HEADER) + opt_size;
    const uint16_t n = file_hdr->NumberOfSections;

    m_sections.reserve(n);
    for (uint16_t i = 0; i < n; ++i) {
        auto s = read_at<IMAGE_SECTION_HEADER>(buf, sz, sect_off + i * sizeof(IMAGE_SECTION_HEADER));
        if (!s) return false;
        Section out;
        char name[9] = {};
        std::memcpy(name, s->Name, 8);
        out.name             = name;
        out.virtual_address  = s->VirtualAddress;
        out.virtual_size     = s->Misc.VirtualSize;
        out.raw_address      = s->PointerToRawData;
        out.raw_size         = s->SizeOfRawData;
        out.characteristics  = s->Characteristics;
        m_sections.push_back(std::move(out));
    }
    return true;
}

// -----------------------------------------------------------------------------
std::optional<uint32_t> PEImage::rva_to_file_offset(uint32_t rva) const {
    for (const auto& s : m_sections) {
        const uint32_t vstart = s.virtual_address;
        const uint32_t vend   = vstart + (s.virtual_size ? s.virtual_size : s.raw_size);
        if (rva >= vstart && rva < vend) {
            const uint32_t delta = rva - vstart;
            if (delta >= s.raw_size) return std::nullopt; // uninitialized part
            return s.raw_address + delta;
        }
    }
    // RVA may lie in PE headers (before first section) — headers are flat-mapped.
    const uint32_t first_sec_rva =
        m_sections.empty() ? UINT32_MAX : m_sections.front().virtual_address;
    if (rva < first_sec_rva) return rva;
    return std::nullopt;
}

std::optional<uint32_t> PEImage::va_to_rva(uint64_t va) const {
    if (va < m_image_base) return std::nullopt;
    const uint64_t d = va - m_image_base;
    if (d > 0xFFFFFFFFull) return std::nullopt;
    return static_cast<uint32_t>(d);
}

const Section* PEImage::find_section_by_rva(uint32_t rva) const {
    for (const auto& s : m_sections) {
        const uint32_t vend = s.virtual_address + (s.virtual_size ? s.virtual_size : s.raw_size);
        if (rva >= s.virtual_address && rva < vend) return &s;
    }
    return nullptr;
}

const Section* PEImage::find_section_by_va(uint64_t va) const {
    auto r = va_to_rva(va);
    return r ? find_section_by_rva(*r) : nullptr;
}

// -----------------------------------------------------------------------------
// Imports
bool PEImage::parse_imports() {
    constexpr uint32_t kImportDir = IMAGE_DIRECTORY_ENTRY_IMPORT;
    if (m_data_dirs.size() <= kImportDir) return false;
    const auto& dir = m_data_dirs[kImportDir];
    if (dir.rva == 0 || dir.size == 0) return false;

    auto off = rva_to_file_offset(dir.rva);
    if (!off) return false;

    const uint8_t* buf = m_file.data();
    const size_t   sz  = m_file.size();

    size_t cur = *off;
    while (true) {
        auto desc = read_at<IMAGE_IMPORT_DESCRIPTOR>(buf, sz, cur);
        if (!desc) break;
        if (desc->Name == 0 && desc->FirstThunk == 0) break; // terminator

        ImportedModule mod;
        if (auto dll_off = rva_to_file_offset(desc->Name)) {
            mod.dll = read_cstr(buf, sz, *dll_off);
        }

        // Use OriginalFirstThunk (INT) for names; FirstThunk (IAT) for runtime addrs.
        const uint32_t int_rva = desc->OriginalFirstThunk ? desc->OriginalFirstThunk
                                                          : desc->FirstThunk;
        const uint32_t iat_rva = desc->FirstThunk;
        auto int_off = rva_to_file_offset(int_rva);
        if (!int_off) { cur += sizeof(IMAGE_IMPORT_DESCRIPTOR); continue; }

        size_t ip = *int_off;
        uint32_t slot = 0;
        while (true) {
            if (m_is64) {
                auto thunk = read_at<uint64_t>(buf, sz, ip);
                if (!thunk || *thunk == 0) break;
                ImportFunction f;
                f.iat_va = m_image_base + iat_rva + slot * 8;
                if (*thunk & 0x8000000000000000ull) {
                    f.ordinal = static_cast<uint16_t>(*thunk & 0xFFFF);
                } else {
                    const uint32_t name_rva = static_cast<uint32_t>(*thunk);
                    if (auto nm_off = rva_to_file_offset(name_rva)) {
                        auto hint = read_at<uint16_t>(buf, sz, *nm_off);
                        if (hint) f.hint = *hint;
                        f.name = read_cstr(buf, sz, *nm_off + 2);
                    }
                }
                mod.functions.push_back(std::move(f));
                ip += 8;
            } else {
                auto thunk = read_at<uint32_t>(buf, sz, ip);
                if (!thunk || *thunk == 0) break;
                ImportFunction f;
                f.iat_va = m_image_base + iat_rva + slot * 4;
                if (*thunk & 0x80000000u) {
                    f.ordinal = static_cast<uint16_t>(*thunk & 0xFFFF);
                } else {
                    const uint32_t name_rva = *thunk;
                    if (auto nm_off = rva_to_file_offset(name_rva)) {
                        auto hint = read_at<uint16_t>(buf, sz, *nm_off);
                        if (hint) f.hint = *hint;
                        f.name = read_cstr(buf, sz, *nm_off + 2);
                    }
                }
                mod.functions.push_back(std::move(f));
                ip += 4;
            }
            ++slot;
        }

        m_imports.push_back(std::move(mod));
        cur += sizeof(IMAGE_IMPORT_DESCRIPTOR);
    }
    return true;
}

// -----------------------------------------------------------------------------
// Exports
bool PEImage::parse_exports() {
    constexpr uint32_t kExportDir = IMAGE_DIRECTORY_ENTRY_EXPORT;
    if (m_data_dirs.size() <= kExportDir) return false;
    const auto& dir = m_data_dirs[kExportDir];
    if (dir.rva == 0 || dir.size == 0) return false;

    auto off = rva_to_file_offset(dir.rva);
    if (!off) return false;

    const uint8_t* buf = m_file.data();
    const size_t   sz  = m_file.size();

    auto ed = read_at<IMAGE_EXPORT_DIRECTORY>(buf, sz, *off);
    if (!ed) return false;

    auto func_off = rva_to_file_offset(ed->AddressOfFunctions);
    auto name_off = rva_to_file_offset(ed->AddressOfNames);
    auto ord_off  = rva_to_file_offset(ed->AddressOfNameOrdinals);
    if (!func_off) return false;

    const uint32_t n_funcs = ed->NumberOfFunctions;
    const uint32_t n_names = ed->NumberOfNames;
    const uint32_t ord_base = ed->Base;

    m_exports.reserve(n_funcs);

    // Build ordinal-to-name map first
    std::vector<std::pair<uint16_t, std::string>> name_by_ord;
    if (name_off && ord_off) {
        for (uint32_t i = 0; i < n_names; ++i) {
            auto name_rva = read_at<uint32_t>(buf, sz, *name_off + i * 4);
            auto ord_idx  = read_at<uint16_t>(buf, sz, *ord_off  + i * 2);
            if (!name_rva || !ord_idx) continue;
            if (auto nm_off = rva_to_file_offset(*name_rva)) {
                name_by_ord.emplace_back(*ord_idx, read_cstr(buf, sz, *nm_off));
            }
        }
    }

    const uint32_t dir_rva_end = dir.rva + dir.size;
    for (uint32_t i = 0; i < n_funcs; ++i) {
        auto f_rva_p = read_at<uint32_t>(buf, sz, *func_off + i * 4);
        if (!f_rva_p) continue;
        if (*f_rva_p == 0) continue;

        ExportedFunction e;
        e.ordinal = static_cast<uint16_t>(ord_base + i);
        e.rva     = *f_rva_p;

        // Match name
        for (const auto& nb : name_by_ord) {
            if (nb.first == i) { e.name = nb.second; break; }
        }

        // Forwarder? RVA lies inside the export directory range.
        if (e.rva >= dir.rva && e.rva < dir_rva_end) {
            if (auto fw_off = rva_to_file_offset(e.rva)) {
                e.forwarder = read_cstr(buf, sz, *fw_off);
            }
        }

        m_exports.push_back(std::move(e));
    }
    return true;
}

// -----------------------------------------------------------------------------
// Debug directory → PDB path (RSDS)
bool PEImage::parse_debug() {
    constexpr uint32_t kDebugDir = IMAGE_DIRECTORY_ENTRY_DEBUG;
    if (m_data_dirs.size() <= kDebugDir) return false;
    const auto& dir = m_data_dirs[kDebugDir];
    if (dir.rva == 0 || dir.size == 0) return false;

    auto off = rva_to_file_offset(dir.rva);
    if (!off) return false;

    const uint8_t* buf = m_file.data();
    const size_t   sz  = m_file.size();
    const uint32_t n_entries = dir.size / sizeof(IMAGE_DEBUG_DIRECTORY);
    for (uint32_t i = 0; i < n_entries; ++i) {
        auto e = read_at<IMAGE_DEBUG_DIRECTORY>(buf, sz, *off + i * sizeof(IMAGE_DEBUG_DIRECTORY));
        if (!e) continue;
        if (e->Type != IMAGE_DEBUG_TYPE_CODEVIEW) continue;

        const size_t raw = e->PointerToRawData;
        if (raw + sizeof(CvInfoPdb70) > sz) continue;
        auto cv = read_at<CvInfoPdb70>(buf, sz, raw);
        if (!cv) continue;
        if (cv->cv_signature != 0x53445352 /* 'SDSR' */) continue;

        std::memcpy(pdb_guid, cv->guid, 16);
        pdb_age = cv->age;
        m_pdb_path = read_cstr(buf, sz, raw + sizeof(CvInfoPdb70));
        return true;
    }
    return false;
}

} // namespace pe
