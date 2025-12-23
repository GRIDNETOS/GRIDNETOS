#pragma once
// CKeyWordState.h

class CKeyWordState {
public:
    virtual ~CKeyWordState() = default;

    /**
     * @brief Gets the unique keyword state identifier.
     * @return The keyword state ID.
     */
    virtual const std::vector<uint8_t>& getKeywordStateID() const = 0;
};
