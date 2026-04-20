#include "Engine.h"

namespace disasm {

bool Engine::init(bool is_64bit) {
    m_is64 = is_64bit;
    const ZydisMachineMode mode = is_64bit ? ZYDIS_MACHINE_MODE_LONG_64
                                           : ZYDIS_MACHINE_MODE_LEGACY_32;
    const ZydisStackWidth  sw   = is_64bit ? ZYDIS_STACK_WIDTH_64
                                           : ZYDIS_STACK_WIDTH_32;
    return ZYAN_SUCCESS(ZydisDecoderInit(&m_decoder, mode, sw));
}

} // namespace disasm
