#pragma once

#include "CalendarTypes.h"

namespace rcal {

struct ScheduledText {
    std::string date;
    std::string time;
    std::string text;
};

// Empty date/time represents an unscheduled task or an all-day event.
void normalizeEvent(Event& event, const std::string& defaultDate);
bool eventOnDate(const Event& event, const std::string& date);
void validateSchedule(const std::string& date, const std::string& time);
ScheduledText parseScheduledText(const std::string& text, bool task);
std::string scheduledText(const std::string& date, const std::string& time,
                          const std::string& text, bool task);
// Local wall-clock comparison; date-only deadlines expire after that date.
bool taskOverdue(const Task& task, const std::string& today, const std::string& time);
std::string localDateString();
std::string localTimeString();
std::string calendarDateString(const Date& date);

} // namespace rcal
