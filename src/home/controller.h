#ifndef HOME_CONTROLLER_H
#define HOME_CONTROLLER_H

#include <escher.h>

namespace Home {

class Controller : public ViewController {
public:
  Controller(Responder *parent, SelectableTableViewDataSource *, App *) :
    ViewController(parent) {}

  View *view() override { return &m_view; }
  bool handleEvent(Ion::Events::Event event) override;

private:
  class ContentView : public View {
  public:
    void drawRect(KDContext *ctx, KDRect rect) const override;
  };
  ContentView m_view;
};

}

#endif
