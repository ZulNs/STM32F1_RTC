/*
 * Clock and Calendar on LCD 2004
 * 
 * Description:
 * Displays clock and calendar on LCD 2004.
 * Displayed date can be selected between Christian (Gregorian) calendar or Islamic Hijri calendar.
 * Displayed time can be selected between 12 hour or 24 hour clock.
 * Likewise date and time parameters for calendar and alarm can be adjusted.
 * All can be done via Serial Monitor.
 * Single character to send as command identifier and ends with a new line character.
 * Command identifiers consist of:
 * - 'I', toggle between Hijri or Gregorian date.
 * - 'C', toggle between 12 hour or 24 hour clock.
 * - 'A', toggle between calendar or alarm entry.
 * - 'E', displays Unix Epoch number
 * To change date and time parameters, send a single character as parameter identifier
 * followed by a numeric string and ends with a new line character.
 * Parameter identifiers consist of:
 * - 'D' as day parameter
 * - 'N' as month parameter
 * - 'Y' as year parameter
 * - 'H' as hour parameter
 * - 'M' as minute parameter
 * - 'S' as second parameter
 * All stages are maintained even if the power supply off.
 * 
 * The circuit:
 * Take a look at "circuit_bb.png", "circuit_schem.png", or "circuit.fzz" (open with Fritzing) files.
 * 
 * Created 5 September 2019
 * by ZulNs
 * @Gorontalo, Indonesia
 * 
 * This example code is in the public domain.
 * 
 * https://github.com/zulns/STM32F1_RTC
 */

#include <stm32f1_rtc.h>
#include <LCD_HD44780.h>

#define LCD_RS PB12
#define LCD_RW PB13
#define LCD_EN PB14
#define LCD_D4 PB15
#define LCD_D5 PA8
#define LCD_D6 PA9
#define LCD_D7 PA10

#define ALARM_ACTIVE_CYCLE 30

#define FLAGS_REGISTER  1
/* Bit 0 used for internal RTC init flag */
#define HIJRI_MODE_BIT  1
#define HOUR_12_BIT     2
#define ALARM_ENTRY_BIT 3

#define HIJRI_MODE_FLAG  (1 << HIJRI_MODE_BIT)
#define HOUR_12_FLAG     (1 << HOUR_12_BIT)
#define ALARM_ENTRY_FLAG (1 << ALARM_ENTRY_BIT)

#define IS_HIJRI_MODE(x)      ((x.getBackupRegister(FLAGS_REGISTER) & HIJRI_MODE_FLAG)  == HIJRI_MODE_FLAG)
#define IS_HOUR_12(x)         ((x.getBackupRegister(FLAGS_REGISTER) & HOUR_12_FLAG)     == HOUR_12_FLAG)
#define IS_ALARM_ENTRY(x)     ((x.getBackupRegister(FLAGS_REGISTER) & ALARM_ENTRY_FLAG) == ALARM_ENTRY_FLAG)

/*
#define SET_HIJRI_MODE(x)     (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) | HIJRI_MODE_FLAG))
#define SET_HOUR_12(x)        (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) | HOUR_12_FLAG))
#define SET_ALARM_ENTRY(x)    (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) | ALARM_ENTRY_FLAG))

#define CLEAR_HIJRI_MODE(x)   (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) & ~HIJRI_MODE_FLAG))
#define CLEAR_HOUR_12(x)      (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) & ~HOUR_12_FLAG))
#define CLEAR_ALARM_ENTRY(x)  (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) & ~ALARM_ENTRY_FLAG))
*/

#define TOGGLE_HIJRI_MODE(x)  (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) ^ HIJRI_MODE_FLAG))
#define TOGGLE_HOUR_12(x)     (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) ^ HOUR_12_FLAG))
#define TOGGLE_ALARM_ENTRY(x) (x.setBackupRegister(FLAGS_REGISTER, x.getBackupRegister(FLAGS_REGISTER) ^ ALARM_ENTRY_FLAG))

const char * weekdayName[] = { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
const char * arabicWeekdayName[] = { "Ahad", "Ithnayn", "Thulatha", "Arba'a", "Khamis", "Jumu'ah", "Sabt" };
const char * monthName[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };
const char * hijriMonthName[] = { "Muharram", "Safar", "Rabi.I", "Rabi.II", "Jumada.I", "Jumada.II", "Rajab", "Sha'ban", "Ramadan", "Shawwal", "Dhul-Qa'dah", "Dhul-Hijjah" };

char     buffer[12];
char     comType = '-';
uint8_t  alarmActiveCtr;
uint8_t  bufferPtr = 0;
uint32_t epochTime;
bool     isHijriMode, isHour12, isAlarmEntry, isAlarmActive = false;

DateVar date;
TimeVar time;

STM32F1_RTC rtc;
LCD_HD44780 lcd(LCD_RS, LCD_RW, LCD_EN, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

void setup() {
  Serial.begin(115200);
  rtc.begin();
  lcd.begin(20, 4);
  lcd.clear();
  isHijriMode = IS_HIJRI_MODE(rtc);
  isHour12 = IS_HOUR_12(rtc);
  isAlarmEntry = IS_ALARM_ENTRY(rtc);
  epochTime = rtc.getTime();
  if (isHijriMode)
    rtc.epochToHijriDate(epochTime, date);
  else
    rtc.epochToDate(epochTime, date);
  rtc.epochToTime(epochTime, time);
  lcdPrintDate();
  lcdPrintTime();
}

void loop() {
  char chr;
  int16_t ival;
  if (rtc.isAlarmTriggered()) {
    rtc.clearAlarmFlag();
    isAlarmActive = true;
    alarmActiveCtr = 0;
  }
  if (rtc.isCounterUpdated()) {
    rtc.clearSecondFlag();
    epochTime = rtc.getTime();
    rtc.epochToTime(epochTime, time);
    if (isAlarmActive) {
      if (alarmActiveCtr == 0) {
        lcd.setCursor(20);
        lcd.clearLine();
        lcd.print("Alarm activated");
        Serial.println("Alarm activated\n");
      }
      else if (alarmActiveCtr == ALARM_ACTIVE_CYCLE) {
        lcd.setCursor(26);
        lcd.insert();
        lcd.insert();
        lcd.print("de");
        Serial.println("Alarm deactivated\n");
      }
      else if (alarmActiveCtr > ALARM_ACTIVE_CYCLE) {
        isAlarmActive = false;
        lcd.setCursor(20);
        lcd.clearLine();
      }
      alarmActiveCtr++;
    }
    lcd.setCursor(6);
    lcdPrint2Digits(time.seconds);
    if (time.seconds == 0) {
      lcd.setCursor(3);
      lcdPrint2Digits(time.minutes);
      if (time.minutes == 0) {
        lcd.setCursor(0);
        lcdPrint2Digits(getDisplayHour(time.hours));
        if (time.hours == 0) {
          if (isHijriMode)
            rtc.epochToHijriDate(epochTime, date);
          else
            rtc.epochToDate(epochTime, date);
          lcdPrintDate();
        }
      }
    }
  }
  while (Serial.available())
  {
    chr = Serial.read();
    if (comType == '-') {
      if ('a' <= chr && chr <= 'z')
        chr -= 32; // Make uppercase
      switch (chr) {
      case 'Y': case 'N': case 'D': case 'H': case 'M': case 'S': case 'T':
        comType = chr;
        break;
      case 'A':
        isAlarmEntry = !isAlarmEntry;
        TOGGLE_ALARM_ENTRY(rtc);
        Serial.print("Settings available for adjust ");
        Serial.print(isAlarmEntry ? "alarm" : "calendar");
        Serial.println(" date & time\n");
        break;
      case 'I':
        isHijriMode = !isHijriMode;
        TOGGLE_HIJRI_MODE(rtc);
        Serial.print(isHijriMode ? "Hijri" : "Gregorian");
        Serial.println(" Calendar Mode\n");
        epochTime = rtc.getTime();
        if (isHijriMode)
          rtc.epochToHijriDate(epochTime, date);
        else
          rtc.epochToDate(epochTime, date);
        lcdPrintDate();
        break;
      case 'C':
        isHour12 = !isHour12;
        TOGGLE_HOUR_12(rtc);
        Serial.print("Hour ");
        Serial.print(isHour12 ? "12" : "24");
        Serial.println(" Mode\n");
        lcdPrintTime();
        break;
      case 'E':
        epochTime = rtc.getTime();
        rtc.epochToTime(epochTime, time);
        if (isHijriMode)
          rtc.epochToHijriDate(epochTime, date);
        else
          rtc.epochToDate(epochTime, date);
        printDate();
        printTime(true);
        Serial.print("Unix Epoch: ");
        Serial.println(epochTime);
        Serial.println();
      }
    }
    else {
      if (chr == '\n') {
        buffer[bufferPtr] = 0;
        if (comType == 'T')
          epochTime = atoi(buffer);
        else {
          epochTime = rtc.getTime();
          rtc.epochToTime(epochTime, time);
          if (isHijriMode)
            rtc.epochToHijriDate(epochTime, date);
          else
            rtc.epochToDate(epochTime, date);
          ival = atoi(buffer);
          switch (comType) {
          case 'Y':
            date.year = ival;
            break;
          case 'N':
            date.month = ival;
            break;
          case 'D':
            date.day = ival;
            break;
          case 'H':
            time.hours = ival;
            break;
          case 'M':
            time.minutes = ival;
            break;
          case 'S':
            time.seconds = ival;
          }
          if (isHijriMode)
            epochTime = rtc.hijriDateTimeToEpoch(date, time);
          else
            epochTime = rtc.dateTimeToEpoch(date, time);
        }
        if (isAlarmEntry)
          rtc.setAlarmTime(epochTime);
        else
          rtc.setTime(epochTime);
        Serial.print("New ");
        Serial.print(isAlarmEntry ? "alarm" : "calendar");
        Serial.println(" date & time value:");
        printDate();
        printTime();
        Serial.println();
        if (!isAlarmEntry) {
          lcdPrintTime();
          lcdPrintDate();
        }
        bufferPtr = 0;
        comType = '-';
      }
      else {
        buffer[bufferPtr] = chr;
        bufferPtr++;
      }
    }
  }
}

void lcdPrintDate() {
  lcd.setCursor(40);
  lcd.clearLine();
  lcd.print(isHijriMode ? arabicWeekdayName[date.weekday] : weekdayName[date.weekday]);
  lcd.println(",");
  lcd.clearLine();
  lcd.print(date.day);
  lcd.print(" ");
  lcd.print(isHijriMode ? hijriMonthName[date.month - 1] : monthName[date.month - 1]);
  lcd.print(" ");
  lcd.print(date.year);
  if (isHijriMode)
    lcd.print("H");
}

void lcdPrintTime() {
  lcd.setCursor(0);
  lcd.clearLine();
  lcdPrint2Digits(getDisplayHour(time.hours));
  lcd.print(":");
  lcdPrint2Digits(time.minutes);
  lcd.print(":");
  lcdPrint2Digits(time.seconds);
  if (isHour12)
    lcd.print(time.hours < 12 ? " AM" : " PM");
}

void lcdPrint2Digits(uint8_t n) {
  if (n < 10)
    lcd.print(0);
  lcd.print(n);
}

void printDate() {
  Serial.print(isHijriMode ? arabicWeekdayName[date.weekday] : weekdayName[date.weekday]);
  Serial.print(", ");
  Serial.print(date.day);
  Serial.print(" ");
  Serial.print(isHijriMode ? hijriMonthName[date.month - 1] : monthName[date.month - 1]);
  Serial.print(" ");
  Serial.print(date.year);
  Serial.println(isHijriMode ? " H" : "");
}

void printTime() {
  printTime(false);
}

void printTime(bool ms) {
  print2Digits(getDisplayHour(time.hours));
  Serial.print(":");
  print2Digits(time.minutes);
  Serial.print(":");
  print2Digits(time.seconds);
  if (ms) {
    Serial.print(".");
    print3Digits(rtc.getMilliseconds());
  }
  if (isHour12)
    Serial.print(time.hours < 12 ? " AM" : " PM");
  Serial.println();
}

void print2Digits(uint8_t n) {
  if (n < 10)
    Serial.print(0);
  Serial.print(n);
}

void print3Digits(uint16_t n) {
  if (n < 100) {
    Serial.print(0);
    print2Digits(n);
  }
  else
    Serial.print(n);
}

uint8_t getDisplayHour(uint8_t h) {
  if (isHour12) {
    h %= 12;
    if (h == 0)
      h = 12;
  }
  return h;
}
