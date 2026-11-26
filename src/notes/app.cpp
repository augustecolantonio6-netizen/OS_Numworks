#include "app.h"

#include <algorithm>
#include <apps/i18n.h>
#include <apps/apps_container.h>
#include <apps/notes/notes_icon.h>
#include <ion/unicode/utf8_helper.h>
#include <string.h>

namespace Notes {
namespace {

using Record = Ion::Storage::Record;
constexpr char Prefix[] = "note_";
constexpr size_t PrefixLength = sizeof(Prefix) - 1;
constexpr char Extension[] = "py";
constexpr char PasswordRecord[] = "notes_password.cfg";
constexpr char DefaultPassword[] = "casio";
constexpr KDCoordinate Header = 24;
constexpr KDCoordinate Footer = 22;
constexpr KDCoordinate Row = 29;

Ion::Storage *storage() {
  return Ion::Storage::sharedStorage();
}

bool validName(const char *name) {
  if (!*name) {
    return false;
  }
  for (; *name; ++name) {
    if (!(*name >= 'a' && *name <= 'z') && !(*name >= '0' && *name <= '9') && *name != '_') {
      return false;
    }
  }
  return true;
}

void draw(KDContext *ctx, const char *text, KDPoint at, KDColor background,
          KDColor color = Palette::PrimaryText, const KDFont *font = KDFont::SmallFont) {
  ctx->drawString(text, at, font, color, background);
}

}

// Upsilon's TextArea provides the Python editor's cursor, scrolling and
// selection. This body draws plain text without a gutter or syntax highlighting.
void Controller::NoteArea::Body::clearRect(KDContext *ctx, KDRect rect) const {
  ctx->fillRect(rect, Palette::BackgroundHard);
}

void Controller::NoteArea::Body::drawLine(KDContext *ctx, int line, const char *text,
    size_t length, int, int, const char *selectionStart, const char *selectionEnd) const {
  drawStringAt(ctx, line, 0, text, length, Palette::PrimaryText, Palette::BackgroundHard,
               selectionStart, selectionEnd, Palette::CodeBackgroundSelected);
}

Controller::Content::Content(Controller *owner)
  : area(owner),
    name(owner, owner->m_nameBuffer, sizeof(owner->m_nameBuffer),
         sizeof(owner->m_nameBuffer), nullptr, owner),
    m_owner(owner) {}

void Controller::Content::refresh() {
  layoutSubviews();
  markRectAsDirty(bounds());
}

int Controller::Content::numberOfSubviews() const {
  const auto screen = m_owner->m_screen;
  return screen == Screen::Edit || screen == Screen::Rename || isPasswordScreen(screen) ? 1 : 0;
}

View *Controller::Content::subviewAtIndex(int) {
  if (m_owner->m_screen == Screen::Rename || isPasswordScreen(m_owner->m_screen)) {
    return &name;
  }
  return &area;
}

void Controller::Content::layoutSubviews(bool force) {
  if (m_owner->m_screen == Screen::Edit) {
    area.setFrame(KDRect(8, Header + 6, bounds().width() - 16,
                         std::max(0, bounds().height() - Header - Footer - 12)), force);
  } else if (m_owner->m_screen == Screen::Rename || isPasswordScreen(m_owner->m_screen)) {
    name.setFrame(KDRect(13, 69, bounds().width() - 26, 30), force);
  }
}

void Controller::Content::drawRect(KDContext *ctx, KDRect) const {
  const KDColor background = Palette::BackgroundHard;
  const KDColor bar = Palette::BackgroundAppsSecondary;
  const KDColor selected = Palette::ListCellBackgroundSelected;
  const KDColor accent = Palette::CodeKeyword;
  const int width = bounds().width();
  const int bottom = bounds().height() - Footer;
  ctx->fillRect(bounds(), background);
  ctx->fillRect(KDRect(0, 0, width, Header), bar);
  ctx->fillRect(KDRect(0, bottom, width, Footer), bar);

  const auto screen = m_owner->m_screen;
  draw(ctx, isPasswordScreen(screen) ? "Notes" : screen == Screen::List ? "Mes notes" : m_owner->m_name, KDPoint(10, 5), bar);
  const char *hint = "OK Ouvrir     > Options";
  if (screen == Screen::List) {
    int count = 0;
    m_owner->noteAt(-1, &count);
    for (int row = 0; row < VisibleRows && m_owner->m_first + row <= count + 1; ++row) {
      const int index = m_owner->m_first + row;
      const int y = Header + row * Row;
      const bool active = index == m_owner->m_selected;
      const KDColor color = active ? selected : row % 2 ? Palette::BackgroundApps : background;
      ctx->fillRect(KDRect(0, y, width, Row - 1), color);
      if (active) {
        ctx->fillRect(KDRect(0, y, 3, Row - 1), accent);
      }
      if (index == count) {
        draw(ctx, "+ Nouvelle note", KDPoint(11, y + 7), color, accent);
      } else if (index == count + 1) {
        draw(ctx, "Changer le mot de passe", KDPoint(11, y + 7), color);
      } else {
        char title[MaxName + 1];
        const char *filename = m_owner->noteAt(index).fullName();
        memcpy(title, filename + PrefixLength, strlen(filename) - PrefixLength - 3);
        title[strlen(filename) - PrefixLength - 3] = 0;
        draw(ctx, title, KDPoint(11, y + 4), color, Palette::PrimaryText, KDFont::LargeFont);
        draw(ctx, "...", KDPoint(width - 31, y + 7), color, accent);
      }
    }
    if (count + 2 > VisibleRows) {
      const int track = Row * VisibleRows;
      const int length = std::max(9, track * VisibleRows / (count + 2));
      const int y = Header + (track - length) * m_owner->m_first / (count + 2 - VisibleRows);
      ctx->fillRect(KDRect(width - 3, Header, 3, track), Palette::ScrollBarBackground);
      ctx->fillRect(KDRect(width - 3, y, 3, length), accent);
    }
  } else if (screen == Screen::Options) {
    for (int row = 0; row < 2; ++row) {
      const bool active = row == m_owner->m_deleteSelected;
      const int y = Header + row * Row;
      const KDColor color = active ? selected : background;
      ctx->fillRect(KDRect(0, y, width, Row), color);
      if (active) {
        ctx->fillRect(KDRect(0, y, 3, Row), accent);
      }
      draw(ctx, row == 0 ? "Renommer" : "Supprimer", KDPoint(11, y + 4),
           color, Palette::PrimaryText, KDFont::LargeFont);
    }
    hint = "OK Valider    BACK Retour";
  } else if (screen == Screen::ConfirmDelete) {
    draw(ctx, "Supprimer cette note ?", KDPoint(12, 48), background,
         Palette::PrimaryText, KDFont::LargeFont);
    hint = "OK Supprimer    BACK Annuler";
  } else if (screen == Screen::Rename || isPasswordScreen(screen)) {
    const bool locked = screen == Screen::Locked;
    const char *prompt = "Nouveau nom";
    switch (screen) {
      case Screen::Locked: prompt = "Mot de passe"; break;
      case Screen::NewPassword: prompt = "Nouveau mot de passe"; break;
      case Screen::ConfirmPassword: prompt = "Confirmer le mot de passe"; break;
      default: break;
    }
    draw(ctx, prompt, KDPoint(12, 48),
         background, Palette::PrimaryText, KDFont::LargeFont);
    ctx->strokeRect(KDRect(12, 68, width - 24, 32), accent);
    if (screen == Screen::Rename || screen == Screen::NewPassword) {
      draw(ctx, "Minuscules, chiffres et _", KDPoint(12, 106), background, Palette::SecondaryText);
    }
    hint = locked ? "OK Valider   HOME Quitter" : "OK Valider    BACK Retour";
  } else {
    hint = "EXE Nouvelle ligne    OK Enregistrer";
  }
  draw(ctx, m_owner->m_status ? m_owner->m_status : hint, KDPoint(9, bottom + 4), bar);
}

Controller::Controller(Responder *parent) : ViewController(parent), m_view(this) {}

Controller::Record Controller::noteAt(int index, int *count) const {
  int matches = 0;
  const int records = storage()->numberOfRecordsWithExtension(Extension);
  for (int i = 0; i < records; ++i) {
    const auto record = storage()->recordWithExtensionAtIndex(Extension, i);
    const char *filename = record.fullName();
    const size_t length = strlen(filename);
    if (strncmp(filename, Prefix, PrefixLength) != 0 ||
        length <= PrefixLength + 3 || length > PrefixLength + MaxName + 3) {
      continue;
    }
    char base[PrefixLength + MaxName + 1];
    memcpy(base, filename, length - 3);
    base[length - 3] = 0;
    if (!validName(base + PrefixLength) ||
        !storage()->recordBaseNamedWithExtension(base, "ltx").isNull()) {
      continue;
    }
    if (matches++ == index) {
      return record;
    }
  }
  if (count) {
    *count = matches;
  }
  return {};
}

void Controller::show(Screen screen, const char *status) {
  m_screen = screen;
  m_status = status;
  m_view.refresh();
  auto *app = Container::activeApp();
  app->setFirstResponder(this);
}

void Controller::failed(const char *status) {
  m_status = status;
  m_view.refresh();
}

void Controller::didBecomeFirstResponder() {
  if (m_screen == Screen::Edit) {
    Container::activeApp()->setFirstResponder(&m_view.area);
  } else if (m_screen == Screen::Rename || isPasswordScreen(m_screen)) {
    if (isPasswordScreen(m_screen)) {
      m_view.name.reinitDraftTextBuffer();
      AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(Ion::Events::ShiftAlphaStatus::AlphaLock);
    }
    m_view.name.setEditing(true);
    Container::activeApp()->setFirstResponder(&m_view.name);
  }
}

void Controller::createNote() {
  char base[PrefixLength + MaxName + 1];
  const char empty[] = {0, 0}; // Python status byte followed by a NUL-terminated body.
  for (int i = 1; i <= 999; ++i) {
    char *digit = base + PrefixLength;
    memcpy(base, Prefix, PrefixLength);
    if (i >= 100) {
      *digit++ = '0' + i / 100;
    }
    if (i >= 10) {
      *digit++ = '0' + i / 10 % 10;
    }
    *digit++ = '0' + i % 10;
    *digit = 0;
    if (!storage()->recordBaseNamedWithExtension(base, Extension).isNull() ||
        !storage()->recordBaseNamedWithExtension(base, "ltx").isNull()) {
      continue;
    }
    if (storage()->createRecordWithExtension(base, Extension, empty, sizeof(empty)) !=
        Record::ErrorStatus::None) {
      failed("Memoire insuffisante");
      return;
    }
    open(storage()->recordBaseNamedWithExtension(base, Extension));
    return;
  }
  failed("Plus de noms disponibles");
}

void Controller::open(Record record) {
  const auto data = record.value();
  const auto *bytes = static_cast<const char *>(data.buffer);
  if (data.size < 2 || data.size > sizeof(m_payload) ||
      bytes[data.size - 1] != 0 || strlen(bytes + 1) != data.size - 2) {
    failed("Note illisible ou trop longue");
    return;
  }
  if (UTF8Helper::CountOccurrences(bytes + 1, '\n') > 999) {
    failed("Note trop longue");
    return;
  }
  memcpy(m_payload, bytes, data.size);
  m_payload[0] = 0;
  m_record = record;
  const char *filename = record.fullName();
  const size_t nameLength = strlen(filename) - PrefixLength - 3;
  memcpy(m_name, filename + PrefixLength, nameLength);
  m_name[nameLength] = 0;
  m_view.area.resetSelection();
  const size_t capacity = std::min(sizeof(m_payload) - 1, data.size - 1 + storage()->availableSize());
  m_view.area.setText(m_payload + 1, capacity);
  show(Screen::Edit);
  m_view.area.setCursorLocation(m_payload + data.size - 1);
}

bool Controller::save() {
  if (m_screen != Screen::Edit) {
    return true;
  }
  const size_t size = strlen(m_payload + 1) + 2;
  const auto current = m_record.value();
  if (size == current.size && memcmp(current.buffer, m_payload, size) == 0) {
    return true;
  }
  if (m_record.setValue({m_payload, size}) == Record::ErrorStatus::None) {
    return true;
  }
  failed("Sauvegarde impossible");
  return false;
}

bool Controller::handleEvent(Ion::Events::Event event) {
  using namespace Ion::Events;
  if (m_screen == Screen::Edit) {
    if (event == Back || event == OK) {
      if (save()) {
        m_view.area.resetSelection();
        show(Screen::List);
      }
      return true;
    }
    if (event == Ion::Events::OnOff) {
      return !save();
    }
    return false;
  }
  if (m_screen == Screen::ConfirmDelete) {
    if (event == OK || event == EXE) {
      m_record.destroy();
      m_record = {};
      int count = 0;
      noteAt(-1, &count);
      m_selected = std::min(m_selected, count);
      m_first = std::min(m_first, std::max(0, count + 2 - VisibleRows));
      show(Screen::List);
      return true;
    }
    if (event == Back || event == Left) {
      show(Screen::Options);
      return true;
    }
    return false;
  }
  if (m_screen == Screen::Options) {
    if (event == Up || event == Down) {
      m_deleteSelected = event == Down;
      m_view.refresh();
      return true;
    }
    if (event == OK || event == EXE) {
      if (m_deleteSelected) {
        show(Screen::ConfirmDelete);
        return true;
      }
      m_view.name.setText(m_name);
      show(Screen::Rename);
      m_view.name.setText(m_name);
      return true;
    }
    if (event == Back || event == Left) {
      show(Screen::List);
      return true;
    }
    return false;
  }
  if (m_screen != Screen::List) {
    return false;
  }
  if (event == Up || event == Down) {
    int count = 0;
    noteAt(-1, &count);
    m_selected = std::max(0, std::min(count + 1, m_selected + (event == Up ? -1 : 1)));
    m_first = std::max(m_selected - VisibleRows + 1, std::min(m_first, m_selected));
    show(Screen::List);
    return true;
  }
  if (event != OK && event != EXE && event != Right) {
    return false;
  }
  int count = 0;
  noteAt(-1, &count);
  if (m_selected == count + 1) {
    if (event != Right) {
      show(Screen::NewPassword);
    }
    return true;
  }
  const auto record = noteAt(m_selected);
  if (record.isNull()) {
    if (event != Right) {
      createNote();
    }
  } else if (event == Right) {
    const char *filename = record.fullName();
    const size_t length = strlen(filename) - PrefixLength - 3;
    memcpy(m_name, filename + PrefixLength, length);
    m_name[length] = 0;
    m_record = record;
    m_deleteSelected = false;
    show(Screen::Options);
  } else {
    open(record);
  }
  return true;
}

bool Controller::textFieldShouldFinishEditing(TextField *, Ion::Events::Event event) {
  return event == Ion::Events::OK || event == Ion::Events::EXE;
}

bool Controller::textFieldDidReceiveEvent(TextField *field, Ion::Events::Event event) {
  if (m_screen == Screen::Locked && event == Ion::Events::Back) {
    field->setText("");
    failed(nullptr);
    return true;
  }
  return false;
}

bool Controller::textFieldDidFinishEditing(TextField *field, const char *text, Ion::Events::Event) {
  if (m_screen == Screen::Locked) {
    const auto record = storage()->recordNamed(PasswordRecord);
    const auto data = record.isNull() ? Record::Data{DefaultPassword, sizeof(DefaultPassword)} : record.value();
    const size_t length = strlen(text) + 1;
    if (data.size != length || memcmp(data.buffer, text, length) != 0) {
      field->reinitDraftTextBuffer();
      field->setCursorLocation(field->draftTextBuffer());
      field->resetSelection();
      failed("Mot de passe incorrect");
      return false;
    }
    AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(Ion::Events::ShiftAlphaStatus::Default);
    show(Screen::List);
    return true;
  }
  if (m_screen == Screen::NewPassword) {
    if (!validName(text)) {
      failed("Utilise a-z, 0-9 ou _");
      return false;
    }
    strlcpy(m_pendingPassword, text, sizeof(m_pendingPassword));
    show(Screen::ConfirmPassword);
    return true;
  }
  if (m_screen == Screen::ConfirmPassword) {
    if (strcmp(text, m_pendingPassword) != 0) {
      field->reinitDraftTextBuffer();
      failed("Les mots de passe different");
      return false;
    }
    const size_t size = strlen(text) + 1;
    auto record = storage()->recordNamed(PasswordRecord);
    auto result = record.isNull() ? storage()->createRecordWithFullName(PasswordRecord, text, size)
                                  : record.setValue({text, size});
    if (result != Record::ErrorStatus::None) {
      failed("Sauvegarde impossible");
      return false;
    }
    memset(m_pendingPassword, 0, sizeof(m_pendingPassword));
    AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(Ion::Events::ShiftAlphaStatus::Default);
    show(Screen::List, "Mot de passe modifie");
    return true;
  }
  if (!validName(text)) {
    failed("Nom invalide");
    return false;
  }
  if (strcmp(text, m_name) != 0) {
    char base[PrefixLength + MaxName + 1];
    memcpy(base, Prefix, PrefixLength);
    strlcpy(base + PrefixLength, text, MaxName + 1);
    if (!storage()->recordBaseNamedWithExtension(base, "ltx").isNull() ||
        !storage()->recordBaseNamedWithExtension(base, Extension).isNull()) {
      failed("Nom deja utilise");
      return false;
    }
    if (m_record.setBaseNameWithExtension(base, Extension) != Record::ErrorStatus::None) {
      failed("Renommage impossible");
      return false;
    }
  }
  show(Screen::List);
  return true;
}

bool Controller::textFieldDidAbortEditing(TextField *) {
  if (isPasswordScreen(m_screen)) {
    memset(m_pendingPassword, 0, sizeof(m_pendingPassword));
    AppsContainer::sharedAppsContainer()->setShiftAlphaStatus(Ion::Events::ShiftAlphaStatus::Default);
    show(m_screen == Screen::Locked ? Screen::Locked : Screen::List);
  } else {
    show(Screen::Options);
  }
  return true;
}

I18n::Message App::Descriptor::name() { return I18n::Message::NotesApp; }
I18n::Message App::Descriptor::upperName() { return I18n::Message::NotesAppCapital; }
const Image *App::Descriptor::icon() { return ImageStore::NotesIcon; }

App *App::Snapshot::unpack(Container *container) {
  return new (container->currentAppBuffer()) App(this);
}

App::Descriptor *App::Snapshot::descriptor() {
  static Descriptor descriptor;
  return &descriptor;
}

App::App(Snapshot *snapshot) : ::App(snapshot, &m_controller), m_controller(&m_modalViewController) {}

}
