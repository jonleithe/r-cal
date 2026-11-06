#pragma once

#include <cstddef>

struct _win_st;
typedef struct _win_st WINDOW;

namespace rcal {

enum class CreateItemResult {
    editing,
    createNote,
    createTask,
    createEvent,
    cancel
};

class CreateItemSelector {
public:
    CreateItemResult handleInput(int key);
    void render(WINDOW* parent) const;

    std::size_t selectedItem() const noexcept;

private:
    std::size_t selectedItem_ = 0;
};

} // namespace rcal
