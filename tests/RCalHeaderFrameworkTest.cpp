#include "RCalHeaderFramework.h"
#include "ColorSelector.h"
#include "CreateItemSelector.h"
#include "NoteEditor.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iterator>

#include <curses.h>

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
    mayDayInfo.notes.push_back({"", "Planlegg møte"});
    mayDayInfo.taskCount = 3;
    mayDayInfo.eventCount = 1;
    holidays.setDayInfo({2024, 5, 1}, mayDayInfo);
    holidayOptions.holidayCountry = "NO";
    holidays.setOptions(holidayOptions);
    assert(holidays.infoIndicator({2024, 5, 1}) == "WHN3E");
    assert(holidays.infoDescription({2024, 5, 1}) ==
           "W Workday | H Arbeidernes dag | N Planlegg møte | 3 tasks | 1 event");
    assert(holidays.dayInfo({2024, 5, 1}) != nullptr);
    holidays.rebuildTimeline();
    assert(holidays.infoIndicator({2024, 5, 1}) == "WHN3E");
    mayDayInfo.taskCount = 12;
    holidays.setDayInfo({2024, 5, 2}, mayDayInfo);
    assert(holidays.infoIndicator({2024, 5, 2}) == "W N+E");
    assert(holidays.infoDescription({2024, 5, 2}) ==
           "W Workday | N Planlegg møte | 12 tasks | 1 event");
    assert(holidays.removeDayInfo({2024, 5, 2}));
    assert(holidays.dayInfo({2024, 5, 2}) == nullptr);

    holidays.selectDate({2024, 5, 3});
    holidays.toggleSelectedWorkStatus(rcal::WorkStatus::vacation);
    assert(holidays.infoIndicator({2024, 5, 3})[0] == 'F');
    assert(holidays.infoDescription({2024, 5, 3}) == "F Vacation");
    holidays.toggleSelectedWorkStatus(rcal::WorkStatus::vacation);
    assert(holidays.infoIndicator({2024, 5, 3})[0] == ' ');
    holidays.toggleSelectedWorkStatus(rcal::WorkStatus::travelling);
    holidays.clearSelectedWorkStatus();
    assert(holidays.dayInfo({2024, 5, 3}) == nullptr);

    auto* inputWindow = reinterpret_cast<WINDOW*>(1);
    assert(holidays.handleInput('w', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.infoIndicator(holidays.selectedDate())[0] == 'W');
    assert(holidays.handleInput('w', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.infoIndicator(holidays.selectedDate())[0] == ' ');
    assert(holidays.handleInput('t', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.infoIndicator(holidays.selectedDate())[0] == 'T');
    assert(holidays.handleInput(' ', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.infoIndicator(holidays.selectedDate())[0] == ' ');
    assert(holidays.handleInput('o', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.infoIndicator(holidays.selectedDate())[0] == 'O');
    assert(holidays.handleInput('a', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.infoIndicator(holidays.selectedDate())[0] == 'A');
    assert(holidays.handleInput('f', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.infoIndicator(holidays.selectedDate())[0] == 'F');
    holidays.clearSelectedWorkStatus();
    assert(holidays.handleInput('W', inputWindow) == rcal::InputResult::ignored);

    holidays.addNote({2024, 5, 3}, "Husk dette");
    holidays.addNote({2024, 5, 3}, "Kjøp melk");
    assert(holidays.infoIndicator({2024, 5, 3})[2] == 'N');
    assert(holidays.infoDescription({2024, 5, 3}) == "N 2 notes");
    assert(holidays.selectedNoteIndex() == 1);
    assert(*holidays.selectedNote() == "Kjøp melk");
    holidays.moveNoteSelection(-1);
    assert(*holidays.selectedNote() == "Husk dette");
    holidays.updateSelectedNote("Endret notat");
    assert(*holidays.selectedNote() == "Endret notat");
    holidays.updateSelectedNote("");
    assert(*holidays.selectedNote() == "Kjøp melk");
    holidays.updateSelectedNote("");
    assert(holidays.dayInfo({2024, 5, 3}) == nullptr);

    holidays.addTask({2024, 5, 3}, "Send rapport");
    assert(holidays.infoIndicator({2024, 5, 3})[3] == '1');
    assert(holidays.selectedItemType() == rcal::DayItemType::task);
    assert(holidays.selectedItemText() == "Send rapport");
    assert(holidays.handleInput('x', inputWindow) == rcal::InputResult::redraw);
    assert(holidays.dayInfo({2024, 5, 3})->tasks[0].completed);
    holidays.addEvent({2024, 5, 3}, "09:30 Planleggingsmøte");
    assert(holidays.infoIndicator({2024, 5, 3})[4] == 'E');
    assert(holidays.selectedItemType() == rcal::DayItemType::event);
    assert(holidays.selectedItemText() == "09:30 Planleggingsmøte");
    holidays.updateSelectedItem("10:00 Endret møte");
    assert(holidays.dayInfo({2024, 5, 3})->events[0].time == "10:00");
    holidays.moveNoteSelection(-1);
    assert(holidays.selectedItemType() == rcal::DayItemType::task);
    holidays.updateSelectedItem("");
    holidays.moveNoteSelection(1);
    holidays.updateSelectedItem("");
    assert(holidays.dayInfo({2024, 5, 3}) == nullptr);

    rcal::NoteEditor noteEditor("Hei");
    noteEditor.handleInput(L' ', false);
    noteEditor.handleInput(L'ø', false);
    assert(noteEditor.note() == "Hei ø");
    noteEditor.handleInput(KEY_LEFT, true);
    noteEditor.handleInput(127, false);
    assert(noteEditor.note() == "Heiø");
    assert(noteEditor.handleInput(L'\n', false) == rcal::NoteEditorResult::save);
    assert(rcal::NoteEditor("uendret").handleInput(27, false) ==
           rcal::NoteEditorResult::cancel);

    rcal::CreateItemSelector createSelector;
    assert(createSelector.selectedItem() == 0);
    createSelector.handleInput(KEY_DOWN);
    assert(createSelector.selectedItem() == 1);
    assert(createSelector.handleInput(KEY_ENTER) == rcal::CreateItemResult::createTask);
    createSelector.handleInput(KEY_DOWN);
    assert(createSelector.handleInput(KEY_ENTER) == rcal::CreateItemResult::createEvent);
    createSelector.handleInput('n');
    assert(createSelector.handleInput(KEY_ENTER) == rcal::CreateItemResult::createNote);
    assert(rcal::CreateItemSelector{}.handleInput(27) == rcal::CreateItemResult::cancel);

    const auto configured = rcal::RCalHeaderFramework::fromConfigFile(
        std::string(RCAL_SOURCE_DIR) + "/config/rcal.conf.example");
    assert(configured.options().bufferDays == 90);
    assert(configured.options().dataFile.empty());
    assert(configured.options().rebuildThresholdDays == 30);
    assert(configured.colors().weekend.foreground == "red");
    assert(configured.colors().currentDay.background == "yellow");
    assert(configured.colors().selectedDay.background == "blue");
    assert(configured.colors().statusBar.background == "white");
    assert(configured.options().holidayCountry == "NO");
    assert(configured.options().holidayInfoPrefix == "Helligdag");
    assert(configured.options().dayOfYearLabel == "DOY");
    assert(configured.options().detailsLabel == "Detail");
    assert(configured.options().cellWidth == 5);
    assert(configured.colors().dayOfYear.foreground == "cyan");
    assert(configured.options().dayNames[5] == "Lør");
    assert(configured.options().dayNames[6] == "Søn");
    assert(configured.colors().holiday.bold);

    rcal::ColorSelector selector(configured.colors(), 16);
    assert(selector.palette().size() == 17);
    assert(selector.palette()[1] == "black");
    selector.handleInput(KEY_DOWN);
    selector.handleInput(KEY_RIGHT);
    selector.handleInput('u');
    assert(selector.selectedRow() == 1);
    assert(selector.colors().label.background == "black");
    assert(selector.colors().label.underline);

    const std::string savedConfig = std::string(RCAL_SOURCE_DIR) + "/build/color-selector-test.conf";
    {
        std::ofstream output(savedConfig);
        output << "# retained comment\nstart_date=2024-01-01\ncolor.label=white,default\n";
    }
    rcal::saveColorsToConfig(savedConfig, selector.colors());
    std::ifstream savedInput(savedConfig);
    const std::string savedText((std::istreambuf_iterator<char>(savedInput)),
                                std::istreambuf_iterator<char>());
    assert(savedText.find("# retained comment") != std::string::npos);
    assert(savedText.find("color.label=" + configured.colors().label.foreground +
                          ",black,bold,underline") != std::string::npos);
    assert(savedText.find("color.grid=") != std::string::npos);
    assert(savedText.find("color.status_bar=") != std::string::npos);
    std::remove(savedConfig.c_str());
}
