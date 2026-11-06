#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace rcal {

struct Date {
    int year = 1970;
    unsigned month = 1;
    unsigned day = 1;
};

enum class WorkStatus {
    unspecified,
    workday,
    travelling,
    outOfOfficeWorking,
    compensatoryLeave,
    vacation
};

using NoteId = std::string;
using TaskId = std::string;
using EventId = std::string;

struct Note {
    NoteId id;
    std::string text;
};

struct Task {
    TaskId id;
    std::string text;
    bool completed = false;
    std::string dueDate;
    std::string dueTime;
};

struct Event {
    EventId id;
    std::string time;
    std::string text;
    std::string startDate;
    std::string endDate;
    std::string endTime;
};

struct DayInfo {
    WorkStatus workStatus = WorkStatus::unspecified;
    std::vector<Note> notes;
    std::vector<Task> tasks;
    std::vector<Event> events;
    unsigned taskCount = 0;
    unsigned eventCount = 0;
};

} // namespace rcal
