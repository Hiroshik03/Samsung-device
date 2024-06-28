#include <Arduino.h>
#include <FastLED.h>
#include <EEPROM.h>
#include <EncButton.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#define DHTPIN 12
#define RELE 13
#define HumidPIN A3
#define HumidPower 2
#define LighPIN A0
#define LED_NUM 64
#define LED_PIN 7
EncButton eb(4, 5, 6);
DHT dht(DHTPIN, DHT11);
LiquidCrystal_I2C lcd(0x27,20,4);  // Устанавливаем дисплей
CRGB leds[LED_NUM];
unsigned long upd_time = 10000;
unsigned long last_time;
unsigned long water_time;
int vals[4] = {0, 0, 5, 10};
bool autoWatering = true;
int8_t arrowPos = 0;
bool TMPflag, HUMIDflag, SOILflag = false;
void setup()
{
  FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, LED_NUM);
  FastLED.setBrightness(10);
  pinMode(HumidPower,OUTPUT);
  pinMode(HumidPIN, INPUT);
  pinMode(RELE, OUTPUT);
  Serial.begin(9600);
  dht.begin();
  lcd.init();                     
  lcd.backlight();// Включаем подсветку
  vals[0] = EEPROM.read(0);
  vals[1] = EEPROM.read(4);
  vals[2] = EEPROM.read(8);
  vals[3] = EEPROM.read(12);
}
void loop()
{
  eb.tick();
  if(millis()- last_time > vals[2]*1000 && !TMPflag) temperature();
  else if(millis()- last_time > vals[2]*1000 && !HUMIDflag)humidity();
  else if(millis()- last_time > vals[2]*1000 && !SOILflag)soilHumidity();
  else if(millis()- last_time > vals[2]*1000)lightLevel();
  if(eb.click(2))menu();
  if(eb.hold()){
    lcd.noBacklight();
    lcd.clear();
    FastLED.clear();
    FastLED.show();
    while(true){
    eb.tick();
    if(eb.hold()) break;
    }
    lcd.backlight();
  }
}
void menu(){
  printGUI1();
  while(true){
    eb.tick();
    if (eb.click(2)){
      switch (arrowPos){
      case 1:
      waterGUI();
      while(true){
        eb.tick();
        if(eb.turn()){
          autoWatering =!autoWatering;
          waterGUI();
        }
        if(eb.click()){
            printGUI1();
            break;
         }
        }
      }
    }
    if (eb.turn()) {
    lcd.clear();
    int increment = 0;
    if (eb.right()) increment = 1;
    if (eb.left()) increment = -1;
    arrowPos += increment;  // двигаем курсор   
    arrowPos = constrain(arrowPos, 0, 4); // ограничиваем
    increment = 0; //обнуление инкремента
    if (eb.rightH()){ increment = 1; if(eb.pressFor()>2500)increment = 10;}
    if (eb.leftH()){increment = -1;if(eb.pressFor()>2500)increment = -10;}
    // меняем параметры
    vals[arrowPos] += increment;
    if (arrowPos == 2 || arrowPos == 1)
    {
      vals[arrowPos] = constrain(vals[arrowPos], 1,86400);
    }
    else vals[arrowPos] = constrain(vals[arrowPos], 0,99);
    printGUI1();
    }
    if (arrowPos == 4 && eb.click(1)){//Выход
      EEPROM.update(0, vals[0]);
      EEPROM.update(4, vals[1]);
      EEPROM.update(8, vals[2]);
      EEPROM.update(12, vals[3]);
      temperature();
      break;
    } 
  }
}
void temperature(){
  last_time = millis();
  float t = dht.readTemperature();
  lcd.clear();
  lcd.print("Tempreture");
  lcd.setCursor(0, 1);
  lcd.print(t);
  TMPflag = true;
}
void humidity(){
  last_time = millis();
  float h = dht.readHumidity();
  lcd.clear();
  lcd.print("air humidity");
  lcd.setCursor(0, 1);
  lcd.print(h);
  lcd.print("%");
  HUMIDflag = true;
}
void soilHumidity(){
  last_time = millis();
  digitalWrite(HumidPower,true);
  delay(100);
  float HumidP = analogRead(HumidPIN);
  digitalWrite(HumidPower,false);
  HumidP = map(HumidP, 0,1023,100,0);
  lcd.clear();
  lcd.print("soil humidity");
  lcd.setCursor(0, 1);
  lcd.print(HumidP);
  lcd.print("%");
  if(HumidP <= vals[1]&& autoWatering){
    digitalWrite(RELE,1);
    delay(5000);
    digitalWrite(RELE,0);
  }
  else if(millis() - water_time > vals[1]*1000 && !autoWatering){
    water_time = millis();
    digitalWrite(RELE,1);
    delay(5000);
    digitalWrite(RELE,0);
  }
  SOILflag = true;
}
void lightLevel(){
  last_time = millis();
  float light =analogRead(LighPIN);
  light = map(light,10,1023,100,0);
  lcd.clear();
  lcd.print("light level");
  lcd.setCursor(0, 1);
  lcd.print(light);
  lcd.print("%");
  if(light<= vals[0]){
    int level = map(vals[3],0,100,0,255);
    FastLED.setBrightness(level);
    for(int i=0;i<LED_NUM; i++){
      leds[i]=CRGB(255,255,255);
      FastLED.show();
    }
  }
  else{
    FastLED.clear();
    FastLED.show();
  }
  TMPflag = !TMPflag; HUMIDflag = !HUMIDflag; SOILflag=!SOILflag;
}
void printGUI1() {
  lcd.clear();
  switch (arrowPos){
    case 0:
    lcd.setCursor(0, 0); lcd.print("Configuration:");
    lcd.setCursor(0, 1); lcd.print("Minimum light:");
    lcd.setCursor(14, 1);lcd.print(vals[0]);
    lcd.setCursor(13, 1);lcd.write(126);
  break;
    case 1:
    lcd.setCursor(0, 0); lcd.print("Configuration:");
    if(autoWatering){
      lcd.setCursor(0, 1); lcd.print("Minimum humid:");
      lcd.setCursor(14, 1);lcd.print(vals[1]);}
    else{
      lcd.setCursor(0, 1); lcd.print("Period:");
      lcd.setCursor(6, 1);lcd.write(126);
      lcd.setCursor(7, 1);
      lcd.print(vals[1]/3600);
      lcd.print("h");
      lcd.print(vals[1]/60);
      lcd.print("m");
      lcd.print(vals[1]%60);
      lcd.print("s");
    }
  break;
    case 2:
    lcd.setCursor(0, 0); lcd.print("Configuration:");
    lcd.setCursor(0, 1); lcd.print("Upd time:");
    lcd.setCursor(9, 1); if (vals[2]/60>0){lcd.print(vals[2]/60);lcd.print("m");}lcd.print(vals[2]%60);lcd.print("s");
    lcd.setCursor(8, 1);lcd.write(126);
  break;
    case 3:
    lcd.setCursor(0, 0); lcd.print("Brightness:");
    lcd.setCursor(7, 1); lcd.print(vals[3]);lcd.print("%");
    lcd.setCursor(6, 1);lcd.write(126);
  break;
    case 4:
    lcd.setCursor(0, 0); lcd.print("Exit:");
    lcd.setCursor(7, 1); lcd.print("yes");
    lcd.setCursor(6, 1);lcd.write(126);
  break;
  }
}
void waterGUI(){
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("auto watering:");
  lcd.setCursor(7, 1); if (autoWatering)lcd.print("On");else lcd.print("Off");
}

