#include "RCalHeaderFramework.h"

#include <cassert>

int main() {
    const rcal::ColorStyle weekend{"red", "default", false, true, false};
    const rcal::ColorStyle selection{"white", "blue", true, false, false};
    const auto selectedWeekend = rcal::highlightedStyle(weekend, selection);
    assert(selectedWeekend.foreground == "red");
    assert(selectedWeekend.background == "blue");
    assert(selectedWeekend.bold);
    assert(selectedWeekend.underline);
    assert(!selectedWeekend.reverse);

    rcal::HeaderOptions options;
    options.startDate = {2024, 12, 30};
    options.dayCount = 4;
    rcal::RCalHeaderFramework header(options);
    const auto& days = header.timeline();
    assert(days.size() == 4);
    assert(days[0].date.year == 2024 && days[0].date.month == 12 && days[0].date.day == 30);
    assert(days[0].isoWeek == 1 && days[0].isoWeekday == 1);
    assert(days[2].date.year == 2025 && days[2].date.month == 1 && days[2].date.day == 1);
    assert(days[2].dayOfYear == 1 && days[2].isoWeek == 1 && days[2].isoWeekday == 3);
    options.startDate = {2024, 2, 28};
    options.dayCount = 3;
    header.setOptions(options);
    assert(header.timeline()[1].date.day == 29);
    assert(header.timeline()[2].date.month == 3 && header.timeline()[2].date.day == 1);
    header.moveSelection(2);
    assert(header.selectedDate().month == 3 && header.selectedDate().day == 1);
    header.moveSelection(-2);
    assert(header.selectedDate().month == 2 && header.selectedDate().day == 28);

    rcal::HeaderOptions holidayOptions;
    holidayOptions.startDate = {2024, 3, 28};
    holidayOptions.dayCount = 55;
    rcal::RCalHeaderFramework holidays(holidayOptions);
    assert(holidays.timeline()[0].holidayName == "Skjærtorsdag");
    assert(holidays.timeline()[1].holidayName == "Langfredag");
    assert(holidays.timeline()[3].holidayName == "Første påskedag");
    assert(holidays.timeline()[4].holidayName == "Andre påskedag");
    assert(holidays.timeline()[34].holidayName == "Arbeidernes dag");
    assert(holidays.timeline()[42].holidayName == "Kristi himmelfartsdag");
    assert(holidays.timeline()[50].holidayName == "Grunnlovsdagen");
    assert(holidays.timeline()[52].holidayName == "Første pinsedag");
    assert(holidays.timeline()[53].holidayName == "Andre pinsedag");
    assert(holidays.selectedHolidayName() == "Skjærtorsdag");
    holidays.moveSelection(1);
    assert(holidays.selectedHolidayName() == "Langfredag");
    holidays.moveSelection(1);
    assert(holidays.selectedHolidayName().empty());
    holidayOptions.holidayCountry = "none";
    holidays.setOptions(holidayOptions);
    assert(holidays.timeline()[0].holidayName.empty());

    rcal::DayInfo mayDayInfo;
    mayDayInfo.workStatus = rcal::WorkStatus::workday;
    mayDayInfo.hasNote = true;
    mayDayInfo.taskCount = 3;
    mayDayInfo.eventCount = 1;
    holidays.setDayInfo({2024, 5, 1}, mayDayInfo);
    holidayOptions.holidayCountry = "NO";
    holidays.setOptions(holidayOptions);
    assert(holidays.infoIndicator({2024, 5, 1}) == "WHN3E");
    assert(holidays.dayInfo({2024, 5, 1}) != nullptr);
    holidays.rebuildTimeline();
    assert(holidays.infoIndicator({2024, 5, 1}) == "WHN3E");
    mayDayInfo.taskCount = 12;
    holidays.setDayInfo({2024, 5, 2}, mayDayInfo);
    assert(holidays.infoIndicator({2024, 5, 2}) == "W N+E");
    assert(holidays.removeDayInfo({2024, 5, 2}));
    assert(holidays.dayInfo({2024, 5, 2}) == nullptr);

    const auto configured = rcal::RCalHeaderFramework::fromConfigFile(
        std::string(RCAL_SOURCE_DIR) + "/config/rcal.conf.example");
    assert(configured.options().bufferDays == 90);
    assert(configured.options().rebuildThresholdDays == 30);
    assert(configured.colors().weekend.foreground == "red");
    assert(configured.options().holidayCountry == "NO");
    assert(configured.options().holidayInfoPrefix == "Helligdag");
    assert(configured.options().dayOfYearLabel == "DOY");
    assert(configured.options().detailsLabel == "Detail");
    assert(configured.options().cellWidth == 5);
    assert(configured.colors().dayOfYear.foreground == "cyan");
    assert(configured.options().dayNames[5] == "Lør");
    assert(configured.options().dayNames[6] == "Søn");
    assert(configured.colors().holiday.bold);
}
