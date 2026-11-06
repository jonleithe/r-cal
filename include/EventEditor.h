#pragma once

#include "CalendarTypes.h"
#include "NoteEditor.h"

#include <array>
#include <utility>

namespace rcal {

class EventEditor {
public:
    explicit EventEditor(Event event);
    NoteEditorResult handleInput(wint_t key, bool keyCode);
    void render(WINDOW* window) const;
    Event event() const;
    std::size_t selectedField() const noexcept { return selectedField_; }
    void setError(std::string error) { error_ = std::move(error); }

private:
    Event original_;
    std::array<NoteEditor, 5> fields_;
    std::size_t selectedField_ = 0;
    std::string error_;
};

} // namespace rcal
