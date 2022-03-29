// Assignment 3 by Maksims Latkovskis
// Performs several functions using the FreeRTOS library (refer the subroutine description)

// Defining the single running core for the RTOS
#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

// Declaring I/O pins
#define ANALOGPIN 36 // Pin for analog input ADC1_CH0
#define ERRORPIN 23 // Pin for the error indication LED
#define WATCHDOGPIN 19 // Pin for the output watchdog waveform 
#define DIGITALINPUTPIN 22 // Pin for the pushbutton input
#define SIGNALPIN 18 // Pin for the external square wave input
#define ANALOGREADPIN 17 // Pin to display the analog reading time

// Declaring several global variables (protected with semaphores)
unsigned int analogReading; // Last analog reading to pass between AnalogRead and AnalogAverage
unsigned int errorCode; // Error code to pass between the AnalogAverage and ErrorCheck

struct memory{ // Defining new data type called "memory"
  char buttonInput; // State of the button
  unsigned int measuredFreq; // Measured frequency variable
  unsigned int analogAverage; // Analog average variable
  } dataOut; // Create new variable of type "memory" to pass the data to the serial port (protected using binary semaphore)

 int temp; //++++++++++++++++++delete it!


//Defining subroutines to be run using RTOS shedulling
void Watchdog(void *Parameters); // Subroutine that generates the 50us pulse watchdog
void AnalogRead(void *Parameters); // Subroutine that reads the analog input pin
void AnalogAverage(void *Parameters); // Subroutine that computes average analog readings over the last 4 measurements
void Nop1000times(void *Parameters); // Subroutine that forces CPU to do absolutely nothing 1000 times
void DigitalInput(void *Parameters); // Subroutine that reads the button state and if pressed outputs stream of data through the serial port
void FrequencyMeasure(void *Parameters); // Subroutine that measures the frequency of the 50% square signal in the range from 300 Hz up to 10 kHz
void ErrorCheck(void *Parameters); // Subroutine that evaluates the average analog value and sets appropriate error state
void ErrorDisplay(void *Parameters); // Subroutine that turns the LED indication on or off, depending on the error state

// Creating the reference parameters of NULL value for the semaphore creation (static prevents compiler from optimization)
static SemaphoreHandle_t  analogSMP=NULL; 
static SemaphoreHandle_t  dataOutSMP=NULL; 
static SemaphoreHandle_t  errorSMP=NULL; 

// Creating the reference parameters of NULL value for the queue creation (static prevents compiler from optimization)
static QueueHandle_t  analogAvgQUEUE=NULL;

//==============================================================================================================================================
void setup(void) 
{
  Serial.begin (115200); // Initialize serial communication with the 115200 baud rate
  pinMode(WATCHDOGPIN, OUTPUT);
  pinMode(ANALOGREADPIN, OUTPUT);
  pinMode(DIGITALINPUTPIN, INPUT);
  pinMode(DIGITALINPUTPIN, INPUT_PULLDOWN); // Prevents pin from being floating 
  pinMode(SIGNALPIN, INPUT);
  pinMode(SIGNALPIN, INPUT_PULLDOWN); // Prevents pin from being floating 
  pinMode(ERRORPIN, OUTPUT);

  analogSMP = xSemaphoreCreateBinary(); // Creating binary semaphore to protect analogReading global variable
  dataOutSMP = xSemaphoreCreateBinary(); // Creating binary semaphore to protect dataOut global variable
  errorSMP = xSemaphoreCreateBinary(); // Creating binary semaphore to protect errorCode global variable
  
  analogAvgQUEUE=xQueueCreate(1,sizeof(unsigned int)); // Creating queue of length 1 to pass the average analog value between tasks

  xTaskCreatePinnedToCore(
    Watchdog // Generates the 50us pulse watchdog
    ,  "Watchdog" // Subroutine name for debugging
    ,  587  // 523 bytes used + 64 bytes (RTOS requirement) = 587 
    ,  NULL // Subroutine parameter
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);
  
  xTaskCreatePinnedToCore(
    AnalogRead // Reads the analog value from the ANALOGPIN pin
    ,  "AnalogRead" // Subroutine name for debugging
    ,  754  // 690 bytes used + 64 bytes (RTOS requirement) = 754 
    ,  NULL // Subroutine parameter
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    AnalogAverage // Computes average of last four analog readings
    ,  "AnalogAverage" // Subroutine name for debugging
    ,  528  // 464 bytes used + 64 bytes (RTOS requirement) = 528 
    ,  NULL // Subroutine parameter
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
    Nop1000times // Sets CPU in no-operation state for 1000 clock cycles
    ,  "Nop1000times" // Subroutine name for debugging
    ,  532  // 468 bytes used + 64 bytes (RTOS requirement) = 532 
    ,  NULL // Subroutine parameter
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
   DigitalInput
    ,  "DigitalInput" // Subroutine name for debugging
    ,  800  // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL // Subroutine parameter
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
   FrequencyMeasure // Measures the frequency of the 50% duty cycle square wave
    ,  "FrequencyMeasure" // Subroutine name for debugging
    ,  540 // This stack size can be checked & adjusted by reading the Stack Highwater
    ,  NULL // Subroutine parameter
    ,  3  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
       ErrorCheck // Checks whether the average analog value is greater than the half of the maximum possible analog reading (?>(4096/2)-1)
    ,  "ErrorCheck"  // Subroutine name for debugging
    ,  544 // 480 bytes used + 64 bytes (RTOS requirement) = 544 
    ,  NULL // Subroutine parameter
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
       ErrorDisplay // Toggles the LED if the error occurs
    ,  "ErrorDisplay" // Subroutine name for debugging
    ,  526 // 462 bytes used + 64 bytes (RTOS requirement) = 526 
    ,  NULL // Subroutine parameter
    ,  1  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xSemaphoreGive(dataOutSMP); // Makes the semaphore to be initially 1 to operate properly 
}
//==============================================================================================================================================

//==============================================================================================================================================
void loop(void) {} //Not used, since it is RTOS
//==============================================================================================================================================

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
  xQueueOverwrite(analogAvgQUEUE,&dataOut.analogAverage); // saving average value into the queue
  xSemaphoreGive(dataOutSMP);
   vTaskDelay(40); //42-2 for synronisation
  }
}


//----------------------------------------------------------------------------------------------------------------------------------------------
void Nop1000times(void *Parameters) // Sets CPU in no-operation state for 1000 clock cycles; two for loops to simplify counter register access
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
  unsigned int average;
  while(1){
 // xSemaphoreTake(dataOutSMP,portMAX_DELAY);
 xQueueReceive(analogAvgQUEUE,&average,0); // read from queue instantly
  if(average>2047) {errorCode = 1;} // Setting the error flag 
  else{errorCode=0;} // Clearing the error flag
  xSemaphoreGive(errorSMP); // synchronisation semaphore
  // xSemaphoreGive(dataOutSMP);
     
   vTaskDelay(333);
  }
  
}
//----------------------------------------------------------------------------------------------------------------------------------------------
void ErrorDisplay(void *Parameters) // Toggles the LED if the error occurs
{ 
  (void) Parameters; // No parameters are being passed to the subroutine
  while(1) // Run this code forever
  {
    xSemaphoreTake(errorSMP,portMAX_DELAY); // Wait infinitely long for the error code to be assigned (synchronisation)
    digitalWrite(ERRORPIN,errorCode); // Switch the LED either ON or OFF
    vTaskDelay(330); // Making the repetition period to be 330 ms to allow smooth syncronisation with ErrorCheck subroutine
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

//----------------------------------------------------------------------------------------------------------------------------------------------
void Watchdog(void *Parameters) // Subroutine that generates the 50us pulse watchdog
{
   (void) Parameters; // No parameters are being passed to the subroutine
   while(1) // Run this code forever
   {
      digitalWrite(WATCHDOGPIN,HIGH);
      delayMicroseconds(50);
      digitalWrite(WATCHDOGPIN,LOW);
      vTaskDelay(25); // Makes the repetition period of 25 miliseconds (40 Hz)
   }
}
//----------------------------------------------------------------------------------------------------------------------------------------------
