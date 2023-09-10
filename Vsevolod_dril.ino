/*
 * Крайні зміни були у основному циклі замінені 
 * умовні блоки на підцикли по режимам. Також 
 * замінені у ф-ції підрахунку лічильника
 * важкі обрахунки на XOR з бінарними флагами
 * замість порівняння парності змінних типу int8_t
 */

#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>

//Наступний блок розпіновку можна змінювати під потреби
#define RELAY_PIN         A3
#define HALL_SENS1_PIN    2
#define HALL_SENS2_PIN    3
#define COUNT1_BTN_PIN    9
#define COUNT2_BTN_PIN    10
#define COUNT3_BTN_PIN    11
#define PLAYPAUSE_BTN_PIN 12

uint8_t Count_modes[] = {65, 45, 10};

//Якщо для включення реле потрібно логічний "0" змінити на LOW
#define RELAY_ON LOW
#define BTN_DELAY 80

#define STANDBY 0
#define RUN 1
#define PAUSE 2
#define DONE 3

LiquidCrystal_I2C lcd(0x27, 16, 2);
uint8_t Desired_value = 0;
int16_t Count = 0;
uint8_t Mode = STANDBY;


void showButtons(){
  lcd.setCursor(0,1);
  lcd.print(Count_modes[0]);
  lcd.print(" ");
  lcd.print(Count_modes[1]);
  lcd.print(" ");
  lcd.print(Count_modes[2]);
  lcd.setCursor(12,1);
  if (Mode == RUN){
    lcd.print("Stop");
  }else if (Mode == PAUSE){
    lcd.print(" RUN");
  }else{
    lcd.print("    ");
  }
}

void showStopBtnOnly(){
  lcd.setCursor(0,1);
  lcd.print("            Stop");
}

void refreshCounts(){
  lcd.setCursor(8,0);
  lcd.print("   ");
  lcd.setCursor(8,0);
  lcd.print(Count);
}

void lcdStartup(){
  lcd.clear();
  lcd.print("Already to work");
  showButtons();
}

void startMode(uint8_t count){
  Count = 0;
  lcd.clear();
  lcd.print("RUNNING ");
  refreshCounts();
  lcd.setCursor(12,0);
  lcd.print("/");
  lcd.print(count);
  showStopBtnOnly();
  
  Desired_value = count;
  Mode = RUN;
  turnOnRelay();
}

void pauseMode(){
  turnOffRelay();
  Mode = PAUSE;
  lcd.home();
  lcd.print("PAUSE  ");
  showButtons();
  while(isPressed(PLAYPAUSE_BTN_PIN)){
    if (isNewCounters()){
      refreshCounts();
    }
  }
}

void afterPauseMode(){
  if (Desired_value > Count){
    lcd.home();
    lcd.print("RUNNING");
    Mode = RUN;
    showStopBtnOnly();
    while(isPressed(PLAYPAUSE_BTN_PIN)){
      if (isNewCounters()){
        refreshCounts();
      }
    }
    turnOnRelay();
  }
}

void done(uint8_t value){
  turnOffRelay(); 
  Mode = DONE;
  lcd.home();
  lcd.print("DONE   ");
  showButtons();
}

void checkSelectMode(){
    if ( isPressed(COUNT1_BTN_PIN) ){
      startMode(Count_modes[0]);
    }else if ( isPressed(COUNT2_BTN_PIN) ){
      startMode(Count_modes[1]);
    }else if ( isPressed(COUNT3_BTN_PIN) ){
      startMode(Count_modes[2]);
    }
}

void setup() {
   lcd.init();
   lcd.backlight();
   lcd.print("Loading");
     
   pinMode(RELAY_PIN, OUTPUT);   
   pinMode(HALL_SENS1_PIN, INPUT);
   pinMode(HALL_SENS2_PIN, INPUT);
   pinMode(PLAYPAUSE_BTN_PIN, INPUT);
   pinMode(COUNT1_BTN_PIN, INPUT);
   pinMode(COUNT2_BTN_PIN, INPUT);
   pinMode(COUNT3_BTN_PIN, INPUT);

   turnOffRelay();

   getModesEEPROM();

   delay(250);
   if (isPressed(PLAYPAUSE_BTN_PIN) ){
    setupMode();
   }
   if ( isPressed(COUNT1_BTN_PIN) || isPressed(COUNT2_BTN_PIN) ){
    changeMasterHall();
   }

   uint8_t masterHall = getMasHallEEPROM();
   attachInterrupt( digitalPinToInterrupt(masterHall), hallHandler, RISING);
   
   lcdStartup();
}

void loop() {
  while (Mode == RUN) { // У режимі роботи можемо тільки зупиняти
    if (Count >= Desired_value){
      done(Desired_value);
    }
    if ( isPressed(PLAYPAUSE_BTN_PIN) ){
      pauseMode();
    }
    if (isNewCounters()){
      refreshCounts();
    }
  }
  while (Mode == DONE) { //Mode DONE
    if (isNewCounters()){
      refreshCounts();
    }
    checkSelectMode();
    if ( isPressed(PLAYPAUSE_BTN_PIN) ){
      Count=0;
      refreshCounts();
    }
  }
  while (Mode == PAUSE){ // У режимі очікування опитуємо всі режими і плей
    if ( Count >= Desired_value ) done(Desired_value);
    if ( isPressed(PLAYPAUSE_BTN_PIN) ){
      afterPauseMode();
    }
    if (isNewCounters()){
      refreshCounts();
    }
    checkSelectMode();
  }
  while (Mode == STANDBY) { // Mode STANDBY
    if (!Desired_value && Count){
        lcd.clear();
        refreshCounts();
        showButtons();
        Desired_value = 1;
    }
    if (isNewCounters()){
      refreshCounts();
    }
    if ( isPressed(PLAYPAUSE_BTN_PIN) ) Count=0;
    checkSelectMode();
  }
  
}

void hallHandler(){ 
  Count++;
}

bool isNewCounters(){
    static int16_t count = 0;
    if (Count != count){
      count = Count;
      return true;
    }
    return false;
}

bool isPressed(uint8_t pin){
  if (digitalRead(pin) == LOW){
    //delay(20); // розкоментувати якщо програмно проти дребезга
    return true;  
  }
  return false;
}

void turnOnRelay(){
  digitalWrite(RELAY_PIN, RELAY_ON);
}
void turnOffRelay(){
  digitalWrite(RELAY_PIN, !RELAY_ON);
}

void getModesEEPROM(){
  uint8_t value = 0;
  for (int i=0; i<sizeof(Count_modes); i++){
    EEPROM.get(i, value);
    if (value>0 && value<255) Count_modes[i] = value;
  }
}

uint8_t getMasHallEEPROM(){
  uint8_t value = 0;
  EEPROM.get(3, value);
  if (value == HALL_SENS1_PIN || value == HALL_SENS2_PIN)
    return value;
  else 
    return HALL_SENS1_PIN;
}

bool saveModesEEPROM(){
  return (bool)EEPROM.put(0, Count_modes);
}

void saveMasHallEEPROM(uint8_t channel){
  EEPROM.update(3, channel);
}

void setupMode(){  
  lcd.clear();
  lcd.print("SETUP choose btn");
  showButtons();
  while( isPressed(PLAYPAUSE_BTN_PIN) ) delay(80); //need to keyup button
  while( !isPressed(PLAYPAUSE_BTN_PIN) ){
    if ( isPressed(COUNT1_BTN_PIN) ){
      changeCountMode(1);
      break;
    }else if ( isPressed(COUNT2_BTN_PIN) ){
      changeCountMode(2);
      break;
    }else if ( isPressed(COUNT3_BTN_PIN) ){
      changeCountMode(3);
      break;
    }
  }
}

void changeCountMode(uint8_t mode){
  lcd.clear();
  lcd.print("Set new for:");
  lcd.print(mode);
  waitToKeyUp(mode);
  uint8_t count = Count_modes[mode-1];
  lcd.setCursor(0,1);
  lcd.print(count);
  while( !isPressed(PLAYPAUSE_BTN_PIN) ){
    if ( isPressed(COUNT1_BTN_PIN) ){ //change to biggest value
      count = (count>253) ? 254 : count+1;
      lcd.setCursor(0,1);
      lcd.print(count);
      delay(BTN_DELAY);
    }
    if ( isPressed(COUNT3_BTN_PIN) ){ //change to lowest value
      count = (count<2) ? 1 : count-1;
      lcd.setCursor(0,1);
      lcd.print(count);
      delay(BTN_DELAY);
    }
  }
  if ( count != Count_modes[mode-1] ){
    Count_modes[mode-1] = count;
    if ( saveModesEEPROM() ){
      lcd.clear();
      lcd.print(" Button #");
      lcd.print(mode);
      lcd.print(" DONE");
      lcd.setCursor(0,1);
      lcd.print("Saved with: ");
      lcd.print(count);
     
    }else{
      lcd.clear();
      lcd.print("Without changes");
    }
    delay(5000);
  }
}

bool waitToKeyUp(uint8_t inMode){
  delay(80);
  if (inMode==1){
    while(isPressed(COUNT1_BTN_PIN));
  }
  if (inMode==2){
    while(isPressed(COUNT2_BTN_PIN));
  }
  if (inMode==3){
    while(isPressed(COUNT3_BTN_PIN));
  }
}

void changeMasterHall(){
  uint8_t channel = 0;
  if( isPressed(COUNT1_BTN_PIN) ) channel = HALL_SENS1_PIN;
  if( isPressed(COUNT2_BTN_PIN) ) channel = HALL_SENS2_PIN;
  if (channel){
    saveMasHallEEPROM(channel);
    lcd.clear();
    lcd.print("Hall will use #");
    lcd.print(channel-1);
  }
  while ( isPressed(COUNT1_BTN_PIN) || isPressed(COUNT2_BTN_PIN) ) delay(BTN_DELAY);
}
