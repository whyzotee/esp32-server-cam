#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <esp32cam.h>

const char *WIFI_SSID = "ไม่รู้ๆๆๆ";
const char *WIFI_PASS = "12345Tee";

const char *ssid = "esp32-cam";
const char *password = "12345Tee";

bool open_led = false;

IPAddress local_ip(192, 4, 0, 1);
IPAddress gateway(192, 4, 0, 1);
IPAddress subnet(255, 255, 255, 0);

WebServer server(80);

void serveJpg()
{
  auto frame = esp32cam::capture();
  if (frame == nullptr)
  {
    Serial.println("CAPTURE FAIL");
    server.send(503, "", "");
    return;
  }
  // Serial.printf("CAPTURE OK %dx%d %db\n", frame->getWidth(), frame->getHeight(),
  //               static_cast<int>(frame->size()));

  server.setContentLength(frame->size());
  server.send(200, "image/jpeg");
  WiFiClient client = server.client();
  frame->writeTo(client);
}

void onLED()
{
  if (!open_led)
    open_led = true;
  server.send(200, "text/plain", "on led");
}

void offLED()
{
  if (open_led)
    open_led = false;
  server.send(200, "text/plain", "off led");
}

void setup()
{
  Serial.begin(115200);
  pinMode(4, OUTPUT);

  Serial.println("[esp32-cam] config camera");
  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(Resolution::find(800, 600));
    // cfg.setBufferCount(2);
    cfg.setJpeg(80);

    bool ok = Camera.begin(cfg);
    Serial.println(ok ? "[esp32-cam] camera is ready" : "[esp32-cam] can't begin camera");
  }

  Serial.println("[esp32-cam] config wifi at STA mode");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  for (uint8_t i = 1; i <= 5; i++)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      delay(1000);
      Serial.print("[esp32-cam] reconnect: ");
      Serial.println(i);

      if (i == 5)
      {
        Serial.println("[esp32-cam] config wifi at AP mode");
        WiFi.mode(WIFI_AP);
        WiFi.softAPConfig(local_ip, gateway, subnet);
        WiFi.softAP(ssid, password);
        Serial.print("[esp32-cam] starting cam server: http://");
        Serial.println(WiFi.softAPIP());
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("[esp32-cam] starting cam server: http://");
    Serial.println(WiFi.localIP());
  }

  server.on("/stream", serveJpg);
  server.on("/led_on", onLED);
  server.on("/led_off", offLED);
  server.begin();

  Serial.println("[esp32-cam] server is ready");
}

void loop()
{
  server.handleClient();
  digitalWrite(4, open_led);
  delay(50);
}
