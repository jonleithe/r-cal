#pragma once

#include <array>
#include <cstddef>
#include <string>
#include <vector>

struct _win_st;
typedef struct _win_st WINDOW;

namespace rcal {

struct Date {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
};

struct ColorStyle {
    std::string foreground = "default";
    std::string background = "default";
    bool bold = false;
    bool underline = false;
    bool reverse = false;
};

struct HeaderColors {
    ColorStyle label;
    ColorStyle year;
    ColorStyle month;
    ColorStyle weekNumber;
    ColorStyle date;
    ColorStyle dayName;
    ColorStyle weekend;
    ColorStyle timeInfo;
    ColorStyle grid;
};

struct HeaderOptions {
    Date startDate;
    std::size_t dayCount = 31;
    unsigned cellWidth = 4;
    bool showGrid = true;
    std::string timeInfo;
    std::array<std::string, 12> monthNames{{
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    }};
    std::array<std::string, 7> dayNames{{
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    }};
    std::string yearLabel = "Year";
    std::string monthLabel = "Month";
    std::string weekLabel = "Week";
    std::string dateLabel = "Date";
    std::string dayLabel = "Day";
    std::string timeInfoLabel = "Info";
};

struct TimelineDay {
    Date date;
    unsigned dayOfYear = 1;
    unsigned isoWeek = 1;
    unsigned isoWeekday = 1;
};

class RCalHeaderFramework {
public:
    RCalHeaderFramework();
    explicit RCalHeaderFramework(HeaderOptions options);

    static RCalHeaderFramework fromConfigFile(const std::string& path);
    void loadConfigFile(const std::string& path);

    const HeaderOptions& options() const noexcept;
    const HeaderColors& colors() const noexcept;
    const std::vector<TimelineDay>& timeline() const noexcept;

    void setOptions(HeaderOptions options);
    void setColors(HeaderColors colors);
    void rebuildTimeline();

    // The caller must initialize curses with initscr() before rendering.
    // Returns the number of terminal rows written.
    int render(WINDOW* window, int top = 0, int left = 0) const;

private:
    HeaderOptions options_;
    HeaderColors colors_;
    std::vector<TimelineDay> timeline_;
};

} // namespace rcal
