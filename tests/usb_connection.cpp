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
}
