#pragma once

#include <Zydis/Zydis.h>

namespace disasm {

class Engine {
public:
    bool init(bool is_64bit);

    const ZydisDecoder* decoder() const { return &m_decoder; }
    bool is_64bit() const { return m_is64; }

private:
    ZydisDecoder m_decoder{};
    bool m_is64 = false;
};

} // namespace disasm
