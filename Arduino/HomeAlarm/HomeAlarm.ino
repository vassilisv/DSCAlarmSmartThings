
 
#include <SmartThingsMega.h>
#include <Timer.h>

//#define DEBUG_ENABLE  
  
#define PIN_THING_LED  13

#define BUFFER_SIZE 128
#define MAX_ZONES 8

#define UPDATE_PERIOD_ARM 25283
#define UPDATE_PERIOD_ALARM 30123
#define UPDATE_PERIOD_ZONE 50234
 
SmartThingsCallout_t messageCallout;    // call out function forward decalaration
SmartThingsMega smartthing(&Serial2, messageCallout);  // constructor
Timer timer;
char buffer[BUFFER_SIZE];
boolean zones[MAX_ZONES];
boolean armed;
boolean alarm;
int bufferIdx;
  
void setup() 
{  
  // setup hardware pins 
  pinMode(PIN_THING_LED, OUTPUT);     // define PIN_LED as an output
  digitalWrite(PIN_THING_LED, LOW);   // set value to LOW (off) to match stateLED=0
  smartthing.shieldSetLED(0, 0, 0);
  // setup IT-100 serial port
  Serial1.begin(9600);
  // setup debug port
#ifdef DEBUG_ENABLE
  Serial.begin(9600);
  Serial.println("Ready");
#endif
  // initialize variables
  bufferIdx = 0;
  armed = false;
  alarm = false;
  for (int n = 0; n < MAX_ZONES; ++n)
  {
    zones[n] = false;
  }
  // setup periodic events
  timer.every(UPDATE_PERIOD_ARM, sendArmStatus);
  timer.every(UPDATE_PERIOD_ALARM, sendAlarmStatus);
  timer.every(UPDATE_PERIOD_ZONE, sendZoneStatus);
}
 
 
void loop()
{
  char data;
  // update periodic events
  timer.update();
  // run smartthing logic
  smartthing.run();
  // process IT-100 messages
  if (Serial1.available() > 0)
  {
    data = Serial1.read();
#ifdef DEBUG_ENABLE
    Serial.print(data);
#endif
    // if end of message then send to the cloud
    if (data == '\r' && bufferIdx > 0)
    {
      // create String object
      buffer[bufferIdx] = '\0';
      bufferIdx = 0;
      String str(buffer);
      // process IT-100 message
      if (str.length() > 5)
      {
        String cmd = str.substring(0,3);
        // zone open
        if (cmd.equals("609"))
        {
          int zone = str.substring(3,6).toInt() - 1;
          if (zone < MAX_ZONES)
          {
            zones[zone] = true;
          }
          sendZoneStatus();
        } 
        // zone closed
        else if (cmd.equals("610"))
        {
          int zone = str.substring(3,6).toInt() - 1;
          if (zone < MAX_ZONES)
          {
            zones[zone] = false;
          }
          sendZoneStatus();
        }
        // armed
        else if (cmd.equals("656") || cmd.equals("700"))
        {
          digitalWrite(PIN_THING_LED, HIGH);  // turn LED on
          smartthing.shieldSetLED(0, 0, 1);
          armed = true;
          sendArmStatus();          
        }    
        // disarmed (and alarm clear)
        else if (cmd.equals("655"))
        {
          // update status
          digitalWrite(PIN_THING_LED, LOW);  // turn LED off
          smartthing.shieldSetLED(0, 0, 0);
          armed = false;
          alarm = false;
          sendArmStatus(); 
          sendAlarmStatus();         
        } 
        // alarm triggered
        else if (cmd.equals("654"))
        {
          alarm = true;
          sendAlarmStatus();  
        }        
      }       
    } 
    // otherwise append to buffer (ignore \n)
    else if (data != '\n')
    {
      buffer[bufferIdx] = data;
      bufferIdx++;
      // check for buffer overruns
      if (bufferIdx >= BUFFER_SIZE) 
      {
        smartthing.send("ER:Buffer overrun"); 
        bufferIdx = 0;
      }
    }
  }
}

void sendArmStatus()
{
  String msg = String("AR:");
  if (armed) 
  {
    msg = msg + "1";
  }
  else
  {
    msg = msg + "0";
  }
  smartthing.send(msg); 
#ifdef DEBUG_ENABLE
  Serial.println();
  Serial.print(msg);
#endif   
}

void sendAlarmStatus()
{
  String msg = String("AL:");
  if (alarm) 
  {
    msg = msg + "1";
  }
  else
  {
    msg = msg + "0";
  }
  smartthing.send(msg); 
#ifdef DEBUG_ENABLE
  Serial.println();
  Serial.print(msg);
#endif  
}

void sendZoneStatus()
{
  String msg = String("ZN:");
  for (int n=0; n<MAX_ZONES; ++n) 
  {
    if (zones[n]) 
    {
      msg = msg + "1";
    } 
    else
    {
      msg = msg + "0"; 
    }
  }
  smartthing.send(msg); 
#ifdef DEBUG_ENABLE
  Serial.println();
  Serial.print(msg);
#endif  
}
 
void alarmArm()
{
  // setup and send arm command
  String cmd = String("0331****00"); <-------- IMPORTANT: replace * with pin code 
  cmd = appendChecksum(cmd);
  Serial1.print(cmd);
#ifdef DEBUG_ENABLE
  Serial.print(cmd);
#endif  
}
 
void alarmDisarm()
{
  // setup and send disarm command
  String cmd = String("0401****00"); <------- IMPORTANT: replace * with pin code
  cmd = appendChecksum(cmd);
  Serial1.print(cmd);
#ifdef DEBUG_ENABLE
  Serial.print(cmd);
#endif    
}

void panic()
{
  // setup and send panic command
  String cmd = String("0603");
  cmd = appendChecksum(cmd);
  Serial1.print(cmd);
#ifdef DEBUG_ENABLE
  Serial.print(cmd);
#endif  
}
  
void messageCallout(String message)
{ 
  if (message.equals("arm"))
  {
    alarmArm();
  }
  else if (message.equals("disarm"))
  {
    alarmDisarm();
  }
  else if (message.equals("panic"))
  {
    panic();
  }
}

String appendChecksum(String str)
{
  unsigned char cs = 0;
  for (int n = 0; n < str.length(); ++n)
  {
    cs += (unsigned char)str.charAt(n);
  }
  String csStr = String(cs, HEX);
  csStr.toUpperCase();
  return String(str + csStr + "\r\n");
}
