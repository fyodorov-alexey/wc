// ESP-32  свободные пины - можно использовать спокойно в скетче GPIO (2), (4), 12*, 14*, 13*, 15*, (16), (17), 25*, 26*, 27*, (32), (33), (35). -  В скобках заюзанные
// * ADC2 - не будут работать в режиме ADC когда работает WiFi, пины шарены и у вайфай выше приортет возможно и в режиме цифры тоже не будут работать когда вифи включен 
// сука блядь юзать только АЦП1 (GPIO32-39)
// The ADC driver API supports ADC1 (8 channels, attached to GPIOs 32 - 39), and ADC2 (10 channels, attached to GPIOs 0, 2, 4, 12 - 15 and 25 - 27).
// However, the usage of ADC2 has some restrictions for the application
// GPIO  6, 7, 8, 9, 10, 11 - SPI FLASH - не трогать!
// GPIO 34, 35, 36, 37, 38, 39 Это GPIs – input only pins. У них нет подтяжек и утяжек, работают только на вход.

#ifndef funcs
#define funcs

#define PRINTS(s)   Serial.print(F(s))
#define PRINT(s, v) { Serial.print(F(s)); Serial.print(v); }

uint64_t millis64();    // миллисекунды в 64 битах, чтобы не париться о переполнении 32-bit счетчика каждые 49 дней, 17 часов и 2 минуты - для корректного вывода аптайма
String uptime();        // возвращает строку (String) с аптаймом контроллера
char *p2dig(uint8_t v, uint8_t mode); // преобразуем byte в char (для печати в UART например), mode - система счисления (DEC или HEX)
uint8_t external_light, prev_external_light = 1; // Глобальная переменная хранит в себе актуальное на данную секунду состояние внешнего света - подаёт сейчас датчик движения напряжение на лампочку или нет
uint8_t rssiCount = 0;
long rssiSum      = 0;
int prev_gas[6];        // предыдущие значения с датчика газоанализатора. Поусредняем скользящим средним за пять выборок, каждая выборка раз в две секунды
uint8_t gas_i     = 0;  // текущий элемент в массиве 
int prev_gas_avg;       // предыдущее усреднённое значение за 30 секунд (6 выборок каждые 10 секунд, среднее арифметическое)

//--------------------------------------------------------------------
// Пины
//****************************************************************
#define PIN_EXTERNAL_CURRENT  32       // датчик наличия света снаружи через оптопару. Единица - света нет. Ноль - свет есть.
#define PIN_RELAY_LIGHT       16       // пин для подключения SSR света - оставшиеся три лампочки, которыми управляет esp32
#define PIN_RELAY_FAN         17       // пин для подключения SSR вентилятора
#define PIN_BEEPER            2        // Пищалка GPIO2 T02-10 черный провод 
#define PIN_WATERLEVEL        33       // Датчик уровня воды Датчик уровня воды ADC1 channel 5 is GPIO33 (ADC1_CHANNEL_5)
#define PIN_WATERLEVEL_POWER  4        // Пин для питания датчиков воды - будем подавать на него HIGH периодически перед опросом, чтобы не молотили напрополую
#define PIN_GAS_SENSOR        35       // Аналоговый выход датчика загрязнающих газов (пердежа) датчик Figaro TGS2600

//****************************************************************

/*
TODO: подключи пищалку наконец
#define SOUND_PWM_CHANNEL   0
#define SOUND_RESOLUTION    8 // 8 bit resolution
#define SOUND_ON            (1<<(SOUND_RESOLUTION-1)) // 50% duty cycle
#define SOUND_OFF           0                         // 0% duty cycle
*/

/*
 * Login page (для OTA firmware update)
 */

const char* indexpage = 
"<!DOCTYPE html><html lang='en'><head></head><body>"
"<div><h3>Toilet controller online</h3></div>"
"</body></html>";
 
/*
 * Server Index Page
 */
 
const char* updater = 
"<script src='http://main.local/js/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')" 
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

#endif