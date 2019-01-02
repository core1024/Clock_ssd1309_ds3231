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

//U8G2_SSD1309_128X64_NONAME0_1_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);
U8G2_SSD1309_128X64_NONAME0_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ 10, /* dc=*/ 9, /* reset=*/ 8);

// End of constructor list



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

uint16_t dateToAlarmTime(uint8_t minute, uint8_t hour, uint8_t day) {
	return minute | hour << 6 | day << 11;
}

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

void padWithChar(uint8_t number, uint8_t padding, char pad) {
  if (padding > 1) {
    for (int power = pow(10, padding - 1); power > 9 && power > number; power /= 10) {
      u8g2.print(pad);
    }
  }
  u8g2.print(number);
}

void padWithZero(uint8_t number, uint8_t padding) {
  padWithChar(number, padding, '0');
}

void displayTime(uint8_t hour, uint8_t minute, uint8_t seconds) {
  uint8_t digit;

  digit = hour;
  padWithZero(digit, 2);

  u8g2.print(':');
  digit = minute;
  padWithZero(digit, 2);

  if (seconds < 60) {
    u8g2.print(':');
    digit = seconds;
    padWithZero(digit, 2);
  }
}

void displayDate(uint8_t day, uint8_t month, uint8_t year) {
  uint8_t digit;

  digit = day;
  padWithZero(digit, 2);

  u8g2.print('.');
  digit = month;
  padWithZero(digit, 2);

  u8g2.print(".20");
  digit = year;
  padWithZero(digit, 2);
}

void displayWeekDays(void) {
  u8g2.print(F("Mo Tu We Th Fr Sa Su"));
}

void displayCalendar(uint8_t x, uint8_t y) {

  uint8_t today = rtc.day();
  uint8_t month = rtc.month();
  uint16_t year = 2000 + rtc.year();
  uint8_t mLen = monthLength(month, year);
  uint8_t mStart = (7 + (rtc.dayOfWeek() + 6) % 7 - (today) % 7) % 7;
  int mDay;

  u8g2.setCursor(x, y);
  displayWeekDays();
  u8g2.setCursor(x, u8g2.ty + u8g2.getMaxCharHeight());

  for (int i = 1; i < 43; i++) {
    mDay = i - mStart;
    if (mDay < 1 || mDay > mLen) {
      u8g2.print(F("   "));
    } else {
      padWithChar(mDay, 2, ' ');
      u8g2.print(' ');
      if (mDay == today) {
        u8g2.setDrawColor(2);
        u8g2.drawBox(u8g2.tx - 3 * u8g2.getMaxCharWidth(), u8g2.ty - u8g2.getMaxCharHeight() + 1, 2 * u8g2.getMaxCharWidth(), u8g2.getMaxCharHeight());
        u8g2.setDrawColor(1);
      }
    }
    if (0 == i % 7) {
      u8g2.setCursor(x, u8g2.ty + u8g2.getMaxCharHeight());
    }
  }
}


//#define RPS(str) (const __FlashStringHelper *)str
//
//const char degStr[] PROGMEM = " \260C";

void infoAll(void) {
  static uint8_t slide = 0, dir = 1, last_sec;
  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.setCursor(slide, u8g2.getMaxCharHeight());
  displayTime(rtc.hour(), rtc.minute(), 60);
  u8g2.print(F("     "));
  displayDate(rtc.day(), rtc.month(), rtc.year());
  u8g2.setCursor(slide, u8g2.ty + u8g2.getMaxCharHeight());
  u8g2.print(rtc.temp());
  u8g2.print(F(" \260C"));
  u8g2.setCursor(slide, u8g2.ty + u8g2.getMaxCharHeight());
  displayCalendar(slide, u8g2.ty);
  if (last_sec / 10 != rtc.second() / 10) {
    last_sec = rtc.second();
    slide += dir;
    if (slide > 28 || slide < 1) dir = -dir;
  }
}

void infoTimeDate(void) {
  static uint8_t slide = 0, dir = 1, last_sec;
  u8g2.setFont(u8g2_font_profont29_tn);
  u8g2.setCursor(0, u8g2.getMaxCharHeight() + slide);
  displayTime(rtc.hour(), rtc.minute(), rtc.second());
  u8g2.setCursor(0, 64 - slide);
  u8g2.setFont(u8g2_font_5x7_tf);
  displayDate(rtc.day(), rtc.month(), rtc.year());
  u8g2.print(F("       "));
  u8g2.print(rtc.temp());
  u8g2.print(F(" \260C"));

  if (last_sec / 10 != rtc.second() / 10) {
    last_sec = rtc.second();
    slide += dir;
    if (slide > 14 || slide < 1) dir = -dir;
  }
}

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
        // RTCLib::set(byte second, byte minute, byte hour, byte dayOfWeek, byte dayOfMonth, byte month, byte year)
        rtc.set(newTime[2], newTime[1], newTime[0], newTime[6], newTime[5], newTime[4], newTime[3]);
        break;
      } else {
        break;
      }
    }
    u8g2.clearBuffer();

    u8g2.setCursor(0, 3 * u8g2.getMaxCharHeight());
    displayTime(newTime[0], newTime[1], newTime[2]);
    u8g2.print(F("  "));
    displayDate(newTime[5], newTime[4], newTime[3]);

    u8g2.setCursor(0, 5 * u8g2.getMaxCharHeight());
    displayWeekDays();
    u8g2.setDrawColor(2);
    u8g2.drawBox(3 * u8g2.getMaxCharWidth() * ((newTime[6] + 5) % 7), 4 * u8g2.getMaxCharHeight() + 1, 2 * u8g2.getMaxCharWidth(), u8g2.getMaxCharHeight());
    u8g2.setDrawColor(1);

    u8g2.setCursor(0, 7 * u8g2.getMaxCharHeight());
    u8g2.print(F("  Set    Cancel"));

    if (setState < 3) {
      u8g2.setCursor(setState * u8g2.getMaxCharWidth() * 3, 2 * u8g2.getMaxCharHeight());
    } else if (setState < 6) {
      u8g2.setCursor((5 - setState) * u8g2.getMaxCharWidth() * 3 + u8g2.getMaxCharWidth() * 10, 2 * u8g2.getMaxCharHeight());
    } else if (setState < 7) {
      u8g2.setCursor(21 * u8g2.getMaxCharWidth(), 5 * u8g2.getMaxCharHeight());
    } else {
      u8g2.setCursor((setState - 7) * u8g2.getMaxCharWidth() * 7, 7 * u8g2.getMaxCharHeight());
    }

    if (adjState < 2) {
      u8g2.print('*');
    }
    if (adjState > 0) {
      adjState = (adjState % 2) + 1;
    }

    u8g2.sendBuffer();
    delay(50);
  }
}

void settingsMenu(void) {
  u8g2.setFont(u8g2_font_5x7_tf);

  do {
    Buttons::update();
    u8g2.clearBuffer();
    u8g2.setCursor(0, u8g2.getMaxCharHeight());
    u8g2.print(F("Set date and time"));
    u8g2.setCursor(0, 64);
    u8g2.print(F("Exit"));
    u8g2.sendBuffer();
    delay(100);
    if (Buttons::justPressed(BTN_UP)) {
      setDateAndTimeMenu();
    }
  } while (! Buttons::justPressed(BTN_DOWN));
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

  // Buzzer
  pinMode(4, OUTPUT);
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
    //    settingsMenu();
    setDateAndTimeMenu();
  }

  if (upd_sec == rtc.second() && !Buttons::changed) return;
  upd_sec = rtc.second();
  u8g2.clearBuffer();
  //  u8g2.firstPage();
  //  do {
  switch (state) {
    case 0:
      infoTimeDate();
      break;
    case 1:
      infoAll();
      break;
    default:
      break;
  }
  //  } while ( u8g2.nextPage() );
  u8g2.sendBuffer();
}

