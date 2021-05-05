#include "CTBot.h"
#include "DHT.h"
#include <FirebaseArduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <LiquidCrystal_I2C.h>

// DHT Sensor PIN
#define DHTPIN 14     
#define DHTTYPE DHT22

// Firebase Config
#define FIREBASE_HOST "###########################"
#define FIREBASE_AUTH "###########################"

// WIFI Config
String ssid = "############";
String pass = "############";
String token = "###########"; // Token Telegram
String FanStatus = "off", AutoStatus = "off";

int relayPin = 2;
int redPin = 12;
int greenPin = 13;
int bluePin = 15;
int minTemp = 27;
int minHum = 77;
int intervalNotif = 30;

int ID_User;
float tmpTemp, tmpHum;
String AutoReport, tmpReport, User, reply, hr, mn, sc;

WiFiUDP ntpUDP;
TBMessage msg;

CTBot zbot;
CTBotReplyKeyboard button;
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", 25200);

byte up[] = {
    B11111,
    B11011,
    B10001,
    B01010,
    B11011,
    B11011,
    B11011,
    B11111};

byte down[] = {
    B11111,
    B11011,
    B11011,
    B11011,
    B01010,
    B10001,
    B11011,
    B11111};

byte equal[] = {
    B11111,
    B11111,
    B10001,
    B11111,
    B11111,
    B10001,
    B11111,
    B11111};

byte mailLeft[] = {
    B11111,
    B11000,
    B10100,
    B10010,
    B10001,
    B10000,
    B10000,
    B11111};

byte mailRight[] = {
    B11111,
    B00011,
    B00101,
    B01001,
    B10001,
    B00001,
    B00001,
    B11111};

void displayLCD(int x, int y, String Word)
{
  lcd.setCursor(x, y);
  lcd.print(Word);
}

void printLCD(int x, int y, String Word)
{
  if (Word.length() >= 16)
  {
    for (int i = 0; i < 16; i++)
    {
      delay(300);
      displayLCD(x, y, Word);
      lcd.scrollDisplayLeft();
    }
  }
  else
  {
    displayLCD(x, y, Word);
  }
}

void blinkLed(int led, int delayTime = 500, int Loop = 1)
{
  for (int i = 0; i < Loop; i++)
  {
    digitalWrite(led, HIGH);
    delay(delayTime);
    digitalWrite(led, LOW);
  }
}

void graphic(float vtmp, float vnow)
{
  if (vtmp > vnow)
  {
    lcd.write(byte(1));
  }
  else if (vtmp < vnow)
  {
    lcd.write(byte(2));
  }
  else
  {
    lcd.write(byte(3));
  }
}

void sendMsg(String message, int ID_User, String User, int Type, int led, int delayTime = 500, int Loop = 1)
{
  Serial.print("\nReply to @" + User + " | Message : " + message);

  if (Type == 1)
  {
    zbot.sendMessage(ID_User, message, button);
  }
  else if (Type == 2)
  {
    zbot.removeReplyKeyboard(ID_User, message);
  }
  else
  {
    zbot.sendMessage(ID_User, message);
  }

  blinkLed(led, delayTime, Loop);
}

String createNotif(int currentHour, int currentMinute, int intervalNotif)
{
  int tmpTimeMinute = currentMinute += intervalNotif;
  int tmpTimeHour = currentHour;

  if (tmpTimeMinute > 60)
  {
    tmpTimeMinute -= 60;
    tmpTimeHour = currentHour + 1;

    if (tmpTimeHour > 24)
    {
      tmpTimeHour -= 24;
    }
  }

  AutoReport = (String)tmpTimeHour + ":" + (String)tmpTimeMinute;
  return AutoReport;
}

void setup()
{
  dht.begin();
  timeClient.begin();
  lcd.begin();

  lcd.createChar(1, up);
  lcd.createChar(2, down);
  lcd.createChar(3, equal);
  lcd.createChar(4, mailLeft);
  lcd.createChar(5, mailRight);

  pinMode(redPin, OUTPUT);
  pinMode(greenPin, OUTPUT);
  pinMode(bluePin, OUTPUT);
  pinMode(relayPin, OUTPUT);

  digitalWrite(redPin, LOW);
  digitalWrite(greenPin, LOW);
  digitalWrite(bluePin, LOW);
  digitalWrite(relayPin, HIGH);

  Serial.begin(115200);
  Serial.println("Starting Fan Control");

  lcd.clear();
  displayLCD(0, 0, "  Fan Control!");
  displayLCD(0, 1, "   ----##----");
  delay(5000);

  Serial.println("Connecting Wifi: " + ssid);

  lcd.clear();
  displayLCD(0, 0, "Connecting Wifi:");
  printLCD(0, 1, ssid);

  zbot.setMaxConnectionRetries(30);
  zbot.wifiConnect(ssid, pass);
  zbot.setTelegramToken(token);
  zbot.enableUTF8Encoding(true);
  delay(1500);

  lcd.clear();
  displayLCD(0, 0, "# Status: ");
  if (zbot.testConnection())
  {
    Serial.print("Connection established!");
    displayLCD(0, 1, "Connected!");
  }
  else
  {
    Serial.print("Unable to connect to the WIFI network!");
    displayLCD(0, 1, "Not Connected!");
  }

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  String tmpID_User = Firebase.getString("Admin/ID");
  String tmpUser = Firebase.getString("Admin/Username");

  if (tmpID_User != NULL && tmpUser != NULL)
  {
    ID_User = tmpID_User.toInt();
    User = tmpUser;

    Serial.print("\nUser found : @" + User + " [" + ID_User + "]");
  }

  String tmpAuto = Firebase.getString("Status/Auto");
  String tmpFan = Firebase.getString("Status/Fan");
  String Maindata = Firebase.getString("MainData");

  sizeof(Maindata)

  if (sizeof(Maindata))
  {
    AutoStatus = tmpAuto;
    Serial.print(" | Auto Status : " + AutoStatus);
  }

  if (tmpFan != NULL)
  {
    FanStatus = tmpFan;

    if (FanStatus == "on")
    {
      digitalWrite(relayPin, LOW);
    }

    Serial.print(" | Fan Status : " + FanStatus);
  }

  Serial.print("\n-----------------------------------------------");

  delay(500);

  button.addButton("Fan");
  button.addButton("Auto");
  button.addRow();
  button.addButton("Help");
  button.enableResize();
}

void loop()
{
  timeClient.update();

  if (timeClient.getHours() < 10)
  {
    hr = "0" + String(timeClient.getHours());
  }
  else
  {
    hr = String(timeClient.getHours());
  }

  if (timeClient.getMinutes() < 10)
  {
    mn = "0" + String(timeClient.getMinutes());
  }
  else
  {
    mn = String(timeClient.getMinutes());
  }

  if (timeClient.getSeconds() < 10)
  {
    sc = "0" + String(timeClient.getSeconds());
  }
  else
  {
    sc = String(timeClient.getSeconds());
  }

  String GetTimeNow = hr + ":" + mn + ":" + sc;

  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  Firebase.setString("Data/Time", GetTimeNow);
  Firebase.setFloat("Data/Temperature", temp);
  Firebase.setFloat("Data/Humidity", hum);

  if (zbot.getNewMessage(msg))
  {
    blinkLed(bluePin, 500, 2);

    Serial.print("\nMessage from @" + msg.sender.username + " (" + msg.sender.id + ")" + " : " + msg.text);

    String LogMsgID = (String) "Message/" + GetTimeNow + "/ID";
    String LogMsgUser = (String) "Message/" + GetTimeNow + "/User";
    String LogMsgText = (String) "Message/" + GetTimeNow + "/Text";
    String LogMsgTime = (String) "Message/" + GetTimeNow + "/Time";

    Firebase.setString(LogMsgID, (String)msg.sender.id);
    Firebase.setString(LogMsgUser, msg.sender.username);
    Firebase.setString(LogMsgText, msg.text);
    Firebase.setString(LogMsgTime, GetTimeNow);

    lcd.clear();
    lcd.setCursor(0, 0);
    displayLCD(0, 0, "From: " + msg.sender.firstName);

    lcd.setCursor(0, 1);
    lcd.write(byte(4));
    lcd.setCursor(1, 1);
    lcd.write(byte(5));
    printLCD(3, 1, msg.text);

    if (ID_User == NULL || User == NULL)
    {
      if (msg.text.equalsIgnoreCase("/start") || msg.text.equalsIgnoreCase("START"))
      {
        ID_User = msg.sender.id;
        User = msg.sender.username;

        Firebase.setString("Admin/ID", (String)ID_User);
        Firebase.setString("Admin/Username", User);
        Firebase.setString("Admin/Time", GetTimeNow);

        String TEXT_START = "Hello " + msg.sender.firstName + ", I'm Zacky IoT a BOT that allows to control fan.\n\nType /help to see what I can do!";
        sendMsg(TEXT_START, msg.sender.id, msg.sender.username, 1, bluePin);
      }
      else
      {
        String NO_START = "You have to register, type /start to do it";
        sendMsg(NO_START, msg.sender.id, msg.sender.username, 0, redPin);
      }
    }
    else if (ID_User == msg.sender.id && User == msg.sender.username)
    {
      if (msg.text.equalsIgnoreCase("/help") || msg.text.equalsIgnoreCase("HELP"))
      {
        const char TEXT_HELP[] =
            "Zacky IoT command lists :\n"
            "\n"
            "Fan - Turn On/Off Fan\n"
            "Auto - Fan mode Auto\n"
            "\n"
            "/start - Show start text\n"
            "/reset - Reset admin rights\n"
            "/temp - Monitor temperature\n"
            "/hum - Monitor humidity\n"
            "/fanstatus - Check fan status\n"
            "/autostatus - Check auto status\n"
            "/show - Displays buttons\n"
            "/hide - Button hidden\n"
            "/help - How to use BOT\n"
            "/about - About project\n";

        sendMsg(TEXT_HELP, msg.sender.id, msg.sender.username, 2, bluePin, 500, 2);
      }
      else if (msg.text.equalsIgnoreCase("/about") || msg.text.equalsIgnoreCase("ABOUT"))
      {
        const char TEXT_ABOUT[] =
            "Zacky IoT build using :\n"
            "- Fan\n"
            "- NodeMCU\n"
            "- DHT22\n"
            "- Relay\n"
            "- LCD\n"
            "- LED RGB\n"
            "\n"
            "If you have any questions, don't hesitate to get in touch @zckyachmd";

        sendMsg(TEXT_ABOUT, msg.sender.id, msg.sender.username, 2, bluePin, 500, 2);
      }
      else if (msg.text.equalsIgnoreCase("/reset") || msg.text.equalsIgnoreCase("RESET"))
      {
        digitalWrite(relayPin, HIGH);

        FanStatus = "off";
        AutoStatus = "off";
        AutoReport = "";
        ID_User = NULL;
        User = "";

        Firebase.remove("/");

        String Reset = msg.sender.firstName + ", your access rights have been Reset!";
        sendMsg(Reset, msg.sender.id, msg.sender.username, 2, redPin, 500, 3);
      }
      else if (msg.text.equalsIgnoreCase("/temp") || msg.text.equalsIgnoreCase("TEMP"))
      {
        reply = (String) "[" + GetTimeNow + (String) "] Temperatur : " + temp + (String) "°C";
        sendMsg(reply, msg.sender.id, msg.sender.username, 1, greenPin);
      }
      else if (msg.text.equalsIgnoreCase("/hum") || msg.text.equalsIgnoreCase("HUM"))
      {
        reply = (String) "[" + GetTimeNow + (String) "] Humidity : " + hum + (String) "%";
        sendMsg(reply, msg.sender.id, msg.sender.username, 1, greenPin);
      }
      else if (msg.text.equalsIgnoreCase("/fanstatus") || msg.text.equalsIgnoreCase("FAN STATUS"))
      {
        reply = (String) "[" + GetTimeNow + (String) "] Fan is " + FanStatus;
        sendMsg(reply, msg.sender.id, msg.sender.username, 1, greenPin);
      }
      else if (msg.text.equalsIgnoreCase("/autostatus") || msg.text.equalsIgnoreCase("AUTO STATUS"))
      {
        reply = (String) "[" + GetTimeNow + (String) "] Auto is " + AutoStatus;
        sendMsg(reply, msg.sender.id, msg.sender.username, 1, greenPin);
      }
      else if (msg.text.equalsIgnoreCase("/FAN") || msg.text.equalsIgnoreCase("FAN"))
      {
        if (FanStatus == "off")
        {
          digitalWrite(relayPin, LOW);

          FanStatus = "on";
          Firebase.setString("Status/Fan", "on");

          reply = (String) "Fan is on | Temp : " + temp + (String) "°C | Humidity : " + hum + (String) "%";
          sendMsg(reply, msg.sender.id, msg.sender.username, 1, greenPin, 500, 2);

          delay(2000);
          lcd.clear();
          displayLCD(0, 0, "# Action : ");
          displayLCD(0, 1, "Fan is on");
        }
        else
        {
          digitalWrite(relayPin, HIGH);
          FanStatus = "off";
          AutoStatus = "off";

          Firebase.setString("Status/Auto", "off");
          Firebase.setString("Status/Fan", "off");

          reply = (String) "Fan is off | Temp : " + temp + (String) "°C | Humidity : " + hum + (String) "%";
          sendMsg(reply, msg.sender.id, msg.sender.username, 1, redPin, 500, 2);

          delay(2000);
          lcd.clear();
          displayLCD(0, 0, "# Action : ");
          displayLCD(0, 1, "Fan is off");
        }
      }
      else if (msg.text.equalsIgnoreCase("/AUTO") || msg.text.equalsIgnoreCase("AUTO"))
      {
        if (AutoStatus == "off")
        {
          if (temp >= minTemp && hum <= minHum)
          {
            digitalWrite(relayPin, LOW);

            FanStatus = "on";
            Firebase.setString("Status/Fan", "on");
          }

          AutoStatus = "on";
          Firebase.setString("Status/Auto", "on");

          reply = (String) "Auto is on & Fan is " + FanStatus + " | Temp : " + temp + (String) "°C | Humidity : " + hum + (String) "%";
          sendMsg(reply, msg.sender.id, msg.sender.username, 1, greenPin, 500, 2);

          tmpReport = createNotif(hr.toInt(), mn.toInt(), intervalNotif);
          Firebase.setString("Status/Report", tmpReport);

          delay(2000);

          lcd.clear();
          displayLCD(0, 0, "# Action : ");
          displayLCD(0, 1, "Auto is on");
        }
        else
        {
          digitalWrite(relayPin, HIGH);

          FanStatus = "off";
          AutoStatus = "off";
          AutoReport = "";

          Firebase.setString("Status/Auto", "off");
          Firebase.setString("Status/Fan", "off");
          Firebase.remove("Status/Report");

          reply = (String) "Auto is off & Fan is " + FanStatus + " | Temp : " + temp + (String) "°C | Humidity : " + hum + (String) "%";
          sendMsg(reply, msg.sender.id, msg.sender.username, 1, redPin, 500, 2);

          delay(2000);

          lcd.clear();
          displayLCD(0, 0, "# Action : ");
          displayLCD(0, 1, "Auto is off");
        }
      }
      else if (msg.text.equalsIgnoreCase("/SHOW") || msg.text.equalsIgnoreCase("SHOW"))
      {
        reply = "Button visible, to hide type /hide";
        sendMsg(reply, msg.sender.id, msg.sender.username, 1, greenPin);
      }
      else if (msg.text.equalsIgnoreCase("/HIDE") || msg.text.equalsIgnoreCase("HIDE"))
      {
        reply = "Button hidden, to show type /show";
        sendMsg(reply, msg.sender.id, msg.sender.username, 2, greenPin);
      }
      else
      {
        reply = "Type /help for commands";
        sendMsg(reply, msg.sender.id, msg.sender.username, 1, redPin);
      }
    }
    else
    {
      String forbidden = "BOT is used by @" + User + ", you can't use it now!";
      sendMsg(forbidden, msg.sender.id, msg.sender.username, 0, redPin, 500, 4);
    }

    Serial.print("\n-----------------------------------------------");
  }

  if (AutoStatus == "on" && temp > minTemp && hum < minHum)
  {
    digitalWrite(relayPin, LOW);
    FanStatus = "on";
    Firebase.setString("Status/Fan", "on");
  }

  if (AutoStatus == "on" && temp <= minTemp && hum >= minHum)
  {
    digitalWrite(relayPin, HIGH);
    FanStatus = "off";
    Firebase.setString("Status/Fan", "off");
  }

  blinkLed(greenPin, 250);

  String Status = (String) "\n[" + GetTimeNow + (String) "]" + (String) " Temperature: " + temp + (String) "°C | Humidity: " + hum + (String) "% | Fan is " + FanStatus + (String) " | Auto is " + AutoStatus;
  Serial.print(Status);

  if (AutoStatus == "on" && AutoReport == GetTimeNow)
  {
    Serial.print((String) " | Sending report to @" + User);
    zbot.sendMessage(ID_User, Status);

    tmpReport = createNotif(hr.toInt(), mn.toInt(), intervalNotif);
    Firebase.setString("Status/Report", tmpReport);

    lcd.clear();
    displayLCD(0, 0, "# Action : ");
    if (FanStatus = "on")
    {
      displayLCD(0, 1, "Fan is on");
    }
    else
    {
      displayLCD(0, 1, "Fan is off");
    }

    delay(1500);
  }
  Serial.print("\n-----------------------------------------------");

  lcd.clear();
  lcd.setCursor(0, 0);
  graphic(tmpTemp, temp);
  displayLCD(2, 0, (String) "Temp : " + temp + (char)223 + "C");

  lcd.setCursor(0, 1);
  graphic(tmpHum, hum);
  displayLCD(2, 1, (String) "Hum  : " + hum + (String) "%");

  tmpTemp = temp;
  tmpHum = hum;

  delay(30000);
}
