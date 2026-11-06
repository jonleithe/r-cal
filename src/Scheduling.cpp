#include "Scheduling.h"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace rcal {
namespace {
bool digits(const std::string& value, std::size_t first, std::size_t count) {
    for (std::size_t i = first; i < first + count; ++i)
        if (value[i] < '0' || value[i] > '9') return false;
    return true;
}
std::tm localNow() {
    const auto now = std::time(nullptr);
    std::tm local{};
#ifdef _WIN32
    localtime_s(&local, &now);
#else
    localtime_r(&now, &local);
#endif
    return local;
}
std::string formattedNow(const char* format) {
    const auto local = localNow();
    std::ostringstream result;
    result << std::put_time(&local, format);
    return result.str();
}
}

void validateSchedule(const std::string& date, const std::string& time) {
    if (!date.empty()) {
        if (date.size() != 10 || date[4] != '-' || date[7] != '-' ||
            !digits(date, 0, 4) || !digits(date, 5, 2) || !digits(date, 8, 2))
            throw std::invalid_argument("date must be YYYY-MM-DD");
        const int year = std::stoi(date.substr(0, 4));
        const int month = std::stoi(date.substr(5, 2));
        const int day = std::stoi(date.substr(8, 2));
        const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        const int lengths[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        if (year < 1 || month < 1 || month > 12 || day < 1 ||
            day > lengths[month - 1] + (month == 2 && leap ? 1 : 0))
            throw std::invalid_argument("invalid calendar date");
    }
    if (!time.empty() && (time.size() != 5 || time[2] != ':' ||
        !digits(time, 0, 2) || !digits(time, 3, 2) || time.substr(0, 2) > "23" ||
        time.substr(3, 2) > "59"))
        throw std::invalid_argument("time must be HH:MM (00:00 to 23:59)");
}

void normalizeEvent(Event& event, const std::string& defaultDate) {
    if (event.startDate.empty()) event.startDate = defaultDate;
    if (event.endDate.empty()) event.endDate = event.startDate;
    validateSchedule(event.startDate, event.time);
    validateSchedule(event.endDate, event.endTime);
    const std::string start = event.startDate + (event.time.empty() ? "00:00" : event.time);
    const std::string end = event.endDate + (event.endTime.empty() ? "23:59" : event.endTime);
    if (end < start) throw std::invalid_argument("event end must not precede its start");
}

bool eventOnDate(const Event& event, const std::string& date) {
    return event.startDate <= date && date <= event.endDate;
}

ScheduledText parseScheduledText(const std::string& value, bool task) {
    ScheduledText result;
    std::istringstream input(value);
    std::string token;
    input >> token;
    const std::string prefix = task ? "due:" : "start:";
    if (token.compare(0, prefix.size(), prefix) == 0) {
        result.date = token.substr(prefix.size());
        if (result.date.empty()) throw std::invalid_argument("schedule requires a date");
        token.clear();
        input >> token;
    }
    if ((!task || !result.date.empty()) && token.size() >= 3 && token[2] == ':') {
        result.time = token;
        token.clear();
        input >> token;
    }
    std::string rest;
    std::getline(input, rest);
    result.text = token + rest;
    const auto last = result.text.find_last_not_of(" \t\r\n");
    result.text = last == std::string::npos ? "" : result.text.substr(0, last + 1);
    validateSchedule(result.date, result.time);
    if ((!result.date.empty() || !result.time.empty()) && result.text.empty())
        throw std::invalid_argument("add a description after the schedule");
    return result;
}

std::string scheduledText(const std::string& date, const std::string& time,
                          const std::string& text, bool task) {
    return (date.empty() ? "" : std::string(task ? "due:" : "start:") + date + " ") +
        (time.empty() ? "" : time + " ") + text;
}

bool taskOverdue(const Task& task, const std::string& today, const std::string& time) {
    return !task.completed && !task.dueDate.empty() &&
        (task.dueDate < today || (task.dueDate == today && !task.dueTime.empty() &&
                                 task.dueTime < time));
}
std::string localDateString() { return formattedNow("%Y-%m-%d"); }
std::string localTimeString() { return formattedNow("%H:%M"); }
std::string calendarDateString(const Date& date) {
    std::ostringstream result;
    result << std::setfill('0') << std::setw(4) << date.year << '-'
           << std::setw(2) << date.month << '-' << std::setw(2) << date.day;
    validateSchedule(result.str(), "");
    return result.str();
}
} // namespace rcal
