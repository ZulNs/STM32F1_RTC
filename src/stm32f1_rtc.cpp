
#include "stm32f1_rtc.h"

bool STM32F1_RTC::begin() {
  bool isInit = isInitialized();
  enableClockInterface();
  if (isInit)
    waitSync();
  else
    init();
  return isInit;
}

void STM32F1_RTC::init() {
  enableBackupWrites();
  RCC_BDCR |= RCC_BDCR_BDRST; // Resets the entire Backup domain
  RCC_BDCR &= ~RCC_BDCR_BDRST; // Deactivates reset of entire Backup domain
  RCC_BDCR |= RCC_BDCR_LSEON; // Enables external low-speed oscillator (LSE)
  while ((RCC_BDCR & RCC_BDCR_LSERDY) == 0); // Waits for LSE ready
  RCC_BDCR |= RCC_BDCR_RTCSEL_LSE; // Selects LSE as RTC clock
  RCC_BDCR |= RCC_BDCR_RTCEN; // Enables the RTC
  waitSync();
  waitFinished();
  enterConfigMode();
  RTC_PRLL = 0x7FFF;
  exitConfigMode();
  waitFinished();
  BKP_DR[RTC_INIT_REG] |= RTC_INIT_FLAG; // Signals that RTC initilized
  disableBackupWrites();
}

void STM32F1_RTC::attachInterrupt(InterruptMode im, void (*handler)(void)) {
  switch (im) {
  case RTC_SECONDS_INTERRUPT:
    waitFinished();
    RTC_CRH |= RTC_CRH_SECIE;
    NVIC_ISER[0] = bit(3);
    ISR_RTC_SEC = handler;
    break;
  case RTC_ALARM_INTERRUPT:
    waitFinished();
    RTC_CRH |= RTC_CRH_ALRIE;
    NVIC_ISER[0] = bit(3);
    ISR_RTC_ALR = handler;
    break;
  case RTC_OVERFLOW_INTERRUPT:
    waitFinished();
    RTC_CRH |= RTC_CRH_OWIE;
    NVIC_ISER[0] = bit(3);
    ISR_RTC_OW = handler;
  }
}

void STM32F1_RTC::detachInterrupt(InterruptMode im) {
  switch (im) {
  case RTC_SECONDS_INTERRUPT:
    NVIC_ICER[0] = bit(3);
    waitFinished();
    RTC_CRH &= ~RTC_CRH_SECIE;
    ISR_RTC_SEC = NULL;
    break;
  case RTC_ALARM_INTERRUPT:
    NVIC_ICER[0] = bit(3);
    waitFinished();
    RTC_CRH &= ~RTC_CRH_ALRIE;
    ISR_RTC_ALR = NULL;
    break;
  case RTC_OVERFLOW_INTERRUPT:
    NVIC_ICER[0] = bit(3);
    waitFinished();
    RTC_CRH &= ~RTC_CRH_OWIE;
    ISR_RTC_OW = NULL;
  }
}

void STM32F1_RTC::setTime(uint32_t time) {
  enableBackupWrites();
  waitFinished();
  enterConfigMode();
  RTC_CNTH = time >> 16;
  RTC_CNTL = time & 0xFFFF;
  exitConfigMode();
  waitFinished();
  disableBackupWrites();
}

// The 32-bit RTC counter is spread over 2 registers so it cannot be read
// atomically. We need to read the high word twice and check if it has rolled
// over. If it has, then read the low word a second time to get its new, rolled
// over value. See the RTC_ReadTimeCounter() function in
// system/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rtc.c of the
// STM32duino Core.
uint32_t STM32F1_RTC::getTime() {
  uint16_t high1 = RTC_CNTH;
  uint16_t low = RTC_CNTL;
  uint16_t high2 = RTC_CNTH;

  if (high1 != high2) {
    low = RTC_CNTL;
    high1 = high2;
  }

  return (high1 << 16) | low; 
}

void STM32F1_RTC::setAlarmTime(uint32_t time) {
  enableBackupWrites();
  waitFinished();
  enterConfigMode();
  RTC_ALRH = time >> 16;
  RTC_ALRL = time & 0xFFFF;
  exitConfigMode();
  waitFinished();
  disableBackupWrites();
}

uint16_t STM32F1_RTC::getMilliseconds() {
  uint32_t ms = (RTC_DIVH << 16) | RTC_DIVL;
  return (uint16_t)((float)(32767 - ms) / 32768.0 * 1000.0);
}

uint16_t STM32F1_RTC::getBackupRegister(uint8_t idx) {
#if (BKP_NR_DATA_REGS == 42) || defined(BKP_DR42_D)
  if (idx > 10)
    idx += 5;
  if (1 <= idx && idx <= 47)
#else
  if (1 <= idx && idx <= 10)
#endif
    return BKP_DR[idx];
  else
    return 0;
}

bool STM32F1_RTC::setBackupRegister(uint8_t idx, uint16_t val) {
#if (BKP_NR_DATA_REGS == 42) || defined(BKP_DR42_D)
  if (idx > 10)
    idx += 5;
  if (1 <= idx && idx <= 47)
#else
  if (1 <= idx && idx <= 10)
#endif
  {
    enableBackupWrites();
    BKP_DR[idx] = val;
    disableBackupWrites();
    return true;
  }
  else
    return false;
}

void STM32F1_RTC::epochToDate(uint32_t time, DateVar & rdate) {
  uint16_t sod, dom;
  rdate.numberOfDays = time / 86400;
  rdate.year = rdate.numberOfDays / 365 + 1970;
  sod = getSumOfDayFromYearValue(rdate.year);
  if (sod > rdate.numberOfDays)
    sod = getSumOfDayFromYearValue(--rdate.year);
  rdate.isLeapYear = ((rdate.year - 1968) % 4) == 0 && rdate.year != 2100;
  sod = rdate.numberOfDays - sod;
  for (int8_t i = DECEMBER; i >= JANUARY; i--) {
    dom = getNumberOfDaysUntilMonth(i, rdate.isLeapYear);
    if (dom <= sod) {
      rdate.day = sod - dom + 1;
      rdate.month = i + 1;
      break;
    }
  }
  rdate.weekday = (rdate.numberOfDays + 4) % 7;
}

void STM32F1_RTC::epochToTime(uint32_t time, TimeVar & rtime) {
  uint32_t tm = time % 86400;
  rtime.hours = tm / 3600;
  tm %= 3600;
  rtime.minutes = tm / 60;
  rtime.seconds = tm % 60;
}

uint32_t STM32F1_RTC::dateTimeToEpoch(DateVar & rdate, TimeVar & rtime) {
  uint32_t time;
  if (rdate.month > 0) {
    rdate.year += (rdate.month - 1) / 12;
    rdate.month = (rdate.month - 1) % 12 + 1;
  }
  else {
    rdate.year -= 1 - rdate.month / 12;
    rdate.month = 12 + rdate.month % 12;
  }
  if (rdate.year < 1970)
    rdate.year = 1970;
  else if (rdate.year > 2105)
    rdate.year = 2105;
  rdate.isLeapYear = ((rdate.year - 1968) % 4) == 0 && rdate.year != 2100;
  rdate.numberOfDays = getSumOfDayFromYearValue(rdate.year) + getNumberOfDaysUntilMonth(rdate.month - 1, rdate.isLeapYear) + rdate.day - 1;
  time = rdate.numberOfDays * 86400 + rtime.hours * 3600 + rtime.minutes * 60 + rtime.seconds;
  epochToDate(time, rdate);
  epochToTime(time, rtime);
  return time;
}

void STM32F1_RTC::epochToHijriDate(uint32_t time, DateVar & rhdate) {
  double nod;
  uint16_t nodim;
  uint8_t dim;
  rhdate.numberOfDays = time / 86400;
	nod = (double)rhdate.numberOfDays + HIJRI_DIFF;
  rhdate.month = (int16_t)floor(nod / MOON_CYCLE);
  nodim = getNumberOfDaysUntillHijriMonth(rhdate.month);
  dim = getNumberOfDaysUntillHijriMonth(rhdate.month + 1) - nodim;
  rhdate.day = (int16_t)floor(nod - (double)nodim) + 1;
  if (rhdate.day > dim) {
    rhdate.day = 1;
    rhdate.month++;
  }
  rhdate.month += 9;
  rhdate.year = rhdate.month / 12 + 1389;
  rhdate.month = rhdate.month % 12 + 1;
  rhdate.weekday = (rhdate.numberOfDays + 4) % 7;
  rhdate.isLeapYear = false;  // Not used in Hijri dates
}

uint32_t STM32F1_RTC::hijriDateTimeToEpoch(DateVar & rhdate, TimeVar & rtime) {
  uint32_t time;
  if (rhdate.month > 0) {
    rhdate.year += (rhdate.month - 1) / 12;
    rhdate.month = (rhdate.month - 1) % 12 + 1;
  }
  else {
    rhdate.year -= 1 - rhdate.month / 12;
    rhdate.month = 12 + rhdate.month % 12;
  }
  if (rhdate.year < 1389)
    rhdate.year = 1389;
  else if (rhdate.year > 1529)
    rhdate.year = 1529;
  rhdate.numberOfDays = (uint16_t)floor((double)((rhdate.year - 1389) * 12 + rhdate.month - 10) * MOON_CYCLE + (double)(rhdate.day - 22));
  time = rhdate.numberOfDays * 86400 + rtime.hours * 3600 + rtime.minutes * 60 + rtime.seconds;
  epochToHijriDate(time, rhdate);
  epochToTime(time, rtime);
  return time;
}

uint16_t STM32F1_RTC::getNumberOfDaysUntilMonth(int16_t monthIndex, bool isLeapYear) {
  uint16_t n = numberOfDaysUntilMonth[monthIndex];
  if (isLeapYear && monthIndex > FEBRUARY)
    n++;
  return n;
}

uint16_t STM32F1_RTC::getSumOfDayFromYearValue(uint16_t year) {
  float fsod = (float)(year - 1970) * 365.25 + 0.25;
  if (year > 2100)
    fsod -= 0.5;
  return (uint16_t)fsod;
}

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _LIBMAPLE_STM32F1_H_
void __irq_rtc(void)
#else
void RTC_IRQHandler(void)
#endif
{
  uint16_t irq = RTC_CRH & RTC_CRL;
  if ((irq & RTC_CRL_SECF) == RTC_CRL_SECF && ISR_RTC_SEC != NULL)
    ISR_RTC_SEC();
  if ((irq & RTC_CRL_ALRF) == RTC_CRL_ALRF && ISR_RTC_ALR != NULL)
    ISR_RTC_ALR();
  if ((irq & RTC_CRL_OWF) == RTC_CRL_OWF && ISR_RTC_OW != NULL)
    ISR_RTC_OW();
  while ((RTC_CRL & RTC_CRL_RTOFF) == 0);
  RTC_CRL &= ~irq;
}

#ifdef __cplusplus
}
#endif
