#include <ion.h>
#include <kandinsky/ion_context.h>

#include "apps.h"
#include "usb_connection.h"
#include "usb_screen.h"

namespace {

using Aos::Key;
using Ion::Events::Event;

constexpr KDColor Background = KDColor::RGB24(0x121920);
constexpr KDColor Panel = KDColor::RGB24(0x1A2630);
constexpr KDColor Edge = KDColor::RGB24(0x354653);
constexpr KDColor Text = KDColor::RGB24(0xF2F6F7);
constexpr KDColor Muted = KDColor::RGB24(0xA6B8C4);
constexpr KDColor Accent = KDColor::RGB24(0x82EDD6);
constexpr KDColor LowBattery = KDColor::RGB24(0xFCA58D);
constexpr KDRect Screen(0, 0, Ion::Display::Width, Ion::Display::Height);
constexpr KDRect AppArea(0, 49, Ion::Display::Width, Ion::Display::Height - 49);
constexpr uint8_t Brightness = 160;
constexpr uint64_t IdleTimeoutMs = 5 * 60 * 1000;

struct Battery {
  Ion::Battery::Charge charge;
  bool charging;

  bool operator!=(const Battery &other) const {
    return charge != other.charge || charging != other.charging;
  }
};

Battery readBattery() {
  return {Ion::Battery::level(), Ion::Battery::isCharging()};
}

Key translateKey(Event event) {
  // Pages in Ion's event table represent Shift and Alpha variants.
  using Ion::Keyboard::Key;
  switch (static_cast<Key>(event.id() % Event::PageSize)) {
    case Key::Left: return Aos::Key::Left;
    case Key::Right: return Aos::Key::Right;
    case Key::Up: return Aos::Key::Up;
    case Key::Down: return Aos::Key::Down;
    case Key::OK:
    case Key::EXE: return Aos::Key::Confirm;
    case Key::Home: return Aos::Key::Home;
    case Key::Back: return Aos::Key::Back;
    default: return Aos::Key::Other;
  }
}

void drawBattery(KDContext &ctx, Battery battery) {
  ctx.fillRect(KDRect(273, 17, 29, 15), Background);
  ctx.strokeRect(KDRect(273, 17, 26, 15), Muted);
  ctx.fillRect(KDRect(299, 21, 3, 7), Muted);
  if (battery.charging) {
    ctx.drawString("+", KDPoint(282, 17), KDFont::SmallFont, Accent, Background);
    return;
  }
  const int level = static_cast<int>(battery.charge);
  const KDColor color = level <= 1 ? LowBattery : Accent;
  for (int i = 0; i < level; ++i) {
    ctx.fillRect(KDRect(276 + 7 * i, 20, 6, 9), color);
  }
}

void drawHome(KDContext &ctx, const Aos::Launcher &launcher) {
  ctx.drawString("Applications", KDPoint(16, 58), KDFont::SmallFont,
                 Text, Background);
  for (int i = 0; i < Aos::Launcher::PageSize; ++i) {
    const int index = launcher.pageStart() + i;
    const int x = 16 + (i % Aos::Launcher::Columns) * 98;
    const int y = 80 + (i / Aos::Launcher::Columns) * 54;
    const KDRect tile(x, y, 92, 46);
    if (index >= launcher.count()) {
      if (launcher.count() == 0) {
        ctx.strokeRect(tile, Edge);
      }
      continue;
    }
    const Aos::App &app = launcher.app(index);
    ctx.fillRect(tile, Panel);
    ctx.strokeRect(tile, index == launcher.selected() ? Accent : Edge);
    ctx.drawString(app.tileLabel, KDPoint(x + 7, y + 15), KDFont::SmallFont,
                   Text, Panel, 10);
  }

  if (launcher.count() == 0) {
    ctx.drawString("Aucune application", KDPoint(16, 204), KDFont::SmallFont,
                   Muted, Background);
    return;
  }
  ctx.drawString(launcher.app(launcher.selected()).name, KDPoint(16, 204),
                 KDFont::SmallFont, Text, Background, 29);
  ctx.drawString("OK", KDPoint(287, 204), KDFont::SmallFont, Accent, Background);
}

void draw(KDContext &ctx, const Aos::Launcher &launcher, Battery battery) {
  ctx.setOrigin(KDPoint(0, 0));
  ctx.setClippingRect(Screen);
  ctx.fillRect(Screen, Background);
  ctx.fillRect(KDRect(0, 0, Ion::Display::Width, 2), Accent);
  ctx.drawString("AOS", KDPoint(16, 15), KDFont::LargeFont, Accent, Background);
  ctx.drawString("Auguste OS", KDPoint(56, 17), KDFont::SmallFont, Muted, Background);
  ctx.fillRect(KDRect(16, 46, 288, 1), Edge);
  drawBattery(ctx, battery);

  if (const Aos::App *app = launcher.activeApp()) {
    ctx.setClippingRect(AppArea);
    ctx.setOrigin(AppArea.origin());
    app->draw(ctx);
    ctx.setOrigin(KDPoint(0, 0));
    ctx.setClippingRect(Screen);
  } else {
    drawHome(ctx, launcher);
  }
}

}

void ion_main(int, const char * const[]) {
  Ion::updateSlotInfo();
  Ion::Backlight::init();
  Ion::Backlight::setBrightness(Brightness);
  Ion::LED::updateColorWithPlugAndCharge();

  Aos::Launcher launcher(Aos::NativeApps, Aos::NativeAppCount);
  Aos::UsbConnection usb;
  Aos::UsbScreen usbScreen;
  usbScreen.setFrame(Screen, true);
  KDContext &ctx = *KDIonContext::sharedContext();
  Battery battery = readBattery();
  const auto redraw = [&]() {
    if (usb.visible()) {
      usbScreen.updateBattery(battery.charge, battery.charging);
      usbScreen.redraw(true);
    } else {
      draw(ctx, launcher, battery);
    }
  };
  usb.update(Ion::USB::isPlugged());
  redraw();
  uint64_t lastKeyAt = Ion::Timing::millis();
  uint64_t lastBatteryRead = lastKeyAt;

  while (true) {
    int timeout = 500;
    const Event event = Ion::Events::getEvent(&timeout);
    uint64_t now = Ion::Timing::millis();
    if (event.isKeyboardEvent()) {
      lastKeyAt = now;
    }
    const bool powerKey = event == Ion::Events::OnOff;
    if (powerKey || now - lastKeyAt >= IdleTimeoutMs) {
      Ion::Events::setShiftAlphaStatus(Ion::Events::ShiftAlphaStatus::Default);
      Ion::Power::suspend(powerKey);
      Ion::Backlight::setBrightness(Brightness);
      battery = readBattery();
      usb.update(Ion::USB::isPlugged());
      redraw();
      lastKeyAt = lastBatteryRead = Ion::Timing::millis();
      continue;
    }

    const bool keyboard = event.isKeyboardEvent();
    const Key key = keyboard ? translateKey(event) : Key::Other;
    const bool wasVisible = usb.visible();
    usb.update(Ion::USB::isPlugged(), key == Key::Back);
    if (usb.visible() != wasVisible) {
      Ion::Events::setShiftAlphaStatus(Ion::Events::ShiftAlphaStatus::Default);
      redraw();
    } else if (!usb.visible() && keyboard && launcher.dispatch({key, event.id()})) {
      redraw();
    }
    const bool powerChanged = event == Ion::Events::USBPlug ||
                              event == Ion::Events::USBEnumeration ||
                              event == Ion::Events::BatteryCharging;
    if (powerChanged || now - lastBatteryRead >= 2000) {
      const Battery current = readBattery();
      if (current != battery) {
        battery = current;
        if (usb.visible()) {
          usbScreen.updateBattery(battery.charge, battery.charging);
          usbScreen.redraw(false);
        } else {
          drawBattery(ctx, battery);
        }
      }
      Ion::LED::updateColorWithPlugAndCharge();
      lastBatteryRead = now;
    }
  }
}
