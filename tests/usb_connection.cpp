#include "usb_connection.h"

#include <cassert>

int main() {
  Aos::UsbConnection usb;
  assert(!usb.visible());
  usb.update(true);
  assert(usb.visible());
  usb.update(true, true);
  assert(!usb.visible());
  usb.update(true);
  assert(!usb.visible());
  usb.update(false);
  usb.update(true);
  assert(usb.visible());
  usb.update(false);
  assert(!usb.visible());

  assert(!usb.shouldIgnoreKeys(0));
  usb.didLeaveScreen(1000);
  assert(usb.shouldIgnoreKeys(1000));
  assert(usb.shouldIgnoreKeys(1179));
  assert(!usb.shouldIgnoreKeys(1180));
  usb.update(true);
  usb.update(false);
  usb.didLeaveScreen(2000);
  assert(usb.shouldIgnoreKeys(2010));
}
