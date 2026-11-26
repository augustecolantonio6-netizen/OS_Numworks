#include "controller.h"
#include "../apps_container.h"

namespace {

constexpr KDColor Background = KDColor::RGB24(0x121920);
constexpr KDColor Panel = KDColor::RGB24(0x1A2630);
constexpr KDColor Edge = KDColor::RGB24(0x354653);
constexpr KDColor Text = KDColor::RGB24(0xF2F6F7);
constexpr KDColor Muted = KDColor::RGB24(0xA6B8C4);
constexpr KDColor Accent = KDColor::RGB24(0x82EDD6);
constexpr KDColor LowBattery = KDColor::RGB24(0xFCA58D);

struct Battery {
  Ion::Battery::Charge charge;
  bool charging;

};

Battery readBattery() {
  return {Ion::Battery::level(), Ion::Battery::isCharging()};
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

}

namespace Home {

bool Controller::handleEvent(Ion::Events::Event event) {
  AppsContainer *container = AppsContainer::sharedAppsContainer();
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    return container->switchTo(container->appSnapshotAtIndex(1));
  }
  return false;
}

void Controller::ContentView::drawRect(KDContext *ctx, KDRect) const {
  ctx->fillRect(bounds(), Background);
  ctx->fillRect(KDRect(0, 0, bounds().width(), 2), Accent);
  ctx->drawString("AOS", KDPoint(16, 15), KDFont::LargeFont, Accent, Background);
  ctx->drawString("Auguste OS", KDPoint(56, 17), KDFont::SmallFont, Muted, Background);
  ctx->drawString("v0.3.1", KDPoint(210, 17), KDFont::SmallFont, Muted, Background);
  ctx->fillRect(KDRect(16, 46, 288, 1), Edge);
  drawBattery(*ctx, readBattery());
  ctx->drawString("Applications", KDPoint(16, 58), KDFont::SmallFont, Text, Background);

  const KDRect tile(16, 80, 92, 46);
  ctx->fillRect(tile, Panel);
  ctx->strokeRect(tile, Accent);
  ctx->drawString(I18n::translate(I18n::Message::CalculApp), KDPoint(23, 95),
                  KDFont::SmallFont, Text, Panel);
  ctx->drawString("OK", KDPoint(287, 204), KDFont::SmallFont, Accent, Background);
}

}
