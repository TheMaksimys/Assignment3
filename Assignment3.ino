// Assignment 3 FreeRToS based
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define WATCHDOGPIN 19 // Pin for the output watchdog waveform 
#define ANALOGPIN 36 
#define ANALOGREADPIN 17
#define DIGITALINPUTPIN 22 // Pin for the pushbutton input
#define SIGNALPIN 18 // Pin for the external square wave input
#define ERRORPIN 23 // Pin for the error indication LED
// Defining global variables

unsigned int analogReading; // Very last value to be stored from the ADC

 int temp;
 unsigned int errorCode; // value to pass between the analog error detection

//Defining subroutines to be run using RTOS shedulling
void Watchdog(void *Parameters);
void AnalogRead(void *Parameters);
void AnalogAverage(void *Parameters);
void Nop1000times(void *Parameters);
void DigitalInput(void *Parameters);
void FrequencyMeasure(void *Parameters);
void ErrorCheck(void *Parameters);
void ErrorDisplay(void *Parameters);

static SemaphoreHandle_t  analogSMP=NULL; 
static SemaphoreHandle_t  dataOutSMP=NULL; 
static SemaphoreHandle_t  errorSMP=NULL; 

struct memory{
  char buttonInput;
  unsigned int measuredFreq;
  unsigned int analogAverage;
} dataOut;



void setup(void) {
  // put your setup code here, to run once:
  Serial.begin (9600);
 pinMode(WATCHDOGPIN, OUTPUT);
pinMode(ANALOGREADPIN, OUTPUT);
pinMode(DIGITALINPUTPIN, INPUT);
pinMode(DIGITALINPUTPIN, INPUT_PULLDOWN); // Prevents pin from being floating 
pinMode(SIGNALPIN, INPUT);
pinMode(SIGNALPIN, INPUT_PULLDOWN); // Prevents pin from being floating 
  pinMode(ERRORPIN, OUTPUT);

analogSMP = xSemaphoreCreateBinary();
dataOutSMP = xSemaphoreCreateBinary();
errorSMP = xSemaphoreCreateBinary();


  if(analogSMP==NULL||dataOutSMP==NULL || errorSMP == NULL)
  {
    Serial.print("Could not create semaphore");
    ESP.restart();
  }

//vTaskStartScheduler(); // maybe it is REQUUIRED BUT I DONT KNOW +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  xTaskCreatePinnedToCore(
    Watchdog
    ,  "Watchdog"   // A name just for humans
    ,  570  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);
  
  xTaskCreatePinnedToCore(
    AnalogRead
    ,  "AnalogRead"   // A name just for humans
    ,  740  // This stack size can be checked & adjusted by reading the Stack Highwater
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


        xTaskCreatePinnedToCore(
    Nop1000times
    ,  "Nop1000times"   // A name just for humans
    ,  530  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);


            xTaskCreatePinnedToCore(
   DigitalInput
    ,  "DigitalInput"   // A name just for humans
    ,  560  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);

    
            xTaskCreatePinnedToCore(
   FrequencyMeasure
    ,  "FrequencyMeasure"   // A name just for humans
    ,  540 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);

            xTaskCreatePinnedToCore(
   ErrorCheck
    ,  "ErrorCheck"   // A name just for humans
    ,  550 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);

     xTaskCreatePinnedToCore(
   ErrorDisplay
    ,  "ErrorDisplay"   // A name just for humans
    ,  530 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // task handler
    ,  ARDUINO_RUNNING_CORE);



//xSemaphoreGive(analogSMP); // Assigning initial state to semaphores
xSemaphoreGive(dataOutSMP);

}

void loop(void) {} //Not used, since it is RTOS


//====================================================================================================================
void AnalogRead(void *Parameters) // Reads the analog value from the ANALOGPIN pin
{
 // 52 microseconds, 670 stack size
  (void) Parameters;

   while(1)
   {
      digitalWrite(ANALOGREADPIN, HIGH); // Indicate the start of the ADC
  
     // xSemaphoreTake(analogSMP,portMAX_DELAY);
      analogReading = analogRead(ANALOGPIN);
      xSemaphoreGive(analogSMP);
      
      digitalWrite(ANALOGREADPIN, LOW); // Indicate the end of the ADC
      vTaskDelay(42);
   }
}

//====================================================================================================================
void AnalogAverage(void *Parameters) // Computes average of last four analog readings
{ 
  (void) Parameters;
// 464 stack size, 4 microseconds execution

  char analogAdress; // Analog readings array indexing
  unsigned int analogData[4]; // Array to store 4 last values from the ADC
  while(1){

  // This section logs the most recent analog reading to the longest-ever updated cell (there are 4 cells in total)
  if(analogAdress>=4) {analogAdress=0;} // Making sure only valid array cells are accessed
  
 xSemaphoreTake(analogSMP,portMAX_DELAY);
 
  analogData[analogAdress]=analogReading; // Loggin the last analog reading into the array cell
    //xSemaphoreGive(analogSMP);
   
  analogAdress++; // Icrementing array adress

  xSemaphoreTake(dataOutSMP,portMAX_DELAY);
  // This section computes the average of last four readings
  dataOut.analogAverage=analogData[0]+analogData[1]+analogData[2]+analogData[3]; // Sums all values in the array
  dataOut.analogAverage=dataOut.analogAverage>>2; // Divides the sum by four to obtain average
  xSemaphoreGive(dataOutSMP);
   vTaskDelay(40); //42-2 for synronisation
  }
}


//----------------------------------------------------------------------------------------------------------------------------------------------
void Nop1000times(void *Parameters) // Sets CPU in no-operation state for 1000 clock cycles (approx. 1ms); two for loops to simplify counter register access
{
  (void) Parameters;
//heap 468, time neglectably small

  while(1){

  for(char i=0; i++; i<4)
  {
    for(char j=0; i++; i<250)
    {
      __asm__ __volatile__ ("nop"); // "volatile" is needed to prevent optimisation by the compiler    
    }
  }
  
  vTaskDelay(100); // 10 Hz

  }
}



//----------------------------------------------------------------------------------------------------------------------------------------------
void DigitalInput(void *Parameters) // Reads the digital state of the button 
{
  (void) Parameters; // 2-42 usec depending whether the buttton is pressed, heap =2000-1508+64
 // int temp;
  
   // unsigned long beginning;
  //unsigned long ending;
   //unsigned long times;

  while(1){
  xSemaphoreTake(dataOutSMP,portMAX_DELAY);
//  beginning=micros();
   dataOut.buttonInput= digitalRead(DIGITALINPUTPIN); // Reads the digital state of the button (either 0 or 1)
  if(dataOut.buttonInput==1)
  {  
    SerialOutput();
    } 
    xSemaphoreGive(dataOutSMP);
  //  ending=micros();
    //times=ending-beginning;
    //temp=uxTaskGetStackHighWaterMark(nullptr);
  //Serial.print("heap:");
  //Serial.print(temp,DEC); // Printing measured frequency of the 50% square signal
  //Serial.print('\n');

   //Serial.print(times,DEC); // Printing measured frequency of the 50% square signal
  //Serial.print('\n');
    vTaskDelay(200);
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void SerialOutput(void) // Printing the data on the serial monitor
{
  Serial.print(temp,DEC); // Printing the button state
  Serial.print(",");
    
  Serial.print(dataOut.measuredFreq,DEC); // Printing measured frequency of the 50% square signal
  Serial.print(",");
  
  Serial.print(dataOut.analogAverage,DEC); // Printing average of 4 last analog readings
  Serial.print('\n');
  
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------

void ErrorCheck(void *Parameters) // Checks whether the average analog value is greater than the half of the maximum possible analog reading (?>(4096/2)-1)
{
  //600-120+64=546 (550)
  (void) Parameters;
  while(1){
  xSemaphoreTake(dataOutSMP,portMAX_DELAY);
  if(dataOut.analogAverage>2047) {errorCode = 1;} // Setting the error flag 
  else{errorCode=0;} // Clearing the error flag
  xSemaphoreGive(errorSMP); // synchronisation semaphore
   xSemaphoreGive(dataOutSMP);
     
   vTaskDelay(333);
  }
  
}
//----------------------------------------------------------------------------------------------------------------------------------------------
void ErrorDisplay(void *Parameters) // Toggles the LED if the error occurs
{ //600-140+64=526 (530)
  (void) Parameters;
  while(1){
    xSemaphoreTake(errorSMP,portMAX_DELAY);
  digitalWrite(ERRORPIN,errorCode); // Switched the LED either ON or OFF
     temp=uxTaskGetStackHighWaterMark(nullptr);
   vTaskDelay(330); //333-3 for syncronisation
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void FrequencyMeasure(void *Parameters) // Function to measure the frequency of the 50% duty cycle square wave
{ // heap 2048-1576+64=536
  (void) Parameters;
  // Parameters to prevent function from occupying more than 3 miliseconds of the CPU time
  unsigned long measureStart; // Absolute time of the measuring start
  unsigned long measureFinish; // Absolute time of the measuring finish
  unsigned int measureTime=0; // Time it took since the function start to measure the frequency

  // Parameters to measure the half-cycle duration of the input square signal (in microseconds)
  unsigned long stateStart;  // Absolute time of the half-cycle start
  unsigned long stateFinish; // Absolute time of the half-cycle finish
  unsigned int stateTime; // Half-cycle duration of the input square signal
  char firstTrigger=0; // Trigger to distinguish between the first and the second detected edges
  char currentReading; // Current signal pin state
  char previousReading; // Previous signal pin state

  unsigned int delayF; // for inserting appropriate delay
 

measurement:
  while(1){
  measureTime=0; 
  firstTrigger=0; // assignign trigger back to zero
  
  //  vTaskSuspendAll(); // suspends the shedulling (this function gets maximum absolute priority & round-robin is disabled)
 vTaskSuspendAll();
 
  measureStart=micros(); // Beggining of the measurement
  previousReading=digitalRead(SIGNALPIN); // Performs the very first reading in the sequence (crucial line, do-not remove)
  while(measureTime<3000) // Prevents function from occupying more than 3 miliseconds of the CPU time
  {
    currentReading=digitalRead(SIGNALPIN); // Reads the digital signal pin input (0 or 1)
    if ((currentReading!= previousReading) & (firstTrigger!=1)) // If the first edge was detected
    {
      stateStart=micros(); // Start counting the half-cycle duration
     //  Serial.println("cycle start");
      firstTrigger=1; // assign trigger to 1 to allow the second edge detection processing    
    }  
    else if ((currentReading!= previousReading) & (firstTrigger==1)) // If the second edge was detected
    {
     //  Serial.println("cycle end");
      stateFinish=micros(); // Finish counting the half-cycle duration
      
      
   //   xTaskResumeAll(); // resumes the sheduller
    //  while(xTaskResumeAll() != pdTRUE)
    //  {xTaskResumeAll(); }

      
      stateTime=stateFinish-stateStart; // Calculate the half-cycle duration
      dataOut.measuredFreq=500000/stateTime; // Compute frequency in Hz [f= 1/T = 2/(T/2)
      goto here;
    }      
    measureFinish=micros(); // End of the measurement
    measureTime=measureFinish-measureStart; // Computing measurement time
    previousReading=currentReading; // Updating the previous reading state
  }
    //  xTaskResumeAll(); // resumes the sheduller
      //      while(xTaskResumeAll() != pdTRUE)
    //  {xTaskResumeAll(); }

  //  Serial.println("measurement fault");
  dataOut.measuredFreq=0; // In case of faulty measurement output zero
 here:
 xTaskResumeAll();
 measureTime=measureTime>>10;
 delayF=1000-measureTime;

  vTaskDelay(delayF);
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

void Watchdog(void *Parameters) // Generates the 50us pulse
{
  (void) Parameters;

   while(1) {

  digitalWrite(WATCHDOGPIN,HIGH);
  delayMicroseconds(50);
  digitalWrite(WATCHDOGPIN,LOW);

  vTaskDelay(25);
   }
}
