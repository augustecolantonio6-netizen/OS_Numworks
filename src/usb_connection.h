#ifndef AOS_USB_CONNECTION_H
#define AOS_USB_CONNECTION_H

#include <stdint.h>

namespace Aos {

class UsbConnection {
public:
  bool visible() const { return m_state == State::Visible; }

  void update(bool plugged, bool back = false) {
    if (!plugged) {
      m_state = State::Unplugged;
    } else if (m_state == State::Unplugged) {
      m_state = State::Visible;
    } else if (back) {
      m_state = State::Dismissed;
    }
  }

  void didLeaveScreen(uint64_t now) { m_ignoreKeysUntil = now + 180; }

  bool shouldIgnoreKeys(uint64_t now) const { return now < m_ignoreKeysUntil; }

private:
  enum class State { Unplugged, Visible, Dismissed };
  State m_state = State::Unplugged;
  uint64_t m_ignoreKeysUntil = 0;
};

}

#endif
