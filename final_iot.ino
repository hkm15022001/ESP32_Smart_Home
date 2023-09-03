#include <WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>
#include <DHT.h>
#include <ArduinoJson.h>

#define ssid "Minh10"
#define password "12341234"

// Thông tin về MQTT Broker
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USER "smart_home"
#define MQTT_PASSWORD "123456"
#define MQTT_TOPIC_PUB "smart_home_humidity_and_temperature"
#define MQTT_TOPIC_SUB "smart_home_control_device"

#define ledPin2 21  // living room
#define fan 19  // living room
#define ledPin3 16  // living room


#define ledPin5 17  // bed room


const long interval = 5000;
StaticJsonDocument<200> mess_subcribe;
StaticJsonDocument<200> mess_publish;

//Khai báo chân của cảm biến nhiệt độ
#define DHTPIN 25
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

Servo myServo;  // khai báo đối tượng servo
#define SERVOPIN 26
int pos;  // góc hiện tại của servo

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  //set up wifi
  setup_wifi();

  dht.begin();               //khỏi động cảm biến đo nhiệt độ độ ẩm
  pinMode(ledPin2, OUTPUT);  // Khai báo đèn id 1
  pinMode(fan, OUTPUT);  // Khai báo đèn id 2
  pinMode(ledPin3, OUTPUT);  // Khai báo đèn id 1
  pinMode(ledPin5, OUTPUT);  // Khai báo đèn id 2
  myServo.attach(SERVOPIN);  // khai báo chấn điều khiển servo

  //set up pubsub
  client.setServer(MQTT_SERVER, MQTT_PORT);
  client.setCallback(callback);

  digitalWrite(ledPin2, LOW);
  digitalWrite(fan, LOW);

  delay(10);
}

// Hàm kết nối wifi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Hàm call back để nhận dữ liệu
void callback(char *topic, byte *payload, unsigned int length) {
  Serial.println("------- New message from broker -----");
  Serial.print("topic: ");
  Serial.println(topic);
  Serial.print("message: ");
  Serial.println();
  char *messageTemp;
  messageTemp = (char *)malloc((length + 1) * sizeof(char));
  memset(messageTemp, 0, length + 1);
  memcpy(messageTemp, payload, length);
  messageTemp[length] = '\0';
  Serial.println(messageTemp);

  // thực hiện chuyển từ string sang JSON
  deserializeJson(mess_subcribe, messageTemp);
  try {
    if (strcmp(mess_subcribe["deviceId"], "17") == 0) {
      Serial.println("door");
      if (strcmp(mess_subcribe["status"], "on") == 0) {
        Serial.println("on");
        myServo.write(150);
      } else if (strcmp(mess_subcribe["status"], "off") == 0) {
        Serial.println("off");
        myServo.write(60);
      }
    } else if (strcmp(mess_subcribe["deviceId"], "2") == 0) {
      if (strcmp(mess_subcribe["status"], "on") == 0) {
        Serial.println("on ledPin2");
        digitalWrite(ledPin2, HIGH);
      } else if (strcmp(mess_subcribe["status"], "off") == 0) {
        Serial.println("off ledPin2");
        digitalWrite(ledPin2, LOW);
      }
    } else if (strcmp(mess_subcribe["deviceId"], "3") == 0) {
      if (strcmp(mess_subcribe["status"], "on") == 0) {
        Serial.println("on led3");
        digitalWrite(ledPin3, HIGH);
      } else if (strcmp(mess_subcribe["status"], "off") == 0) {
        Serial.println("off led3");
        digitalWrite(ledPin3, LOW);
      }
    } else if (strcmp(mess_subcribe["deviceId"], "4") == 0) {
      if (strcmp(mess_subcribe["status"], "on") == 0) {
        Serial.println("on fan");
        digitalWrite(fan, HIGH);
      } else if (strcmp(mess_subcribe["status"], "off") == 0) {
        Serial.println("off fan");
        digitalWrite(fan, LOW);
      }
    } else if (strcmp(mess_subcribe["deviceId"], "5") == 0) {
      if (strcmp(mess_subcribe["status"], "on") == 0) {
        Serial.println("on led5");
        digitalWrite(ledPin5, HIGH);
      } else if (strcmp(mess_subcribe["status"], "off") == 0) {
        Serial.println("off led5");
        digitalWrite(ledPin5, LOW);
      }
    }  
    else {
      Serial.println("khong ton tai thiet bi");
    }
  } catch (const std::exception &e) {
    Serial.println("Invalid task.");
  }
  free(messageTemp);
}

// Hàm reconnect thực hiện kết nối lại khi mất kết nối với MQTT Broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "472e4164-ab08-431a-9078-8fde55eb6e7c";
    if (client.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("connected");
      client.subscribe(MQTT_TOPIC_SUB);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}

// Publish nhiệt độ, độ ẩm
void Publish() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    float h = dht.readHumidity();
    float t = dht.readTemperature();
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      delay(500);
      return;
    } else {
      char buffer[256];
      mess_publish["humidityAir"] = h;
      mess_publish["temperature"] = round(t);
      serializeJson(mess_publish, buffer);
      client.publish(MQTT_TOPIC_PUB, buffer);
      previousMillis = currentMillis;
    }
  }
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Publish();
}
