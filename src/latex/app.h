#ifndef AOS_LATEX_APP_H
#define AOS_LATEX_APP_H

#include <apps/shared/poincare_local_pool.h>
#include <apps/shared/shared_app.h>
#include <escher.h>

namespace Latex {

class Editor : public ViewController, public LayoutFieldDelegate,
               public InputEventHandlerDelegate, public TextFieldDelegate {
public:
  explicit Editor(Responder *parent);
  View *view() override { return &m_content; }
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  Poincare::Context *context() const override;
  Toolbox *toolboxForInputEventHandler(InputEventHandler *) override;
  NestedMenuController *variableBoxForInputEventHandler(InputEventHandler *) override;
  bool layoutFieldShouldFinishEditing(LayoutField *, Ion::Events::Event) override { return false; }
  bool layoutFieldDidReceiveEvent(LayoutField *, Ion::Events::Event event) override;
  void layoutFieldDidChangeSize(LayoutField *) override;
  bool textFieldShouldFinishEditing(TextField *, Ion::Events::Event event) override;
  bool textFieldDidReceiveEvent(TextField *, Ion::Events::Event) override { return false; }
  bool textFieldDidFinishEditing(TextField *, const char *text, Ion::Events::Event) override;
  bool textFieldDidAbortEditing(TextField *) override;

private:
  enum class Screen { List, Options, Rename, Edit };
  using Record = Ion::Storage::Record;
  static constexpr int MaxName = 24;
  static constexpr int VisibleScripts = 6;

  class MathField : public LayoutField {
  public:
    explicit MathField(Editor *editor) : LayoutField(editor, editor, editor), m_editor(editor) {}
    bool handleEvent(Ion::Events::Event event) override;

  private:
    Editor *m_editor;
  };

  class Content : public View {
  public:
    explicit Content(Editor *editor);
    void refresh();
    void drawRect(KDContext *ctx, KDRect rect) const override;
    ExpressionView previous;
    MathField field;
    ExpressionView next;
    TextField name;

  private:
    Editor *m_editor;
    int numberOfSubviews() const override;
    View *subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    KDRect rowFrame(int offset) const;
    void drawList(KDContext *ctx) const;
    void drawEditor(KDContext *ctx) const;
    void drawOptions(KDContext *ctx) const;
  };

  Record scriptAt(int index) const;
  int scriptCount() const;
  void transition(Screen screen, const char *status);
  bool openScript();
  void createScript();
  void renameScript();
  void loadLine(int line);
  bool saveLine();
  const char *lineStart(int line) const;
  Poincare::Layout lineLayout(int line);
  void scriptName(Record record, char *base) const;
  bool hasLegacy() const;

  char m_nameBuffer[MaxName + 1] = {};
  Content m_content;
  Screen m_screen = Screen::List;
  const char *m_status = nullptr;
  int m_selected = 0;
  int m_first = 0;
  int m_line = 0;
  int m_lines = 0;
  char m_script[MaxName + 1] = {};
  char m_source[4096] = {};
  char m_updated[4096] = {};
  char m_export[4096] = {};
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
