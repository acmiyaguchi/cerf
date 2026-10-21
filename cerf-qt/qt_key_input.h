#pragma once

#include <cerfcore/core/service.h>

#include <bitset>
#include <cstdint>

class QKeyEvent;

struct QtHostKey {
    uint8_t vk       = 0;
    uint8_t sided_vk = 0;
};

class QtKeyInput : public cerfcore::Service {
public:
    using cerfcore::Service::Service;

    bool Translate(const QKeyEvent* event, QtHostKey& out);
    void Note(const QtHostKey& key, bool key_up);
    bool Down(uint8_t vk) const { return down_.test(vk); }

private:
    std::bitset<256> down_;
};
