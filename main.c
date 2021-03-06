#include <16f877A.h>
#device ADC=10
#fuses HS,NOWDT,NOPROTECT,NOCPD,NOPUT,NOBROWNOUT,NOLVP,NOWRT
#use delay(clock=4000000)
#use rs232 (baud=9600, xmit=pin_c6, rcv=pin_c7, parity=N, stop=1)

#define LIGHTS pin_d0
#define WASHER pin_d1
#define AIRCON pin_d2
#define CURTAIN_LED pin_d3
#define INTRUDER_LED pin_d4
#define SMOKE_LED pin_d5
#define GAS_LED pin_d6
#define FIRE_LED pin_d7
#define MOTOR_INPUT_1 pin_b1
#define MOTOR_INPUT_2 pin_b2

#define H_PIN(a) output_high(a) 
#define L_PIN(a) output_low(a)


char cmd;

char isIntruderAlarmActive = 0;

char isLightOn = 0;
char isWasherOn = 0;
char isAcOn =0;

char curtainState = 0; // 0 = closed
char curtain_led_blink_counter = 0;
char cur_cmd;

char canSendIntruderAlert = 1;
char canSendFireAlert = 1;
char canSendGasAlert = 1;
char canSendSmokeAlert = 1;

float analogRead(int portNum);
void handleSensorData(char* canSendAlert,int threshold, float sensorVal,char msg,int led);
void handleIntruderAlarm(float sensorval);
void handleFireAlarm(int threshold,float temp);
float modifyTempData(float val);
void checkPins();


#int_ext
void ext_isr(){
   puts("ext_interrupt");
   isIntruderAlarmActive = 0;
}

#int_rda
void rda_isr(){
   
   cmd = getc();
   
   switch(cmd){
      case 'L':
         //output_high(LIGHTS);
         H_PIN(LIGHTS);
         isLightOn = 1;
         break;
      case 'l':
         //output_low(LIGHTS);
         L_PIN(LIGHTS);
         isLightOn = 0;
         break;
      case 'w':
         //output_low(WASHER);
         L_PIN(WASHER);
         isWasherOn = 0;
         break;
      case 'W':
         //output_high(WASHER);
         H_PIN(WASHER);
         isWasherOn = 1;
         break;
      case 'A':
         //output_high(AIRCON);
         H_PIN(AIRCON);
         isAcOn =1;
         break;
      case 'a':
         //output_low(AIRCON);
         L_PIN(AIRCON);
         isAcOn =0;
         break;
      case 'i':
         isIntruderAlarmActive = 0;
         break;
      case 'I':
         isIntruderAlarmActive = 1;
         break;
      case 'C':
         //open curtains
         if(curtainState == 2) return;
         cur_cmd = 'o';
         curtainState = 0;
         output_low(CURTAIN_LED);
         break;
      case 'c':
         //close curtains
         if(curtainState == 0) return;
         cur_cmd = 'c';
         curtainState = 0;
         output_high(CURTAIN_LED);
         break;
   }
   
}

void main(){
   
   set_tris_a(0x0f);
   set_tris_b(0x01); output_b(0x00);
   set_tris_d(0x00); output_d(0x00);
   
   setup_adc(adc_clock_div_32);
   setup_adc_ports(all_analog);
   
   ext_int_edge(L_TO_H);
   enable_interrupts(int_ext);
   enable_interrupts(int_rda);
   enable_interrupts(global);
   
   float intruder_alarm;
   float temperature;
   float smokeSensor;
   float gasSensor;
   
   while(true){
      
      intruder_alarm = analogRead(1);
      temperature = analogRead(0);
      smokeSensor = analogRead(2);
      gasSensor = analogRead(3);
//!      
      temperature = modifyTempData(temperature);//convert to degree celcius
//!      
      handleSenSorData(&canSendSmokeAlert,250,smokeSensor,'Z',SMOKE_LED);
      handleSensorData(&canSendGasAlert,200,gasSensor,'X',GAS_LED);
      handleIntruderAlarm(intruder_alarm);
      handleFireAlarm(50,temperature);
      
      checkPins();
      printf("%f",temperature);
      delay_ms(1000);
   }

}

float analogRead(int portNum){

   unsigned long int value = 0;
   
   set_adc_channel(portNum);
   delay_ms(20);
   
   value = read_adc();
   
   return value * 0.4889;
}

void handleSensorData(char* canSendAlert,int threshold, float sensorVal,char msg,int led){
   if(*canSendAlert == 1 && sensorVal > threshold){
      *canSendAlert = 0;
      putc(msg);
      delay_ms(1000);
   }else if(*canSendAlert == 0 && sensorVal > threshold){
      //blink led   
      output_toggle(led);
   }else if(sensorVal <= threshold){
      output_low(led);
      *canSendAlert = 1;
   }
}

void handleIntruderAlarm(float sensorval){
   if(isIntruderAlarmActive == 1){
      handleSensorData(&canSendIntruderAlert,395,sensorVal,'Y',INTRUDER_LED);
   }else{
      output_low(INTRUDER_LED);
   }
}

void handleFireAlarm(int threshold,float temp){
   if(canSendSmokeAlert != 1 && temp > threshold && canSendFireAlert == 1 ){
      putc('M');
      canSendFireAlert = 0;
      delay_ms(1000);
   }else if(canSendSmokeAlert != 1 && temp > threshold && canSendFireAlert != 1){
      output_toggle(FIRE_LED);
   }else if(canSendSmokeAlert == 1 || temp < threshold){
      canSendFireAlert = 1;
      output_low(FIRE_LED);
   }
}

float modifyTempData(float val){
   val = val / 0.4889;
   val = (val/1024)* 5000;
   return val/10;
}

void checkPins(){
   if(isLightOn == 1){
      output_high(LIGHTS);
   }
   
   if(isWasherOn == 1){
      output_high(WASHER);
   }
   
   if(isAcOn == 1){
      output_high(AIRCON);
   }
}


void handleCurtains(){
   if(cur_cmd == 'c'){
      //the command is to close
      output_low(MOTOR_INPUT_1);
      output_high(MOTOR_INPUT_2);
      
      if(curtain_led_blink_counter >= 10){
         curtain_led_blink_counter = 0;
         output_low(CURTAIN_LED);
         output_low(MOTOR_INPUT_2);
         curtainState = 0;
         return;
      }
   }else if(cur_cmd == 'o'){
      output_high(MOTOR_INPUT_1);
      output_low(MOTOR_INPUT_2); 
      
       if(curtain_led_blink_counter >= 10){
         curtain_led_blink_counter = 0;
         output_high(CURTAIN_LED);
         output_low(MOTOR_INPUT_1);
         curtainState = 2;
         return;
      }
   }
   
   output_toggle(CURTAIN_LED);
   curtain_led_blink_counter++;
}
