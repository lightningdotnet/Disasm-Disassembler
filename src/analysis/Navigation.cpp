#include "Navigation.h"

namespace analysis {

void Navigation::go(uint64_t addr) {
    if (m_current != UINT64_MAX && m_current != addr) {
        m_back.push_back(m_current);
    }
    m_current = addr;
    m_forward.clear();
}

uint64_t Navigation::back() {
    if (m_back.empty()) return UINT64_MAX;
    if (m_current != UINT64_MAX) m_forward.push_back(m_current);
    m_current = m_back.back();
    m_back.pop_back();
    return m_current;
}

uint64_t Navigation::forward() {
    if (m_forward.empty()) return UINT64_MAX;
    if (m_current != UINT64_MAX) m_back.push_back(m_current);
    m_current = m_forward.back();
    m_forward.pop_back();
    return m_current;
}

void Navigation::clear() {
    m_back.clear();
    m_forward.clear();
    m_current = UINT64_MAX;
}

} // namespace analysis
