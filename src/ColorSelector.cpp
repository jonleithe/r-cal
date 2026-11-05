#include "ColorSelector.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <curses.h>

namespace rcal {
namespace {

constexpr std::size_t elementCount = 14;
constexpr std::size_t rowCount = elementCount * 2;

const std::array<const char*, elementCount> elementLabels{{
    "Label", "Year", "Month", "Week number", "Day of year", "Date",
    "Day name", "Weekend", "Holiday", "Current day", "Selected day",
    "Time info", "Grid", "Status bar"
}};

const std::array<const char*, elementCount> configKeys{{
    "color.label", "color.year", "color.month", "color.week_number",
    "color.day_of_year", "color.date", "color.day_name", "color.weekend",
    "color.holiday", "color.current_day", "color.selected_day",
    "color.time_info", "color.grid", "color.status_bar"
}};

std::string colorName(int color) {
    static const std::array<const char*, 8> standard{{
        "black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"
    }};
    return color >= 0 && color < static_cast<int>(standard.size())
        ? standard[static_cast<std::size_t>(color)] : std::to_string(color);
}

std::string serialize(const ColorStyle& style) {
    std::string value = style.foreground + "," + style.background;
    if (style.bold) value += ",bold";
    if (style.underline) value += ",underline";
    if (style.reverse) value += ",reverse";
    return value;
}

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string keyFromLine(const std::string& line) {
    const std::string clean = trim(line);
    if (clean.empty() || clean.front() == '#' || clean.front() == ';') return {};
    const auto equals = clean.find('=');
    return equals == std::string::npos ? std::string{} : trim(clean.substr(0, equals));
}

void drawText(WINDOW* window, int y, int x, const std::string& text, int width) {
    if (width <= 0) return;
    mvwaddnstr(window, y, x, text.c_str(), width);
}

short contrastingColor(short background) {
    if (background < 0) return COLORS >= 8 ? COLOR_WHITE : (COLORS > 1 ? 1 : 0);
    if (COLORS < 8) return background == 0 && COLORS > 1 ? 1 : 0;
    short red = 0;
    short green = 0;
    short blue = 0;
    if (color_content(background, &red, &green, &blue) == ERR) return COLOR_WHITE;
    const int brightness = (299 * red + 587 * green + 114 * blue) / 1000;
    return brightness >= 500 ? COLOR_BLACK : COLOR_WHITE;
}

} // namespace

ColorSelector::ColorSelector(HeaderColors colors, int terminalColorCount)
    : colors_(std::move(colors)), palettePositions_(rowCount, 0) {
    palette_.push_back("default");
    for (int color = 0; color < std::max(0, terminalColorCount); ++color)
        palette_.push_back(colorName(color));

    for (std::size_t row = 0; row < rowCount; ++row) {
        const ColorStyle& current = style(row / 2);
        const std::string& name = row % 2 == 0 ? current.foreground : current.background;
        const auto found = std::find(palette_.begin(), palette_.end(), name);
        if (found != palette_.end())
            palettePositions_[row] = static_cast<std::size_t>(found - palette_.begin());
    }
}

ColorStyle& ColorSelector::style(std::size_t element) {
    switch (element) {
        case 0: return colors_.label;
        case 1: return colors_.year;
        case 2: return colors_.month;
        case 3: return colors_.weekNumber;
        case 4: return colors_.dayOfYear;
        case 5: return colors_.date;
        case 6: return colors_.dayName;
        case 7: return colors_.weekend;
        case 8: return colors_.holiday;
        case 9: return colors_.currentDay;
        case 10: return colors_.selectedDay;
        case 11: return colors_.timeInfo;
        case 12: return colors_.grid;
        default: return colors_.statusBar;
    }
}

const ColorStyle& ColorSelector::style(std::size_t element) const {
    return const_cast<ColorSelector*>(this)->style(element);
}

std::string& ColorSelector::selectedColor() {
    ColorStyle& current = style(selectedRow_ / 2);
    return selectedRow_ % 2 == 0 ? current.foreground : current.background;
}

void ColorSelector::selectPaletteOffset(int offset) {
    const long long current = static_cast<long long>(palettePositions_[selectedRow_]);
    const long long last = static_cast<long long>(palette_.size() - 1);
    const std::size_t next = static_cast<std::size_t>(std::max(0LL, std::min(last, current + offset)));
    palettePositions_[selectedRow_] = next;
    selectedColor() = palette_[next];
}

ColorSelectorResult ColorSelector::handleInput(int key) {
    switch (key) {
        case KEY_UP:
            if (selectedRow_ > 0) --selectedRow_;
            break;
        case KEY_DOWN:
            if (selectedRow_ + 1 < rowCount) ++selectedRow_;
            break;
        case KEY_LEFT: selectPaletteOffset(-1); break;
        case KEY_RIGHT: selectPaletteOffset(1); break;
        case 'b': case 'B': style(selectedRow_ / 2).bold = !style(selectedRow_ / 2).bold; break;
        case 'u': case 'U': style(selectedRow_ / 2).underline = !style(selectedRow_ / 2).underline; break;
        case 'r': case 'R': style(selectedRow_ / 2).reverse = !style(selectedRow_ / 2).reverse; break;
        case '\n': case '\r': case KEY_ENTER: return ColorSelectorResult::save;
        case 27: return ColorSelectorResult::cancel;
        default: break;
    }
    return ColorSelectorResult::editing;
}

void ColorSelector::render(WINDOW* parent) {
    int parentHeight = 0;
    int parentWidth = 0;
    getmaxyx(parent, parentHeight, parentWidth);
    const int height = std::min(parentHeight, 30);
    const int width = std::min(parentWidth, std::max(54, std::min(parentWidth, 96)));
    if (height < 7 || width < 32) {
        drawText(parent, 0, 0, "Terminal too small for color selector", parentWidth);
        return;
    }

    const int top = (parentHeight - height) / 2;
    const int left = (parentWidth - width) / 2;
    WINDOW* dialog = newwin(height, width, top, left);
    if (!dialog) throw std::runtime_error("cannot create color selector window");
    box(dialog, 0, 0);
    drawText(dialog, 0, 2, " Color selector ", width - 4);
    drawText(dialog, 1, 2, "Arrows: navigate  b/u/r: attributes  Enter: save  Esc: cancel", width - 4);

    const int labelWidth = std::min(24, width / 3);
    const int paletteX = labelWidth + 3;
    const int layoutPaletteWidth = std::max(1, width - paletteX - 2);
    // COLOR_PAIR() only carries eight bits of pair data with the traditional
    // curses API, even when a terminal advertises many more extended pairs.
    // Keeping previews in the portable range prevents them from rendering as
    // an uncoloured window background.
    const int maximumPair = has_colors() ? std::min(COLOR_PAIRS - 1, 255) : 0;
    const int paletteWidth = has_colors()
        ? std::max(1, std::min(layoutPaletteWidth, maximumPair / 2))
        : layoutPaletteWidth;
    const int pairBase = has_colors() ? maximumPair - paletteWidth + 1 : 0;
    const int visibleRows = height - 4;
    const std::size_t firstRow = selectedRow_ < static_cast<std::size_t>(visibleRows)
        ? 0 : selectedRow_ - static_cast<std::size_t>(visibleRows) + 1;
    const std::size_t selectedColorIndex = palettePositions_[selectedRow_];
    const std::size_t firstColor = selectedColorIndex < static_cast<std::size_t>(paletteWidth)
        ? 0 : selectedColorIndex - static_cast<std::size_t>(paletteWidth) + 1;

    for (int visible = 0; visible < visibleRows; ++visible) {
        const std::size_t row = firstRow + static_cast<std::size_t>(visible);
        if (row >= rowCount) break;
        const int y = visible + 2;
        const ColorStyle& current = style(row / 2);
        std::string label = std::string(elementLabels[row / 2]) +
            (row % 2 == 0 ? " foreground" : " background");
        if (current.bold) label += " B";
        if (current.underline) label += " U";
        if (current.reverse) label += " R";
        if (row == selectedRow_) wattron(dialog, A_REVERSE);
        drawText(dialog, y, 2, label, labelWidth);
        if (row == selectedRow_) wattroff(dialog, A_REVERSE);

        for (int column = 0; column < paletteWidth; ++column) {
            const std::size_t paletteIndex = firstColor + static_cast<std::size_t>(column);
            if (paletteIndex >= palette_.size()) break;
            const bool selected = row == selectedRow_ && paletteIndex == palettePositions_[row];
            if (has_colors()) {
                const short pair = static_cast<short>(pairBase + column);
                if (pair <= 0 || pair >= COLOR_PAIRS) break;
                const short color = paletteIndex == 0 ? -1 : static_cast<short>(paletteIndex - 1);
                init_pair(pair, contrastingColor(color), color);
                wattrset(dialog, COLOR_PAIR(pair) | (selected ? A_BOLD : A_NORMAL));
            } else {
                wattrset(dialog, selected ? A_REVERSE : A_NORMAL);
            }
            mvwaddch(dialog, y, paletteX + column,
                      selected && has_colors() ? ACS_DIAMOND : ' ');
        }
        wattrset(dialog, A_NORMAL);
    }

    const std::string status = "Selected: " + selectedColor() + "  [" +
        (style(selectedRow_ / 2).bold ? std::string("B") : "-") +
        (style(selectedRow_ / 2).underline ? "U" : "-") +
        (style(selectedRow_ / 2).reverse ? "R" : "-") + "]";
    drawText(dialog, height - 2, 2, status, width - 4);
    copywin(dialog, parent, 0, 0, top, left, top + height - 1, left + width - 1, 0);
    delwin(dialog);
    wrefresh(parent);
}

const HeaderColors& ColorSelector::colors() const noexcept { return colors_; }
std::size_t ColorSelector::selectedRow() const noexcept { return selectedRow_; }
const std::vector<std::string>& ColorSelector::palette() const noexcept { return palette_; }

void saveColorsToConfig(const std::string& path, const HeaderColors& colors) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open config file for reading: " + path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(input, line)) lines.push_back(line);
    if (!input.eof()) throw std::runtime_error("cannot read config file: " + path);

    const std::array<const ColorStyle*, elementCount> styles{{
        &colors.label, &colors.year, &colors.month, &colors.weekNumber,
        &colors.dayOfYear, &colors.date, &colors.dayName, &colors.weekend,
        &colors.holiday, &colors.currentDay, &colors.selectedDay,
        &colors.timeInfo, &colors.grid, &colors.statusBar
    }};
    std::array<bool, elementCount> found{};
    for (std::string& currentLine : lines) {
        const std::string key = keyFromLine(currentLine);
        for (std::size_t i = 0; i < elementCount; ++i) {
            if (key == configKeys[i]) {
                currentLine = std::string(configKeys[i]) + "=" + serialize(*styles[i]);
                found[i] = true;
                break;
            }
        }
    }
    for (std::size_t i = 0; i < elementCount; ++i)
        if (!found[i]) lines.push_back(std::string(configKeys[i]) + "=" + serialize(*styles[i]));

    const std::string temporary = path + ".tmp";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) throw std::runtime_error("cannot open temporary config file: " + temporary);
        for (const std::string& outputLine : lines) output << outputLine << '\n';
        if (!output) throw std::runtime_error("cannot write temporary config file: " + temporary);
    }
    if (std::rename(temporary.c_str(), path.c_str()) != 0) {
        std::remove(temporary.c_str());
        throw std::runtime_error("cannot replace config file: " + path);
    }
}

} // namespace rcal
