#include "app.h"
#include "serializer.h"

#include <apps/apps_container.h>
#include <apps/i18n.h>
#include <apps/code/code_icon.h>
#include <string.h>

namespace Latex {
namespace {

constexpr char FileName[] = "aos_latex.py";
constexpr char Opening[] = "\\[\n";
constexpr char Closing[] = "\n\\]\n";
constexpr KDColor Gutter = KDColor::RGB24(0xEDF0F3);

}

Editor::Content::Content(Editor *editor) :
  preview(0.0f, 0.5f, Palette::PrimaryText, Palette::BackgroundHard),
  field(editor, editor, editor),
  status(KDFont::SmallFont, 0.0f, 0.5f, Palette::PrimaryText, Gutter) {
  field.setBackgroundColor(Palette::BackgroundHard);
  status.setText("EXE: sauvegarder la ligne");
}

void Editor::Content::drawRect(KDContext *ctx, KDRect) const {
  ctx->fillRect(bounds(), Palette::BackgroundHard);
  ctx->fillRect(KDRect(0, 0, 18, bounds().height() - 24), Gutter);
  ctx->drawString(">", KDPoint(5, bounds().height() / 2 + 10), KDFont::SmallFont,
                  Palette::PrimaryText, Gutter);
  ctx->fillRect(KDRect(0, bounds().height() - 24, bounds().width(), 24), Gutter);
}

View *Editor::Content::subviewAtIndex(int index) {
  View *views[] = {&preview, &field, &status};
  return views[index];
}

void Editor::Content::layoutSubviews(bool force) {
  const KDCoordinate width = bounds().width();
  const KDCoordinate height = bounds().height();
  const KDCoordinate split = height / 2;
  preview.setFrame(KDRect(22, 6, width - 26, split - 10), force);
  field.setFrame(KDRect(22, split, width - 26, height - split - 24), force);
  status.setFrame(KDRect(6, height - 24, width - 12, 24), force);
}

Editor::Editor(Responder *parent) : ViewController(parent), m_content(this) {}

void Editor::didBecomeFirstResponder() {
  m_content.field.setEditing(true);
  Container::activeApp()->setFirstResponder(&m_content.field);
}

Poincare::Context *Editor::context() const {
  return AppsContainer::sharedAppsContainer()->globalContext();
}

Toolbox *Editor::toolboxForInputEventHandler(InputEventHandler *) {
  return AppsContainer::sharedAppsContainer()->mathToolbox();
}

NestedMenuController *Editor::variableBoxForInputEventHandler(InputEventHandler *) {
  return nullptr;
}

bool Editor::layoutFieldDidReceiveEvent(LayoutField *, Ion::Events::Event event) {
  if (event == Ion::Events::EXE || event == Ion::Events::OK) {
    saveLine();
    return true;
  }
  return false;
}

void Editor::saveLine() {
  if (!m_content.field.hasText()) {
    m_content.status.setText("Ecris d'abord une expression");
    return;
  }
  Poincare::Layout layout = m_content.field.layout();
  const char *error = appendLine(layout);
  if (error) {
    m_content.status.setText(error);
    return;
  }
  m_content.field.clearLayout();
  layout.removeGraySquaresFromAllMatrixChildren();
  m_content.preview.setLayout(layout);
  m_content.status.setText("Enregistre: aos_latex.py");
}

const char *Editor::appendLine(Poincare::Layout layout) {
  using Record = Ion::Storage::Record;
  auto *storage = Ion::Storage::sharedStorage();
  Record record = storage->recordNamed(FileName);
  size_t offset = 1;
  m_document[0] = 0; // NumWorks' .py record flag: do not auto-import.
  if (!record.isNull()) {
    const Record::Data data = record.value();
    const char *bytes = static_cast<const char *>(data.buffer);
    if (data.size >= sizeof(m_document)) {
      return "aos_latex.py est plein";
    }
    if (data.size < 1 + sizeof(Opening) || bytes[0] != 0 ||
        bytes[data.size - 1] != 0 || strncmp(bytes + 1, Opening, sizeof(Opening) - 1) != 0) {
      return "Nom deja pris: aos_latex.py";
    }
    offset = data.size - 1;
    memcpy(m_document, bytes, offset);
  }
  const size_t available = sizeof(m_document) - offset;
  if (available < sizeof(Opening) + sizeof(Closing)) {
    return "aos_latex.py est plein";
  }
  memcpy(m_document + offset, Opening, sizeof(Opening));
  offset += sizeof(Opening) - 1;
  if (!serialize(layout, m_document + offset, sizeof(m_document) - offset - sizeof(Closing) + 1)) {
    return "Expression incomplete ou trop longue";
  }
  offset += strlen(m_document + offset);
  memcpy(m_document + offset, Closing, sizeof(Closing));
  const size_t size = offset + sizeof(Closing);
  const Record::ErrorStatus status = record.isNull()
    ? storage->createRecordWithFullName(FileName, m_document, size)
    : record.setValue({m_document, size});
  return status == Record::ErrorStatus::None ? nullptr : "Memoire insuffisante";
}

I18n::Message App::Descriptor::name() { return I18n::Message::LatexApp; }
I18n::Message App::Descriptor::upperName() { return I18n::Message::LatexAppCapital; }
const Image *App::Descriptor::icon() {
  return ImageStore::CodeIcon;
}

App *App::Snapshot::unpack(Container *container) {
  return new (container->currentAppBuffer()) App(this);
}

App::Descriptor *App::Snapshot::descriptor() {
  static Descriptor descriptor;
  return &descriptor;
}

App::App(Snapshot *snapshot) :
  ::App(snapshot, &m_editor),
  m_pool(),
  m_editor(&m_modalViewController) {}

}
