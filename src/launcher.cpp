#include "launcher.h"

namespace Aos {

bool Launcher::select(int index) {
  if (index < 0 || index >= m_count || index == m_selected) {
    return false;
  }
  m_selected = index;
  return true;
}

bool Launcher::dispatch(Input input) {
  if (m_active) {
    if (input.key == Key::Home || input.key == Key::Back) {
      if (m_active->leave) {
        m_active->leave();
      }
      m_active = nullptr;
      return true;
    }
    return m_active->handle && m_active->handle(input);
  }

  switch (input.key) {
    case Key::Left: return select(m_selected - 1);
    case Key::Right: return select(m_selected + 1);
    case Key::Up: return select(m_selected - Columns);
    case Key::Down: return select(m_selected + Columns);
    case Key::Confirm:
      if (m_count == 0) {
        return false;
      }
      m_active = &m_apps[m_selected];
      if (m_active->enter) {
        m_active->enter();
      }
      return true;
    case Key::Home: return select(0);
    default: return false;
  }
}

}
