#include "NoteEditor.h"

#include <algorithm>
#include <codecvt>
#include <locale>
#include <stdexcept>

#include <curses.h>

namespace rcal {
namespace {

std::wstring fromUtf8(const std::string& text) {
    try {
        return std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.from_bytes(text);
    } catch (const std::range_error&) {
        throw std::runtime_error("invalid UTF-8 note text");
    }
}

std::string toUtf8(const std::wstring& text) {
    try {
        return std::wstring_convert<std::codecvt_utf8<wchar_t>>{}.to_bytes(text);
    } catch (const std::range_error&) {
        throw std::runtime_error("cannot encode note text as UTF-8");
    }
}

} // namespace

NoteEditor::NoteEditor(const std::string& note)
    : text_(fromUtf8(note)), cursor_(text_.size()) {}

NoteEditorResult NoteEditor::handleInput(wint_t key, bool keyCode) {
    if (keyCode) {
        switch (static_cast<int>(key)) {
            case KEY_LEFT: if (cursor_ > 0) --cursor_; break;
            case KEY_RIGHT: if (cursor_ < text_.size()) ++cursor_; break;
            case KEY_HOME: cursor_ = 0; break;
            case KEY_END: cursor_ = text_.size(); break;
            case KEY_BACKSPACE:
                if (cursor_ > 0) text_.erase(--cursor_, 1);
                break;
            case KEY_DC:
                if (cursor_ < text_.size()) text_.erase(cursor_, 1);
                break;
            default: break;
        }
        return NoteEditorResult::editing;
    }

    if (key == 27) return NoteEditorResult::cancel;
    if (key == L'\n' || key == L'\r') return NoteEditorResult::save;
    if (key == 8 || key == 127) {
        if (cursor_ > 0) text_.erase(--cursor_, 1);
    } else if (key >= 32 && key != 127) {
        text_.insert(text_.begin() + static_cast<std::ptrdiff_t>(cursor_),
                     static_cast<wchar_t>(key));
        ++cursor_;
    }
    return NoteEditorResult::editing;
}

void NoteEditor::render(WINDOW* window) const {
    int height = 0;
    int width = 0;
    getmaxyx(window, height, width);
    if (height < 2 || width < 1) return;

    const int row = height - 2;
    const std::wstring prompt = L" Note: ";
    const int promptWidth = std::min(width, static_cast<int>(prompt.size()));
    const int inputWidth = std::max(0, width - promptWidth - 1);
    std::size_t first = 0;
    if (inputWidth > 0 && cursor_ >= static_cast<std::size_t>(inputWidth))
        first = cursor_ - static_cast<std::size_t>(inputWidth) + 1;
    const std::wstring visible = text_.substr(first, static_cast<std::size_t>(inputWidth));

    wattrset(window, A_REVERSE);
    mvwhline(window, row, 0, ' ', width);
    mvwaddnwstr(window, row, 0, prompt.c_str(), promptWidth);
    if (inputWidth > 0)
        mvwaddnwstr(window, row, promptWidth, visible.c_str(), inputWidth);
    wattrset(window, A_NORMAL);
    curs_set(1);
    const int cursorColumn = std::min(width - 1,
        promptWidth + static_cast<int>(cursor_ - first));
    wmove(window, row, cursorColumn);
    wrefresh(window);
}

std::string NoteEditor::note() const { return toUtf8(text_); }
std::size_t NoteEditor::cursor() const noexcept { return cursor_; }

} // namespace rcal
