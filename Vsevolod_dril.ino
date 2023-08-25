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

//Далі не змінювати
#define __GETBIT(a, b) (a & (1<<b))
#define __SETBIT(a, b) (a |= (1<<b))
#define __CLEARBIT(a, b) (a &= ~(1<<b))
#define __TOGGLEBIT(a, b) (a ^= (1<<b))
#define __RUNNING 0
#define __NEXTOPERATION 1
#define __LASTHALL 2
#define ISRUN(a) __GETBIT(a, __RUNNING)
#define SETRUN(a) (__SETBIT (a, __RUNNING))
#define SETSTOP(a) (__CLEARBIT (a, __RUNNING))
#define GETHALL(a) __GETBIT(a, __LASTHALL)
#define SETHALL1(a) (__SETBIT (a, __LASTHALL))
#define HALL1 1
#define SETHALL2(a) (__CLEARBIT (a, __LASTHALL))
#define HALL2 0
#define TOGGLEDIRECTION(a) (__TOGGLEBIT (a, __DIRECTION))
#define GETOPERATION(a) __GETBIT(a, __NEXTOPERATION)
#define SETOPERATION(a) __SETBIT(a, __NEXTOPERATION)
#define TOGGLEOPERATION(a) (__TOGGLEBIT (a, __NEXTOPERATION))


LiquidCrystal_I2C lcd(0x27, 16, 2);
uint8_t Desired_value = 0;
volatile uint8_t Count = 0;
volatile byte Status = 0;

void setup() {
   lcd.init();
   lcd.backlight();
   lcd.print("Loading");
     
   pinMode(RELAY_PIN, OUTPUT);   
   pinMode(HALL_SENS1_PIN , INPUT);
   pinMode(HALL_SENS2_PIN , INPUT);
   pinMode(PLAYPAUSE_BTN_PIN, INPUT);
   pinMode(COUNT1_BTN_PIN, INPUT);
   pinMode(COUNT2_BTN_PIN, INPUT);
   pinMode(COUNT3_BTN_PIN, INPUT);

   SETHALL1(Status);
   SETOPERATION(Status);
   turnOffRelay();

   getModesEEPROM();
   
   attachInterrupt( digitalPinToInterrupt(HALL_SENS1_PIN), hall1Handler, RISING);
   attachInterrupt( digitalPinToInterrupt(HALL_SENS2_PIN), hall2Handler, RISING);

   delay(250);
   if (isPressed(PLAYPAUSE_BTN_PIN) ){
    setupMode();
   }
   
   lcdStartup();
}

void loop() {
  if (ISRUN(Status)) { // У режимі роботи можемо тільки зупиняти
    onRunning();
    if (Count == 0) SETOPERATION(Status); // При відкручуванні назад і при "0" робимо наступні операцію додавання
    if ( isPressed(PLAYPAUSE_BTN_PIN) ){
      pauseMode();
    }
  }else { // У режимі очікування опитуємо всі кнопки
    if ( isPressed(PLAYPAUSE_BTN_PIN) ){
      afterPauseMode();
    }else if ( isPressed(COUNT1_BTN_PIN) ){
      startMode(Count_modes[0]);
    }else if ( isPressed(COUNT2_BTN_PIN) ){
      startMode(Count_modes[1]);
    }else if ( isPressed(COUNT3_BTN_PIN) ){
      startMode(Count_modes[2]);
    }
//    if ( !ISRUN(Status) && Desired_value ){
//      refreshCounts();
//    }
  }

}

void hall1Handler(){ 
  if ( GETHALL(Status) == HALL1 ){
    TOGGLEOPERATION(Status);
  }
  GETOPERATION(Status) ?  Count++ : Count--;
  SETHALL1(Status);
}
void hall2Handler(){
  if ( GETHALL(Status) == HALL2 ){
    TOGGLEOPERATION(Status);
  }
//  GETOPERATION(Status) ?  Count++ : Count--;
  SETHALL2(Status);  
}

void onRunning(){
  if (Count >= Desired_value){
    done(Desired_value);
  }else{
    refreshCounts();
  }
}

void refreshCounts(){
  uint8_t pos = 10;
  pos -= log10(Count);
  lcd.setCursor(pos,0);
  lcd.print(Count);
  
}

void startMode(uint8_t count){
  Count = 0;
  lcd.clear();
  lcd.print("RUNNING ");
//  lcd.setCursor(3,1);
//  lcd.print("  0/");
//  lcd.print(count);
  showStopBtnOnly();
  
  Desired_value = count;
  SETRUN(Status);
  turnOnRelay();
}

void showStopBtnOnly(){
  lcd.setCursor(0,1);
  lcd.print("           Stop");
}

void pauseMode(){
  turnOffRelay();
  SETSTOP(Status);
  lcd.clear();
  lcd.print("PAUSE  ");
  showButtons();
  delay(700);
}

void done(uint8_t value){
  turnOffRelay(); 
  SETSTOP(Status);
  lcd.setCursor(0,0);
  lcd.print(" DONE   ");
  lcd.print(value);
  lcd.print("/");
  lcd.print(Desired_value);
  showButtons();
}

void afterPauseMode(){
  if (Desired_value > Count){
    lcd.home();
    lcd.print("RUNNING");
    showButtons();
    delay(400);
    SETRUN(Status);
    turnOnRelay();
  }
}

void lcdStartup(){
  lcd.clear();
  lcd.print("Already to work");
  showButtons();
}

void showButtons(){
  lcd.setCursor(0,1);
  lcd.print(Count_modes[0]);
  lcd.print(" ");
  lcd.print(Count_modes[1]);
  lcd.print(" ");
  lcd.print(Count_modes[2]);
  if (Desired_value > 0){
    lcd.setCursor(11,1);
    if (ISRUN(Status)){
      lcd.print("Pause");
    }else{
      lcd.print("RUN  ");
    }
  }
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

bool saveModesEEPROM(){
  return (bool)EEPROM.put(0, Count_modes);
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
