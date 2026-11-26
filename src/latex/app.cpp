#include "app.h"
#include "serializer.h"

#include <algorithm>
#include <apps/apps_container.h>
#include <apps/code/code_icon.h>
#include <apps/i18n.h>
#include <poincare/expression.h>
#include <poincare/horizontal_layout.h>
#include <poincare/print_float.h>
#include <string.h>

namespace Latex {
namespace {

using Record = Ion::Storage::Record;
constexpr char SourceExtension[] = "ltx";
constexpr char ExportExtension[] = "py";
constexpr char LegacyName[] = "aos_latex";
constexpr KDCoordinate HeaderHeight = 24;
constexpr KDCoordinate FooterHeight = 22;
constexpr KDCoordinate ListRowHeight = 29;
constexpr KDCoordinate NeighborHeight = 40;
constexpr KDCoordinate GutterWidth = 34;

void label(KDContext *ctx, const char *text, KDPoint origin,
           KDColor color = Palette::PrimaryText, KDColor background = Palette::BackgroundHard,
           const KDFont *font = KDFont::SmallFont) {
  ctx->drawString(text, origin, font, color, background);
}

void keyHint(KDContext *ctx, const char *key, const char *action, KDCoordinate x, KDCoordinate y) {
  const KDCoordinate width = KDFont::SmallFont->stringSize(key).width() + 8;
  const KDRect button(x, y, width, 16);
  ctx->fillRect(button, Palette::BackgroundHard);
  ctx->strokeRect(button, Palette::ExpressionInputBorder);
  label(ctx, key, KDPoint(x + 4, y + 1));
  label(ctx, action, KDPoint(x + width + 5, y + 1),
        Palette::SecondaryText, Palette::BackgroundAppsSecondary);
}

void numberText(unsigned number, char *out) {
  char reversed[10];
  int size = 0;
  do {
    reversed[size++] = '0' + number % 10;
    number /= 10;
  } while (number);
  while (size) {
    *out++ = reversed[--size];
  }
  *out = 0;
}

Ion::Storage *store() { return Ion::Storage::sharedStorage(); }

Record lookup(const char *name, const char *extension) {
  return store()->recordBaseNamedWithExtension(name, extension);
}

Record::ErrorStatus write(const char *name, const char *extension, const void *data, size_t size) {
  auto record = lookup(name, extension);
  return record.isNull() ? store()->createRecordWithExtension(name, extension, data, size)
                         : record.setValue({data, size});
}

size_t requiredGrowth(Record record, size_t size, const char *name, const char *extension) {
  if (record.isNull()) {
    return size + strlen(name) + strlen(extension) + 4;
  }
  size_t oldSize = record.value().size;
  return size > oldSize ? size - oldSize : 0;
}

bool validName(const char *name) {
  if (!*name || strlen(name) > 24 || (*name >= '0' && *name <= '9')) {
    return false;
  }
  for (const char *c = name; *c; ++c) {
    if (!(*c >= 'a' && *c <= 'z') && !(*c >= '0' && *c <= '9') && *c != '_') {
      return false;
    }
  }
  return true;
}

Poincare::Layout parseLine(const char *start, size_t length, Poincare::Context *context) {
  char line[512];
  if (!length || length >= sizeof(line)) {
    return {};
  }
  memcpy(line, start, length);
  line[length] = 0;
  auto expression = Poincare::Expression::Parse(line, context);
  return expression.isUninitialized() ? Poincare::Layout() :
    expression.createLayout(Poincare::Preferences::PrintFloatMode::Decimal,
                            Poincare::PrintFloat::k_numberOfStoredSignificantDigits);
}

}

bool Editor::MathField::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::ShiftUp || event == Ion::Events::ShiftDown) {
    return m_editor->layoutFieldDidReceiveEvent(this, event);
  }
  return LayoutField::handleEvent(event);
}

Editor::Content::Content(Editor *editor) :
  previous(0.0f, 0.5f, Palette::PrimaryText, Palette::BackgroundHard),
  field(editor),
  next(0.0f, 0.5f, Palette::PrimaryText, Palette::BackgroundHard),
  name(editor, editor->m_nameBuffer, sizeof(editor->m_nameBuffer),
       sizeof(editor->m_nameBuffer), nullptr, editor),
  m_editor(editor) {
  field.setBackgroundColor(Palette::BackgroundHard);
}

void Editor::Content::refresh() {
  layoutSubviews();
  markRectAsDirty(bounds());
}

int Editor::Content::numberOfSubviews() const {
  return m_editor->m_screen == Screen::Edit ? 3 : m_editor->m_screen == Screen::Rename ? 1 : 0;
}

View *Editor::Content::subviewAtIndex(int index) {
  if (m_editor->m_screen == Screen::Rename) {
    return &name;
  }
  View *views[] = {&previous, &field, &next};
  return views[index];
}

KDRect Editor::Content::rowFrame(int offset) const {
  const KDCoordinate top = HeaderHeight;
  const KDCoordinate bottom = bounds().height() - FooterHeight;
  const bool showPrevious = m_editor->m_line > 0;
  const bool showNext = m_editor->m_line < m_editor->m_lines;
  const KDCoordinate activeTop = top + (showPrevious ? NeighborHeight : 0);
  const KDCoordinate maxHeight = bottom - activeTop - (showNext ? NeighborHeight : 0);
  const KDCoordinate activeHeight = std::min(maxHeight,
    std::max<KDCoordinate>(34, field.minimalSizeForOptimalDisplay().height() + 12));
  switch (offset) {
    case -1: return showPrevious ? KDRect(0, top, bounds().width(), NeighborHeight) : KDRectZero;
    case 1: return showNext ?
      KDRect(0, activeTop + activeHeight, bounds().width(), NeighborHeight) : KDRectZero;
    default: return KDRect(0, activeTop, bounds().width(), activeHeight);
  }
}

void Editor::Content::layoutSubviews(bool force) {
  const KDCoordinate width = bounds().width();
  if (m_editor->m_screen == Screen::Edit) {
    const auto formulaFrame = [this](int offset) {
      const KDRect row = rowFrame(offset);
      return row.isEmpty() ? KDRectZero :
        KDRect(GutterWidth + 7, row.y() + 2, row.width() - GutterWidth - 12, row.height() - 4);
    };
    previous.setFrame(formulaFrame(-1), force);
    field.setFrame(formulaFrame(0), force);
    next.setFrame(formulaFrame(1), force);
  } else if (m_editor->m_screen == Screen::Rename) {
    name.setFrame(KDRect(13, 69, width - 26, 30), force);
  }
}

void Editor::Content::drawList(KDContext *ctx) const {
  auto *editor = m_editor;
  const int width = bounds().width();
  const int count = editor->scriptCount();
  label(ctx, "Scripts LaTeX", KDPoint(10, 5), Palette::PrimaryText, Palette::BackgroundAppsSecondary);
  for (int row = 0; row < VisibleScripts && editor->m_first + row <= count; ++row) {
    const int index = editor->m_first + row;
    const int y = HeaderHeight + row * ListRowHeight;
    const bool selected = index == editor->m_selected;
    const KDColor color = selected ? Palette::ListCellBackgroundSelected :
      (row % 2 ? Palette::BackgroundApps : Palette::ListCellBackground);
    ctx->fillRect(KDRect(0, y, width, ListRowHeight - 1), color);
    ctx->fillRect(KDRect(0, y + ListRowHeight - 1, width, 1), Palette::ListCellBorder);
    if (selected) {
      ctx->fillRect(KDRect(0, y, 3, ListRowHeight - 1), Palette::AccentText);
    }
    auto record = editor->scriptAt(index);
    if (record.isNull()) {
      label(ctx, "+  Nouveau script", KDPoint(10, y + 7), Palette::AccentText, color);
      continue;
    }
    char base[MaxName + 1] = {};
    editor->scriptName(record, base);
    label(ctx, base, KDPoint(10, y + 6), Palette::PrimaryText, color, KDFont::LargeFont);
    const bool editable = !lookup(base, SourceExtension).isNull();
    label(ctx, editable ? "..." : "Export", KDPoint(width - (editable ? 30 : 51), y + 7),
          Palette::SecondaryText, color);
  }
  if (count >= VisibleScripts) {
    const int track = VisibleScripts * ListRowHeight;
    const int thumb = track * VisibleScripts / (count + 1);
    ctx->fillRect(KDRect(width - 3, HeaderHeight, 3, track), Palette::ScrollBarBackground);
    ctx->fillRect(KDRect(width - 3, HeaderHeight + track * editor->m_first / (count + 1), 3, thumb),
                  Palette::ScrollBarForeground);
  }
}

void Editor::Content::drawEditor(KDContext *ctx) const {
  char filename[MaxName + 4];
  strlcpy(filename, m_editor->m_script, sizeof(filename));
  strlcat(filename, ".py", sizeof(filename));
  label(ctx, filename, KDPoint(10, 5), Palette::PrimaryText, Palette::BackgroundAppsSecondary);
  const KDRect active = rowFrame(0);
  ctx->fillRect(KDRect(0, HeaderHeight, GutterWidth,
                       bounds().height() - HeaderHeight - FooterHeight),
                Palette::CodeGutterViewBackground);
  for (int offset = -1; offset <= 1; ++offset) {
    const KDRect row = rowFrame(offset);
    if (row.isEmpty()) {
      continue;
    }
    const bool focused = offset == 0;
    const KDColor gutter = focused ? Palette::CodeBackgroundSelected : Palette::CodeGutterViewBackground;
    ctx->fillRect(KDRect(0, row.y(), GutterWidth, row.height()), gutter);
    ctx->fillRect(KDRect(GutterWidth, row.y(), 1, row.height()), Palette::ListCellBorder);
    char number[11];
    numberText(m_editor->m_line + offset + 1, number);
    const KDCoordinate x = GutterWidth - KDFont::SmallFont->stringSize(number).width() - 6;
    const KDCoordinate y = row.y() + (row.height() - 14) / 2;
    label(ctx, number, KDPoint(x, y), focused ? Palette::PrimaryText : Palette::SecondaryText, gutter);
  }
  ctx->fillRect(KDRect(0, active.y(), 3, active.height()), Palette::AccentText);
  ctx->fillRect(KDRect(GutterWidth + 1, active.y(), active.width() - GutterWidth - 1, 1), Palette::ListCellBorder);
  ctx->fillRect(KDRect(GutterWidth + 1, active.bottom(), active.width() - GutterWidth - 1, 1), Palette::ListCellBorder);
}

void Editor::Content::drawOptions(KDContext *ctx) const {
  label(ctx, m_editor->m_script, KDPoint(10, 5), Palette::PrimaryText, Palette::BackgroundAppsSecondary);
  if (m_editor->m_screen == Screen::Options) {
    ctx->fillRect(KDRect(0, HeaderHeight + 4, bounds().width(), 31), Palette::ListCellBackgroundSelected);
    ctx->fillRect(KDRect(0, HeaderHeight + 4, 3, 31), Palette::AccentText);
    label(ctx, "Renommer", KDPoint(10, HeaderHeight + 10), Palette::PrimaryText,
          Palette::ListCellBackgroundSelected, KDFont::LargeFont);
    label(ctx, ">", KDPoint(bounds().width() - 17, HeaderHeight + 11), Palette::AccentText,
          Palette::ListCellBackgroundSelected);
    return;
  }
  label(ctx, "Nouveau nom", KDPoint(12, 47), Palette::PrimaryText, Palette::BackgroundHard, KDFont::LargeFont);
  ctx->strokeRect(KDRect(11, 67, bounds().width() - 22, 34), Palette::AccentText);
  label(ctx, "Minuscules, chiffres et _", KDPoint(12, 109), Palette::SecondaryText);
}

void Editor::Content::drawRect(KDContext *ctx, KDRect) const {
  const KDCoordinate footer = bounds().height() - FooterHeight;
  ctx->fillRect(bounds(), Palette::BackgroundHard);
  ctx->fillRect(KDRect(0, 0, bounds().width(), HeaderHeight - 1), Palette::BackgroundAppsSecondary);
  ctx->fillRect(KDRect(0, HeaderHeight - 1, bounds().width(), 1), Palette::ListCellBorder);
  ctx->fillRect(KDRect(0, footer, bounds().width(), FooterHeight), Palette::BackgroundAppsSecondary);
  ctx->fillRect(KDRect(0, footer, bounds().width(), 1), Palette::ListCellBorder);
  switch (m_editor->m_screen) {
    case Screen::List: drawList(ctx); break;
    case Screen::Edit: drawEditor(ctx); break;
    case Screen::Options:
    case Screen::Rename: drawOptions(ctx); break;
  }
  if (m_editor->m_status) {
    label(ctx, m_editor->m_status, KDPoint(8, footer + 4), Palette::PrimaryText, Palette::BackgroundAppsSecondary);
    return;
  }
  switch (m_editor->m_screen) {
    case Screen::List:
      keyHint(ctx, "OK", "Ouvrir", 8, footer + 3);
      keyHint(ctx, ">", "Options", 102, footer + 3);
      break;
    case Screen::Edit:
      keyHint(ctx, "EXE", "Ligne suivante", 8, footer + 3);
      keyHint(ctx, "SHIFT", "Haut / Bas", 169, footer + 3);
      break;
    case Screen::Options:
    case Screen::Rename:
      keyHint(ctx, "OK", "Valider", 8, footer + 3);
      keyHint(ctx, "BACK", "Retour", 102, footer + 3);
      break;
  }
}

Editor::Editor(Responder *parent) : ViewController(parent), m_content(this) {}

bool Editor::hasLegacy() const {
  auto r = lookup(LegacyName, ExportExtension);
  if (r.isNull() || !lookup(LegacyName, SourceExtension).isNull()) {
    return false;
  }
  const auto data = r.value();
  const auto *bytes = static_cast<const char *>(data.buffer);
  return data.size >= 5 && bytes[0] == 0 && memcmp(bytes + 1, "\\[\n", 3) == 0;
}

int Editor::scriptCount() const {
  return store()->numberOfRecordsWithExtension(SourceExtension) + hasLegacy();
}

Editor::Record Editor::scriptAt(int index) const {
  const int count = store()->numberOfRecordsWithExtension(SourceExtension);
  if (index < count) {
    return store()->recordWithExtensionAtIndex(SourceExtension, index);
  }
  return index == count && hasLegacy() ? lookup(LegacyName, ExportExtension) : Record();
}

void Editor::scriptName(Record record, char *base) const {
  const char *name = record.fullName();
  const char *dot = strchr(name, '.');
  const size_t length = dot ? dot - name : strlen(name);
  const size_t copied = length > MaxName ? MaxName : length;
  memcpy(base, name, copied);
  base[copied] = 0;
}

void Editor::transition(Screen screen, const char *status) {
  m_screen = screen;
  m_status = status;
  m_content.refresh();
  didBecomeFirstResponder();
}

void Editor::didBecomeFirstResponder() {
  if (m_screen == Screen::Edit) {
    m_content.field.setEditing(true);
    Container::activeApp()->setFirstResponder(&m_content.field);
  } else if (m_screen == Screen::Rename) {
    m_content.name.setEditing(true);
    Container::activeApp()->setFirstResponder(&m_content.name);
  }
}

Poincare::Context *Editor::context() const {
  return AppsContainer::sharedAppsContainer()->globalContext();
}

Toolbox *Editor::toolboxForInputEventHandler(InputEventHandler *) {
  return AppsContainer::sharedAppsContainer()->mathToolbox();
}

NestedMenuController *Editor::variableBoxForInputEventHandler(InputEventHandler *) { return nullptr; }

void Editor::createScript() {
  for (unsigned index = 1; index <= 99; ++index) {
    memcpy(m_script, "script", 6);
    numberText(index, m_script + 6);
    if (lookup(m_script, SourceExtension).isNull() && lookup(m_script, ExportExtension).isNull()) {
      const char empty[] = {0, 0};
      if (store()->availableSize() < 2 * strlen(m_script) + 20 ||
          write(m_script, SourceExtension, empty, 1) != Record::ErrorStatus::None) {
        m_status = "Memoire insuffisante";
        return;
      }
      if (write(m_script, ExportExtension, empty, sizeof(empty)) != Record::ErrorStatus::None) {
        lookup(m_script, SourceExtension).destroy();
        m_status = "Memoire insuffisante";
        return;
      }
      openScript();
      return;
    }
  }
  m_status = "Plus de noms disponibles";
}

bool Editor::openScript() {
  const auto record = lookup(m_script, SourceExtension);
  if (record.isNull()) {
    m_status = "Ancien export conserve sur PC/USB";
    return false;
  }
  const auto data = record.value();
  if (!data.size || data.size > sizeof(m_source) ||
      static_cast<const char *>(data.buffer)[data.size - 1] != 0 ||
      (data.size > 1 && static_cast<const char *>(data.buffer)[data.size - 2] != '\n')) {
    m_status = "Script illisible";
    return false;
  }
  memcpy(m_source, data.buffer, data.size);
  m_lines = 0;
  for (const char *c = m_source; *c; ++c) {
    m_lines += *c == '\n';
  }
  loadLine(0);
  transition(Screen::Edit, nullptr);
  return true;
}

void Editor::renameScript() {
  m_content.name.setText(m_script);
  transition(Screen::Rename, nullptr);
  m_content.name.setText(m_script);
}

bool Editor::textFieldShouldFinishEditing(TextField *, Ion::Events::Event event) {
  return event == Ion::Events::OK || event == Ion::Events::EXE;
}

bool Editor::textFieldDidFinishEditing(TextField *, const char *text, Ion::Events::Event) {
  char newName[MaxName + 1];
  strlcpy(newName, text, sizeof(newName));
  if (!validName(newName)) {
    m_status = "Nom invalide";
    m_content.refresh();
    return false;
  }
  if (strcmp(newName, m_script) != 0) {
    auto source = lookup(m_script, SourceExtension);
    auto exportFile = lookup(m_script, ExportExtension);
    if (source.isNull() || !lookup(newName, SourceExtension).isNull() ||
        !lookup(newName, ExportExtension).isNull()) {
      m_status = "Nom indisponible";
      m_content.refresh();
      return false;
    }
    const size_t oldLength = strlen(m_script);
    const size_t newLength = strlen(newName);
    if (newLength > oldLength && 2 * (newLength - oldLength) > store()->availableSize()) {
      m_status = "Memoire insuffisante";
      m_content.refresh();
      return false;
    }
    if (source.setBaseNameWithExtension(newName, SourceExtension) != Record::ErrorStatus::None) {
      return false;
    }
    if (!exportFile.isNull() &&
        exportFile.setBaseNameWithExtension(newName, ExportExtension) != Record::ErrorStatus::None) {
      lookup(newName, SourceExtension).setBaseNameWithExtension(m_script, SourceExtension);
      return false;
    }
    strlcpy(m_script, newName, sizeof(m_script));
  }
  transition(Screen::List, "Nom enregistre");
  Container::activeApp()->setFirstResponder(this);
  return true;
}

bool Editor::textFieldDidAbortEditing(TextField *) {
  transition(Screen::Options, nullptr);
  Container::activeApp()->setFirstResponder(this);
  return true;
}

bool Editor::handleEvent(Ion::Events::Event event) {
  if (m_screen == Screen::Options) {
    if (event == Ion::Events::OK || event == Ion::Events::EXE) {
      renameScript();
      return true;
    }
    if (event == Ion::Events::Back) {
      transition(Screen::List, nullptr);
      return true;
    }
  }
  if (m_screen != Screen::List) {
    return false;
  }
  if (event == Ion::Events::Up || event == Ion::Events::Down) {
    const int next = m_selected + (event == Ion::Events::Up ? -1 : 1);
    if (next >= 0 && next <= scriptCount()) {
      m_selected = next;
      m_status = nullptr;
      if (m_selected < m_first) m_first = m_selected;
      if (m_selected >= m_first + VisibleScripts) m_first = m_selected - VisibleScripts + 1;
    }
  } else if (event == Ion::Events::OK || event == Ion::Events::EXE || event == Ion::Events::Right) {
    auto record = scriptAt(m_selected);
    if (record.isNull()) {
      if (event != Ion::Events::Right) createScript();
    } else {
      scriptName(record, m_script);
      if (event == Ion::Events::Right && !lookup(m_script, SourceExtension).isNull()) {
        transition(Screen::Options, nullptr);
      } else {
        openScript();
      }
    }
  } else {
    return false;
  }
  m_content.refresh();
  return true;
}

const char *Editor::lineStart(int line) const {
  const char *start = m_source;
  for (int index = 0; index < line && *start; ++index) {
    start = strchr(start, '\n') + 1;
  }
  return start;
}

Poincare::Layout Editor::lineLayout(int line) {
  if (line < 0 || line >= m_lines) {
    return {};
  }
  const char *start = lineStart(line);
  return parseLine(start, strchr(start, '\n') - start, context());
}

void Editor::loadLine(int line) {
  m_line = line;
  auto current = lineLayout(line);
  if (current.isUninitialized()) {
    current = Poincare::HorizontalLayout::Builder();
  } else if (current.type() != Poincare::LayoutNode::Type::HorizontalLayout) {
    current = Poincare::HorizontalLayout::Builder(current);
  }
  m_content.field.setLayout(current);
  m_content.previous.setLayout(lineLayout(line - 1));
  m_content.next.setLayout(lineLayout(line + 1));
  m_content.refresh();
}

void Editor::layoutFieldDidChangeSize(LayoutField *) {
  if (m_screen == Screen::Edit) {
    m_content.refresh();
  }
}

bool Editor::layoutFieldDidReceiveEvent(LayoutField *, Ion::Events::Event event) {
  const bool finish = event == Ion::Events::Back || event == Ion::Events::Home;
  const bool previous = event == Ion::Events::ShiftUp;
  if (!finish && !previous && event != Ion::Events::ShiftDown &&
      event != Ion::Events::EXE && event != Ion::Events::OK) {
    return false;
  }
  if (saveLine()) {
    if (finish) {
      transition(Screen::List, nullptr);
      Container::activeApp()->setFirstResponder(this);
      return event == Ion::Events::Back;
    }
    int target = m_line + (previous ? -1 : 1);
    if (target >= 0 && target <= m_lines) {
      loadLine(target);
    }
    m_status = nullptr;
  }
  m_content.refresh();
  return true;
}

bool Editor::saveLine() {
  const char *oldStart = lineStart(m_line);
  const char *oldEnd = m_line == m_lines ? oldStart : strchr(oldStart, '\n') + 1;
  char input[512] = {};
  if (m_content.field.hasText()) {
    int size = m_content.field.layout().serializeForParsing(input, sizeof(input));
    if (size <= 0 || size >= static_cast<int>(sizeof(input) - 1) ||
        strchr(input, '\n') || parseLine(input, size, context()).isUninitialized()) {
      m_status = "Formule incomplete";
      return false;
    }
  }
  const size_t first = oldStart - m_source;
  const size_t count = strlen(input);
  const size_t last = strlen(oldEnd) + 1;
  if (first + count + (count != 0) + last > sizeof(m_updated)) {
    m_status = "Script plein";
    return false;
  }
  memcpy(m_updated, m_source, first);
  memcpy(m_updated + first, input, count);
  if (count) m_updated[first + count] = '\n';
  memcpy(m_updated + first + count + (count != 0), oldEnd, last);
  if (strcmp(m_source, m_updated) == 0) {
    return true;
  }

  size_t used = 1;
  m_export[0] = 0;
  m_export[1] = 0;
  for (const char *line = m_updated; *line;) {
    const char *end = strchr(line, '\n');
    auto layout = parseLine(line, end - line, context());
    constexpr char opening[] = "\\[\n";
    constexpr char closing[] = "\n\\]\n";
    if (layout.isUninitialized() || sizeof(m_export) - used < sizeof(opening) + sizeof(closing)) {
      m_status = "Export trop long ou invalide";
      return false;
    }
    memcpy(m_export + used, opening, sizeof(opening));
    used += sizeof(opening) - 1;
    if (!serialize(layout, m_export + used, sizeof(m_export) - used - sizeof(closing) + 1)) {
      m_status = "Symbole non exportable";
      return false;
    }
    used += strlen(m_export + used);
    memcpy(m_export + used, closing, sizeof(closing));
    used += sizeof(closing) - 1;
    line = end + 1;
  }
  const size_t size = strlen(m_updated) + 1;
  if (requiredGrowth(lookup(m_script, SourceExtension), size, m_script, SourceExtension) +
      requiredGrowth(lookup(m_script, ExportExtension), used + 1, m_script, ExportExtension) >
      store()->availableSize()) {
    m_status = "Memoire insuffisante";
    return false;
  }
  if (write(m_script, SourceExtension, m_updated, size) != Record::ErrorStatus::None ||
      write(m_script, ExportExtension, m_export, used + 1) != Record::ErrorStatus::None) {
    m_status = "Sauvegarde impossible";
    return false;
  }
  memcpy(m_source, m_updated, size);
  m_lines += (count != 0) - (oldStart != oldEnd);
  if (m_line > m_lines) m_line = m_lines;
  return true;
}

I18n::Message App::Descriptor::name() { return I18n::Message::LatexApp; }
I18n::Message App::Descriptor::upperName() { return I18n::Message::LatexAppCapital; }
const Image *App::Descriptor::icon() { return ImageStore::CodeIcon; }
App *App::Snapshot::unpack(Container *container) {
  return new (container->currentAppBuffer()) App(this);
}
App::Descriptor *App::Snapshot::descriptor() {
  static Descriptor descriptor;
  return &descriptor;
}
App::App(Snapshot *snapshot) : ::App(snapshot, &m_editor), m_pool(), m_editor(&m_modalViewController) {}

}
