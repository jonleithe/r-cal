#include "RCalHeaderFramework.h"

#include <cassert>

int main() {
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

    const auto configured = rcal::RCalHeaderFramework::fromConfigFile(
        std::string(RCAL_SOURCE_DIR) + "/config/rcal.conf.example");
    assert(configured.options().bufferDays == 90);
    assert(configured.options().rebuildThresholdDays == 30);
    assert(configured.colors().weekend.foreground == "red");
    assert(configured.options().holidayCountry == "NO");
    assert(configured.options().holidayInfoPrefix == "Helligdag");
    assert(configured.options().dayNames[5] == "Lør");
    assert(configured.options().dayNames[6] == "Søn");
    assert(configured.colors().holiday.bold);
    assert(configured.colors().currentDay.foreground == "black");
    assert(configured.colors().currentDay.background == "yellow");
    assert(configured.colors().currentDay.bold);
    assert(configured.colors().selectedDay.background == "blue");
}
