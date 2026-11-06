#pragma once

#include "CalendarTypes.h"

#include <cstddef>
#include <string>

namespace rcal {

class CalendarStore {
public:
    virtual ~CalendarStore() = default;

    virtual DayInfo loadDay(const Date& date) = 0;
    virtual void saveWorkStatus(const Date& date, WorkStatus status) = 0;
    virtual Note addNote(const Date& date, std::string text, std::size_t position) = 0;
    virtual void updateNote(const NoteId& id, std::string text) = 0;
    virtual void deleteNote(const NoteId& id) = 0;
    virtual Task addTask(const Date& date, std::string text, std::size_t position,
                         std::string dueDate = "", std::string dueTime = "") = 0;
    virtual void updateTask(const TaskId& id, std::string text,
                            std::string dueDate = "", std::string dueTime = "") = 0;
    virtual void deleteTask(const TaskId& id) = 0;
    virtual void setTaskCompleted(const TaskId& id, bool completed) = 0;
    virtual Event addEvent(const Date& date, std::string time, std::string text,
                           std::size_t position, std::string startDate = "",
                           std::string endDate = "", std::string endTime = "") = 0;
    virtual void updateEvent(const EventId& id, std::string time, std::string text,
                             std::string startDate = "",
                           std::string endDate = "", std::string endTime = "") = 0;
    virtual void deleteEvent(const EventId& id) = 0;
};

} // namespace rcal
