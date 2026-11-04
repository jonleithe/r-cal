#pragma once

#include <array>
#include <cstddef>
#include <map>
#include <string>
#include <vector>

struct _win_st;
typedef struct _win_st WINDOW;

namespace rcal{



struct Date{
    int year        = 1970;
    unsigned month  = 1;
    unsigned day    = 1;
};



enum class WorkStatus{
    unspecified,
    workday,
    travelling,
    outOfOfficeWorking,
    compensatoryLeave
};



struct DayInfo{
    WorkStatus workStatus = WorkStatus::unspecified;
    bool hasNote = false;
    unsigned taskCount = 0;
    unsigned eventCount = 0;
};



struct ColorStyle{
    std::string foreground = "default";
    std::string background = "default";

    bool bold       = false;
    bool underline  = false;
    bool reverse    = false;
};

// Applies a highlight background while retaining the content's foreground and attributes.
ColorStyle highlightedStyle(const ColorStyle& base, const ColorStyle& highlight);



struct HeaderColors{
    ColorStyle label;
    ColorStyle year;
    ColorStyle month;
    ColorStyle weekNumber;
    ColorStyle dayOfYear;
    ColorStyle date;
    ColorStyle dayName;
    ColorStyle weekend;
    ColorStyle holiday{"red", "default", true, false, false};
    ColorStyle currentDay{"black", "yellow", true, false, false};
    ColorStyle selectedDay{"default", "blue", false, false, false};
    ColorStyle timeInfo;
    ColorStyle grid;
};



struct HeaderOptions{
    Date startDate;
    std::size_t dayCount = 31;
    std::size_t bufferDays = 90;
    std::size_t rebuildThresholdDays = 30;
    unsigned cellWidth = 5;
    bool showGrid = true;
    std::string holidayCountry = "NO";

    std::array<std::string, 12> monthNames{{
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    }};

    std::array<std::string, 7> dayNames{{
        "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"
    }};

    std::string yearLabel       = "Year";
    std::string monthLabel      = "Month";
    std::string weekLabel       = "Week";
    std::string dayOfYearLabel  = "DOY";
    std::string dateLabel       = "Date";
    std::string dayLabel        = "Day";
    std::string timeInfoLabel   = "Info";
    std::string detailsLabel    = "Detail";
    std::string holidayInfoPrefix = "Helligdag";
};



struct TimelineDay{
    Date date;
    unsigned dayOfYear  = 1;
    unsigned isoWeek    = 1;
    unsigned isoWeekday = 1;
    std::string holidayName;
};



enum class InputResult{
    ignored,
    redraw,
    quit
};



class RCalHeaderFramework{
public:
    RCalHeaderFramework();
    explicit RCalHeaderFramework(HeaderOptions options);

    static RCalHeaderFramework fromConfigFile(const std::string& path);
    void loadConfigFile(const std::string& path);

    const HeaderOptions& options() const noexcept;
    const HeaderColors& colors() const noexcept;
    const std::vector<TimelineDay>& timeline() const noexcept;
    const Date& selectedDate() const noexcept;
    const std::string& selectedHolidayName() const noexcept;
    std::size_t firstVisibleDay() const noexcept;
    std::size_t visibleDayCount() const noexcept;
    const DayInfo* dayInfo(Date date) const noexcept;
    std::string infoIndicator(Date date) const;

    void setOptions(HeaderOptions options);
    void setColors(HeaderColors colors);
    void setDayInfo(Date date, DayInfo info);
    bool removeDayInfo(Date date);
    void rebuildTimeline();
    void moveSelection(int days);
    void selectDate(Date date);
    InputResult handleInput(int key, WINDOW* window);

    // The caller must initialize curses with initscr() before rendering.
    // Returns the number of terminal rows written.
    int render(WINDOW* window, int top = 0, int left = 0);

private:
    void ensureViewport(WINDOW* window, int left);
    void rebuildBufferedTimeline(Date selected, std::size_t visibleDays,
                                 std::size_t selectedOffset);

    HeaderOptions options_;
    HeaderColors colors_;
    std::vector<TimelineDay> timeline_;
    std::map<long long, DayInfo> dayInformation_;
    Date selectedDate_;
    std::size_t selectedDayIndex_ = 0;
    std::size_t firstVisibleDay_ = 0;
    std::size_t visibleDayCount_ = 0;
    bool viewportInitialized_ = false;
};

} // namespace rcal
