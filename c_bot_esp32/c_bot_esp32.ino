/*******************************************************************
    An example of bot that receives commands and turns on and off
    an LED.
 *                                                                 *
    written by Giacarlo Bacchio (Gianbacchio on Github)
    adapted by Brian Lough
 *******************************************************************/
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <HTTPClient.h>

#include "Grove_I2C_Motor_Driver.h"

// default I2C address is 0x0f
#define I2C_ADDRESS 0x0f

//Sensor
//Light
#define LIGHT_L 32
#define LIGHT_R 33
//Moisture
#define MOIST_S 35
//Ultrasound
#define US_PING 26
#define US_ECHO 34


// Initialize Wifi connection to the router
char ssid[] {"schoolex"};    // your network SSID (name)
char password[] {"jellybear2"}; // your network key

// Initialize Telegram BOT
#define BOTtoken "991983428:AAFhvCLtXGdw4IR5rf9kGOBq7p7gs5VQAXc"  // your Bot Token (Get from Botfather)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

int Bot_mtbs{100}; //mean time between scan messages
long Bot_lasttime{0};   //last time messages' scan has been done
bool Start{false};

int movStatus{0};  //0: stop, 1: forward, 2: backward


void movBot (String chat_id, bool forward) {
  String msg{};
  if (forward) {
    movStatus = 1;
    msg = "Moving forward";
    // Move forward
    Motor.speed(MOTOR1, 100);
    Motor.speed(MOTOR2, -100);
  } else {
    movStatus = 2;
    msg = "Moving backward";
    //Backward
    Motor.speed(MOTOR1, -100);
    Motor.speed(MOTOR2, 100);
  }
  bot.sendMessage(chat_id, msg, "");
  delay(10);
  // Stop MOTOR1 and MOTOR2
  Motor.stop(MOTOR1);
  Motor.stop(MOTOR2);
  movStatus = 0;
  bot.sendMessage(chat_id, "Stopped", "");
  delay(10);
}

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));

  /*
    for (int i = 0; i < numNewMessages; i++) {
      String chat_id = String(bot.messages[i].chat_id);
      String text = bot.messages[i].text;

      String from_name = bot.messages[i].from_name;
      if (from_name == "") from_name = "Guest";

      if (text == "/forward") {
        forward(chat_id);
      }
      else if (text == "/backward") {
        backward(chat_id);
      }

      else if (text == "/status") {
        switch (movStatus) {
          case 0:
            bot.sendMessage(chat_id, "Stopped", "");
            break;
          case 1:
            bot.sendMessage(chat_id, "Moving forward", "");
            break;
          case 2:
            bot.sendMessage(chat_id, "Moving backward", "");
            break;
        }
      }

      if (text == "/start") {
        String welcome = "Hi " + from_name + ", I'm Jade the cactus bot!\n";
        welcome += "Below are the available commands:\n\n";
        welcome += "/forward : Move forward for 2s\n";
        welcome += "/backward : Move backward for 2s\n";
        welcome += "/status : Returns current status of Jade\n";
        bot.sendMessage(chat_id, welcome, "Markdown");
      }
    }
  */
  int i{numNewMessages - 1};
  String chat_id{String(bot.messages[i].chat_id)};
  String text{bot.messages[i].text};

  String from_name{bot.messages[i].from_name};
  if (from_name == "") from_name = "Guest";

  if (text == "/forward") {
    movBot(chat_id, true);
  }
  else if (text == "/backward") {
    movBot(chat_id, false);
  }
  else if (text == "/status") {
    getStatus(chat_id, movStatus);
  }

  if (text == "/start") {
    String welcome = "Hi " + from_name + ", I'm Jade the cactus bot!\n";
    welcome += "Below are the available commands:\n\n";
    welcome += "/forward : Move forward for 2s\n";
    welcome += "/backward : Move backward for 2s\n";
    welcome += "/status : Returns current status of Jade\n";
    bot.sendMessage(chat_id, welcome, "Markdown");
  }
}


void getStatus(String chat_id, int movStatus) {
  String sendMsg{};
  switch (movStatus) {
    case 0:
      sendMsg = "Stopped";
      break;
    case 1:
      sendMsg = "Moving forward";
      break;
    case 2:
      sendMsg =  "Moving backward";
      break;
  }
  bot.sendMessage(chat_id, sendMsg, "");

  //Light sensor
  sendMsg = mergeData(false);
  bot.sendMessage(chat_id, sendMsg, "");
}

String mergeData(bool raw) {
  String mg{};
  //sENSOR VALUE
  int l_l{analogRead(LIGHT_L)};
  int l_r{analogRead(LIGHT_R)};
  int mst{analogRead(MOIST_S)};
  if (!raw) {
    mg = "Light L: " + String(l_l) + "\nLight R: " + String(l_r) + "\nMoisture: " + String(mst);
  } else {
    mg = String(l_l) + "+" + String(l_r) + "+" + String(mst);
  }
  Serial.println(mg);
  return mg;
}

void setup() {
  //Init sensors
  //Lght sensor
  pinMode(LIGHT_L, INPUT);
  pinMode(LIGHT_R, INPUT);
  //Moisture sensor
  pinMode(MOIST_S, INPUT);

  //Init motors
  Motor.begin(I2C_ADDRESS);
  //WiFi
  Serial.begin(115200);
  // Attempt to connect to Wifi network:
  Serial.print("Connecting Wifi: ");
  Serial.println(ssid);

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages{bot.getUpdates(bot.last_message_received + 1)};
    while (numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    Bot_lasttime = millis();
  }
  sendGetJSON();
  Serial.println(getDist());
}

long getDist() {
  long duration, inches, cm;
  pinMode(pingPin, OUTPUT);
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  pinMode(echoPin, INPUT);
  duration = pulseIn(echoPin, HIGH);
  cm = microsecondsToCentimeters(duration);
  Serial.print(cm);
  Serial.println("cm");
  delay(100);
  return cm;
}
long microsecondsToCentimeters(long microseconds) {
  return microseconds / 29 / 2;
}

void sendGetJSON() {
  String payload = "https://dry-meadow-15993.herokuapp.com/plants/update?data=" + mergeData(true);
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;
    // http.begin("https://api.telegram.org/bot991983428:AAFhvCLtXGdw4IR5rf9kGOBq7p7gs5VQAXc/");  //Specify destination for HTTP request
    http.begin(payload);  //Specify destination for HTTP request
    //http.addHeader("Content-Type", "text/plain");             //Specify content-type header
    int httpResponseCode = http.GET();   //Send the actual POST

    http.end();  //Free resources
  } else {
    Serial.println("Error in WiFi connection");
  }
}

void postJSON() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;
    // http.begin("https://api.telegram.org/bot991983428:AAFhvCLtXGdw4IR5rf9kGOBq7p7gs5VQAXc/");  //Specify destination for HTTP request
    http.begin("https://dry-meadow-15993.herokuapp.com/plants/update");  //Specify destination for HTTP request
    http.addHeader("Content-Type", "text/plain");             //Specify content-type header
    int httpResponseCode = http.POST(mergeData(true));   //Send the actual POST

    http.end();  //Free resources
  } else {
    Serial.println("Error in WiFi connection");
  }
}

void post() {
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    HTTPClient http;
    http.begin("https://api.telegram.org/bot991983428:AAFhvCLtXGdw4IR5rf9kGOBq7p7gs5VQAXc/");  //Specify destination for HTTP request
    http.addHeader("Content-Type", "text/plain");             //Specify content-type header
    int httpResponseCode = http.POST("Hellow from ESP32");   //Send the actual POST

    if (httpResponseCode > 0) {
      String response = http.getString();                       //Get the response to the request
      Serial.println(httpResponseCode);   //Print return code
      Serial.println(response);           //Print request answer
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();  //Free resources
  } else {
    Serial.println("Error in WiFi connection");
  }
}
