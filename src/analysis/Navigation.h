#pragma once

#include <cstdint>
#include <vector>

namespace analysis {

class Navigation {
public:
    // Navigate to addr, pushing previous onto back-stack, clearing forward.
    void go(uint64_t addr);

    // Returns the new current addr, or UINT64_MAX if nothing to pop.
    uint64_t back();
    uint64_t forward();

    uint64_t current()     const { return m_current; }
    bool     can_back()    const { return !m_back.empty(); }
    bool     can_forward() const { return !m_forward.empty(); }

    void clear();

private:
    std::vector<uint64_t> m_back;
    std::vector<uint64_t> m_forward;
    uint64_t              m_current = UINT64_MAX;
};

} // namespace analysis
