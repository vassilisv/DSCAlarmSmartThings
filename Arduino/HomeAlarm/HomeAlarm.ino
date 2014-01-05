
#include <SoftwareSerial.h>   
#include <SmartThings.h>
 
 
#define PIN_THING_RX    3
#define PIN_THING_TX    2
#define PIN_THING_LED  13

#define BUFFER_SIZE 256
 
SmartThingsCallout_t messageCallout;    // call out function forward decalaration
SmartThings smartthing(PIN_THING_RX, PIN_THING_TX, messageCallout);  // constructor
char buffer[BUFFER_SIZE];
int bufferIdx;
  
void setup()
{  
  // setup hardware pins 
  pinMode(PIN_THING_LED, OUTPUT);     // define PIN_LED as an output
  digitalWrite(PIN_THING_LED, LOW);   // set value to LOW (off) to match stateLED=0
  // setup IT-100 serial port
  Serial.begin(9600);
  // initialize variables
  bufferIdx = 0;
}
 
 
void loop()
{
  char data;
  // run smartthing logic
  smartthing.run();
  // process IT-100 messages
  if (Serial.available() > 0)
  {
    data = Serial.read();
    // if end of message then send to the cloud
    if (data == 'r' || data == 'n')
    {
      buffer[bufferIdx] = '\0';
      smartthing.send(buffer);
      bufferIdx = 0;
    } 
    // otherwise append to buffer
    else
    {
      buffer[bufferIdx] = data;
      bufferIdx++;
      // check for buffer overruns
      if (bufferIdx >= BUFFER_SIZE) 
      {
        bufferIdx = 0;
      }
    }
  }
}
 
void on()
{
  digitalWrite(PIN_THING_LED, HIGH);  // turn LED on
  smartthing.shieldSetLED(0, 0, 1);
  smartthing.send("on");        // send message to cloud
  Serial.println("on");
}
 
void off()
{
  digitalWrite(PIN_THING_LED, LOW);   // turn LED off
  smartthing.shieldSetLED(0, 0, 0);
  smartthing.send("off");       // send message to cloud
  Serial.println("off");
}
  
 
void messageCallout(String message)
{ 
  // if message contents equals to 'on' then call on() function
  // else if message contents equals to 'off' then call off() function
  if (message.equals("on"))
  {
    on();
  }
  else if (message.equals("off"))
  {
    off();
  }
}
