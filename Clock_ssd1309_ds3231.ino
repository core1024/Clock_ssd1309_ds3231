#include <Arduino.h>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#include "uRTCLib.h"

// uRTCLib rtc;
uRTCLib rtc(0x68, 0x57);

U8G2_SSD1309_128X64_NONAME0_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
// U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

// End of constructor list

const uint16_t song[] PROGMEM = {

  // First line
  0, 800, 196, 200, 185, 200, 165, 200, 165, 200,
  185, 800, 0, 800,

  0, 600, 110, 200,
  196, 200, 185, 200, 165, 200, 165, 400,

  185, 600,
  147, 400, 165, 200, 110, 1000,

  0, 600, 110, 200,

  // Second line
  165, 400, 185, 200, 196, 600, 165, 200, 139, 400,
  147, 600, 165, 400, 110, 200, 110, 400,
  185, 600, 0, 800,

  0, 800, 196, 200, 185, 200, 165, 200, 165, 200,
  185, 800, 0, 800,

  // Third line
  0, 400, 0, 200, 110, 200, 196, 200, 185, 200, 165, 200, 165, 600,
  185, 200, 147, 600, 165, 200, 110, 1000,
  0, 800,

  165, 400, 185, 200, 196, 600, 165, 200, 139, 600,
  147, 200, 165, 400, 110, 200, 147, 200, 165, 200,

  // Fourth line
  175, 200, 165, 200, 147, 200, 131, 200, 0, 400, 110, 200, 117, 200,
  131, 400, 175, 400, 165, 200, 147, 200, 147, 200, 131, 200,
  147, 200, 131, 200, 131, 400, 131, 400, 110, 200, 117, 200,
  131, 400, 175, 400, 196, 200, 175, 200, 165, 200, 147, 200,
  147, 200, 165, 200, 175, 400, 175, 400, 196, 200, 220, 200,

  // Fifth line
  233, 200, 233, 200, 220, 400, 196, 400, 175, 200, 196, 200,
  220, 200, 220, 200, 196, 400, 175, 400, 147, 200, 131, 200,
  147, 200, 175, 200, 175, 200, 165, 400, 165, 200, 185, 200, 185, 600,



  // and belive me…
  0, 800, 220, 200, 220, 200,
  247, 200, 220, 200, 185, 200, 147, 400, 165, 200, 185, 200, 185, 600,
  0, 600, 220, 200, 220, 200, 220, 200,


  // science and im still…
  247, 200, 220, 200, 185, 200, 147, 400, 165, 200, 185, 200, 185, 600,

  0, 600, 220, 200, 220, 200, 220, 200,
  247, 200, 220, 200, 185, 200, 147, 400, 165, 200, 185, 200, 185, 600,

  0, 800, 220, 200, 220, 200,
  247, 200, 220, 200, 185, 200, 147, 400, 165, 200, 185, 200, 185, 600,

  0, 600, 220, 200, 220, 200, 220, 200,
  247, 200, 220, 200, 185, 200, 147, 400, 165, 200, 185, 200, 185, 600,

  0, 600, 220, 200, 220, 200, 220, 200,
  247, 200, 220, 200, 185, 200, 147, 400, 165, 200, 185, 200, 185, 600,

  // still alive… still alive…
  0, 600, 196, 200, 220, 200, 220, 600,
  0, 600, 196, 200, 185, 200, 185, 600,

  // End of song
  0, 2000,
  0, 0
};


// Buttons section
//////////////////

#define BTN_UP 128
#define BTN_SET 64
#define BTN_DOWN 32

namespace Buttons {
  uint8_t state;
  uint8_t changed;

  void update(void) {
    changed = state ^ (PIND & (BTN_UP | BTN_DOWN | BTN_SET));
    state = PIND & (BTN_UP | BTN_DOWN | BTN_SET);
    if (changed && state ^ (BTN_UP | BTN_DOWN | BTN_SET)) {
      tone(4, 2000, 50);
      delay(50);
    }
  }

  bool pressed(const uint8_t button) {
    return !(state & button);
  }

  bool justPressed(const uint8_t button) {
    return pressed(button) && (changed & button);
  }
}

// Alarm functions

struct alarm_t {
  uint8_t hour;
  uint8_t minute;
  uint8_t days;
};

struct alarm_t alarm;

uint32_t next_alarm;

uint8_t monthLength(uint8_t month, uint8_t year) {
  if (month == 4 || month == 6 || month == 9 || month == 11)
    return 30;
  if (month == 2)
  {
    // Leap Year
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
      return 29;
    else
      return 28;
  }
  return 31;
}

// 6  5  5  4  7
// 63 31 31 15 127
// 59 23 31 12 99

uint32_t dateToAlarm(uint8_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute) {
  return (uint32_t)((uint32_t)minute | ((uint32_t)hour << 6) | (((uint32_t)day - 1) << 11) | (uint32_t)month << 16 | ((uint32_t)year << 20));
}

uint8_t alarmMinute(uint32_t alarm_time) {
  return alarm_time & 63;
}

uint8_t alarmHour(uint32_t alarm_time) {
  return (alarm_time >> 6) & 31;
}

uint8_t alarmDay(uint32_t alarm_time) {
  return ((alarm_time >> 11) & 31) + 1;
}

uint8_t alarmMonth(uint32_t alarm_time) {
  return (alarm_time >> 16) & 15;
}

uint8_t alarmYear(uint32_t alarm_time) {
  return (alarm_time >> 20) & 127;
}

uint32_t nowToAlarm(void) {
  return dateToAlarm(rtc.year(), rtc.month(), rtc.day(), rtc.hour(), rtc.minute());
}

uint32_t alarmNext(struct alarm_t alarm) {
  // FIXME: ~0 is not the best solution here
  if(! alarm.days) return ~0; // Alarm is off
  rtc.refresh();
  uint8_t dow = (rtc.dayOfWeek() + 5) % 7;
  uint8_t year = rtc.year();
  uint8_t month = rtc.month();
  uint8_t day = rtc.day();
  uint8_t mLen = monthLength(month, 2000 + year);
  uint32_t atNow = nowToAlarm();
  uint32_t atAlarm = dateToAlarm(year, month, day, alarm.hour, alarm.minute);
  while((alarm.days & (1 << dow)) == 0 || atNow >= atAlarm) {
    dow = (dow + 1) % 7;
    day++;
    if(day > mLen) {
      day = 1;
      month++;
      if(month > 12) {
        month = 1;
        year++;
      }
    }
    atAlarm = dateToAlarm(year, month, day, alarm.hour, alarm.minute);
  }
  return atAlarm;
}

// FIXME: If I decide to support more alarms, this is a big no-no
void storeAlarm(void) {
  rtc.eeprom_write(0, next_alarm);
  rtc.eeprom_write(4, (byte *) &alarm, sizeof(alarm_t));
}

// Helper functions with side effects
void padWithSingleChar(uint8_t number, char pad) {
  if(number < 10) {
    u8g2.print(pad);
  }
  u8g2.print(number);
}

void padWithSingleZero(uint8_t number) {
  if(number < 10) {
    u8g2.print('0');
  }
  u8g2.print(number);
}

void txtGoTo(uint8_t x, uint8_t y) {
  u8g2.setCursor(x * u8g2.getMaxCharWidth(), y * u8g2.getMaxCharHeight());
}

void txtNextRow(uint8_t x) {
  u8g2.setCursor(x, u8g2.ty + u8g2.getMaxCharHeight());
}


// Printing functions

void displayTime(uint8_t hour, uint8_t minute, uint8_t seconds) {
  padWithSingleZero(hour);

  u8g2.print(':');
  padWithSingleZero(minute);

  if (seconds < 60) {
    u8g2.print(':');
    padWithSingleZero(seconds);
  }
}

void displayDate(uint8_t day, uint8_t month, uint8_t year) {
  padWithSingleZero(day);

  u8g2.print('.');
  padWithSingleZero(month);

  u8g2.print(".20");
  padWithSingleZero(year);
}

void displayISODate(uint8_t day, uint8_t month, uint8_t year) {
  u8g2.print("20");
  padWithSingleZero(year);

  u8g2.print("-");
  padWithSingleZero(month);

  u8g2.print('-');
  padWithSingleZero(day);
}

void displayWeekDays(void) {
  u8g2.print(F("Mo Tu We Th Fr Sa Su"));
}

// info state functions

void infoAll(void) {
  static uint8_t slide = 0, dir = 1, last_sec;
  uint8_t today = rtc.day();
  uint8_t month = rtc.month();
  uint8_t year = rtc.year();
  uint8_t mLen = monthLength(month, 2000 + year);
  uint8_t mStart = (7 + (rtc.dayOfWeek() + 6) % 7 - (today) % 7) % 7;
  int mDay;

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.firstPage();
  do {
    u8g2.setCursor(slide, u8g2.getMaxCharHeight());
    displayTime(rtc.hour(), rtc.minute(), 60);
    u8g2.print(F("     "));
    displayDate(today, month, year);
    txtNextRow(slide);
    u8g2.print(rtc.temp());
    u8g2.print(F(" \260C"));
    txtNextRow(slide);

    displayWeekDays();
    txtNextRow(slide);

    for (int i = 1; i < 43; i++) {
      mDay = i - mStart;
      if (mDay < 1 || mDay > mLen) {
        u8g2.print(F("   "));
      } else {
        padWithSingleChar(mDay, ' ');
        u8g2.print(' ');
        if (mDay == today) {
          u8g2.setDrawColor(2);
          u8g2.drawBox(u8g2.tx - 3 * u8g2.getMaxCharWidth(), u8g2.ty - u8g2.getMaxCharHeight() + 1, 2 * u8g2.getMaxCharWidth(), u8g2.getMaxCharHeight());
          u8g2.setDrawColor(1);
        }
      }
      if (0 == i % 7) {
        txtNextRow(slide);
      }
    }
  } while ( u8g2.nextPage() );
  if (last_sec / 10 != rtc.second() / 10) {
    last_sec = rtc.second();
    slide += dir;
    if (slide > 28 || slide < 1) dir = -dir;
  }
}

void infoTimeDate(void) {
  static uint8_t slide = 0, dir = 1, last_sec;
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont29_tn);
    u8g2.setCursor(0, u8g2.getMaxCharHeight() + slide);
    displayTime(rtc.hour(), rtc.minute(), rtc.second());
    u8g2.setCursor(0, 64 - slide);
    u8g2.setFont(u8g2_font_5x7_tf);
    displayDate(rtc.day(), rtc.month(), rtc.year());
    u8g2.print(F("       "));
    u8g2.print(rtc.temp());
    u8g2.print(F(" \260C"));
  } while ( u8g2.nextPage() );

  if (last_sec / 10 != rtc.second() / 10) {
    last_sec = rtc.second();
    slide += dir;
    if (slide > 14 || slide < 1) dir = -dir;
  }
}

void infoAlarm(void) {
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_profont17_tn);
    u8g2.setCursor(0, 0);
    txtNextRow(0);
    uint8_t minutes = (60 + alarmMinute(next_alarm) - rtc.minute() - 1) % 60;
    uint8_t hours = (24 + alarmHour(next_alarm) - rtc.hour() - (rtc.minute() >= alarmMinute(next_alarm))) % 24;
    uint8_t days = (monthLength(rtc.month(), rtc.year()) + alarmDay(next_alarm) - rtc.day() - (rtc.hour() >= alarmHour(next_alarm))) % monthLength(rtc.month(), rtc.year());

    // FIXME: This is hack. Anyway as of now you can't set an alarm for more than 6 days ahead
    if(days > 7) days = 0;

    padWithSingleZero(days);
    u8g2.print(F("  "));
    padWithSingleZero(hours);
    u8g2.print(F("  "));
    padWithSingleZero(minutes);
    u8g2.print(F("  "));
    padWithSingleZero((59 - rtc.second()));

    u8g2.setFont(u8g2_font_5x7_tf);
    txtNextRow(0);
    u8g2.print(F("days   hours  mins    secs"));
    txtNextRow(0);

    txtNextRow(2);

    u8g2.print(F("Now is:  "));
    displayISODate(rtc.day(), rtc.month(), rtc.year());
    u8g2.print(' ');
    displayTime(rtc.hour(), rtc.minute(), 60);
    txtNextRow(2);

    u8g2.print(F("Alarm:   "));
    displayISODate(alarmDay(next_alarm), alarmMonth(next_alarm), alarmYear(next_alarm));
    u8g2.print(' ');
    displayTime(alarmHour(next_alarm), alarmMinute(next_alarm), 60);
  } while ( u8g2.nextPage() );
}

// Settings functions

void setDateAndTimeMenu(void) {
  const uint8_t num_states = 9;
  const uint8_t num_settings = 7;
  uint8_t setState = 8;
  uint8_t adjState = 0;
  uint8_t newTime[] = {
    rtc.hour(),
    rtc.minute(),
    rtc.second(),
    rtc.year(),
    rtc.month(),
    rtc.day(),
    rtc.dayOfWeek()
  };
  uint8_t limitLo[] = {0, 0, 0, 0, 1, 1, 0};
  uint8_t limitHi[] = {24, 60, 60, 100, 12, 31, 7};

  u8g2.setFont(u8g2_font_5x7_tf);
  for (;;) {
    Buttons::update();
    if ((Buttons::pressed(BTN_UP) || Buttons::pressed(BTN_DOWN))) {
      if (adjState && setState < num_settings) {
        newTime[setState] = (limitHi[setState] + newTime[setState] - limitLo[setState] + Buttons::pressed(BTN_UP) - Buttons::pressed(BTN_DOWN)) % limitHi[setState] + limitLo[setState];
      } else {
        setState = (num_states + setState + Buttons::pressed(BTN_UP) - Buttons::pressed(BTN_DOWN)) % num_states;
        limitHi[5] = monthLength(newTime[4], newTime[3]);
        if (newTime[5] > limitHi[5]) {
          newTime[5] = limitHi[5];
        }
      }
    }
    if (Buttons::justPressed(BTN_SET)) {
      if (setState < num_settings) {
        adjState = !adjState;
      } else if (setState == 7) {
        // RTCLib::set(second, minute, hour, dayOfWeek, dayOfMonth, month, year)
        rtc.set(newTime[2], newTime[1], newTime[0], newTime[6], newTime[5], newTime[4], newTime[3]);
        break;
      } else {
        break;
      }
    }
    u8g2.firstPage();
    do {
      txtGoTo(0, 3);
      displayTime(newTime[0], newTime[1], newTime[2]);
      u8g2.print(F("  "));
      displayDate(newTime[5], newTime[4], newTime[3]);

      txtGoTo(0, 5);
      displayWeekDays();
      u8g2.setDrawColor(2);
      u8g2.drawBox(3 * u8g2.getMaxCharWidth() * ((newTime[6] + 5) % 7), 4 * u8g2.getMaxCharHeight() + 1, 2 * u8g2.getMaxCharWidth(), u8g2.getMaxCharHeight());
      u8g2.setDrawColor(1);

      txtGoTo(0, 7);
      u8g2.print(F("  Set    Cancel"));

      if (setState < 3) {
        txtGoTo(setState * 3, 2);
      } else if (setState < 6) {
        u8g2.setCursor((5 - setState) * u8g2.getMaxCharWidth() * 3 + u8g2.getMaxCharWidth() * 10, 2 * u8g2.getMaxCharHeight());
      } else if (setState < 7) {
        txtGoTo(21, 5);
      } else {
        txtGoTo((setState - 7) * 7, 7);
      }

      if (adjState < 2) {
        u8g2.print('*');
      }
    } while ( u8g2.nextPage() );
    if (adjState > 0) {
      adjState = (adjState % 2) + 1;
    }
  }
}

void setDateAlarmMenu(void) {
  const uint8_t num_states = 11;
  const uint8_t num_settings = 2;
  uint8_t setState = num_states - 1;
  uint8_t adjState = 0;
  uint8_t newTime[] = {
    alarm.hour,
    alarm.minute,
    alarm.days
  };
  uint8_t limitLo[] = {0, 0};
  uint8_t limitHi[] = {24, 60};

  u8g2.setFont(u8g2_font_5x7_tf);
  for (;;) {
    Buttons::update();
    if ((Buttons::pressed(BTN_UP) || Buttons::pressed(BTN_DOWN))) {
      if (adjState && setState < num_settings) {
        newTime[setState] = (limitHi[setState] + newTime[setState] - limitLo[setState] + Buttons::pressed(BTN_UP) - Buttons::pressed(BTN_DOWN)) % limitHi[setState] + limitLo[setState];
      } else {
        setState = (num_states + setState + Buttons::pressed(BTN_UP) - Buttons::pressed(BTN_DOWN)) % num_states;
      }
    }
    if (Buttons::justPressed(BTN_SET)) {
      if (setState < num_settings) {
        adjState = !adjState;
      } else if (setState < 9) {
        newTime[2] = newTime[2] ^ (1 << (setState - 2));
      } else if(setState == 9) {
        alarm.hour = newTime[0];
        alarm.minute = newTime[1];
        alarm.days = newTime[2];
        next_alarm = alarmNext(alarm);
        storeAlarm();
        break;
      } else {
        break;
      }
    }
    u8g2.firstPage();
    do {

      txtGoTo(0, 3);
      displayTime(newTime[0], newTime[1], 60);

      txtGoTo(0, 6);
      displayWeekDays();
      u8g2.setDrawColor(2);
      for(int i = 0; i < 7;i++) {
        if(newTime[2] & (1 << i)) {
          u8g2.drawBox(3 * u8g2.getMaxCharWidth() * (i % 7), 5 * u8g2.getMaxCharHeight() + 1, 2 * u8g2.getMaxCharWidth(), u8g2.getMaxCharHeight());
        }
      }
      u8g2.setDrawColor(1);

      txtGoTo(0, 8);
      u8g2.print(F("  Set    Cancel"));

      if (setState < 2) {
        txtGoTo(setState * 3, 2);
      } else if (setState < 9) {
        txtGoTo((setState - 2) * 3, 5);
      } else {
        txtGoTo((setState - 9) * 7, 8);
      }

      if (adjState < 2) {
        u8g2.print('*');
      }
    } while ( u8g2.nextPage() );
    if (adjState > 0) {
      adjState = (adjState % 2) + 1;
    }
  }
}

void triggerAlarm(void) {
  int i = 0;
  unsigned long note_time = 0;
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.setFontMode(1);
  do {
    Buttons::update();
    if(millis() > note_time) {
      const uint16_t note = pgm_read_word(&song[i]);
      const uint16_t duration = pgm_read_word(&song[i + 1]);

      if(! duration) {
        i = 0;
        continue;
      }

      u8g2.firstPage();
      do {
        u8g2.setDrawColor((i / 2) % 2);
        u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
        u8g2.setDrawColor(2);
        txtGoTo(7, 5);
        u8g2.print(F("A L A R M !!!"));
      } while ( u8g2.nextPage() );

      if(note) {
        tone(4, note, duration);
      }

      const uint16_t pauseBetweenNotes = duration * 1.3;
      note_time = millis() + pauseBetweenNotes;
      i += 2;
    }
  } while(!Buttons::changed);
  u8g2.setDrawColor(1);
  next_alarm = alarmNext(alarm);
  storeAlarm();
}

void setup(void) {
  // RTC
  Wire.begin();

  // Display
  u8g2.begin();

  // Buttons
  pinMode(5, INPUT_PULLUP);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  Buttons::update();

  // Buzzer
  pinMode(4, OUTPUT);

  rtc.eeprom_read(0, &next_alarm);
  rtc.eeprom_read(4, (byte *) &alarm, sizeof(alarm_t));
}

void loop(void) {
  const uint8_t num_states = 3;
  static uint8_t upd_sec, state = 0;
  rtc.refresh();
  Buttons::update();

  if (Buttons::changed) {
    state = (state + num_states + Buttons::justPressed(BTN_UP) - Buttons::justPressed(BTN_DOWN)) % num_states;
  }

  if (Buttons::justPressed(BTN_SET)) {
    switch(state) {
      case 0:
        setDateAndTimeMenu();
      break;
      case 2:
        setDateAlarmMenu();
      break;
    }
  }

  if (upd_sec == rtc.second() && !Buttons::changed) return;
  upd_sec = rtc.second();

  if(next_alarm <= nowToAlarm()) {
    triggerAlarm();
    return;
  }

  switch (state) {
    case 0:
      infoTimeDate();
      break;
    case 1:
      infoAll();
      break;
    default:
      infoAlarm();
    break;
  }
}

