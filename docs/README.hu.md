# 🌱 Okos Melegház Vezérlő (IoT Projekt)

Ez a projekt egy okos melegház vezérlő, amely Arduino és ESP8266 alapú, a különböző környezeti tényezők monitorozására és vezérlésére a melegházban. A rendszer több szenzortól gyűjt adatokat, és távolról, egy webes felületen keresztül figyelhető.

## Jellemzők
- **Szenzorok:**
  - DHT11 – Hőmérséklet- és páratartalom mérő
  - Talajnedvesség szenzor (analóg kimenet)
  - Fényérzékelő (LDR)
- **ESP8266 WiFi Modul** – Távoli monitorozás weboldalon keresztül.
- **OLED Képernyő** – Valós idejű adatok megjelenítése hőmérsékletről, páratartalomról, talajnedvességről és fényintenzitásról.
- **Joystick** – Manuális vezérlés a paraméterek, például öntözés vagy ventilátor sebességének beállításához.
- **Jövőbeli terveink** – CO2 monitorozási képességek hozzáadása.

📄 Dokumentáció  
A teljes projekt dokumentációja két nyelven (magyar és angol) elérhető a /docs mappában. Tartalmazza a hardver áramköri terveit, a rendszer áttekintését és a használati utasításokat.

## Alkatrészek
- **Mikrokontroller:**
  - Arduino (a szenzoradatok feldolgozásához és vezérléshez)
  - ESP8266 (WiFi és web szerverhez)

- **Szenzorok:**
  - DHT11 – A hőmérséklet és páratartalom méréséhez
  - Talajnedvesség szenzor – A talajnedvesség méréséhez
  - Fényérzékelő (LDR) – A fényintenzitás méréséhez

- **Képernyő:**
  - OLED SSD1306 – A szenzoradatok megjelenítéséhez

- **Joystick:**
  - Két tengelyes analóg joystick a manuális bemenethez

- **Könyvtárak:**
  - `#include <SPI.h>` – SPI kommunikációhoz
  - `#include <Wire.h>` – I2C kommunikációhoz
  - `#include <Adafruit_GFX.h>` – A képernyőn történő grafika megjelenítéséhez
  - `#include <dht.h>` – DHT11/DHT22 szenzor kezeléséhez

## PCB és Áramköri Tervek
A projekt tartalmaz egy egyedi PCB-t a melegház vezérlő számára. Az alábbiakban találhatóak a PCB fájlok és az áramköri terv:
- **PCB fájlok**: A PCB elrendezési fájlok (Gerber) a `/hardware/` mappában találhatók.
- **Áramköri diagram**: Az áramköri terv PDF formátumban elérhető: [PDF](docs/schematic.pdf) [SVG](Schematic.svg).

**Vezetékek és összeszerelés:**
- A vezetékek diagramja elérhető a `/docs/wiring_diagram.png` alatt, amely bemutatja, hogyan kell csatlakoztatni a szenzorokat és a mikrokontrollert.

## 3D Nyomtatott Tok
A projekt tartalmaz egy 3D nyomtatott tokot a vezérlő és a szenzorok elhelyezésére. Ez a tok segít megvédeni az elektronikát, és tiszta, tartós megoldást biztosít az alkatrészek felszereléséhez.

- **3D Nyomtatás Fájlok**: A 3D nyomtatott tok STL fájljai a `/3d_case/` mappában találhatók.
- A tok bármilyen 3D nyomtatóval kinyomtatható.
- **Nyomtatási beállítások**: A javasolt nyomtatási beállítások:
  - Rétegmagasság: 0.2mm
  - Kitöltés: 20-30%
  - Anyag: PLA vagy ABS

## Beállítás és Telepítés
1. **Arduino IDE telepítése**: Ha még nem tetted meg, telepítsd az [Arduino IDE-t](https://www.arduino.cc/en/software).
2. **Könyvtárak telepítése**: 
   - Adafruit GFX: `Sketch > Include Library > Manage Libraries > Adafruit GFX`
   - DHT szenzor könyvtár: `Sketch > Include Library > Manage Libraries > DHT sensor library`
   
3. **Vezetékek csatlakoztatása**:
   - Csatlakoztasd a szenzorokat az Arduinohoz a vezeték diagram szerint. A vezeték diagramot a `/docs/wiring_diagram.png` alatt találod.

4. **Kód feltöltése**:
   - Töltsd fel a kódot a `firmware/SmartGreenhouse.ino` fájlból az Arduino IDE segítségével.

## Jövőbeli Munka
1. **További modulok integrálása**
   CO2 szenzor hozzáadása a levegőminőség monitorozásához.
   I2C óra modul hozzáadása, idő alapú öntözéshez.

2. **Memória optimalizálás**  
   A rendszer jelenleg redundáns, ismétlődő szövegeket jelenít meg, például az OLED kijelzőn. Ezeket a szövegeket statikus karakter tömbökké vagy `const char*` változókká kell szervezni, csökkentve ezzel a SRAM használatot.

3. **AT parancsok eltávolítása**  
   Az ESP8266 jelenleg AT parancsokkal van vezérelve, ami nem a leghatékonyabb megoldás. Az AT firmware helyett jobb lenne a NodeMCU (Lua) vagy az ESP8266 Arduino alapú rendszer használata, közvetlenül a mikrokontroller programozásával. Ez eltávolítaná az AT parancsokat a soros kapcsolatról, lehetővé téve az ESP8266 számára, hogy közvetlenül végezze el a vezérlést, miközben az Arduino Mega csak az adatokat küldi. Ez javítja a válaszidőt, csökkenti a hibák lehetőségét, és további lehetőségeket nyit meg (pl. HTTPS kapcsolatok, JSON kezelés). Jelenleg, amikor a vezérlő kommunikál, a hardver interfész egy pillanatra lefagy.

4. **Külső hálózati elérhetőség**  
   A rendszer jelenleg csak a helyi hálózaton érhető el. A jövőben a webes felületet elérhetővé lehetne tenni az interneten.

5. **Adatbiztonság és titkosítás**  
   Az átadott adatok valószínűleg titkosítatlanok a hálózaton, így a titkosítás kritikus funkció lenne a megvalósítás során.

6. **Hibakeresés és diagnosztika**  
   Hasznos lenne egy alapvető hibatűrő rendszer bevezetése, például egy figyelő (watchdog) időzítő alkalmazása, amely újraindítja a rendszert, ha az lefagy, vagy ha egy meghatározott időn belül nem érkezik adat. Egy naplózó funkció, amely tárolja a hibákat vagy fontos eseményeket SD kártyán.

## Licenc
Ez a projekt a [MIT Licenc](/LICENSE) alatt van licencelve.  
Szabadon használhatod, módosíthatod és terjesztheted a kódot, még kereskedelmi projektekben is, amennyiben tartalmazza az eredeti licencet és tiszteletben tartja a szerző jogait.

## Kapcsolat
Ha bárminemű kérdésed vagy javaslatod van, bátran kereshetsz:
ata.kedves@gmail.com

---

🌱 Boldog kertészkedést az okos melegházaddal!
