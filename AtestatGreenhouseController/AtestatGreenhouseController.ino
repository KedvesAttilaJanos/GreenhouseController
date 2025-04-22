//A használt könyvtárak beillesztése
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
//#include <avr/pgmspace.h>
#include <dht.h>
#include <SoftwareSerial.h>
#include <Vector.h>
#include <avr/wdt.h>

//SoftwareSerial espSerial(10, 11); //rx,tx
//Az eszközök csatlakoztatott tüskéi és az állandó értékek megadása, illetve a használandó objektumok deklarálása
//Temp and humidity
#define dht1 3
#define dht2 4
// A két DHT szenzor objektumok deklarálása
dht DHT1;
dht DHT2;
//Moisture sensor
#define MOISTURESENSOR A0
#define MINMOISTURE 500
#define MAXMOISTURE 180
//Output relays
#define FANPIN 7
#define IRRIGATIONPIN 8
#define SPRAYPIN 9
#define HEATERPIN 10
#define LIGHTPIN 6
// Az oled kijelző deklarálása i2c kommunikációval
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1 
#define SSD1306_I2C_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Joystick
#define JOYSTICK_X A1
#define JOYSTICK_Y A2
#define JOYSTICK_BUTTON 2
#define JOYSTICK_MAXTRESHOLD 900
#define JOYSTICK_MINTRESHOLD 100

#define MAX_MENU_COUNT 7 // a menupontok 0-tol vannak indexelve, igy ez a szam 1-el kissebb
#define MIN_CHANGE_SPEED 0
#define MAX_CHANGE_SPEED 100
#define SERIALPRINTINTERVALL 5000
#define DEBUG 0
#define NETWORKTYPE 0 //0-STA, 1 AP
#define SERIALNUMBER 123456
#define WIFINAME "DIGI_57c120"
#define PASSWORD "195da83e"
#define AITHINKER "AI-THINKER_394EDC"
// Bemeneti és kimeneti változók deklarálása:
  // Bemeneti változók (érzékelők adatai):
    byte insideTemperature, outsideTemperature, insideHumidity, outsideHumidity, soilMoistureLevel, lightIntensity; 
  // Kimeneti változók (vezérelt eszközök):
    enum DeviceState 
    {
      OFF,   // 0
      ON,    // 1
      AUTO   // 2
    };
    DeviceState fanState, irrigationState, sprayState, heaterState, lightState;

//Belső változók deklarálása:
  //wifi
  String ssid = String(WIFINAME);    // A wifi neve AI-THINKER_394EDC
  String password = String(PASSWORD);    // A wifi hálózat jelszava
  String ipAddress = ""; 
  int joystickAxisX, joystickAxisY; //A beviteli eszkoz (joystick) tengelyekre vonatkozo valtozoi 
  bool joystickButtonState;
  byte currentMenuNumber;
  byte currentSubmenuNumber;
  byte maxMenuCount;
  bool isSubMenu;
  long long lastTimeMenuRefresh;
  // Button lenyomas kozotti eltelt ido vizsgalata
  const byte menuChangeIntervall = 500; // menu/submenu valtozas
  long long lastTimeChange = 0;// Left/Right valtozas kozott eltelt ido vizsgalata
  long long lastTimeSerial = 0;
  const char deviceStateStrings[3][4] = {"OFF", "ON","AUTO"}; // kiirashoz a konnyebben erthetosegert kiirando szoveg
  //Min és max célváltozók deklarálása:
  byte maxTemp, minTemp, maxHumidity, minHumidity, maxMoisture, minMoisture, minLightLevel;
  const char* languages[3] = {"Eng","Hun","Ro"};
  byte languageSet; 
  bool enableSerial = DEBUG;
  bool networkType = NETWORKTYPE;
  byte changeSpeed = 20;
  byte connectionId;
//A fömenü értékeinek flash memóriában való tárolása
  const char menu1[] PROGMEM = "Device Owerview";
  const char menu2[] PROGMEM = "Sensor Overview";
  const char menu3[] PROGMEM = "Temperature Controll";
  const char menu4[] PROGMEM = "Humidity Controll";
  const char menu5[] PROGMEM = "Irrigation Controll";
  const char menu6[] PROGMEM = "Light Controll";
  const char menu7[] PROGMEM = "System Settings";
  const char menu8[] PROGMEM = "Network Settings";
  const char* const messages[] PROGMEM = {menu1,menu2,menu3,menu4,menu5,menu6,menu7,menu8};

void readSensor()//A szenzorok által mért értékek olvasása
{
  int insideSensor = DHT1.read11(dht1); 
  insideTemperature = DHT1.temperature;
  insideHumidity = DHT1.humidity;
  int outsideSensor = DHT2.read11(dht2);
  outsideTemperature = DHT2.temperature;
  outsideHumidity = DHT2.humidity;
  soilMoistureLevel = map(analogRead(MOISTURESENSOR), MINMOISTURE, MAXMOISTURE, 0, 100);
}
//Fuggvények
void restartArduino() // az arduino ujraindíndítása
{
  wdt_enable(WDTO_15MS); // Watchdog aktiválása 15ms időre
  while (1);             // Várakozás az újraindításra/Lefagyasztjuk a programot
}
void espSetup() // az esp inicializálása
{    
  sendData("AT+RST\r\n",2000,enableSerial); // reset module
  if(networkType)
  {
    ssid = String(AITHINKER);
    password = "-";
    sendData("AT+CWMODE=2\r\n",1000,enableSerial); // configure as access point
  }
  else
  {
    ssid = String(WIFINAME);
    password = String(PASSWORD);
  delay(1000); // kis extra várakozás
  //configure as station mode, connect to wifi
  sendData("AT+CWMODE=1\r\n", 1000, enableSerial);
  // Connect to wifi (ssid+password)
  String connectCommand = "AT+CWJAP=\"" + ssid + "\",\"" + password + "\"\r\n";
  sendData(connectCommand, 8000, enableSerial);
  } 
  sendData("AT+CIPMUX=1\r\n",1000,enableSerial); // configure for multiple connections
  sendData("AT+CIPSERVER=1,80\r\n",1000,enableSerial); // turn on server on port 80
}
void getIp() // Az ip cím lekérdezése
{ 
  delay(1000);
  // IP cím lekérése
  ipAddress = sendData("AT+CIFSR\r\n", 1000, enableSerial);
  byte first = ipAddress.indexOf('"');
  byte second = ipAddress.indexOf('"', first + 1);
  if (first != -1 && second != -1) 
  {
  ipAddress = ipAddress.substring(first + 1, second);
  } else 
  {
    Serial.println("ip error");
  }
}
String booleanStateOnOff(bool state) // bool stringként való visszatérítése kiiratás céljából
{
  if(state)
    return "On";
  else
    return "Off";
}
String sendData(String command, const int timeout, boolean debug) //Parancsok küldése az esp-nek, és a válaszának fogadása
{
    String response = "";
    Serial1.print(command);
    long int time = millis();
    while( (time+timeout) > millis())
    {
      while(Serial1.available())
        {
            char c = Serial1.read(); // read the next character.
            response+=c;
        }  
    }
    if(debug)
          {
          Serial.print(response); //displays the esp response messages in arduino Serial monitor
          }
    return response;
}
void espsend(String date) //Szoveg parancsának felépitése és elkuldése
{
  String cipSend = " AT+CIPSEND=";
  cipSend += connectionId; 
  cipSend += ",";
  cipSend +=date.length();
  cipSend +="\r\n";
  sendData(cipSend,1000,enableSerial);
  sendData(date,1000,enableSerial); 
}
void showMessageOnScreen(String message) // Szöveg megjelenítése a kijelző közepén 
{
  display.clearDisplay();
  display.setTextSize(1);      // Szöveg méretének meghatározása
  display.setTextColor(WHITE);
  display.setCursor(0, 10);   // Szöveg pozíciójának megadása
  display.println(message);
  display.display();
}
void transmitDateOnEsp(String data)// Az esp-vel való kommunikáció kezelése
{
  if(Serial1.available())
  {
    if(Serial1.find("+IPD,")) //kérés érkezésének figyelése
    {
     showMessageOnScreen("Network communication\nin progress...");
     delay(300);
     connectionId = Serial1.read()-48;
      String webpage = data;
      webpage+=connectionId;
      espsend(webpage);
     }
     String closeCommand = "AT+CIPCLOSE=";  ////////////////close the socket connection////esp command 
     closeCommand+=connectionId; // append connection id
     closeCommand+="\r\n";
     sendData(closeCommand,3000,enableSerial);
  }
}
String constructDateString() //A hálózatra kiküldött string felépítése a vezérlő adatai alapján
{
  String dataString = "InTemp=" + String(insideTemperature) + "&OutTemp=" + String(outsideTemperature) + "&InHum=" + String(insideHumidity) + "&OutHum=" + String(outsideHumidity) + "&Moi=" + String(soilMoistureLevel) + "&Light=" + String(lightIntensity) + "&maxTemp=" + String(maxTemp) + "&minTemp=" + String(minTemp) + "&maxHum=" + String(maxHumidity) + "&minHum=" + String(minHumidity) + "&maxMoi=" + String(maxMoisture) ;
  dataString +="&minMoi=" + String(minMoisture) + "&minLightLevel=" + String(minLightLevel)+"&fanState=" + String(fanState) + "&irrigationState=" + String(irrigationState) + "&sprayState=" + String(sprayState) + "&heaterState=" + String(heaterState) + "&lightState=" + String(lightState);
  String html = "<!DOCTYPE html><html><head><meta http-equiv=\\\"refresh\\\" content=\\\"5\\\"><title>Greenhouse" + String(SERIALNUMBER) + "</title></head><body>" + dataString + "</body></html>";

  return html;
}
void outRelays() // A rellé modulok be- és kikapcsolása az állapotuk alapjánminLightLevel
{
  if(heaterState == 2)
  {
    if(insideTemperature < minTemp)
      digitalWrite(HEATERPIN, HIGH);
    if(insideTemperature >(minTemp+maxTemp)/2)
      digitalWrite(HEATERPIN, LOW);
  } 
    if (heaterState == 1) 
      digitalWrite(HEATERPIN, HIGH);
    if (heaterState == 0)
      digitalWrite(HEATERPIN, LOW);
  if(fanState == 2)
  {
    if(insideTemperature > maxTemp)
      digitalWrite(FANPIN, HIGH);
    if(insideTemperature < (minTemp+maxTemp)/2)
      digitalWrite(FANPIN, LOW);
  }
  if(fanState == 1)
      digitalWrite(FANPIN, HIGH);
  if(fanState == 0)
      digitalWrite(FANPIN, LOW);
  if(irrigationState == 2)
  {
    if(soilMoistureLevel < minMoisture)
      digitalWrite(IRRIGATIONPIN, HIGH);
    if(soilMoistureLevel > maxMoisture)
      digitalWrite(IRRIGATIONPIN, LOW);
  }
  if(irrigationState == 1)
      digitalWrite(IRRIGATIONPIN, HIGH);
  if(irrigationState == 0)
      digitalWrite(IRRIGATIONPIN, LOW);
  if(lightState == 2)
  {
    if(lightIntensity < minLightLevel)
      digitalWrite(LIGHTPIN, HIGH);
    if(lightIntensity>=minLightLevel)
      digitalWrite(LIGHTPIN,LOW);
  }
  if(lightState == 1)
      digitalWrite(LIGHTPIN, HIGH);
  if(lightState == 0)
      digitalWrite(LIGHTPIN, LOW);
  switch (sprayState)
  {
    case 0:digitalWrite(SPRAYPIN, LOW);break;
    case 1:digitalWrite(SPRAYPIN, HIGH);break;
    case 2:
    if(insideHumidity < minHumidity)
      digitalWrite(SPRAYPIN, HIGH);
    if(insideHumidity > (minHumidity+maxHumidity)/2)
      digitalWrite(SPRAYPIN, LOW);
      break;
  }
  
}
short languageChooser (short currentLanguage)// A nyelvválasztó vátltozó változtatása
{
    if (joystickAxisY > JOYSTICK_MAXTRESHOLD)
  {
    currentLanguage ++;
    if(currentLanguage > 2)
      currentLanguage = 0;
  }
  else
  {
    if (joystickAxisY < JOYSTICK_MINTRESHOLD)
    {
      currentLanguage --;
      if(currentLanguage < 0)
        currentLanguage = 2;

    }

  }
  delay(changeSpeed+100);
  return currentLanguage;
}
void changeSubmenuBoolean(bool &booleanToChange)// Bool menüváltozó megváltoztatása, joystickAxisY alapján 
{
  if (joystickAxisY > JOYSTICK_MAXTRESHOLD || joystickAxisY < JOYSTICK_MINTRESHOLD)
  {
    booleanToChange = !booleanToChange;
  }
  delay(changeSpeed+30);
}
short changeSubmenuVariable(short numberToChange) // A számértékek változtatása az almenükben
{

  if (joystickAxisY > JOYSTICK_MAXTRESHOLD)
  {
    numberToChange++;
  }
  else
  {
    if (joystickAxisY < JOYSTICK_MINTRESHOLD)
    {
      numberToChange--;
    }
  }
  delay(changeSpeed+30);
  return numberToChange;
  
}
DeviceState changeDeviceState(DeviceState deviceState)// Az eszközok állapotának változtatása
{
  short counter = deviceState;
  
  if (joystickAxisY > JOYSTICK_MAXTRESHOLD)
  {
    counter ++;
    if(counter > 2)
      counter = 0;
  }
  else
  {
    if (joystickAxisY < JOYSTICK_MINTRESHOLD)
    {
      counter --;
      if(counter < 0)
        counter = 2;
    }
    else return static_cast<DeviceState>(counter);
  }
  delay(changeSpeed+100);
  return static_cast<DeviceState>(counter);
}
void changeMenuNumber(bool isSubMenu, byte maxNumber)// A menü és az almenü értékének növelése/csökkentése
{
  if(millis()-lastTimeMenuRefresh > changeSpeed+50)//Módosítások közötti idő vizsgálata, a megfelő működésért
  {
      lastTimeMenuRefresh = millis();
    if(isSubMenu) // dontés, hogy a főmenü, vagy az almenü értékét kell-e változtatni
    {
      if (joystickAxisX > JOYSTICK_MAXTRESHOLD)
      {
        if(maxNumber > currentSubmenuNumber)
          currentSubmenuNumber++;
        else 
          currentSubmenuNumber = 0;
        
      }
    else
    {
      if (joystickAxisX < JOYSTICK_MINTRESHOLD)
      {
        if(currentSubmenuNumber > 0)
          currentSubmenuNumber--;
        else
        currentSubmenuNumber = maxNumber;
        
      }     
    }
    }
    else 
    {
        if (joystickAxisX > JOYSTICK_MAXTRESHOLD)
      {
        if(maxNumber > currentMenuNumber)
          currentMenuNumber++;
        else 
          currentMenuNumber = 0;
        
      }
    else
    {
      if (joystickAxisX < JOYSTICK_MINTRESHOLD)
      {
        if(currentMenuNumber > 0)
          currentMenuNumber--;
        else
        currentMenuNumber = maxNumber;
        
      }     
    }
    }
  }
}
void readJoystickValues()// A joystick modul állapotának olvasása
{
  joystickAxisX = 1024-analogRead(JOYSTICK_X);
  joystickAxisY = analogRead(JOYSTICK_Y);
  joystickButtonState = !digitalRead(JOYSTICK_BUTTON);
}
void serialMonitorPrint() // Adatok kikuldese sorosan tesytelés céljából
  { 
    if(millis()-lastTimeSerial > SERIALPRINTINTERVALL)
    {
      lastTimeSerial = millis();
      Serial.println();
      Serial.println(millis());
      Serial.print(F("Kimeneti változók:")); // Az F() szükséges, ugyanis az SRAM korlatozott, es igy a Flash memoriat terheli
      Serial.print(F(" fanState: "));
      Serial.print(fanState);
      Serial.print(F(" irrigationState: "));
      Serial.print(irrigationState);
      Serial.print(F(" sprayState: "));
      Serial.print(sprayState);
      Serial.print(F(" fanState: "));
      Serial.print(fanState);
      Serial.print(F(" lightState: "));
      Serial.println(lightState);
      Serial.println();
      Serial.print(F("Szenzor értékek: "));
      Serial.print(F(" insideTemperature: "));
      Serial.print(insideTemperature);
      Serial.print(F(" outsideTemperature: "));
      Serial.print(outsideTemperature);
      Serial.print(F(" insideHumidity: "));
      Serial.print(insideHumidity);
      Serial.print(F(" outsideHumidity: "));
      Serial.print(outsideHumidity);
      Serial.print(F(" soilMoistureLevel: "));
      Serial.print(soilMoistureLevel);
      Serial.print(F(" lightIntensity: "));
      Serial.println(lightIntensity);
      Serial.println();
      Serial.print(F("Joystick értékek: "));
      Serial.print(F(" joystickAxisX: "));
      Serial.print(joystickAxisX);
      Serial.print(F(" joystickAxisY: "));
      Serial.print(joystickAxisY);
      Serial.print(F(" joystickButtonState: "));
      Serial.println(joystickButtonState);
      Serial.println();
      Serial.print(F("Menu value: "));
      Serial.print(currentMenuNumber);
      Serial.print(F("Submenu value: "));
      Serial.print(currentSubmenuNumber);
      Serial.print(F("isSubmenu: "));
      Serial.print(isSubMenu);
      Serial.println("\n-------------------------------------");
      
    }
    
  }
void killAllDevices()//Minden eszköz OFF-ra állítása, és minden rellélekapcsolása
{
  //az eszkök állapotát off-ra állítani
  fanState = OFF;
  irrigationState = OFF;
  sprayState = OFF;
  heaterState = OFF;
  lightState = OFF;
  //megadni a parancsot a relléknek
  digitalWrite(FANPIN, LOW);
  digitalWrite(IRRIGATIONPIN, LOW);
  digitalWrite(SPRAYPIN, LOW);
  digitalWrite(HEATERPIN, LOW);
  digitalWrite(LIGHTPIN, LOW);
}
void displayInitialize()//Kezdőképernyő mutatása
{
  if(!display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Nem sikerült létrehozni a kapcsolatot, végtelen ciklus
  }
  display.clearDisplay();// Clear the buffer

  display.setTextSize(1);      // Szöveg méretének meghatározása
  display.setTextColor(WHITE);
  display.setCursor(0, 0);   // Szöveg pozíciójának megadása
  display.println(F("Kedves Attila Janos"));
  display.println("Github:");
  display.println(F("https://github.com/KedvesAttilaJanos"));
  display.display();
  delay(5000);
  showLoadingScreen();
}
void loaderSwitch(byte loadNum) // A betöltő képernyőnek megfelelően, a setup feladatok végrehalytási sorrendjének meghatározása
{
  switch(loadNum)
  {
    case 30: espSetup(); Serial.println(F("Esp setuped")); break;
    case 50: getIp(); Serial.println(F("Ip get")); break;
    case 60: killAllDevices(); Serial.println(F("Devices off")); break;
    case 70: zeroAllSensor(); Serial.println(F("Sensors Zero")); break;
    case 80: zeroAllInternalVariable(); Serial.println(F("Internal variables zero")); break;
    case 90: readSensor(); Serial.println(F("First read")); break;
  }
}
bool joystickAxisYMoved(String direction) //overload, paraméterként megadható, hogy merre nézze a joystick elmozdulását
{
  if(direction == "right")
    return joystickAxisY > JOYSTICK_MAXTRESHOLD;
  if(direction == "left")
    return joystickAxisY < JOYSTICK_MINTRESHOLD;
}
bool joystickAxisYMoved() // A jobbra/balra történő elmozdulás figyelése
{
  return joystickAxisY < JOYSTICK_MINTRESHOLD || joystickAxisY > JOYSTICK_MAXTRESHOLD;
}
void showLoadingScreen()//Betöltőképernyő megjelenítése
{
  display.clearDisplay();
  display.setTextSize(1);      // Szöveg méretének meghatározása
  display.setTextColor(WHITE);
  display.setCursor(10, 10);   // Szöveg pozíciójának megadása
  display.println(F("Loading..."));
  display.display();
  
  // Animációs betöltési sáv
  for (int i = 0; i <= 100; i += 2) {
    display.fillRect(10, 30, i, 10, WHITE); // Sáv rajzolása
    loaderSwitch(i);
    display.display();
    delay(2); // Sáv frissítési sebessége
  }
  
  // Törlés vagy következő képernyő
  delay(500); 
  display.clearDisplay();
  display.display();
}
void zeroAllSensor()//Minden szenzorérték lenullázása
{
  insideTemperature = 0;
  outsideTemperature = 0;
  insideHumidity = 0;
  outsideHumidity = 0;
  soilMoistureLevel = 0;
  lightIntensity = 0; 
}
void zeroAllInternalVariable()// A belső vátozók alaphelyzetbe állítása
{
  joystickAxisX = 0;
  joystickAxisY = 0;
  joystickButtonState = 0;
  isSubMenu = false;
  currentMenuNumber = 0;
  currentSubmenuNumber = -1;
  lastTimeMenuRefresh = 0;
  maxMenuCount = MAX_MENU_COUNT;
  maxTemp = 0;
  minTemp = 0;
  maxHumidity = 0;
  minHumidity = 0;
  maxMoisture = 0;
  minMoisture = 0;
  minLightLevel = 0;
  languageSet = 0;
  enableSerial = DEBUG;
}
void printCentered(const String &text, int y)// A címek középreigazított kiírása
{
  int textWidth = text.length() * 6; // 6 pixel széles karakterek
  int x = (SCREEN_WIDTH - textWidth) / 2;
  display.setCursor(x, y);
  display.println(text);
}
void changeSelectedMenuVariable()// A kiválasztott értékek megváltoztatása
{
  if(isSubMenu)
  {
    switch (currentMenuNumber)
    {
      case 0:
        if (currentSubmenuNumber == 0) fanState = changeDeviceState(fanState);
        if (currentSubmenuNumber == 1) irrigationState = changeDeviceState(irrigationState);
        if (currentSubmenuNumber == 2) sprayState = changeDeviceState(sprayState);
        if (currentSubmenuNumber == 3) heaterState = changeDeviceState(heaterState);
        if (currentSubmenuNumber == 4) lightState = changeDeviceState(lightState);
      break; 
    }
  }
}
void mainMenuSystem()// A menük kiírása, döntésekkel, hogy az adott menüpontot háttérrel jelenítse meg
{
  display.setTextSize(1);
  display.clearDisplay();
  short devices[5] {fanState, irrigationState, sprayState, heaterState, lightState};
  if(!isSubMenu) //ellenörizzük, hogy almenüben, vagy főmnüben vagyunk-e
  {
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    char buffer[20]; // Ide töltjük a PROGMEM-ből
    for (size_t i = 0; i < 8; i++) 
    {
      strcpy_P(buffer, (char*)pgm_read_word(&(messages[i])));
      if (currentMenuNumber == i)
        display.setTextColor(BLACK,WHITE);
      else
        display.setTextColor(WHITE);
      display.println(buffer);
    }
      display.display();
    /*switch(currentMenuNumber)
    {
      case 0:
        display.setTextColor(BLACK,WHITE);
        display.println(F("Device Owerview"));
        display.setTextColor(WHITE);
        display.println(F("Sensor Overview"));
        display.println(F("Temperature Controll"));
        display.println(F("Humidity Controll"));
        display.println(F("Irrigation Controll"));
        display.println(F("Light Controll"));
        display.println(F("System Settings"));
        display.println(F("Network Settings"));
        display.display();
      break;
      case 1:
 
        display.println(F("Device Overview"));
        display.setTextColor(BLACK,WHITE);
        display.println(F("Sensor Overview"));
        display.setTextColor(WHITE);
        display.println(F("Temperature Controll"));
        display.println(F("Humidity Controll"));
        display.println(F("Irrigation Controll"));
        display.println(F("Light Controll"));
        display.println(F("System Settings"));
        display.println(F("Network Settings"));
        display.display();
      break;
      case 2: 
        display.println(F("Device Overview"));
        display.println(F("Sensor Overview"));
        display.setTextColor(BLACK,WHITE);
        display.println(F("Temperature Controll"));
        display.setTextColor(WHITE);
        display.println(F("Humidity Controll"));
        display.println(F("Irrigation Controll"));
        display.println(F("Light Controll"));
        display.println(F("System Settings"));
        display.println(F("Network Settings"));
        display.display();
      break;
      case 3:
        display.println(F("Device Overview"));
        display.println(F("Sensor Overview"));
        display.println(F("Temperature Controll"));
        display.setTextColor(BLACK,WHITE);
        display.println(F("Humidity Controll"));
        display.setTextColor(WHITE);
        display.println(F("Irrigation Controll"));
        display.println(F("Light Controll"));
        display.println(F("System Settings"));
        display.println(F("Network Settings"));
        display.display();
      break;
      case 4:
        display.println(F("Device Overview"));
        display.println(F("Sensor Overview"));
        display.println(F("Temperature Controll"));
        display.println(F("Humidity Controll"));
        display.setTextColor(BLACK,WHITE);
        display.println(F("Irrigation Controll"));
        display.setTextColor(WHITE);
        display.println(F("Light Controll"));
        display.println(F("System Settings"));
        display.println(F("Network Settings"));
        display.display();
      break;
      case 5:
        display.println(F("Device Overview"));
        display.println(F("Sensor Overview"));
        display.println(F("Temperature Controll"));
        display.println(F("Humidity Controll"));
        display.println(F("Irrigation Controll"));
        display.setTextColor(BLACK,WHITE);
        display.println(F("Light Controll"));
        display.setTextColor(WHITE);
        display.println(F("System Settings"));
        display.println(F("Network Settings"));
        display.display();
      break;
      case 6:
        display.println(F("Device Overview"));
        display.println(F("Sensor Overview"));
        display.println(F("Temperature Controll"));
        display.println(F("Humidity Controll"));
        display.println(F("Irrigation Controll"));
        display.println(F("Light Controll"));
        display.setTextColor(BLACK,WHITE);
        display.println(F("System Settings"));
        display.setTextColor(WHITE);
        display.println(F("Network Settings"));
        display.display();
      break;
      case 7:
        display.println(F("Device Overview"));
        display.println(F("Sensor Overview"));
        display.println(F("Temperature Controll"));
        display.println(F("Humidity Controll"));
        display.println(F("Irrigation Controll"));
        display.println(F("Light Controll"));
        display.println(F("System Settings"));
        display.setTextColor(BLACK,WHITE);
        display.println(F("Network Settings"));
        display.setTextColor(WHITE);
        display.display();
      break;
    }*/
  }
  else 
  {
    switch(currentMenuNumber)
    {
      case 0:
        maxMenuCount = 4;
        switch(currentSubmenuNumber)
        {
        case 0:
        printCentered(F("Device Overview"),0);
        display.setTextColor(BLACK,WHITE);
        display.print(F("Fan State        "));
        display.println(deviceStateStrings[fanState]);
        display.setTextColor(WHITE);
        display.print(F("Irrigation State "));
        display.println(deviceStateStrings[irrigationState]);
        display.print(F("Spray State      "));
        display.println(deviceStateStrings[sprayState]);
        display.print(F("Heater State     "));
        display.println(deviceStateStrings[heaterState]);
        display.print(F("Light State      "));
        display.println(deviceStateStrings[lightState]);
        display.display();
        fanState = changeDeviceState(fanState);
        break;
        case 1:
        printCentered(F("Device Overview"),0);
        display.print(F("Fan State        "));
        display.println(deviceStateStrings[fanState]);
        display.setTextColor(BLACK,WHITE);
        display.print(F("Irrigation State "));
        display.println(deviceStateStrings[irrigationState]);
        display.setTextColor(WHITE);
        display.print(F("Spray State      "));
        display.println(deviceStateStrings[sprayState]);
        display.print(F("Heater State     "));
        display.println(deviceStateStrings[heaterState]);
        display.print(F("Light State      "));
        display.println(deviceStateStrings[lightState]);
        display.display();
        irrigationState = changeDeviceState(irrigationState);
        break;
        case 2:
        printCentered(F("Device Overview"),0);
        display.print(F("Fan State        "));
        display.println(deviceStateStrings[fanState]);
        display.print(F("Irrigation State "));
        display.println(deviceStateStrings[irrigationState]);
        display.setTextColor(BLACK,WHITE);
        display.print(F("Spray State      "));
        display.println(deviceStateStrings[sprayState]);
        display.setTextColor(WHITE);
        display.print(F("Heater State     "));
        display.println(deviceStateStrings[heaterState]);
        display.print(F("Light State      "));
        display.println(deviceStateStrings[lightState]);
        display.display();
        sprayState = changeDeviceState(sprayState);
        break;
        case 3:
        printCentered(F("Device Overview"),0);
        display.print(F("Fan State        "));
        display.println(deviceStateStrings[fanState]);
        display.print(F("Irrigation State "));
        display.println(deviceStateStrings[irrigationState]);
        display.print(F("Spray State      "));
        display.println(deviceStateStrings[sprayState]);
        display.setTextColor(BLACK,WHITE);
        display.print(F("Heater State     "));
        display.println(deviceStateStrings[heaterState]);
        display.setTextColor(WHITE);
        display.print(F("Light State      "));
        display.println(deviceStateStrings[lightState]);
        display.display();
        heaterState = changeDeviceState(heaterState);
        break;
        case 4:
        printCentered(F("Device Overview"),0);
        display.print(F("Fan State        "));
        display.println(deviceStateStrings[fanState]);
        display.print(F("Irrigation State "));
        display.println(deviceStateStrings[irrigationState]);
        display.print(F("Spray State      "));
        display.println(deviceStateStrings[sprayState]);
        display.print(F("Heater State     "));
        display.println(deviceStateStrings[heaterState]);
        display.setTextColor(BLACK,WHITE);
        display.print(F("Light State      "));
        display.println(deviceStateStrings[lightState]);
        display.setTextColor(WHITE);
        display.display();
        lightState = changeDeviceState(lightState);
        break;
        
        }   
      break;
      case 1:
        printCentered(F("Sensor Overview "),0);
        display.print(F("Inside Temp.    "));
        display.println(insideTemperature);
        display.print(F("Outside Temp.   "));
        display.println(outsideTemperature);
        display.print(F("Inside Humidity "));
        display.println(insideHumidity);
        display.print(F("Outside Humidity"));
        display.println(outsideHumidity);
        display.print(F("Soil Moisture   "));
        display.println(soilMoistureLevel);
        display.print(F("Light Level     "));
        display.println(lightIntensity);
        display.display();
      break;
      case 2:
        maxMenuCount = 3;
        switch(currentSubmenuNumber)
        {
          case 0:
            printCentered(F("Temp Controll"),0);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Max Temp         "));
            display.println(maxTemp);
            display.setTextColor(WHITE);
            display.print(F("Min Temp         "));
            display.println(minTemp);
            display.print(F("Fan State        "));
            display.println(deviceStateStrings[fanState]);
            display.print(F("Heater State     "));
            display.println(deviceStateStrings[heaterState]);
            display.print(F("Inside Temp      "));
            display.println(insideTemperature);
            display.print(F("Outside Temp     "));
            display.println(outsideTemperature);
            display.display();
            maxTemp = changeSubmenuVariable(maxTemp);
          break;
          case 1:
            printCentered(F("Temp Controll"),0);
            display.print(F("Max Temp         "));
            display.println(maxTemp);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Min Temp         "));
            display.println(minTemp);
            display.setTextColor(WHITE);
            display.print(F("Fan State        "));
            display.println(deviceStateStrings[fanState]);
            display.print(F("Heater State     "));
            display.println(deviceStateStrings[heaterState]);
            display.print(F("Inside Temp      "));
            display.println(insideTemperature);
            display.print(F("Outside Temp     "));
            display.println(outsideTemperature);
            display.display();
            minTemp = changeSubmenuVariable(minTemp);
          break;
          case 2:
            printCentered(F("Temp Controll"),0);
            display.print(F("Max Temp         "));
            display.println(maxTemp);
            display.print(F("Min Temp         "));
            display.println(minTemp);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Fan State        "));
            display.println(deviceStateStrings[fanState]);
            display.setTextColor(WHITE);
            display.print(F("Heater State     "));
            display.println(deviceStateStrings[heaterState]);
            display.print(F("Inside Temp      "));
            display.println(insideTemperature);
            display.print(F("Outside Temp     "));
            display.println(outsideTemperature);
            display.display();
            fanState = changeDeviceState(fanState);
          break;
          case 3:
            printCentered(F("Temp Controll"),0);
            display.print(F("Max Temp         "));
            display.println(maxTemp);
            display.print(F("Min Temp         "));
            display.println(minTemp);
            display.print(F("Fan State        "));
            display.println(deviceStateStrings[fanState]);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Heater State     "));
            display.println(deviceStateStrings[heaterState]);
            display.setTextColor(WHITE);
            display.print(F("Inside Temp      "));
            display.println(insideTemperature);
            display.print(F("Outside Temp     "));
            display.println(outsideTemperature);
            display.display();
            heaterState = changeDeviceState(heaterState);
          break;
        }

      break;
      case 3:
        maxMenuCount = 2;
        switch(currentSubmenuNumber)
        {
          case 0:
            printCentered(F("Humidity Controll"),0);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Max Humidity     "));
            display.println(maxHumidity);
            display.setTextColor(WHITE);
            display.print(F("Min Humidity     "));
            display.println(minHumidity);
            display.print(F("Spray State      "));
            display.println(deviceStateStrings[sprayState]);
            display.print(F("Inside Humidity  "));
            display.println(insideHumidity);
            display.print(F("Outside Humidity "));
            display.println(outsideHumidity);
            display.display();
            maxHumidity = changeSubmenuVariable(maxHumidity);
          break;
          case 1:
            printCentered(F("Humidity Controll"),0);
            display.print(F("Max Humidity     "));
            display.println(maxHumidity);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Min Humidity     "));
            display.println(minHumidity);
            display.setTextColor(WHITE);
            display.print(F("Spray State      "));
            display.println(deviceStateStrings[sprayState]);
            display.print(F("Inside Humidity  "));
            display.println(insideHumidity);
            display.print(F("Outside Humidity "));
            display.println(outsideHumidity);
            display.display();
            minHumidity = changeSubmenuVariable(minHumidity);
          break;
          case 2:
            printCentered(F("Humidity Controll"),0);
            display.print(F("Max Humidity     "));
            display.println(maxHumidity);
            display.print(F("Min Humidity     "));
            display.println(minHumidity);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Spray State      "));
            display.println(deviceStateStrings[sprayState]);
            display.setTextColor(WHITE);
            display.print(F("Inside Humidity  "));
            display.println(insideHumidity);
            display.print(F("Outside Humidity "));
            display.println(outsideHumidity);
            display.display();
            sprayState = changeDeviceState(sprayState);
          break;
        }
      break;
      case 4:
        maxMenuCount = 2;
        switch(currentSubmenuNumber)
        {
          case 0:
            printCentered(F("Irrigation Controll"),0);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Max Moisture     "));
            display.println(maxMoisture);
            display.setTextColor(WHITE);
            display.print(F("Min Moisture     "));
            display.println(minMoisture);
            display.print(F("Irrigation State "));
            display.println(deviceStateStrings[irrigationState]);
            display.print(F("Moisture Level   "));
            display.println(soilMoistureLevel);
            display.display();
            maxMoisture = changeSubmenuVariable(maxMoisture);
          break;
          case 1:
            printCentered(F("Irrigation Controll"),0);
            display.print(F("Max Moisture     "));
            display.println(maxMoisture);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Min Moisture     "));
            display.println(minMoisture);
            display.setTextColor(WHITE);
            display.print(F("Irrigation State "));
            display.println(deviceStateStrings[irrigationState]);
            display.print(F("Moisture Level   "));
            display.println(soilMoistureLevel);
            display.display();
            minMoisture = changeSubmenuVariable(minMoisture);
          break;
          case 2:
            printCentered(F("Irrigation Controll"),0);
            display.print(F("Max Moisture     "));
            display.println(maxMoisture);
            display.print(F("Min Moisture     "));
            display.println(minMoisture);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Irrigation State "));
            display.println(deviceStateStrings[irrigationState]);
            display.setTextColor(WHITE);
            display.print(F("Moisture Level   "));
            display.println(soilMoistureLevel);
            display.display();
            irrigationState = changeDeviceState(irrigationState);
          break;
        }
      break;
      case 5:
        maxMenuCount = 1;
        switch(currentSubmenuNumber)
        {
          case 0:
            printCentered(F("Light Controll"),0);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Min Light Level: "));
            display.println(minLightLevel);
            display.setTextColor(WHITE);
            display.print(F("Ligth State      "));
            display.println(deviceStateStrings[lightState]);
            display.print(F("Ligth Intensity  "));
            display.println(lightIntensity);
            display.display();
            minLightLevel = changeSubmenuVariable(minLightLevel);
          break;
          case 1:
            printCentered(F("Light Controll"),0);
            display.print(F("Min Light Level: "));
            display.println(minLightLevel);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Ligth State      "));
            display.println(deviceStateStrings[lightState]);
            display.setTextColor(WHITE);
            display.print(F("Ligth Intensity  "));
            display.println(lightIntensity);
            display.display();
            lightState = changeDeviceState(lightState);
          break;
          
        }
      break;
      case 6:
        maxMenuCount = 5;
        switch(currentSubmenuNumber)
        {
          case 0:
            printCentered(F("System Setting"),0);
            display.setTextColor(BLACK,WHITE);
            display.println(F("Restart Sensors "));
            display.setTextColor(WHITE);
            display.println(F("Restart Devices "));
            display.print(F("Language          "));
            display.println(languages[languageSet]);
            display.print(F("Enable Serial     "));
            display.println(booleanStateOnOff(enableSerial));
            display.print(F("Change Slowenes   "));
            display.println(changeSpeed);
            display.println(F("Restart Arduino   "));
            display.print(F("Name: "));
            display.print(SERIALNUMBER);
            display.display();
            if(joystickAxisYMoved())
            { 
              zeroAllSensor();
              display.clearDisplay();
              printCentered(F("Sensors Restarted"),16);
              display.display();
              delay(500);
            }
          break;
          case 1:
            printCentered(F("System Setting"),0);
            display.println(F("Restart Sensors "));
            display.setTextColor(BLACK,WHITE);
            display.println(F("Restart Devices "));
            display.setTextColor(WHITE);
            display.print(F("Language          "));
            display.println(languages[languageSet]);
            display.print(F("Enable Serial     "));
            display.println(booleanStateOnOff(enableSerial));
            display.print(F("Change Slowenes   "));
            display.println(changeSpeed);
            display.println(F("Restart Arduino   "));
            display.print(F("Name: "));
            display.print(SERIALNUMBER);
            display.display();
            if(joystickAxisYMoved("left"))
            { 
              killAllDevices();
              display.clearDisplay();
              printCentered(F("Devices Restarted"),16);
              display.display();
              delay(500);
            }
          break;
          case 2:
            printCentered(F("System Setting"),0);
            display.println(F("Restart Sensors "));
            display.println(F("Restart Devices "));
            display.setTextColor(BLACK,WHITE);
            display.print(F("Language          "));
            display.println(languages[languageSet]);
            display.setTextColor(WHITE);
            display.print(F("Enable Serial     "));
            display.println(booleanStateOnOff(enableSerial));
            display.print(F("Change Slowenes   "));
            display.println(changeSpeed);
            display.println(F("Restart Arduino   "));
            display.print(F("Name: "));
            display.print(SERIALNUMBER);
            display.display();
            languageSet = languageChooser(languageSet);
          break;
          case 3:
            printCentered(F("System Setting"),0);
            display.println(F("Restart Sensors "));
            display.println(F("Restart Devices "));
            display.print(F("Language          "));
            display.println(languages[languageSet]);
            display.setTextColor(BLACK,WHITE);
            display.print(F("Enable Serial     "));
            display.println(booleanStateOnOff(enableSerial));
            display.setTextColor(WHITE);
            display.print(F("Change Slowenes   "));
            display.println(changeSpeed);
            display.println(F("Restart Arduino   "));
            display.print(F("Name: "));
            display.print(SERIALNUMBER);
            display.display();
            changeSubmenuBoolean(enableSerial);
          break;
          case 4:
            printCentered(F("System Setting"),0);
            display.println(F("Restart Sensors "));
            display.println(F("Restart Devices "));
            display.print(F("Language          "));
            display.println(languages[languageSet]);
            display.print(F("Enable Serial     "));
            display.println(booleanStateOnOff(enableSerial));
            display.setTextColor(BLACK,WHITE);
            display.print(F("Change Slowenes   "));
            display.println(changeSpeed);
            display.setTextColor(WHITE);
            display.println(F("Restart Arduino   "));
            display.print(F("Name: "));
            display.print(SERIALNUMBER);
            display.display();
            changeSpeed = changeSubmenuVariable(changeSpeed);
            if(changeSpeed <= MIN_CHANGE_SPEED)
            {
              changeSpeed = MAX_CHANGE_SPEED;
            }
            else if(changeSpeed > MAX_CHANGE_SPEED)
              {
              changeSpeed = MIN_CHANGE_SPEED;
              }
          break;
          case 5:
            printCentered(F("System Setting"),0);
            display.println(F("Restart Sensors "));
            display.println(F("Restart Devices "));
            display.print(F("Language          "));
            display.println(languages[languageSet]);
            display.print(F("Enable Serial     "));
            display.println(booleanStateOnOff(enableSerial));
            display.print(F("Change Slowenes   "));
            display.println(changeSpeed);
            display.setTextColor(BLACK,WHITE);
            display.println(F("Restart Arduino   "));
            display.setTextColor(WHITE);
            display.print(F("Name: "));
            display.print(SERIALNUMBER);
            display.display();
            if(joystickAxisYMoved())
            {
              showMessageOnScreen("Arduino restarting");
              restartArduino();
            }
              
          break;
        }
      break;
      case 7:
        maxMenuCount = 1;
        printCentered(F("Network Settings"),0);
        switch (currentSubmenuNumber)
        {
          case 0:
            display.setTextColor(BLACK,WHITE);
            display.println(F("Left for reset"));
            if(joystickAxisY > JOYSTICK_MAXTRESHOLD)
            {
              display.clearDisplay();
              printCentered(F("Network restarting"),16);
              display.display();
              espSetup();
              getIp();
              display.clearDisplay();
              printCentered(F("Network restarted"),16);
              display.display();
              Serial.println(F("Esp restarted"));
              delay(500);
            }
            display.setTextColor(WHITE);
            display.print("Operation mode: ");
            if(networkType)
              display.println("AP");
            else
              display.println("STA");
          break; 
          
          case 1:
            display.setTextColor(WHITE);
            display.println(F("Left for reset"));
            display.setTextColor(BLACK,WHITE);
            display.print("Operation mode: ");
            changeSubmenuBoolean(networkType);
            if(networkType)
              display.println("AP");
            else
              display.println("STA");
              display.setTextColor(WHITE);            
          break;

        }
        display.print(F("Network Name: "));
          display.println(ssid);
          display.print(F("Password: "));
          display.println(password);
          display.print(F("Ip address: "));
          display.println(ipAddress);
          display.display();
      break;
    }
    
  }
  
  
}
void setup() // az indításkor fut le
{
  // Setup resz, az inditaskor egyszer fut le:
  Serial.begin(9600); //Soros Kommunikacio a teszteleshez
  Serial1.begin(115200);
  //espSetup();
  //espSerial.begin(9600);
  //meghívjuk a létrehozott függvényeket, amik az első lefutásnál kell végrehajtódjanak
  //killAllDevices();
  //zeroAllSensor();
  displayInitialize();
  //zeroAllInternalVariable();
  //megadjuk a, hogy a tüskék funkcióját
  pinMode(JOYSTICK_BUTTON, INPUT_PULLUP);
  pinMode(FANPIN, OUTPUT);
  pinMode(IRRIGATIONPIN, OUTPUT); 
  pinMode(SPRAYPIN, OUTPUT);     
  pinMode(HEATERPIN, OUTPUT); 
  pinMode(LIGHTPIN, OUTPUT);

}

void loop() {

  // put your main code here, to run repeatedly:
  readJoystickValues();
  if(enableSerial) //csak akkor küldi ki sorosan az adatokat, ha kap bemenetet sorosan, igy nem vész el foloslegesen erőforrás
    serialMonitorPrint();
  changeMenuNumber(isSubMenu,maxMenuCount);
  mainMenuSystem();
  if(joystickButtonState == true) //isSubMenu == false && 
  {
    if(millis()-lastTimeChange > menuChangeIntervall)
    {
      isSubMenu = !isSubMenu;
      maxMenuCount = MAX_MENU_COUNT;
      currentSubmenuNumber = 0;
      lastTimeChange = millis();
    }
      
  }
  transmitDateOnEsp(constructDateString());
  readSensor();
  outRelays();
  display.clearDisplay();
}
