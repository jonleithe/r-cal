#pragma once

#include "RCalHeaderFramework.h"

#include <cstddef>
#include <string>
#include <vector>

struct _win_st;
typedef struct _win_st WINDOW;

namespace rcal {

enum class ColorSelectorResult {
    editing,
    save,
    cancel
};

class ColorSelector {
public:
    ColorSelector(HeaderColors colors, int terminalColorCount);

    ColorSelectorResult handleInput(int key);
    void render(WINDOW* parent);

    const HeaderColors& colors() const noexcept;
    std::size_t selectedRow() const noexcept;
    const std::vector<std::string>& palette() const noexcept;

private:
    ColorStyle& style(std::size_t element);
    const ColorStyle& style(std::size_t element) const;
    std::string& selectedColor();
    void selectPaletteOffset(int offset);

    HeaderColors colors_;
    std::vector<std::string> palette_;
    std::size_t selectedRow_ = 0;
    std::vector<std::size_t> palettePositions_;
};

void saveColorsToConfig(const std::string& path, const HeaderColors& colors);

} // namespace rcal
