#include <SPI.h>
#include <Ethernet2.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <DHT_U.h>


// opis pinów:
// 29 okno salon L
// 28 okno salon P
// 27 okno łazienka góra
//  okno kuchnia L
//  okno kuchnia P
//  okno poczekalnia
//  okno gabinet
//  okno garaż
//  okno kotłownia
//  okno schody
//  okno garderoba
// 26 drzwi taras sypialnia 
//  drzwi taras gościnny
//  okno gościnny L
//  okno gościnny P
//  okno olek P
//  okno olek L

const char* MQTTtopic = "arduino/kontaktrony/";
const char* nazwa[] = {"o_salon_L", "o_salon_P", "o_lazienka_gora", "d_sypialnia", "o_gabinet", "o_poczekalnia", "o_kuchnia_P", "o_kuchnia_L", "o_garaz"};
//char *tablica3 [] = {"o_salon_L", "o_salon_P", "o_lazienka_gora", "d_sypialnia", "o_gabinet", "o_poczekalnia", "o_kuchnia_P", "o_kuchnia_L", "o_garaz", "o_kotlownia", "o_schody", "o_garderoba", "d_goscinny", "o_goscinny_L", "o_goscinny_P", "o_olek_L", "o_olek_P"};
byte nr_pinu[] = {29, 28, 27, 26, 25, 24, 23, 22, 21};
byte stan_pinu[9] = {};
const int table_lenght = 9;

unsigned long aktualny_czas = 0;
unsigned long zapamietany_czas = 0;
unsigned long interwal_status = 60000UL;

//deklaracja sensora DHT22
#define DHT_lazienka_PIN 2  
#define DHTTYPE DHT22
DHT dht_lazienka(DHT_lazienka_PIN, DHTTYPE);
//float temp_lazienka_gora;
//float temp_lazienka_gora_before;
//float hum_lazienka_gora;
//float hum_lazienka_gora_before;
String temp_lazienka_gora;
String temp_lazienka_gora_before;
String hum_lazienka_gora;
String hum_lazienka_gora_before;

//#define
//oknoSalon 1;
//const int testSwitchPin = 5;
//byte testSwitchState;
//byte testSwitchStateNew;

//unsigned long readTime;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xAD };
IPAddress ip(192, 168, 1, 105);
IPAddress server(192, 168, 1, 100);
char message_buff[100];
EthernetClient ethClient;
PubSubClient client(ethClient);

void callback(char* topic, byte* message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("arduinoClient", "pi", "Lasica246"))
    {
      Serial.println("connected");
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
  Serial.begin(9600);
  client.setServer("192.168.1.100", 1883);
  client.setCallback(callback);
  Ethernet.begin(mac);
  dht_lazienka.begin();
  
  Serial.println(Ethernet.localIP());
  //readTime = 0;

  for (int i = 0; i < table_lenght; i++)
  {
    pinMode(nr_pinu[i], INPUT_PULLUP);
  }
  
  send_status_kontaktrony();

  temp_lazienka_gora_before = String(dht_lazienka.readTemperature(), 1);
  Serial.println(temp_lazienka_gora_before);
  hum_lazienka_gora_before = String(dht_lazienka.readHumidity(), 1);
  Serial.println(hum_lazienka_gora_before);
  send_status_DHT();

  //if (client.connect("arduinoClient", "pi", "Lasica246"))
  //{
  //  client.publish(MQTTtopic, "Hello!", true);
  //client.publish(test_topic, String(testSwitchState).c_str(), true);
  //}
}

bool checkBound(float newValue, float prevValue, float maxDiff)
{
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}


void loop()
{
  aktualny_czas = millis();

  if (!client.connected())
  {
    reconnect();
    send_status_kontaktrony();
    send_status_DHT();
    Serial.println("Send status after reconnect");
  }

  client.loop();

  for (int i = 0; i < table_lenght; i++)
  {
    if (digitalRead(nr_pinu[i]) != stan_pinu[i])
    {
      stan_pinu[i] = digitalRead(nr_pinu[i]);

      char topic_to_send[40];
      strcpy(topic_to_send, MQTTtopic);
      strcat(topic_to_send, nazwa[i]);

      client.publish(topic_to_send, String(stan_pinu[i]).c_str(), true);
      Serial.println(topic_to_send);
    }

    if (aktualny_czas - zapamietany_czas >= interwal_status)
    {
      zapamietany_czas = aktualny_czas;
      send_status_kontaktrony();
      send_status_DHT();
    }
  }
}

void send_status_kontaktrony()
{
  for (int i = 0; i < table_lenght; i++)
  {
    stan_pinu[i] = digitalRead(nr_pinu[i]);
    char topic_to_send[40];
    strcpy(topic_to_send, MQTTtopic);
    strcat(topic_to_send, nazwa[i]);
    client.publish(topic_to_send, String(stan_pinu[i]).c_str(), true);   
  }
  Serial.println("Send interval status");
}

//void send_status_DHT()
//{
//  temp_lazienka_gora = dht.readTemperature();
//  if (temp_lazienka_gora != temp_lazienka_gora_before)
//  {
//    client.publish("arduino/DHT/temp", String(temp_lazienka_gora).c_str(), true);
//    temp_lazienka_gora_before = temp_lazienka_gora;
//    Serial.println(temp_lazienka_gora);
//  }
//
//  hum_lazienka_gora = dht.readHumidity();
//  if (hum_lazienka_gora != hum_lazienka_gora_before)
//  {
//    client.publish("arduino/DHT/hum", String(hum_lazienka_gora).c_str(), true);
//    hum_lazienka_gora_before = hum_lazienka_gora;
//    Serial.println(hum_lazienka_gora);
//  }
//}

void send_status_DHT()
{
  temp_lazienka_gora = String(dht_lazienka.readTemperature(), 1);
  if (temp_lazienka_gora != temp_lazienka_gora_before)
  {
    client.publish("arduino/DHT/temp/lazienka_gora", String(temp_lazienka_gora).c_str(), true);
    temp_lazienka_gora_before = temp_lazienka_gora;
    Serial.print("Send temp_lazienka_gora: ");
    Serial.println(temp_lazienka_gora);
  }

  hum_lazienka_gora = String(dht_lazienka.readHumidity(), 1);
  if (hum_lazienka_gora != hum_lazienka_gora_before)
  {
    client.publish("arduino/DHT/hum/lazienka_gora", String(hum_lazienka_gora).c_str(), true);
    hum_lazienka_gora_before = hum_lazienka_gora;
    Serial.print("Send hum_lazienka_gora: ");
    Serial.println(hum_lazienka_gora);
  }
}
