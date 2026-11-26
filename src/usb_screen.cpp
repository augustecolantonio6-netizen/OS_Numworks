#include "usb_screen.h"

#include <apps/i18n.h>
namespace I18n {

// The stock controller uses Upsilon's message IDs. AOS displays the USB
// connection itself, so its first paragraph describes the cable.
const char *translate(Message message) {
  switch (message) {
    case Message::USBAppCapital: return "USB";
    case Message::USBConnected: return "THE CALCULATOR IS CONNECTED";
    case Message::ConnectedMessage1: return "The USB cable is connected.";
    case Message::ConnectedMessage4: return "Press the BACK key of your";
    case Message::ConnectedMessage5: return "calculator or unplug it to";
    case Message::ConnectedMessage6: return "disconnect it.";
    default: return "";
  }
}

}

namespace Aos {

UsbScreen::UsbScreen() :
  m_title(KDFont::SmallFont, I18n::Message::USBAppCapital, 0.5f, 0.5f,
          Palette::ToolbarText, Palette::Toolbar) {}

void UsbScreen::updateBattery(Ion::Battery::Charge charge, bool charging) {
  m_battery.setChargeState(charge);
  m_battery.setIsCharging(charging);
  // Upsilon's icon shows the cable tick only when the battery is full.
  m_battery.setIsPlugged(charge == Ion::Battery::Charge::FULL);
}

void UsbScreen::drawRect(KDContext *context, KDRect) const {
  context->fillRect(KDRect(0, 0, bounds().width(), Metric::TitleBarHeight),
                    Palette::Toolbar);
}

View *UsbScreen::subviewAtIndex(int index) {
  if (index == 0) {
    return &m_title;
  }
  if (index == 1) {
    return &m_battery;
  }
  return m_controller.view();
}

void UsbScreen::layoutSubviews(bool force) {
  const KDCoordinate height = Metric::TitleBarHeight;
  const KDCoordinate width = bounds().width();
  const KDSize batterySize = m_battery.minimalSizeForOptimalDisplay();
  m_title.setFrame(KDRect(0, 2, width, height - 2), force);
  m_battery.setFrame(KDRect(width - batterySize.width() - Metric::TitleBarExternHorizontalMargin,
                            (height - batterySize.height()) / 2, batterySize), force);
  m_controller.view()->setFrame(KDRect(0, height, width, bounds().height() - height), force);
}

}
