#include <WiFi.h>               // کتابخانه WiFi برای اتصال ESP32 به شبکه بی‌سیم
#include <WebServer.h>          // کتابخانه وب سرور برای راه‌اندازی سرور وب

// تعریف پین‌های مورد استفاده
#define TRIG_PIN 5              // پین ارسال سیگنال التراسونیک
#define ECHO_PIN 18             // پین دریافت سیگنال بازگشتی التراسونیک
#define RED_LED_PIN 23          // پین مربوط به LED قرمز برای نمایش "اشغال‌شده"
#define GREEN_LED_PIN 21        // پین مربوط به LED سبز برای نمایش "خالی"

// اطلاعات اتصال به شبکه WiFi
const char* ssid = "A30";             // نام شبکه وای‌فای
const char* password = "alinematii";  // رمز عبور شبکه وای‌فای

WebServer server(80);                 // ایجاد یک سرور وب روی پورت 80

// متغیرها برای مدیریت وضعیت پارکینگ
unsigned long objectDetectedTime = 0;           // زمانی که جسم برای اولین بار شناسایی شد
bool objectPresent = false;                     // آیا جسمی در محدوده وجود دارد؟
const unsigned long detectionThreshold = 3000;  // مدت زمان لازم برای تغییر وضعیت به "اشغال‌شده" (3 ثانیه)
String parkingStatus = "Empty";                 // وضعیت پارکینگ (پیش‌فرض: "خالی")

// تابع برای مدیریت درخواست وضعیت پارکینگ
void handleStatus();

void setup() {
  Serial.begin(115200); // شروع ارتباط سریال برای دیباگ

  // تنظیم پین‌های ESP32
  pinMode(TRIG_PIN, OUTPUT);   // تنظیم پین TRIG ماژول التراسونیک به‌عنوان خروجی
  pinMode(ECHO_PIN, INPUT);    // تنظیم پین ECHO ماژول التراسونیک به‌عنوان ورودی
  pinMode(RED_LED_PIN, OUTPUT); // تنظیم پین LED قرمز به‌عنوان خروجی
  pinMode(GREEN_LED_PIN, OUTPUT); // تنظیم پین LED سبز به‌عنوان خروجی

  // اتصال به شبکه WiFi
  WiFi.begin(ssid, password);               // شروع فرآیند اتصال به شبکه WiFi
  Serial.println("Connecting to WiFi...");  // پیام دیباگ: در حال اتصال به WiFi
  while (WiFi.status() != WL_CONNECTED) {   // انتظار برای اتصال موفق به شبکه
    delay(1000);                            // وقفه یک ثانیه‌ای
    Serial.print(".");                      // چاپ نقطه برای نمایش پیشرفت اتصال
  }
  Serial.println("\nConnected to WiFi!");

  // تعریف مسیرهای سرور وب
  server.on("/", handleRoot);           // مسیری که صفحه اصلی وب را نمایش می‌دهد
  server.on("/status", HTTP_GET, handleStatus); // مسیری که وضعیت پارکینگ را ارسال می‌کند
  server.begin();                       // شروع به کار سرور وب
  Serial.println("Server started");     // پیام دیباگ: سرور وب راه‌اندازی شد

  Serial.print("Server Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();  // رسیدگی به درخواست‌های HTTP کلاینت

  // فرآیند سنجش فاصله با استفاده از ماژول التراسونیک
  digitalWrite(TRIG_PIN, LOW);           // سیگنال TRIG را به LOW تنظیم کنید
  delayMicroseconds(2);                  // وقفه 2 میکروثانیه‌ای برای پایدارسازی
  digitalWrite(TRIG_PIN, HIGH);          // ارسال سیگنال HIGH از طریق پین TRIG
  delayMicroseconds(10);                 // مدت سیگنال HIGH به اندازه 10 میکروثانیه
  digitalWrite(TRIG_PIN, LOW);           // بازگشت سیگنال TRIG به LOW

  long duration = pulseIn(ECHO_PIN, HIGH);  // مدت زمان بازگشت پالس را اندازه‌گیری کنید
  float distance = (duration * 0.034) / 2;  // محاسبه فاصله بر حسب سانتی‌متر (مدت زمان را به متر تبدیل کنید و بر 2 تقسیم کنید)

  // بررسی وضعیت جسم شناسایی‌شده
  if (distance < 20) { // اگر فاصله کمتر از 20 سانتی‌متر باشد (یعنی جسم در محدوده است)
    if (!objectPresent) { // اگر برای اولین بار جسم شناسایی شود
      objectPresent = true;            // پرچم حضور جسم را به true تغییر دهید
      objectDetectedTime = millis();   // زمان شناسایی جسم را ثبت کنید
    } else {
      // اگر جسم برای مدت زمان تعیین‌شده در محدوده باشد
      if (millis() - objectDetectedTime >= detectionThreshold) {
        digitalWrite(GREEN_LED_PIN, LOW);  // خاموش کردن LED سبز
        digitalWrite(RED_LED_PIN, HIGH);  // روشن کردن LED قرمز
        parkingStatus = "Occupied";       // به‌روزرسانی وضعیت پارکینگ به "اشغال‌شده"
      }
    }
  } else { // اگر جسم در محدوده نباشد
    objectPresent = false;               // پرچم حضور جسم را به false تغییر دهید
    digitalWrite(GREEN_LED_PIN, HIGH);   // روشن کردن LED سبز
    digitalWrite(RED_LED_PIN, LOW);      // خاموش کردن LED قرمز
    parkingStatus = "Empty";             // به‌روزرسانی وضعیت پارکینگ به "خالی"
  }
}

// تابع برای مدیریت درخواست به صفحه اصلی وب
void handleRoot() {
String html = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>Parking</title>
    <style>
      body {
        font-family: 'Roboto', sans-serif;
        background: linear-gradient(135deg, #00bcd4, #8bc34a); /* Smooth blue-green gradient */
        margin: 0;
        padding: 0;
        display: flex;
        justify-content: center;
        align-items: center;
        height: 100vh;
        text-align: center;
        color: #fff;
      }
      .container {
        background-color: rgba(255, 255, 255, 0.9);
        border-radius: 16px;
        box-shadow: 0 6px 20px rgba(0, 0, 0, 0.1);
        padding: 40px;
        width: 85%;
        max-width: 500px;
        transition: transform 0.3s ease;
      }

      .container:hover {
        transform: scale(1.02);
      }
      .status-container {
        margin-top: 30px;
        display: flex;
        justify-content: center;
        align-items: center;
        position: relative;
      }
      .parking-svg.animate {
        animation: scaleUp 1.5s infinite;
      }

      @keyframes scaleUp {
        0%, 100% {
          transform: scale(1);
        }
        50% {
          transform: scale(1.1);
        }
      }
      .status-text {
        font-size: 1.8em;
        font-weight: 700;
        color: #333;
        margin-top: 20px;
        text-transform: uppercase;
      }
      .parking-svg svg {
        transition: transform 0.3s ease;
      }

      .parking-svg svg:hover {
        transform: scale(1.1);
      }
    </style>
    <script>
      // Function to fetch parking status from the server
      function getStatus() {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/status", true);  // Send a request to /status endpoint
        xhr.onreadystatechange = function() {
          if (xhr.readyState == 4 && xhr.status == 200) {
            // Update the SVG's class and the text
            var response = xhr.responseText;
            var svgElement = document.getElementById("parkingSVG");
            var statusTextElement = document.getElementById("statusText");

            if (response === "Occupied") {
              svgElement.innerHTML = `
                <svg height="300px" width="300px" version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" 
                  viewBox="0 0 496.159 496.159" xml:space="preserve">
                <path style="fill:#D61E1E;" d="M248.083,0.003C111.071,0.003,0,111.063,0,248.085c0,137.001,111.07,248.07,248.083,248.07
                  c137.006,0,248.076-111.069,248.076-248.07C496.159,111.062,385.089,0.003,248.083,0.003z"/>
                <path style="fill:#F4EDED;" d="M248.082,39.002C132.609,39.002,39,132.602,39,248.084c0,115.463,93.609,209.072,209.082,209.072
                  c115.468,0,209.077-93.609,209.077-209.072C457.159,132.602,363.55,39.002,248.082,39.002z"/>
                <path style="fill:#5B5147;" d="M367.084,145.289c-5.992-12.249-13.903-21.775-23.728-28.575c-9.83-6.797-22.012-11.41-36.553-13.833
                  c-10.368-1.884-25.378-2.828-45.033-2.828H150.094v296.051h39.178V275.746h75.931c41.869,0,70.813-8.715,86.837-26.152
                  c16.019-17.434,24.031-38.739,24.031-63.915C376.071,171.005,373.073,157.541,367.084,145.289z M319.727,226.673
                  c-10.636,9.426-28.61,14.136-53.919,14.136h-76.537V134.99H265c17.771,0,29.954,0.877,36.552,2.625
                  c10.23,2.827,18.478,8.652,24.738,17.468c6.26,8.819,9.39,19.422,9.39,31.807C335.681,203.989,330.362,217.251,319.727,226.673z"/>
                <path style="fill:#D61E1E;" d="M85.851,60.394c-9.086,7.86-17.596,16.37-25.457,25.456l349.914,349.914
                  c9.086-7.861,17.596-16.37,25.456-25.456L85.851,60.394z"/>
                </svg> 
              `;
              statusTextElement.innerHTML = "Occupied";
              svgElement.classList.remove("empty");
              svgElement.classList.add("occupied");
            } else {
              svgElement.innerHTML = `
                <svg fill="#4caf50" height="300px" width="300px" version="1.1" id="Layer_1" xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" 
                  viewBox="0 0 512 512" xml:space="preserve">
                <g>
                  <g>
                    <g>
                      <path d="M256,0C114.618,0,0,114.618,0,256s114.618,256,256,256s256-114.618,256-256S397.382,0,256,0z M256,469.333
                        c-117.818,0-213.333-95.515-213.333-213.333S138.182,42.667,256,42.667S469.333,138.182,469.333,256S373.818,469.333,256,469.333
                        z"/>
                      <path d="M277.333,106.667h-64C201.551,106.667,192,116.218,192,128v128v149.333c0,11.782,9.551,21.333,21.333,21.333
                        c11.782,0,21.333-9.551,21.333-21.333v-128h42.667c47.131,0,85.333-38.202,85.333-85.333S324.465,106.667,277.333,106.667z
                        M277.333,234.667h-42.667v-85.333h42.667C300.901,149.333,320,168.433,320,192S300.901,234.667,277.333,234.667z"/>
                    </g>
                  </g>
                </g>
                </svg>
              `;
              statusTextElement.innerHTML = "Empty";
              svgElement.classList.remove("occupied");
              svgElement.classList.add("empty");
            }
          }
        };
        xhr.send();
      }

      // Fetch the status every 2 seconds (2000 ms)
      setInterval(getStatus, 2000);
    </script>
  </head>
  <body>
    <div class="container">
      <div class="status-container">
        <!-- Placeholder for SVG -->
        <div id="parkingSVG" class="parking-svg empty">
          <!-- Initial Empty SVG here will be updated by JS -->
        </div>
      </div>
      <div id="statusText" class="status-text">Empty</div>
    </div>
  </body>
  </html>
)rawliteral";

  // ارسال کد HTML به مرورگر کلاینت
  server.send(200, "text/html", html);
}

// تابع برای مدیریت درخواست وضعیت پارکینگ
void handleStatus() {
  // ارسال وضعیت پارکینگ ("خالی" یا "اشغال‌شده") به صورت متن ساده
  server.send(200, "text/plain", parkingStatus);
}
