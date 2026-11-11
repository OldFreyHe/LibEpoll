#ifndef LIBEPOLL_MESSAGE_ID_H
#define LIBEPOLL_MESSAGE_ID_H

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

// Process-wide registry used only during startup to catch protocol mistakes.
class MessageIdRegistry
{
public:
    static MessageIdRegistry& instance()
    {
        static MessageIdRegistry value;
        return value;
    }

    void addCategory(uint16_t id, const char* name)
    {
        if (!categories_.insert(id).second)
        {
            std::cerr << "Duplicate message category: " << name
                      << " (0x" << std::hex << id << ")\n";
            assert(false);
        }
    }

    void addMessage(const char* name, uint16_t id)
    {
        if (!messages_.emplace(id, name).second)
        {
            std::cerr << "Duplicate message ID: " << name
                      << " (0x" << std::hex << id << ")\n";
            assert(false);
        }
    }

private:
    MessageIdRegistry() = default;
    std::unordered_map<uint16_t, std::string> messages_;
    std::unordered_set<uint16_t> categories_;
};

// IDs are 8-bit category followed by 8-bit message value.
#define LIBEPOLL_CATEGORY_ID(category, value) \
    inline constexpr uint16_t category = static_cast<uint16_t>(value); \
    inline const bool category##_registered = \
        (MessageIdRegistry::instance().addCategory(category, #category), true)

#define LIBEPOLL_MESSAGE_ID(category, message, value) \
    inline constexpr uint16_t category##_##message = \
        static_cast<uint16_t>((static_cast<uint16_t>(category) << 8) | \
                              static_cast<uint16_t>(value)); \
    inline const bool category##_##message##_registered = \
        (MessageIdRegistry::instance().addMessage(#category "_" #message, \
                                                   category##_##message), true)

namespace libepoll::example
{
LIBEPOLL_CATEGORY_ID(ECHO_CATEGORY, 0x01);
LIBEPOLL_MESSAGE_ID(ECHO_CATEGORY, REQUEST, 0x01);
LIBEPOLL_MESSAGE_ID(ECHO_CATEGORY, RESPONSE, 0x02);
}

#endif
