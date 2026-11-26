#include "launcher.h"

#include <cassert>

namespace {

int entries = 0;
int exits = 0;
uint8_t lastEvent = 0;

void enter() { ++entries; }
void leave() { ++exits; }
void draw(KDContext &) {}
bool handle(Aos::Input input) {
  lastEvent = input.rawEvent;
  return true;
}

}

int main() {
  using Aos::Key;
  Aos::Launcher empty(nullptr, 0);
  assert(!empty.dispatch({Key::Confirm, 0}));
  assert(!empty.dispatch({Key::Right, 0}));
  assert(empty.activeApp() == nullptr);

  const Aos::App app{"Test", "Test", draw, enter, leave, handle};
  const Aos::App apps[] = {app, app, app, app, app, app, app};
  Aos::Launcher launcher(apps, 7);
  assert(!launcher.dispatch({Key::Left, 0}));
  assert(launcher.dispatch({Key::Down, 0}));
  assert(launcher.selected() == 3 && launcher.pageStart() == 0);
  assert(launcher.dispatch({Key::Down, 0}));
  assert(launcher.selected() == 6 && launcher.pageStart() == 6);
  assert(!launcher.dispatch({Key::Right, 0}));
  assert(!launcher.dispatch({Key::Down, 0}));
  assert(launcher.dispatch({Key::Confirm, 0}));
  assert(launcher.activeApp() == &apps[6] && entries == 1);
  assert(launcher.dispatch({Key::Other, 42}) && lastEvent == 42);
  assert(launcher.dispatch({Key::Back, 0}));
  assert(launcher.activeApp() == nullptr && launcher.selected() == 6 && exits == 1);
  assert(launcher.dispatch({Key::Confirm, 0}));
  assert(launcher.dispatch({Key::Home, 0}));
  assert(launcher.activeApp() == nullptr && exits == 2);
  assert(launcher.dispatch({Key::Home, 0}) && launcher.selected() == 0);
}
