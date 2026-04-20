#pragma once

#include "MappedFile.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace pe {

struct Section {
    std::string name;
    uint32_t virtual_address = 0;
    uint32_t virtual_size    = 0;
    uint32_t raw_address     = 0;
    uint32_t raw_size        = 0;
    uint32_t characteristics = 0;

    bool executable() const;
    bool readable()   const;
    bool writable()   const;
};

struct ImportFunction {
    std::string name;          // empty when imported by ordinal
    uint16_t    ordinal = 0;
    uint16_t    hint    = 0;
    uint64_t    iat_va  = 0;   // VA of the IAT slot
};

struct ImportedModule {
    std::string dll;
    std::vector<ImportFunction> functions;
};

struct ExportedFunction {
    std::string name;          // empty when exported by ordinal
    uint16_t    ordinal = 0;
    uint32_t    rva     = 0;
    std::string forwarder;     // non-empty when the export forwards
};

class PEImage {
public:
    bool load(const std::wstring& path);
    bool loaded() const { return m_loaded; }

    const std::wstring& path() const { return m_path; }

    bool     is_64bit()        const { return m_is64; }
    uint16_t machine()         const { return m_machine; }
    uint64_t image_base()      const { return m_image_base; }
    uint64_t image_size()      const { return m_image_size; }
    uint32_t entry_point_rva() const { return m_entry_rva; }
    uint64_t entry_point_va()  const { return m_image_base + m_entry_rva; }
    uint32_t section_alignment() const { return m_section_alignment; }
    uint32_t file_alignment()    const { return m_file_alignment; }

    const std::vector<Section>&          sections() const { return m_sections; }
    const std::vector<ImportedModule>&   imports()  const { return m_imports;  }
    const std::vector<ExportedFunction>& exports()  const { return m_exports;  }

    const uint8_t* data()      const { return m_file.data(); }
    size_t         file_size() const { return m_file.size(); }

    // Address translation
    std::optional<uint32_t> rva_to_file_offset(uint32_t rva) const;
    std::optional<uint32_t> va_to_rva(uint64_t va) const;
    const Section* find_section_by_rva(uint32_t rva) const;
    const Section* find_section_by_va(uint64_t va) const;

    // PDB path from CODEVIEW debug directory (if present)
    std::string pdb_path() const { return m_pdb_path; }
    // GUID + age for PDB matching
    uint8_t  pdb_guid[16]{};
    uint32_t pdb_age = 0;

private:
    bool parse_headers();
    bool parse_sections();
    bool parse_imports();
    bool parse_exports();
    bool parse_debug();

    MappedFile   m_file;
    std::wstring m_path;
    bool     m_loaded            = false;
    bool     m_is64              = false;
    uint16_t m_machine           = 0;
    uint64_t m_image_base        = 0;
    uint64_t m_image_size        = 0;
    uint32_t m_entry_rva         = 0;
    uint32_t m_section_alignment = 0;
    uint32_t m_file_alignment    = 0;
    uint32_t m_nt_headers_off    = 0;
    uint32_t m_num_data_dirs     = 0;

    struct DataDir { uint32_t rva; uint32_t size; };
    std::vector<DataDir> m_data_dirs;

    std::vector<Section>          m_sections;
    std::vector<ImportedModule>   m_imports;
    std::vector<ExportedFunction> m_exports;

    std::string m_pdb_path;
};

} // namespace pe
