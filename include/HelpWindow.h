#pragma once

#include <cstddef>

struct _win_st;
typedef struct _win_st WINDOW;

namespace rcal {

class HelpWindow {
public:
    // Returns true when the user closes help.
    bool handleInput(int key);
    void render(WINDOW* parent);

private:
    std::size_t firstLine_ = 0;
    std::size_t lineCount_ = 0;
    std::size_t visibleLines_ = 1;
};

} // namespace rcal
