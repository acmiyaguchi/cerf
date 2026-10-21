#include "qt_key_input.h"

#include "qt_evdev_vk.h"

#include <cerfcore/core/cerf_emulator.h>
#include <cerfcore/core/log.h>

#include <QGuiApplication>
#include <QKeyEvent>

#include <iterator>

REGISTER_SERVICE(QtKeyInput);

#if !defined(__linux__)
#error "cerf-qt reads key positions as Linux evdev codes; this host has no key position table"
#endif

bool QtKeyInput::Translate(const QKeyEvent* event, QtHostKey& out) {
    const uint32_t keycode = event->nativeScanCode();
    if (keycode == 0u) {
        LOG(Cerf, "cerf-qt: Qt platform %s gives Qt key 0x%X no key position\n",
            QGuiApplication::platformName().toUtf8().constData(),
            static_cast<unsigned>(event->key()));
        return false;
    }
    const uint32_t evdev   = keycode - kXKeycodeEvdevOffset;
    const uint8_t  vk      = keycode >= kXKeycodeEvdevOffset && evdev < std::size(kEvdevToVk)
                                 ? kEvdevToVk[evdev]
                                 : uint8_t{0};
    if (vk == 0u) {
        LOG(Cerf, "cerf-qt: host keycode %u has no Windows virtual key\n", keycode);
        return false;
    }
    out.vk       = vk;
    out.sided_vk = SidedVk(evdev, vk);
    return true;
}

void QtKeyInput::Note(const QtHostKey& key, bool key_up) {
    down_.set(key.vk, !key_up);
    down_.set(key.sided_vk, !key_up);
}
