// Assignment 3 by Maksims Latkovskis
// Performs several functions using the FreeRTOS library (refer the task description)

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
    ,  560  // 496 bytes used + 64 bytes (RTOS requirement) = 560 
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
   DigitalInput // Reads the digital state of the button and prints data to the serial port if the button is pressed
    ,  "DigitalInput" // Subroutine name for debugging
    ,  564  // 500 bytes used + 64 bytes (RTOS requirement) = 564
    ,  NULL // Subroutine parameter
    ,  2  // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
    ,  NULL // Subroutine handler (not used in this code)
    ,  ARDUINO_RUNNING_CORE);

  xTaskCreatePinnedToCore(
   FrequencyMeasure // Measures the frequency of the 50% duty cycle square wave
    ,  "FrequencyMeasure" // Subroutine name for debugging
    ,  532 // 468 bytes used + 64 bytes (RTOS requirement) = 532
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

//----------------------------------------------------------------------------------------------------------------------------------------------
void DigitalInput(void *Parameters) // Reads the digital state of the button and prints data to the serial port if the button is pressed
{
  (void) Parameters; // No parameters are being passed to the subroutine
  while(1) // Run this code forever
  {
    xSemaphoreTake(dataOutSMP,portMAX_DELAY); // Wait infinitely long for the protected accessing of the dataOut variable 
    dataOut.buttonInput= digitalRead(DIGITALINPUTPIN); // Reads the digital state of the button (either 0 or 1)
    if(dataOut.buttonInput==1) // If the button is pressed
    {  
      SerialOutput(); // Printing button_state, measured_frequency, average_analog_value, to the serial port in the CSV format
    } 
    xSemaphoreGive(dataOutSMP); // Releases the semaphore and exits the critical region of the dataOut access
    vTaskDelay(200); // Makes the repetition period of 200 miliseconds (5 Hz)
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void FrequencyMeasure(void *Parameters) // Function to measure the frequency of the 50% duty cycle square wave
{ 
  (void) Parameters; // No parameters are being passed to the subroutine
  
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

  unsigned int delayF; // for inserting appropriate delay after the task finished
 
  while(1) // Run this code forever
  {
    measureTime=0;  // Assign measuring time back to zero
    firstTrigger=0; // Assign edge trigger back to zero
  
    vTaskSuspendAll();  // suspends the shedulling (this function gets maximum absolute priority to precisely measure the time)
    measureStart=micros(); // Beggining of the measurement
    previousReading=digitalRead(SIGNALPIN); // Performs the very first reading in the sequence (crucial line, do-not remove)
    while(measureTime<3000) // Prevents function from occupying more than 3 miliseconds of the CPU time
    {
      currentReading=digitalRead(SIGNALPIN); // Reads the digital signal pin input (0 or 1)
      if ((currentReading!= previousReading) & (firstTrigger!=1)) // If the first edge was detected
      {
        stateStart=micros(); // Start counting the half-cycle duration
        firstTrigger=1; // assign trigger to 1 to allow the second edge detection processing    
      }  
      else if ((currentReading!= previousReading) & (firstTrigger==1)) // If the second edge was detected
      { 
        stateFinish=micros(); // Finish counting the half-cycle duration      
        stateTime=stateFinish-stateStart; // Calculate the half-cycle duration
        dataOut.measuredFreq=500000/stateTime; // Compute frequency in Hz [f= 1/T = 2/(T/2)
        goto here; // Complete measurement procedure
      }      
      measureFinish=micros(); // End of the measurement
      measureTime=measureFinish-measureStart; // Computing measurement time
      previousReading=currentReading; // Updating the previous reading state
    }
    
    dataOut.measuredFreq=0; // In case of faulty measurement output zero
    here: // Address line
    xTaskResumeAll(); // Resume the shedulling
    measureTime=measureTime>>10; // Divide the consumed time by roughly a thousand
    delayF=1000-measureTime; // Assign required delay time
    vTaskDelay(delayF);  // Makes the repetition period of approximately 1 second (Approx. 1 Hz)
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void AnalogRead(void *Parameters) // Reads the analog value from the ANALOGPIN pin
{
  (void) Parameters; // No parameters are being passed to the subroutine
  while(1) // Run this code forever
  {
    digitalWrite(ANALOGREADPIN, HIGH); // Indicate the start of the ADC
    analogReading = analogRead(ANALOGPIN);
    xSemaphoreGive(analogSMP);  // Syncronising this function with the AnalogAverage to prevent data corruption 
    digitalWrite(ANALOGREADPIN, LOW); // Indicate the end of the ADC
    vTaskDelay(40); // Makes the repetition period of 41.5 miliseconds (Approx. 24 Hz) The function itself takes around 1.5 miliseconds to execute, so the error becames smaller
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void AnalogAverage(void *Parameters) // Computes average of last four analog readings
{ 
  (void) Parameters; // No parameters are being passed to the subroutine
  char analogAdress; // Analog readings array indexing
  unsigned int analogData[4]; // Array to store 4 last values from the ADC
  unsigned int analogAvg; // Temporary analog average value storage to reduce the critical section length
  while(1) // Run this code forever
  {
    // This section logs the most recent analog reading to the longest-ever updated cell (there are 4 cells in total)
    if(analogAdress>=4) {analogAdress=0;} // Making sure only valid array cells are accessed
  
    xSemaphoreTake(analogSMP,portMAX_DELAY); // Wait infinitely long for the analog reading to be assigned (synchronisation with the AnalogRead funtion)
    analogData[analogAdress]=analogReading; // Loggin the last analog reading into the array cell
    analogAdress++; // Icrementing array adress
    
    // This section computes the average of last four readings
    analogAvg=analogData[0]+analogData[1]+analogData[2]+analogData[3]; // Sums all values in the array
    analogAvg=analogAvg>>2; // Divides the sum by four to obtain average
    xQueueOverwrite(analogAvgQUEUE,&analogAvg); // Saves the average value into the queue to pass it to the ErrorCheck function
    
    xSemaphoreTake(dataOutSMP,portMAX_DELAY); // Wait infinitely long for the protected accessing of the dataOut variable 
    dataOut.analogAverage=analogAvg; // Assigning the serial output data to computed analog average value
    xSemaphoreGive(dataOutSMP); // Releases the semaphore and exits the critical region of the dataOut access
    vTaskDelay(39); // Making the repetition period to be 39 ms to allow smooth syncronisation with AnalogRead subroutine
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void ErrorCheck(void *Parameters) // Checks whether the average analog value is greater than the half of the maximum possible analog reading (?>(4096/2)-1)
{
  (void) Parameters; // No parameters are being passed to the subroutine
  unsigned int average; // Variable to read the average analog value from the designated queue
  while(1) // Run this code forever
  {
    xQueueReceive(analogAvgQUEUE,&average,0); // Reading value from the queue instantly (wait is unnecessary because queue is getting updated more frequently than this function is called)
    if(average>2047) {errorCode = 1;} // Setting the error flag if the average value is greater than the half of maximum input value
    else{errorCode=0;} // Clearing the error flag if otherwise
    xSemaphoreGive(errorSMP); // Syncronising this function with the ErrorDisplay to prevent data corruption
    vTaskDelay(333); // Makes the repetition period of 333 miliseconds (Approx. 3 Hz)
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void ErrorDisplay(void *Parameters) // Toggles the LED if the error occurs
{ 
  (void) Parameters; // No parameters are being passed to the subroutine
  while(1) // Run this code forever
  {
    xSemaphoreTake(errorSMP,portMAX_DELAY); // Wait infinitely long for the error code to be assigned (synchronisation with the ErrorCheck funtion)
    digitalWrite(ERRORPIN,errorCode); // Switch the LED either ON or OFF
    vTaskDelay(330); // Making the repetition period to be 330 ms to allow smooth syncronisation with ErrorCheck subroutine
  }
}
//----------------------------------------------------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------------------------------------------------
void Nop1000times(void *Parameters) // Sets CPU in no-operation state for 1000 clock cycles; two for loops to simplify counter register access
{
  (void) Parameters; // No parameters are being passed to the subroutine
  while(1) // Run this code forever
  {
    for(char i=0; i++; i<4)
    {
      for(char j=0; i++; i<250)
      {
        __asm__ __volatile__ ("nop"); // "volatile" is needed to prevent optimisation by the compiler    
      }
    }
    vTaskDelay(100); // Makes the repetition period of 100 miliseconds (10 Hz)
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

//----------------------------------------------------------------------------------------------------------------------------------------------
void SerialOutput(void) // Printing the data on the serial monitor (Not the RTOS task)
{
  Serial.print(dataOut.buttonInput,DEC); // Printing the button state
  Serial.print(",");
    
  Serial.print(dataOut.measuredFreq,DEC); // Printing measured frequency of the 50% duty cycle square signal
  Serial.print(",");
  
  Serial.println(dataOut.analogAverage,DEC); // Printing average of the 4 last analog readings
}
//----------------------------------------------------------------------------------------------------------------------------------------------
