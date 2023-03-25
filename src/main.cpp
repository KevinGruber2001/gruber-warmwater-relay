#include <Arduino.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#define RELAY 4

const char *ssid = "";
const char *password = "";

const char *mqtt_server = "";

// = RUNNING, STOPPING
const char *deviceTopic = "warmwasser/device";
// = STARTING
const char *clientTopic = "warmwasser/client";
const char *genTopic = "warmwasser";

const char *deviceId = "Device";

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
bool currentlyRunning = false;
int runningTimeStamp = 0;
// 7 min
int timeSpan = 7 * 60 * 1000;

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  Serial.println(messageTemp);

  Serial.println(topic);

  if (String(topic) == clientTopic)
  {
    if (messageTemp == "START" && !currentlyRunning)
    {
      client.publish(deviceTopic, "RUN", true);
    }
  }
  else if (String(topic) == deviceTopic)
  {
    if (messageTemp == "RUN" && !currentlyRunning)
    {
      Serial.println("Relay activates");
      digitalWrite(RELAY, LOW);
      currentlyRunning = true;
      runningTimeStamp = millis();
    }
    if (messageTemp == "STOP" && currentlyRunning)
    {
      Serial.println("Relay shout down");
      digitalWrite(RELAY, HIGH);
      currentlyRunning = false;
    }
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(deviceId))
    {
      Serial.println("connected");

      // Subscribe
      client.subscribe(clientTopic);
      client.subscribe(deviceTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  pinMode(RELAY, OUTPUT);
  Serial.begin(115200);

  digitalWrite(RELAY, HIGH);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  long now = millis();
  if (currentlyRunning && now - runningTimeStamp > timeSpan)
  {
    Serial.println("Relay gets shout down");
    digitalWrite(RELAY, HIGH);
    currentlyRunning = false;
    client.publish(deviceTopic, "STOP", true);
  }
  delay(1000);
}