
#ifndef STM32F1_RTC_H
#define STM32F1_RTC_H

#include "Arduino.h"

#ifdef _LIBMAPLE_STM32F1_H_

#include <libmaple/bkp.h>
#include <libmaple/pwr.h>

struct rtc_reg_map {
  uint32_t CRH;
  uint32_t CRL;
  uint32_t PRLH;
  uint32_t PRLL;
  uint32_t DIVH;
  uint32_t DIVL;
  uint32_t CNTH;
  uint32_t CNTL;
  uint32_t ALRH;
  uint32_t ALRL;
};
    
#define RTC_BASE ((volatile rtc_reg_map*)0x40002800)

#define RTC_CRH_OWIE_BIT  2
#define RTC_CRH_ALRIE_BIT 1
#define RTC_CRH_SECIE_BIT 0

#define RTC_CRH_OWIE  BIT(RTC_CRH_OWIE_BIT)
#define RTC_CRH_ALRIE BIT(RTC_CRH_ALRIE_BIT)
#define RTC_CRH_SECIE BIT(RTC_CRH_SECIE_BIT)
  
#define RTC_CRL_RTOFF_BIT 5
#define RTC_CRL_CNF_BIT   4
#define RTC_CRL_RSF_BIT   3
#define RTC_CRL_OWF_BIT   2
#define RTC_CRL_ALRF_BIT  1
#define RTC_CRL_SECF_BIT  0
  
#define RTC_CRL_RTOFF BIT(RTC_CRL_RTOFF_BIT)
#define RTC_CRL_CNF   BIT(RTC_CRL_CNF_BIT)
#define RTC_CRL_RSF   BIT(RTC_CRL_RSF_BIT)
#define RTC_CRL_OWF   BIT(RTC_CRL_OWF_BIT)
#define RTC_CRL_ALRF  BIT(RTC_CRL_ALRF_BIT)
#define RTC_CRL_SECF  BIT(RTC_CRL_SECF_BIT)

#define RTC_CRH  RTC_BASE->CRH
#define RTC_CRL  RTC_BASE->CRL
#define RTC_PRLH RTC_BASE->PRLH
#define RTC_PRLL RTC_BASE->PRLL
#define RTC_DIVH RTC_BASE->DIVH
#define RTC_DIVL RTC_BASE->DIVL
#define RTC_CNTH RTC_BASE->CNTH
#define RTC_CNTL RTC_BASE->CNTL
#define RTC_ALRH RTC_BASE->ALRH
#define RTC_ALRL RTC_BASE->ALRL

#define RCC_APB1ENR RCC_BASE->APB1ENR
#define RCC_BDCR    RCC_BASE->BDCR
#define PWR_CR      PWR_BASE->CR
#define NVIC_ISER   NVIC_BASE->ISER
#define NVIC_ICER   NVIC_BASE->ICER
#define EXTI_IMR    EXTI_BASE->IMR
#define EXTI_RTSR   EXTI_BASE->RTSR

#else

#ifndef STM32F1xx
#error "This library only suites for STM32F1xx development board."
#endif

#ifndef HAL_RTC_MODULE_ENABLED
#error "RTC configuration is missing. Check flag HAL_RTC_MODULE_ENABLED in stm32f1xx_hal_conf_default.h"
#endif

#define RTC_CRH  RTC->CRH
#define RTC_CRL  RTC->CRL
#define RTC_PRLH RTC->PRLH
#define RTC_PRLL RTC->PRLL
#define RTC_DIVH RTC->DIVH
#define RTC_DIVL RTC->DIVL
#define RTC_CNTH RTC->CNTH
#define RTC_CNTL RTC->CNTL
#define RTC_ALRH RTC->ALRH
#define RTC_ALRL RTC->ALRL

#define RCC_APB1ENR RCC->APB1ENR
#define RCC_BDCR    RCC->BDCR
#define PWR_CR      PWR->CR
#define NVIC_ISER   NVIC->ISER
#define NVIC_ICER   NVIC->ICER
#define EXTI_IMR    EXTI->IMR
#define EXTI_RTSR   EXTI->RTSR

#endif

#define BKP_DR        (((volatile uint32_t*)0x40006C00))
#define RTC_INIT_REG  1
#define RTC_INIT_BIT  0
#define RTC_INIT_FLAG (1 << RTC_INIT_BIT)
#define MOON_CYCLE    29.5305882
#define HIJRI_DIFF    21.252353

enum Weekday:uint8_t {
  SUNDAY,
  MONDAY,
  TUESDAY,
  WEDNESDAY,
  THURSDAY,
  FRIDAY,
  SATURDAY
};

enum MonthIndex:uint8_t {
  JANUARY,
  FEBRUARY,
  MARCH,
  APRIL,
  MAY,
  JUNE,
  JULY,
  AUGUST,
  SEPTEMBER,
  OCTOBER,
  NOVEMBER,
  DECEMBER
};
  
enum InterruptMode:uint8_t {
  RTC_SECONDS_INTERRUPT,
  RTC_ALARM_INTERRUPT,
  RTC_OVERFLOW_INTERRUPT
};

struct DateVar {
  uint16_t numberOfDays;
  uint16_t year;
  int16_t  month;
  int16_t  day;
  uint8_t  weekday;
  bool     isLeapYear;
};

struct TimeVar {
  int16_t  hours;
  int16_t  minutes;
  int16_t  seconds;
};

struct DateTime {
  DateVar date;
  TimeVar time;
};

static void (*ISR_RTC_SEC)(void);
static void (*ISR_RTC_ALR)(void);
static void (*ISR_RTC_OW)(void);

class STM32F1_RTC {
public:
  
  bool begin();
  void init();
  void attachInterrupt(InterruptMode im, void (*handler)(void));
  void detachInterrupt(InterruptMode im);
  void setTime(uint32_t time);
  uint32_t getTime();
  void setAlarmTime(uint32_t time);
  uint16_t getMilliseconds();
  uint16_t getBackupRegister(uint8_t idx);
  bool setBackupRegister(uint8_t idx, uint16_t val);
  void epochToDate(uint32_t time, DateVar & rdate);
  void epochToTime(uint32_t time, TimeVar & rtime);
  uint32_t dateTimeToEpoch(DateVar & rdate, TimeVar & rtime);
  void epochToHijriDate(uint32_t time, DateVar & rhdate);
  uint32_t hijriDateTimeToEpoch(DateVar & rhdate, TimeVar & rtime);
  
  void epochToDateTime(uint32_t time, DateVar & rdate, TimeVar & rtime) {
    epochToDate(time, rdate);
    epochToTime(time, rtime);
  }
  
  void epochToDateTime(uint32_t time, DateTime & rDateTime) {
    epochToDateTime(time, rDateTime.date, rDateTime.time);
  }
  
  uint32_t dateTimeToEpoch(DateTime & rDateTime) {
    return dateTimeToEpoch(rDateTime.date, rDateTime.time);
  }
  
  inline bool isInitialized() {
    return (BKP_DR[RTC_INIT_REG] & RTC_INIT_FLAG) == RTC_INIT_FLAG;
  }
  
  inline bool isCounterUpdated() {
    return (RTC_CRL & RTC_CRL_SECF) == RTC_CRL_SECF;
  }
  
  inline bool isAlarmTriggered() {
    return (RTC_CRL & RTC_CRL_ALRF) == RTC_CRL_ALRF;
  }
  
  inline bool isCounterOverflow() {
    return (RTC_CRL & RTC_CRL_OWF) == RTC_CRL_OWF;
  }
  
  inline void clearSecondFlag() {
    RTC_CRL &= ~RTC_CRL_SECF;
  }
  
  inline void clearAlarmFlag() {
    RTC_CRL &= ~RTC_CRL_ALRF;
  }
  
  inline void clearOverflowFlag() {
    RTC_CRL &= ~RTC_CRL_OWF;
  }
  
  inline void enableClockInterface() {
    RCC_APB1ENR |= RCC_APB1ENR_PWREN | RCC_APB1ENR_BKPEN;
  }
  
  inline void waitSync() {
    RTC_CRL &= ~RTC_CRL_RSF;
    while ((RTC_CRL & RTC_CRL_RSF) == 0);
  }
  
  inline void enableBackupWrites() {
    PWR_CR |= PWR_CR_DBP;
  }
  
  inline void disableBackupWrites() {
    PWR_CR &= ~PWR_CR_DBP;
  }
  
private:

  uint16_t numberOfDaysUntilMonth[12] = { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 };
  
  uint16_t getNumberOfDaysUntilMonth(int16_t monthIndex, bool isLeapYear);
  uint16_t getSumOfDayFromYearValue(uint16_t year);
  
  uint16_t getNumberOfDaysUntillHijriMonth(int16_t hmonth) {
    return (uint16_t)floor((double)hmonth * MOON_CYCLE);
  }
  
  inline void waitFinished() {
    while ((RTC_CRL & RTC_CRL_RTOFF) == 0);
  }
  
  inline void enterConfigMode() {
    RTC_CRL |= RTC_CRL_CNF;
  }
  
  inline void exitConfigMode() {
    RTC_CRL &= ~RTC_CRL_CNF;
  }
};

#endif /* STM32F1_RTC_H */
