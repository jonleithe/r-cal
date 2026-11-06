#pragma once

#include "CalendarTypes.h"
#include "NoteEditor.h"

#include <array>
#include <utility>

namespace rcal {

class TaskEditor {
public:
    explicit TaskEditor(Task task);
    NoteEditorResult handleInput(wint_t key, bool keyCode);
    void render(WINDOW* window) const;
    Task task() const;
    std::size_t selectedField() const noexcept { return selectedField_; }
    void setError(std::string error) { error_ = std::move(error); }

private:
    Task original_;
    std::array<NoteEditor, 3> fields_;
    std::size_t selectedField_ = 0;
    std::string error_;
};

} // namespace rcal
