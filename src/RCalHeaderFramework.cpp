#include "RCalHeaderFramework.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>

#include <curses.h>

namespace rcal {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::vector<std::string> split(const std::string& value, char delimiter) {
    std::vector<std::string> result;
    std::istringstream input(value);
    std::string item;
    while (std::getline(input, item, delimiter)) result.push_back(trim(item));
    return result;
}

long parseLong(const std::string& value, const std::string& key, long minimum,
               long maximum) {
    char* end = nullptr;
    errno = 0;
    const long number = std::strtol(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' || number < minimum ||
        number > maximum) {
        throw std::runtime_error("invalid value for '" + key + "': " + value);
    }
    return number;
}

bool parseBool(const std::string& value, const std::string& key) {
    const std::string normalized = lower(trim(value));
    if (normalized == "true" || normalized == "yes" || normalized == "1") return true;
    if (normalized == "false" || normalized == "no" || normalized == "0") return false;
    throw std::runtime_error("invalid boolean for '" + key + "': " + value);
}

Date parseDate(const std::string& value) {
    const auto parts = split(value, '-');
    if (parts.size() != 3) throw std::runtime_error("start_date must be YYYY-MM-DD");
    return {
        static_cast<int>(parseLong(parts[0], "start_date year", 1, 9999)),
        static_cast<unsigned>(parseLong(parts[1], "start_date month", 1, 12)),
        static_cast<unsigned>(parseLong(parts[2], "start_date day", 1, 31))
    };
}

bool leapYear(int year) {
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

unsigned daysInMonth(int year, unsigned month) {
    static constexpr std::array<unsigned, 12> days{{
        31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    }};
    return days.at(month - 1) + (month == 2 && leapYear(year) ? 1U : 0U);
}

void validateDate(const Date& date) {
    if (date.year < 1 || date.year > 9999 || date.month < 1 || date.month > 12 ||
        date.day < 1 || date.day > daysInMonth(date.year, date.month)) {
        throw std::runtime_error("invalid calendar date");
    }
}

long long daysFromCivil(int year, unsigned month, unsigned day) {
    year -= month <= 2;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned yearOfEra = static_cast<unsigned>(year - era * 400);
    const unsigned dayOfYear =
        (153 * (month > 2 ? month - 3 : month + 9) + 2) / 5 + day - 1;
    const unsigned dayOfEra = yearOfEra * 365 + yearOfEra / 4 - yearOfEra / 100 +
                              dayOfYear;
    return static_cast<long long>(era) * 146097 + dayOfEra - 719468;
}

Date civilFromDays(long long days) {
    days += 719468;
    const long long era = (days >= 0 ? days : days - 146096) / 146097;
    const unsigned dayOfEra = static_cast<unsigned>(days - era * 146097);
    const unsigned yearOfEra =
        (dayOfEra - dayOfEra / 1460 + dayOfEra / 36524 - dayOfEra / 146096) / 365;
    int year = static_cast<int>(yearOfEra) + static_cast<int>(era) * 400;
    const unsigned dayOfYear = dayOfEra -
        (365 * yearOfEra + yearOfEra / 4 - yearOfEra / 100);
    const unsigned monthPart = (5 * dayOfYear + 2) / 153;
    const unsigned day = dayOfYear - (153 * monthPart + 2) / 5 + 1;
    const unsigned month = monthPart < 10 ? monthPart + 3 : monthPart - 9;
    year += month <= 2;
    return {year, month, day};
}

unsigned isoWeekday(long long days) {
    long long weekday = (days + 3) % 7;
    if (weekday < 0) weekday += 7;
    return static_cast<unsigned>(weekday + 1);
}

unsigned ordinalDay(const Date& date) {
    static constexpr std::array<unsigned, 12> offsets{{
        0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
    }};
    return offsets[date.month - 1] + date.day +
           (date.month > 2 && leapYear(date.year) ? 1U : 0U);
}

unsigned isoWeeksInYear(int year) {
    const unsigned jan1 = isoWeekday(daysFromCivil(year, 1, 1));
    return jan1 == 4 || (jan1 == 3 && leapYear(year)) ? 53U : 52U;
}

unsigned isoWeek(const Date& date, unsigned weekday) {
    int week = (static_cast<int>(ordinalDay(date)) - static_cast<int>(weekday) + 10) / 7;
    if (week < 1) return isoWeeksInYear(date.year - 1);
    if (week > static_cast<int>(isoWeeksInYear(date.year))) return 1;
    return static_cast<unsigned>(week);
}

Date currentLocalDate() {
    const std::time_t now = std::time(nullptr);
    std::tm local{};
#if defined(_WIN32)
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return {local.tm_year + 1900, static_cast<unsigned>(local.tm_mon + 1),
            static_cast<unsigned>(local.tm_mday)};
}

bool sameDate(const Date& left, const Date& right) {
    return left.year == right.year && left.month == right.month && left.day == right.day;
}

Date addDays(const Date& date, long long days) {
    return civilFromDays(daysFromCivil(date.year, date.month, date.day) + days);
}

Date gregorianEasterSunday(int year) {
    const int a = year % 19;
    const int b = year / 100;
    const int c = year % 100;
    const int d = b / 4;
    const int e = b % 4;
    const int f = (b + 8) / 25;
    const int g = (b - f + 1) / 3;
    const int h = (19 * a + b - d - g + 15) % 30;
    const int i = c / 4;
    const int k = c % 4;
    const int l = (32 + 2 * e + 2 * i - h - k) % 7;
    const int m = (a + 11 * h + 22 * l) / 451;
    const unsigned month = static_cast<unsigned>((h + l - 7 * m + 114) / 31);
    const unsigned day = static_cast<unsigned>((h + l - 7 * m + 114) % 31 + 1);
    return {year, month, day};
}

std::string norwegianHolidayName(const Date& date) {
    if (date.month == 1 && date.day == 1) return "Nyttårsdag";
    if (date.month == 5 && date.day == 1) return "Arbeidernes dag";
    if (date.month == 5 && date.day == 17) return "Grunnlovsdagen";
    if (date.month == 12 && date.day == 25) return "Første juledag";
    if (date.month == 12 && date.day == 26) return "Andre juledag";

    const Date easterSunday = gregorianEasterSunday(date.year);
    const long long easter = daysFromCivil(date.year, easterSunday.month, easterSunday.day);
    const long long offset = daysFromCivil(date.year, date.month, date.day) - easter;
    switch (offset) {
        case -3: return "Skjærtorsdag";
        case -2: return "Langfredag";
        case 0: return "Første påskedag";
        case 1: return "Andre påskedag";
        case 39: return "Kristi himmelfartsdag";
        case 49: return "Første pinsedag";
        case 50: return "Andre pinsedag";
        default: return {};
    }
}

std::string holidayName(const Date& date, const std::string& country) {
    if (lower(country) == "no") return norwegianHolidayName(date);
    return {};
}

ColorStyle parseStyle(const std::string& value) {
    const auto parts = split(value, ',');
    ColorStyle style;
    if (!parts.empty() && !parts[0].empty()) style.foreground = lower(parts[0]);
    if (parts.size() > 1 && !parts[1].empty()) style.background = lower(parts[1]);
    for (std::size_t i = 2; i < parts.size(); ++i) {
        const std::string flag = lower(parts[i]);
        if (flag == "bold") style.bold = true;
        else if (flag == "underline") style.underline = true;
        else if (flag == "reverse") style.reverse = true;
        else if (!flag.empty()) throw std::runtime_error("unknown color attribute: " + flag);
    }
    return style;
}

short cursesColor(const std::string& name) {
    static const std::map<std::string, short> names{{
        {"default", -1}, {"black", COLOR_BLACK}, {"red", COLOR_RED},
        {"green", COLOR_GREEN}, {"yellow", COLOR_YELLOW}, {"blue", COLOR_BLUE},
        {"magenta", COLOR_MAGENTA}, {"cyan", COLOR_CYAN}, {"white", COLOR_WHITE}
    }};
    const auto found = names.find(lower(name));
    if (found != names.end()) return found->second;
    return static_cast<short>(parseLong(name, "color", 0, SHRT_MAX));
}

struct StyleRegistry {
    short nextPair = 1;
    std::map<std::pair<short, short>, short> pairs;

    attr_t attributes(const ColorStyle& style) {
        attr_t result = A_NORMAL;
        if (style.bold) result |= A_BOLD;
        if (style.underline) result |= A_UNDERLINE;
        if (style.reverse) result |= A_REVERSE;
        if (!has_colors()) return result;
        const short foreground = cursesColor(style.foreground);
        const short background = cursesColor(style.background);
        const auto key = std::make_pair(foreground, background);
        auto found = pairs.find(key);
        if (found == pairs.end()) {
            if (nextPair >= COLOR_PAIRS) return result;
            init_pair(nextPair, foreground, background);
            found = pairs.emplace(key, nextPair++).first;
        }
        return result | COLOR_PAIR(found->second);
    }
};

std::string centered(std::string text, unsigned width) {
    if (text.size() > width) text.resize(width);
    const std::size_t left = (width - text.size()) / 2;
    return std::string(left, ' ') + text + std::string(width - left - text.size(), ' ');
}

void put(WINDOW* window, int y, int x, const std::string& text, attr_t attributes) {
    int windowHeight = 0;
    int windowWidth = 0;
    getmaxyx(window, windowHeight, windowWidth);
    if (y < 0 || y >= windowHeight || x >= windowWidth || text.empty()) return;

    std::size_t offset = 0;
    if (x < 0) {
        offset = std::min<std::size_t>(static_cast<std::size_t>(-x), text.size());
        x = 0;
    }
    const int available = windowWidth - x;
    const int length = std::min<int>(available, static_cast<int>(text.size() - offset));
    if (length <= 0) return;

    wattrset(window, attributes);
    mvwaddnstr(window, y, x, text.c_str() + offset, length);
}

} // namespace

RCalHeaderFramework::RCalHeaderFramework() { rebuildTimeline(); }

RCalHeaderFramework::RCalHeaderFramework(HeaderOptions options)
    : options_(std::move(options)) { rebuildTimeline(); }

RCalHeaderFramework RCalHeaderFramework::fromConfigFile(const std::string& path) {
    RCalHeaderFramework result;
    result.loadConfigFile(path);
    return result;
}

void RCalHeaderFramework::loadConfigFile(const std::string& path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open config file: " + path);
    HeaderOptions newOptions = options_;
    HeaderColors newColors = colors_;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        const std::string clean = trim(line);
        if (clean.empty() || clean[0] == '#' || clean[0] == ';') continue;
        const auto equals = clean.find('=');
        if (equals == std::string::npos) {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) +
                                     ": expected key=value");
        }
        const std::string key = lower(trim(clean.substr(0, equals)));
        const std::string value = trim(clean.substr(equals + 1));
        try {
            if (key == "start_date") newOptions.startDate =
                lower(value) == "today" ? currentLocalDate() : parseDate(value);
            else if (key == "days") newOptions.dayCount =
                static_cast<std::size_t>(parseLong(value, key, 1, 3660));
            else if (key == "buffer_days") newOptions.bufferDays =
                static_cast<std::size_t>(parseLong(value, key, 1, 3660));
            else if (key == "rebuild_threshold_days") newOptions.rebuildThresholdDays =
                static_cast<std::size_t>(parseLong(value, key, 1, 3659));
            else if (key == "cell_width") newOptions.cellWidth =
                static_cast<unsigned>(parseLong(value, key, 2, 20));
            else if (key == "show_grid") newOptions.showGrid = parseBool(value, key);
            else if (key == "time_info") newOptions.timeInfo = value;
            else if (key == "holiday_country") {
                const std::string country = lower(value);
                if (country == "no") newOptions.holidayCountry = "NO";
                else if (country == "none") newOptions.holidayCountry = "none";
                else throw std::runtime_error("unsupported holiday country '" + value + "'");
            }
            else if (key == "month_names") {
                const auto names = split(value, ',');
                if (names.size() != 12) throw std::runtime_error("month_names needs 12 values");
                std::copy(names.begin(), names.end(), newOptions.monthNames.begin());
            } else if (key == "day_names") {
                const auto names = split(value, ',');
                if (names.size() != 7) throw std::runtime_error("day_names needs 7 values");
                std::copy(names.begin(), names.end(), newOptions.dayNames.begin());
            } else if (key == "label.year") newOptions.yearLabel = value;
            else if (key == "label.month") newOptions.monthLabel = value;
            else if (key == "label.week") newOptions.weekLabel = value;
            else if (key == "label.date") newOptions.dateLabel = value;
            else if (key == "label.day") newOptions.dayLabel = value;
            else if (key == "label.time_info") newOptions.timeInfoLabel = value;
            else if (key == "color.label") newColors.label = parseStyle(value);
            else if (key == "color.year") newColors.year = parseStyle(value);
            else if (key == "color.month") newColors.month = parseStyle(value);
            else if (key == "color.week_number") newColors.weekNumber = parseStyle(value);
            else if (key == "color.date") newColors.date = parseStyle(value);
            else if (key == "color.day_name") newColors.dayName = parseStyle(value);
            else if (key == "color.weekend") newColors.weekend = parseStyle(value);
            else if (key == "color.holiday") newColors.holiday = parseStyle(value);
            else if (key == "color.current_day") newColors.currentDay = parseStyle(value);
            else if (key == "color.selected_day") newColors.selectedDay = parseStyle(value);
            else if (key == "color.time_info") newColors.timeInfo = parseStyle(value);
            else if (key == "color.grid") newColors.grid = parseStyle(value);
            else throw std::runtime_error("unknown setting '" + key + "'");
        } catch (const std::exception& error) {
            throw std::runtime_error(path + ":" + std::to_string(lineNumber) + ": " + error.what());
        }
    }
    options_ = std::move(newOptions);
    colors_ = std::move(newColors);
    rebuildTimeline();
}



const HeaderOptions& RCalHeaderFramework::options() const noexcept { return options_; }
const HeaderColors& RCalHeaderFramework::colors() const noexcept { return colors_; }
const std::vector<TimelineDay>& RCalHeaderFramework::timeline() const noexcept { return timeline_; }
const Date& RCalHeaderFramework::selectedDate() const noexcept { return selectedDate_; }
std::size_t RCalHeaderFramework::firstVisibleDay() const noexcept { return firstVisibleDay_; }
std::size_t RCalHeaderFramework::visibleDayCount() const noexcept { return visibleDayCount_; }



void RCalHeaderFramework::setOptions(HeaderOptions options) {
    options_ = std::move(options);
    rebuildTimeline();
}



void RCalHeaderFramework::setColors(HeaderColors colors)
{
    colors_ = std::move(colors);
}



void RCalHeaderFramework::rebuildTimeline() {
    validateDate(options_.startDate);
    if (options_.dayCount == 0) throw std::runtime_error("dayCount must be at least 1");
    if (options_.bufferDays == 0) throw std::runtime_error("bufferDays must be at least 1");
    if (options_.rebuildThresholdDays >= options_.bufferDays)
        throw std::runtime_error("rebuildThresholdDays must be smaller than bufferDays");
    if (options_.cellWidth < 2) throw std::runtime_error("cellWidth must be at least 2");
    timeline_.clear();
    timeline_.reserve(options_.dayCount);
    const long long first = daysFromCivil(options_.startDate.year, options_.startDate.month,
                                          options_.startDate.day);
    for (std::size_t offset = 0; offset < options_.dayCount; ++offset) {
        const long long absolute = first + static_cast<long long>(offset);
        const Date date = civilFromDays(absolute);
        const unsigned weekday = isoWeekday(absolute);
        timeline_.push_back({date, ordinalDay(date), isoWeek(date, weekday), weekday,
                             holidayName(date, options_.holidayCountry)});
    }
    selectedDate_ = options_.startDate;
    selectedDayIndex_ = 0;
    firstVisibleDay_ = 0;
    visibleDayCount_ = 0;
    viewportInitialized_ = false;
}



void RCalHeaderFramework::rebuildBufferedTimeline(Date selected, std::size_t visibleDays,
                                                  std::size_t selectedOffset) {
    validateDate(selected);
    selectedOffset = std::min(selectedOffset, visibleDays - 1);
    const std::size_t buffer = options_.bufferDays;
    const Date firstDate = addDays(selected,
        -static_cast<long long>(buffer + selectedOffset));
    const std::size_t count = buffer + visibleDays + buffer;

    timeline_.clear();
    timeline_.reserve(count);
    const long long first = daysFromCivil(firstDate.year, firstDate.month, firstDate.day);
    for (std::size_t offset = 0; offset < count; ++offset) {
        const Date date = civilFromDays(first + static_cast<long long>(offset));
        const unsigned weekday = isoWeekday(first + static_cast<long long>(offset));
        timeline_.push_back({date, ordinalDay(date), isoWeek(date, weekday), weekday,
                             holidayName(date, options_.holidayCountry)});
    }

    selectedDate_ = selected;
    firstVisibleDay_ = buffer;
    selectedDayIndex_ = buffer + selectedOffset;
    visibleDayCount_ = visibleDays;
    viewportInitialized_ = true;
}



void RCalHeaderFramework::ensureViewport(WINDOW* window, int left) {
    int windowHeight = 0;
    int windowWidth = 0;
    getmaxyx(window, windowHeight, windowWidth);
    (void)windowHeight;

    const std::array<std::string, 6> labels{{
        options_.yearLabel, options_.monthLabel, options_.weekLabel,
        options_.dateLabel, options_.dayLabel, options_.timeInfoLabel
    }};
    std::size_t labelWidth = 1;
    for (const auto& label : labels) labelWidth = std::max(labelWidth, label.size());
    const int firstCell = left + static_cast<int>(labelWidth) + 1;
    const int available = std::max(0, windowWidth - firstCell);
    const std::size_t stride = options_.cellWidth + (options_.showGrid ? 1U : 0U);
    const std::size_t completeCells = static_cast<std::size_t>(available) +
        (options_.showGrid ? 1U : 0U);
    const std::size_t visibleDays = completeCells / stride;
    if (visibleDays == 0) {
        visibleDayCount_ = 0;
        return;
    }

    if (!viewportInitialized_ || visibleDays != visibleDayCount_) {
        rebuildBufferedTimeline(selectedDate_, visibleDays, visibleDays / 2);
    }
}



void RCalHeaderFramework::moveSelection(int days) {
    if (days == 0) return;
    const Date moved = addDays(selectedDate_, days);
    validateDate(moved);

    if (!viewportInitialized_ || visibleDayCount_ == 0) {
        selectedDate_ = moved;
        options_.startDate = moved;
        return;
    }

    const long long candidate = static_cast<long long>(selectedDayIndex_) + days;
    if (candidate < 0 || candidate >= static_cast<long long>(timeline_.size())) {
        rebuildBufferedTimeline(moved, visibleDayCount_, visibleDayCount_ / 2);
        return;
    }

    selectedDate_ = moved;
    selectedDayIndex_ = static_cast<std::size_t>(candidate);
    if (selectedDayIndex_ < firstVisibleDay_) {
        firstVisibleDay_ = selectedDayIndex_;
    } else if (selectedDayIndex_ >= firstVisibleDay_ + visibleDayCount_) {
        firstVisibleDay_ = selectedDayIndex_ - visibleDayCount_ + 1;
    }

    const std::size_t rightEdge = firstVisibleDay_ + visibleDayCount_;
    const bool nearLeft = firstVisibleDay_ < options_.rebuildThresholdDays;
    const bool nearRight = timeline_.size() - std::min(rightEdge, timeline_.size()) <
                           options_.rebuildThresholdDays;
    if (nearLeft || nearRight) {
        const std::size_t selectedOffset = selectedDayIndex_ - firstVisibleDay_;
        rebuildBufferedTimeline(selectedDate_, visibleDayCount_, selectedOffset);
    }
}



void RCalHeaderFramework::selectDate(Date date) {
    validateDate(date);
    selectedDate_ = date;
    if (viewportInitialized_ && visibleDayCount_ > 0)
        rebuildBufferedTimeline(date, visibleDayCount_, visibleDayCount_ / 2);
    else
        options_.startDate = date;
}



InputResult RCalHeaderFramework::handleInput(int key, WINDOW* window) {
    if (!window) throw std::invalid_argument("input window must not be null");
    switch (key) {
        case 'q':
        case 'Q':
        case 27:
            return InputResult::quit;
        case KEY_LEFT:
        case 'h':
            moveSelection(-1);
            return InputResult::redraw;
        case KEY_RIGHT:
        case 'l':
            moveSelection(1);
            return InputResult::redraw;
        case KEY_PPAGE:
            moveSelection(-static_cast<int>(std::max<std::size_t>(1, visibleDayCount_)));
            return InputResult::redraw;
        case KEY_NPAGE:
            moveSelection(static_cast<int>(std::max<std::size_t>(1, visibleDayCount_)));
            return InputResult::redraw;
        case KEY_HOME:
        case 't':
            selectDate(currentLocalDate());
            return InputResult::redraw;
        case KEY_RESIZE:
            viewportInitialized_ = false;
            return InputResult::redraw;
        default:
            return InputResult::ignored;
    }
}



int RCalHeaderFramework::render(WINDOW* window, int top, int left) {
    if (!window) throw std::invalid_argument("render window must not be null");
    if (top < 0 || left < 0)
        throw std::invalid_argument("render position must not be negative");

    int windowHeight = 0;
    int windowWidth = 0;
    getmaxyx(window, windowHeight, windowWidth);
    if (top >= windowHeight || left >= windowWidth) return 0;
    ensureViewport(window, left);

    curs_set(0);
    if (has_colors()) { start_color(); use_default_colors(); }
    StyleRegistry styles;
    const unsigned width = options_.cellWidth;
    const std::array<std::string, 6> labels{{
        options_.yearLabel, options_.monthLabel, options_.weekLabel,
        options_.dateLabel, options_.dayLabel, options_.timeInfoLabel
    }};
    std::size_t labelWidth = 1;
    for (const auto& label : labels) labelWidth = std::max(labelWidth, label.size());
    const int firstCell = left + static_cast<int>(labelWidth) + 1;

    for (int row = 0; row < 6; ++row) {
        put(window, top + row, left, labels[static_cast<std::size_t>(row)],
            styles.attributes(colors_.label));
        if (options_.showGrid)
            put(window, top + row, firstCell - 1, "|", styles.attributes(colors_.grid));
    }

    const unsigned stride = width + (options_.showGrid ? 1U : 0U);
    auto renderSpans = [&](int row, const std::vector<std::string>& values,
                           const ColorStyle& style) {
        std::size_t begin = 0;
        while (begin < values.size()) {
            std::size_t end = begin + 1;
            while (end < values.size() && values[end] == values[begin]) ++end;
            const unsigned spanWidth = static_cast<unsigned>((end - begin) * stride -
                                       (options_.showGrid ? 1U : 0U));
            const int x = firstCell + static_cast<int>(begin * stride);
            if (x >= windowWidth) break;

            // Center a group title in the visible part of the span. This keeps
            // long groups, such as a whole month, labelled even when their
            // natural centre is outside the terminal viewport.
            const int visibleEnd = std::min(windowWidth, x + static_cast<int>(spanWidth));
            const unsigned visibleWidth = static_cast<unsigned>(std::max(0, visibleEnd - x));
            put(window, top + row, x, centered(values[begin], visibleWidth),
                styles.attributes(style));
            if (options_.showGrid && x + static_cast<int>(spanWidth) < windowWidth)
                put(window, top + row, x + static_cast<int>(spanWidth), "|",
                    styles.attributes(colors_.grid));
            begin = end;
        }
    };

    std::vector<std::string> years;
    std::vector<std::string> months;
    std::vector<std::string> weeks;
    const std::size_t visibleEnd = std::min(timeline_.size(),
                                             firstVisibleDay_ + visibleDayCount_);
    years.reserve(visibleDayCount_);
    months.reserve(visibleDayCount_);
    weeks.reserve(visibleDayCount_);
    for (std::size_t i = firstVisibleDay_; i < visibleEnd; ++i) {
        const auto& day = timeline_[i];
        years.push_back(std::to_string(day.date.year));
        months.push_back(options_.monthNames.at(day.date.month - 1));
        weeks.push_back(std::to_string(day.isoWeek));
    }
    renderSpans(0, years, colors_.year);
    renderSpans(1, months, colors_.month);
    renderSpans(2, weeks, colors_.weekNumber);

    const Date today = currentLocalDate();
    for (std::size_t i = firstVisibleDay_; i < visibleEnd; ++i) {
        const auto& day = timeline_[i];
        const std::size_t visibleIndex = i - firstVisibleDay_;
        const int x = firstCell + static_cast<int>(visibleIndex * stride);
        if (x >= windowWidth) break;
        const bool isToday = sameDate(day.date, today);
        const bool isSelected = i == selectedDayIndex_;
        const bool isHoliday = !day.holidayName.empty();
        const ColorStyle& dateStyle = isToday ? colors_.currentDay
            : (isSelected ? colors_.selectedDay
            : (isHoliday ? colors_.holiday : colors_.date));
        const ColorStyle& dayStyle = isToday ? colors_.currentDay
            : (isSelected ? colors_.selectedDay
            : (isHoliday ? colors_.holiday
            : (day.isoWeekday >= 6 ? colors_.weekend : colors_.dayName)));
        const ColorStyle& infoStyle = isToday ? colors_.currentDay
            : (isSelected ? colors_.selectedDay
            : (isHoliday ? colors_.holiday : colors_.timeInfo));
        put(window, top + 3, x, centered(std::to_string(day.date.day), width), styles.attributes(dateStyle));
        put(window, top + 4, x, centered(options_.dayNames.at(day.isoWeekday - 1), width), styles.attributes(dayStyle));
        put(window, top + 5, x, centered(options_.timeInfo, width), styles.attributes(infoStyle));
        if (options_.showGrid) {
            const int separator = x + static_cast<int>(width);
            for (int row = 3; row < 6; ++row)
                put(window, top + row, separator, "|", styles.attributes(colors_.grid));
        }
    }
    wattrset(window, A_NORMAL);
    return std::min(6, windowHeight - top);
}

} // namespace rcal
