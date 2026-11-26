#ifndef AOS_LAUNCHER_H
#define AOS_LAUNCHER_H

#include <stdint.h>

class KDContext;

namespace Aos {

enum class Key { Left, Right, Up, Down, Confirm, Back, Home, Other };

struct Input {
  Key key;
  uint8_t rawEvent;
};

struct App {
  const char *name;
  const char *tileLabel;
  void (*draw)(KDContext &);
  void (*enter)();
  void (*leave)();
  bool (*handle)(Input);
};

class Launcher {
public:
  static constexpr int Columns = 3;
  static constexpr int PageSize = 6;

  Launcher(const App *apps, int count) : m_apps(apps), m_count(count) {}
  int count() const { return m_count; }
  int selected() const { return m_selected; }
  int pageStart() const { return m_selected / PageSize * PageSize; }
  const App &app(int index) const { return m_apps[index]; }
  const App *activeApp() const { return m_active; }
  bool dispatch(Input input);

private:
  bool select(int index);
  const App *m_apps;
  int m_count;
  int m_selected = 0;
  const App *m_active = nullptr;
};

}
#endif
