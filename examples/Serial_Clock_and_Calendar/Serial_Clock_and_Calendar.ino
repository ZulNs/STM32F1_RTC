/*
 * Clock and Calendar on Serial Monitor
 * 
 * Description:
 * Displays clock and calendar on Serial Monitor.
 * Displayed date can be selected between Christian (Gregorian) calendar or Islamic Hijri calendar.
 * Displayed time can be selected between 12 hour or 24 hour clock.
 * Likewise date and time parameters for calendar and alarm can be adjusted.
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
const char * hijriMonthName[] = { "Muharram", "Safar", "Rabi'al-Ula", "Rabi'ath-Thani", "Jumadal-Ula", "Jumadath-Thani",
                                  "Rajab", "Sha'ban", "Ramadan", "Shawwal", "Dhul-Qa'dah", "Dhul-Hijjah" };

char     buffer[12];
char     comType       = '-';
uint8_t  alarmActiveCtr;
uint8_t  bufferPtr = 0;
uint32_t epochTime;
bool     isHijriMode, isHour12, isAlarmEntry, isAlarmActive = false;

DateVar date;
TimeVar time;

STM32F1_RTC rtc;

void setup() {
  Serial.begin(115200);
  rtc.begin();
  isHijriMode = IS_HIJRI_MODE(rtc);
  isHour12 = IS_HOUR_12(rtc);
  isAlarmEntry = IS_ALARM_ENTRY(rtc);
  epochTime = rtc.getTime();
  if (isHijriMode)
    rtc.epochToHijriDate(epochTime, date);
  else
    rtc.epochToDate(epochTime, date);
  rtc.epochToTime(epochTime, time);
}

void loop() {
  char chr;
  int16_t ival;
  uint8_t len;
  if (rtc.isAlarmTriggered()) {
    rtc.clearAlarmFlag();
    isAlarmActive = true;
    alarmActiveCtr = 0;
  }
  if (rtc.isCounterUpdated()) {
    rtc.clearSecondFlag();
    epochTime = rtc.getTime();
    rtc.epochToTime(epochTime, time);
    printTime();
    if (isAlarmActive) {
      Serial.print("Alarm ");
      if (alarmActiveCtr >= ALARM_ACTIVE_CYCLE) {
        isAlarmActive = false;
        Serial.println("deactivated");
      }
      else if (alarmActiveCtr > 0)
        Serial.println("active");
      else
        Serial.println("activated");
      alarmActiveCtr++;
    }
    if (time.seconds % 10 == 0) {
      if (isHijriMode)
        rtc.epochToHijriDate(epochTime, date);
      else
        rtc.epochToDate(epochTime, date);
      printDate();
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
        Serial.print("\nSettings available for adjust ");
        Serial.print(isAlarmEntry ? "alarm" : "calendar");
        Serial.println(" date & time\n");
        break;
      case 'I':
        isHijriMode = !isHijriMode;
        TOGGLE_HIJRI_MODE(rtc);
        Serial.print(isHijriMode ? "\nHijri" : "\nGregorian");
        Serial.println(" Calendar Mode\n");
        break;
      case 'C':
        isHour12 = !isHour12;
        TOGGLE_HOUR_12(rtc);
        Serial.print("\nHour ");
        Serial.print(isHour12 ? "12" : "24");
        Serial.println(" Mode\n");
        break;
      case 'E':
        epochTime = rtc.getTime();
        rtc.epochToTime(epochTime, time);
        if (isHijriMode)
          rtc.epochToHijriDate(epochTime, date);
        else
          rtc.epochToDate(epochTime, date);
        Serial.println();
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
        Serial.print("\nNew ");
        Serial.print(isAlarmEntry ? "alarm" : "calendar");
        Serial.println(" date & time value:");
        printDate();
        printTime();
        Serial.println();
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
  uint8_t h = time.hours;
  if (isHour12) {
    h %= 12;
    if (h == 0)
      h = 12;
  }
  print2Digits(h);
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
