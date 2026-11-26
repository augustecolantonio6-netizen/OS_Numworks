#ifndef AOS_LATEX_APP_H
#define AOS_LATEX_APP_H

#include <apps/shared/poincare_local_pool.h>
#include <apps/shared/shared_app.h>
#include <escher.h>

namespace Latex {

class Editor : public ViewController, public LayoutFieldDelegate, public InputEventHandlerDelegate {
public:
  explicit Editor(Responder *parent);
  View *view() override { return &m_content; }
  void didBecomeFirstResponder() override;
  Poincare::Context *context() const override;
  Toolbox *toolboxForInputEventHandler(InputEventHandler *) override;
  NestedMenuController *variableBoxForInputEventHandler(InputEventHandler *) override;
  bool layoutFieldShouldFinishEditing(LayoutField *, Ion::Events::Event) override { return false; }
  bool layoutFieldDidReceiveEvent(LayoutField *, Ion::Events::Event event) override;

private:
  class Content : public View {
  public:
    explicit Content(Editor *editor);
    void drawRect(KDContext *ctx, KDRect rect) const override;
    ExpressionView preview;
    LayoutField field;
    BufferTextView status;

  private:
    int numberOfSubviews() const override { return 3; }
    View *subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
  };

  void saveLine();
  const char *appendLine(Poincare::Layout layout);

  Content m_content;
  char m_document[4096];
};

class App : public ::App {
public:
  class Descriptor : public ::App::Descriptor {
  public:
    I18n::Message name() override;
    I18n::Message upperName() override;
    const Image *icon() override;
  };

  class Snapshot : public SharedApp::Snapshot {
  public:
    App *unpack(Container *container) override;
    Descriptor *descriptor() override;
  };

private:
  explicit App(Snapshot *snapshot);
  Shared::PoincareLocalPool m_pool;
  Editor m_editor;
};

}

#endif
