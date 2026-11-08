#include <SPI.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Preferences.h>

#define EPD_CLASS GxEPD2_290_T94_V2

#define PIN_UP    1
#define PIN_OK    2
#define PIN_DOWN  3
#define PIN_BAT   4
#define EPD_CS    5
#define EPD_DC    6
#define EPD_RST   43
#define EPD_BUSY  44
#define EPD_SCK   7
#define EPD_MOSI  9

#define ROWS       6
#define ROW_H      17
#define TOP        22
#define AWAKE_MS   4000
#define HOLD_MS    1500
#define FULL_EVERY 15

const char *items[] = 
{
  "Finish homework",
  "Study for physics test",
  "Watch a video",
  "Work on devlog",
  "Make Bill of Materials",
  "Reply to emails",
  "Pack backpack",
  "Finish reading article",
  "Take out the recycling",
  "Make bed"
};
const int itemCount = sizeof(items) / sizeof(items[0]);

GxEPD2_BW<EPD_CLASS, EPD_CLASS::HEIGHT> display(EPD_CLASS(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));
Preferences prefs;

RTC_DATA_ATTR int sel = 0;
RTC_DATA_ATTR int partialCount = 0;
RTC_DATA_ATTR bool booted = false;

uint32_t doneMask = 0;
bool forceFull = false;
uint32_t lastActive = 0;
const uint8_t BTN[3] = {PIN_UP, PIN_OK, PIN_DOWN};
bool wasDown[3];

void loadDone() 
{
  prefs.begin("list", true);
  int n = prefs.getInt("n", -1);
  doneMask = (n == itemCount) ? prefs.getUInt("done", 0) : 0;
  prefs.end();
}

void saveDone() 
{
  prefs.begin("list", false);
  prefs.putInt("n", itemCount);
  prefs.putUInt("done", doneMask);
  prefs.end();
}

int batteryPercent() 
{
  uint32_t mv = 0;
  for (int i = 0; i < 8; i++) mv += analogReadMilliVolts(PIN_BAT);
  mv = mv / 8 * 2;
  return constrain(map(mv, 3300, 4200, 0, 100), 0, 100);
}

String fit(String s, int maxW) 
{
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  if (w <= maxW) return s;
  while (s.length() > 1) 
  {
    s.remove(s.length() - 1);
    display.getTextBounds(s + "...", 0, 0, &x1, &y1, &w, &h);
    if (w <= maxW) return s + "...";
  }
  return s;
}

void beginDraw() 
{
  if (!booted || forceFull || partialCount >= FULL_EVERY) 
  {
    display.setFullWindow();
    partialCount = 0;
    forceFull = false;
  } 
  else 
  {
    display.setPartialWindow(0, 0, display.width(), display.height());
    partialCount++;
  }
  booted = true;
}

void drawList() 
{
  int bat = batteryPercent();
  int doneCount = __builtin_popcount(doneMask);
  int first = (sel / ROWS) * ROWS;
  int pages = (itemCount + ROWS - 1) / ROWS;
  beginDraw();
  display.firstPage();
  do 
  {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(6, 15);
    display.print("TO-DO");
    display.setFont(&FreeSans9pt7b);
    String info = String(doneCount) + "/" + String(itemCount) + " done";
    if (pages > 1) info += "   pg " + String(first / ROWS + 1) + "/" + String(pages);
    display.setCursor(80, 15);
    display.print(info);
    String b = String(bat) + "%";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(b, 0, 0, &x1, &y1, &w, &h);
    display.setCursor(display.width() - w - 6, 15);
    display.print(b);
    display.drawLine(0, 19, display.width(), 19, GxEPD_BLACK);

    for (int r = 0; r < ROWS; r++) 
    {
      int i = first + r;
      if (i >= itemCount) break;
      int y = TOP + r * ROW_H;
      bool done = doneMask & (1UL << i);
      uint16_t fg = GxEPD_BLACK;
      if (i == sel) 
      {
        display.fillRect(0, y, display.width(), ROW_H, GxEPD_BLACK);
        fg = GxEPD_WHITE;
      }
      display.drawRect(6, y + 3, 11, 11, fg);
      if (done) display.fillRect(8, y + 5, 7, 7, fg);
      display.setTextColor(fg);
      String t = fit(items[i], display.width() - 32);
      display.setCursor(24, y + 13);
      display.print(t);
      if (done) 
      {
        display.getTextBounds(t, 0, 0, &x1, &y1, &w, &h);
        display.drawLine(24, y + 8, 24 + w, y + 8, fg);
      }
    }
  } while (display.nextPage());
}

void waitRelease(uint8_t pin) 
{
  while (digitalRead(pin) == LOW) delay(10);
  delay(30);
}

bool isHold(uint8_t pin, uint32_t start) 
{
  while (digitalRead(pin) == LOW) 
  {
    if (millis() - start >= HOLD_MS) return true;
    delay(10);
  }
  return false;
}

void handle(uint8_t pin, uint32_t start) 
{
  if (pin == PIN_UP) 
  {
    sel = (sel + itemCount - 1) % itemCount;
    drawList();
  } 
  else if (pin == PIN_OK) 
  {
    doneMask ^= 1UL << sel;
    saveDone();
    drawList();
    waitRelease(pin);
  } 
  else if (isHold(pin, start)) 
  {
    doneMask = 0;
    saveDone();
    drawList();
    waitRelease(pin);
  } 
  else 
  {
    sel = (sel + 1) % itemCount;
    drawList();
  }
  lastActive = millis();
}

void goToSleep() 
{
  display.hibernate();
  uint64_t mask = (1ULL << PIN_UP) | (1ULL << PIN_OK) | (1ULL << PIN_DOWN);
  esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);
  esp_deep_sleep_start();
}

void setup() 
{
  for (int i = 0; i < 3; i++) pinMode(BTN[i], INPUT_PULLUP);
  SPI.begin(EPD_SCK, -1, EPD_MOSI, EPD_CS);
  display.init(0, !booted, 2, false);
  display.setRotation(1);
  loadDone();
  if (sel >= itemCount) sel = 0;

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) 
  {
    uint64_t s = esp_sleep_get_ext1_wakeup_status();
    if (s & (1ULL << PIN_UP)) handle(PIN_UP, 0);
    else if (s & (1ULL << PIN_OK)) handle(PIN_OK, 0);
    else if (s & (1ULL << PIN_DOWN)) handle(PIN_DOWN, 0);
    else drawList();
  } 
  else 
  {
    forceFull = true;
    drawList();
  }

  for (int i = 0; i < 3; i++)
    wasDown[i] = digitalRead(BTN[i]) == LOW;
  lastActive = millis();
}

void loop() 
{
  for (int i = 0; i < 3; i++) {
    bool down = digitalRead(BTN[i]) == LOW;
    if (down && !wasDown[i]) {
      delay(20);
      if (digitalRead(BTN[i]) == LOW) handle(BTN[i], millis());
    }
    wasDown[i] = digitalRead(BTN[i]) == LOW;
  }
  if (millis() - lastActive > AWAKE_MS) goToSleep();
  delay(5);
}