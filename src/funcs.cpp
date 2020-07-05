#include <Arduino.h>

uint64_t timer0 = 0 ; // таймер для аптайма, чтобы не переполнялся каждые 50 дней
uint64_t millis64() // миллисекунды в 64 битах, чтобы не париться о переполнении 32-bit счетчика каждые 49 дней, 17 часов и 2 минуты - для корректного вывода аптайма
{
    static uint32_t low32, high32;
    uint32_t new_low32 = millis();
    if (new_low32 < low32) high32++;
    low32 = new_low32;
    return (uint64_t) high32 << 32 | low32;
}

String uptime() // возвращает строку (String) с аптаймом контроллера
{
   long days, hours, mins, secs = 0;
   timer0 = millis64();
   secs = timer0/1000;
   mins = secs/60;
   hours = mins/60;
   days = hours/24;
   mins = mins-(hours*60);
   hours = hours-(days*24);
   String res = "up ";
   if (days > 0) res = res + days + " days, ";
   if (hours > 0) res = res + hours + " hours, ";
   res = res + mins + " minutes";
   return res;
}

// Давай от этой хуйни вообще-то избавимся. Пока просто от большой лени оставляю
char *p2dig(uint8_t v, uint8_t mode) // преобразуем byte в char (для печати в UART например), mode - система счисления (DEC или HEX)
// на выходе массив буков - две цифры с незначащим нулём (если нужен). третья букова должна быть нулём.
{
  static char s[3];
  uint8_t i = 0;
  uint8_t n = 0;
 
  switch(mode)
  {
    case HEX: n = 16;  break;
    case DEC: n = 10;  break;
  }

  if (v < n) s[i++] = '0';
  itoa(v, &s[i], n);
  
  return(s);
} // конец p2dig()