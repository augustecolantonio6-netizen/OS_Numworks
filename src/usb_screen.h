#ifndef AOS_USB_SCREEN_H
#define AOS_USB_SCREEN_H

#include <apps/battery_view.h>
#include <apps/usb/usb_connected_controller.h>

namespace Aos {

class UsbScreen : public Window {
public:
  UsbScreen();
  void updateBattery(Ion::Battery::Charge charge, bool charging);
  void drawRect(KDContext *context, KDRect rect) const override;

private:
  int numberOfSubviews() const override { return 3; }
  View *subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;

  USB::USBConnectedController m_controller;
  MessageTextView m_title;
  BatteryView m_battery;
};

}

#endif
