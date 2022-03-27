// Assignment 3 FreeRToS based
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define WATCHDOGPIN 22 // Pin for the output watchdog waveform 
#define ANALOGPIN 36 
#define ANALOGREADPIN 17


// Defining global variables

unsigned int analogReading; // Very last value to be stored from the ADC

unsigned int analogAverage; // Average of the last 4 values from the ACC

//Defining subroutines to be run using RTOS shedulling
void Watchdog(void *Parameters);
void AnalogRead(void *Parameters);
void AnalogAverage(void *Parameters);

SemaphoreHandle_t  analogSMP; 



void setup(void) {
  // put your setup code here, to run once:
  Serial.begin (9600);
 pinMode(WATCHDOGPIN, OUTPUT);
pinMode(ANALOGREADPIN, OUTPUT);

  xTaskCreatePinnedToCore(
    Watchdog
    ,  "Watchdog"   // A name just for humans
    ,  550  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);
  
  xTaskCreatePinnedToCore(
    AnalogRead
    ,  "AnalogRead"   // A name just for humans
    ,  720  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);

      xTaskCreatePinnedToCore(
    AnalogAverage
    ,  "AnalogAverage"   // A name just for humans
    ,  615  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);
  

analogSMP = xSemaphoreCreateBinary();


}

void loop(void) {} //Not used, since it is RTOS



void AnalogRead(void *Parameters) // Reads the analog value from the ANALOGPIN pin
{
 // 52 microseconds, 670 stack size
  (void) Parameters;


   while(1){
  digitalWrite(ANALOGREADPIN, HIGH); // Indicate the start of the ADC
  
  xSemaphoreGive(analogSMP);
  analogReading = analogRead(ANALOGPIN);
  xSemaphoreTake(analogSMP,portMAX_DELAY);
  
  digitalWrite(ANALOGREADPIN, LOW); // Indicate the end of the ADC
    
 vTaskDelay(42);
   }
}


void AnalogAverage(void *Parameters) // Computes average of last four analog readings
{ 
  (void) Parameters;
// 464 stack size, 4 microseconds execution
  int temp;

    unsigned long beginning;
  unsigned long ending;
   unsigned long times;
   
  char analogAdress; // Analog readings array indexing
   unsigned int analogData[4]; // Array to store 4 last values from the ADC
  while(1){
    beginning=micros();
  // This section logs the most recent analog reading to the longest-ever updated cell (there are 4 cells in total)
  if(analogAdress>=4) {analogAdress=0;} // Making sure only valid array cells are accessed

   xSemaphoreGive(analogSMP);
  analogData[analogAdress]=analogReading; // Loggin the last analog reading into the array cell
   xSemaphoreTake(analogSMP,portMAX_DELAY);
   
  analogAdress++; // Icrementing array adress

  // This section computes the average of last four readings
  analogAverage=analogData[0]+analogData[1]+analogData[2]+analogData[3]; // Sums all values in the array
  analogAverage=analogAverage>>2; // Divides the sum by four to obtain average
  ending=micros();
 times=ending-beginning;


  temp=uxTaskGetStackHighWaterMark(nullptr);
  Serial.print("heap:");
  Serial.print(temp,DEC); // Printing measured frequency of the 50% square signal
  Serial.print('\n');

   Serial.print(times,DEC); // Printing measured frequency of the 50% square signal
  Serial.print('\n');

   vTaskDelay(42);
  }
}


















void Watchdog(void *Parameters) // Generates the 50us pulse
{
  (void) Parameters;
 // unsigned long beginning;
 // unsigned long ending;
//   unsigned long times;
//int temp;
   while(1) {
 // beginning=micros();

  digitalWrite(WATCHDOGPIN,HIGH);
  delayMicroseconds(5000);
  digitalWrite(WATCHDOGPIN,LOW);
 // temp=uxTaskGetStackHighWaterMark(nullptr);
  //ending=micros();
 // times=ending-beginning;
 //  Serial.print(temp,DEC); // Printing measured frequency of the 50% square signal
//  Serial.print('\n');
  vTaskDelay(250);
   }
}
