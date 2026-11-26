#ifndef AOS_NOTES_APP_H
#define AOS_NOTES_APP_H

#include <apps/shared/shared_app.h>
#include <escher.h>

namespace Notes {

class Controller : public ViewController, public TextFieldDelegate {
public:
  explicit Controller(Responder *parent);
  View *view() override { return &m_view; }
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  bool save();

  bool textFieldShouldFinishEditing(TextField *, Ion::Events::Event event) override;
  bool textFieldDidReceiveEvent(TextField *, Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(TextField *, const char *text, Ion::Events::Event) override;
  bool textFieldDidAbortEditing(TextField *) override;

private:
  enum class Screen { Locked, List, Options, Rename, ConfirmDelete, Edit };
  using Record = Ion::Storage::Record;
  static constexpr int MaxName = 24;
  static constexpr int MaxText = 3000;
  static constexpr int VisibleRows = 6;

  class NoteArea : public TextArea {
  public:
    explicit NoteArea(Responder *parent)
      : TextArea(parent, &m_text, KDFont::SmallFont) {}
  private:
    class Body : public TextArea::ContentView {
    public:
      Body() : ContentView(KDFont::SmallFont) {}
      void clearRect(KDContext *ctx, KDRect rect) const override;
      void drawLine(KDContext *ctx, int line, const char *text, size_t length,
                    int fromColumn, int toColumn, const char *selectionStart,
                    const char *selectionEnd) const override;
    };
    Body m_text;
    const TextInput::ContentView *nonEditableContentView() const override { return &m_text; }
  };

  class Content : public View {
  public:
    explicit Content(Controller *owner);
    void refresh();
    void drawRect(KDContext *ctx, KDRect rect) const override;
    NoteArea area;
    TextField name;

  private:
    int numberOfSubviews() const override;
    View *subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    Controller *m_owner;
  };

  Record noteAt(int index, int *count = nullptr) const;
  void createNote();
  void open(Record note);
  void show(Screen screen, const char *status = nullptr);
  void failed(const char *status);

  Screen m_screen = Screen::Locked;
  int m_selected = 0;
  bool m_deleteSelected = false;
  int m_first = 0;
  const char *m_status = nullptr;
  Record m_record;
  char m_name[MaxName + 1] = {};
  char m_nameBuffer[MaxName + 1] = {};
  char m_payload[1 + MaxText + 1] = {};
  Content m_view;
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
  bool prepareForExit() override { return m_controller.save(); }
private:
  explicit App(Snapshot *snapshot);
  Controller m_controller;
};

}

#endif
