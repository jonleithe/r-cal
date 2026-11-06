#pragma once

#include <cstddef>
#include <cwchar>
#include <string>

struct _win_st;
typedef struct _win_st WINDOW;

namespace rcal {

enum class NoteEditorResult {
    editing,
    save,
    cancel
};

class NoteEditor {
public:
    explicit NoteEditor(const std::string& note);

    NoteEditorResult handleInput(wint_t key, bool keyCode);
    void render(WINDOW* window, int row, std::size_t noteIndex) const;

    void renderField(WINDOW* window, int row, int column, int width, bool active) const;

    std::string note() const;
    std::size_t cursor() const noexcept;

private:
    std::wstring text_;
    std::size_t cursor_ = 0;
};

} // namespace rcal
