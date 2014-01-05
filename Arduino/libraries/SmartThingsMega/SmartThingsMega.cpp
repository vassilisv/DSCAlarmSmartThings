//*******************************************************************************
/// @file
/// @brief
///   SmartThingsMega Arduino Library 
/// @section License
///   (C) Copyright 2013 Physical Graph
//*******************************************************************************
#include <SmartThingsMega.h>

//*****************************************************************************
void SmartThingsMega::debugPrintBuffer(String prefix, uint8_t * pBuf, uint_fast8_t nBuf)
{
  if(_isDebugEnabled)
  {
    Serial.print(prefix);
    for(uint_fast8_t i = 0; i < nBuf; i++)
    {
      Serial.print(char(pBuf[i]));
    }
    Serial.println();
  }
}

//*****************************************************************************
bool SmartThingsMega::isRxLine(uint8_t * pLine)
{
  // line starts with "T00000000:RX"
  int validRxLineLength = 12; // TODO: What is a real value for this length?

  //  return line.length > validRxLineLength && line[0] == 'T' && line[9] = ':' && line[10] == 'R' && line[11] == 'X';
  return ((pLine[0] == 'T') && (pLine[9] == ':') && (pLine[10] == 'R') && (pLine[11] == 'X'));
}

//*******************************************************************************
bool SmartThingsMega::isAsciiHex(uint8_t ascii)
{
  bool retVal = false;
  if (
      ((ascii >= 'A') && (ascii <= 'F')) || 
      ((ascii >= 'a') && (ascii <= 'f')) ||
      ((ascii >= '0') && (ascii <= '9'))
     ) 
  {
    retVal = true;
  }
  return retVal;
}

//*******************************************************************************
/// @note this function doesn't check for hex validity before converting
//*******************************************************************************
uint8_t SmartThingsMega::asciiToHexU8(uint8_t pAscii[2])
{ 
  uint8_t hex;
  hex = ((pAscii[0] - (((pAscii[0]>>6) & 0x1) * 0x37)) & 0xF); 
  hex <<= 4;
  hex |= ((pAscii[1] - (((pAscii[1]>>6) & 0x1) * 0x37)) & 0xF); 
  return hex;
}

//*****************************************************************************
uint_fast8_t SmartThingsMega::translatePayload(uint8_t *pBuf, uint_fast8_t nBuf, char* payloadString)
{
  uint_fast8_t payloadLength = 0; // return value
  uint_fast8_t payloadStart = 0;
  uint_fast8_t payloadEnd   = 0;
  
  uint_fast8_t i;
  
  // find [ ] message from the back of the message
  for(i = nBuf-1; i > 0; i--)
  {
    if(pBuf[i] == ']')
    {
      payloadEnd = i;
    }
    else if(pBuf[i] == '[')
    {
      payloadStart = i;
      break;
    }
  }
 
  if(_isDebugEnabled)
  {
    Serial.print("payload start: ");
    Serial.print(payloadStart);
    Serial.print(" end: ");
    Serial.print(payloadEnd);
    Serial.print(" : ");
    for(i = payloadStart+1; i < payloadEnd; i++)
    {
      Serial.print(pBuf[i]);
      Serial.print(' ');
    }
    Serial.println();
  }

  //  int expectedPayloadLength = (payloadEnd - payloadStart) / 3; // TODO: Verify this, but 2 chars for byte and 1 for space char
  //  char payloadString[expectedPayloadLength];
  if((payloadStart != 0) && (payloadEnd !=  0) && (payloadEnd - payloadStart > 4) && (pBuf[payloadStart+1] == '0') && (pBuf[payloadStart+2] == 'A'))
  { // if valid message then parse
    i = payloadStart+4; // start+3 should be ' '  
    while (i < payloadEnd)
    {
      if(pBuf[i] != ' ')
      {
        if(isAsciiHex(pBuf[i]) && isAsciiHex(pBuf[i+1]) )
        {
          uint8_t hex = asciiToHexU8(&(pBuf[i]));
          payloadString[payloadLength++] = hex;
          i++;
        }
      }
      i++;
    }
  }

  payloadString[payloadLength] = 0x0; // null-terminate the string
  return payloadLength;
}

//*****************************************************************************
void SmartThingsMega::_process(void)
{
  uint32_t nowMilliseconds = millis();

  if((nowMilliseconds < _lastShieldMS) || ((nowMilliseconds - _lastShieldMS) > 5000))
  {
    _shieldGetNetworkInfo();
    _lastShieldMS = nowMilliseconds;
  }
  else if((_networkState == STATE_JOINED) && 
      ((nowMilliseconds < _lastPingMS) || ((nowMilliseconds - _lastPingMS) > 60000)))
  { // ping every minutes or on rollover
    send("ping");
    _lastPingMS = nowMilliseconds;
  }

  // check for return character
}

//*****************************************************************************
void SmartThingsMega::handleLine(void)
{
  if(_nBufRX > 0)
  {
    if(isRxLine(_pBufRX))
    {
      debugPrintBuffer("->| ", _pBufRX, _nBufRX);
      {
        char messageBuf[255]; // TODO: Figure this out
        uint_fast8_t messageBufLength = translatePayload(_pBufRX, _nBufRX, messageBuf);

        if(messageBufLength > 0)
        {
          debugPrintBuffer("->| payload :: ",(uint8_t *)messageBuf, messageBufLength);

          _calloutFunction(messageBuf);  // call out to main application
          // that.handleSmartThingMessage(message);
        }
        else
        {
          debugPrintBuffer("->| no payload from :: ", _pBufRX, _nBufRX);
        }
      }
    }
    else 
    { //XXX Totally slapped together since this is temp-- will go away with command set change 
      uint_fast8_t i=0;
      bool found = false;
      if (_nBufRX >= 32) //netinfo:0022A3000000B675,E30E,02 
      {
        while (i < _nBufRX)
        {
          if (
              (_pBufRX[i  ] == 'n') && 
              (_pBufRX[i+1] == 'e') && 
              (_pBufRX[i+2] == 't') && 
              (_pBufRX[i+3] == 'i') && 
              (_pBufRX[i+4] == 'n') && 
              (_pBufRX[i+5] == 'f') && 
              (_pBufRX[i+6] == 'o') && 
              (_pBufRX[i+7] == ':') &&
              (_pBufRX[i+24] == ',') &&
              (_pBufRX[i+29] == ',') 
             )
          { 
            // parse off EUI 
            if(
                isAsciiHex(_pBufRX[i+8]) && 
                isAsciiHex(_pBufRX[i+9]) && 
                isAsciiHex(_pBufRX[i+10]) && 
                isAsciiHex(_pBufRX[i+11]) && 
                isAsciiHex(_pBufRX[i+12]) && 
                isAsciiHex(_pBufRX[i+13]) && 
                isAsciiHex(_pBufRX[i+14]) && 
                isAsciiHex(_pBufRX[i+15]) && 
                isAsciiHex(_pBufRX[i+16]) && 
                isAsciiHex(_pBufRX[i+17]) && 
                isAsciiHex(_pBufRX[i+18]) && 
                isAsciiHex(_pBufRX[i+19]) && 
                isAsciiHex(_pBufRX[i+20]) && 
                isAsciiHex(_pBufRX[i+21]) && 
                isAsciiHex(_pBufRX[i+22]) && 
                isAsciiHex(_pBufRX[i+23]) &&

                isAsciiHex(_pBufRX[i+25]) && 
                isAsciiHex(_pBufRX[i+26]) && 
                isAsciiHex(_pBufRX[i+27]) && 
                isAsciiHex(_pBufRX[i+28]) &&

                isAsciiHex(_pBufRX[i+30]) && 
                isAsciiHex(_pBufRX[i+31]) 
                )
                {
                  uint8_t tempNetStat = asciiToHexU8(&(_pBufRX[i+30]));
                  if (tempNetStat <= STATE_LEAVING) // make sure it maps to the enum
                  {
                    _networkState = (SmartThingsNetworkState_t)tempNetStat;

                    _nodeID = asciiToHexU8(&(_pBufRX[i+25]));
                    _nodeID <<= 8; 
                    _nodeID |= asciiToHexU8(&(_pBufRX[i+27]));

                    _eui64[7] = asciiToHexU8(&(_pBufRX[i+8 ]));
                    _eui64[6] = asciiToHexU8(&(_pBufRX[i+10]));
                    _eui64[5] = asciiToHexU8(&(_pBufRX[i+12]));
                    _eui64[4] = asciiToHexU8(&(_pBufRX[i+14]));
                    _eui64[3] = asciiToHexU8(&(_pBufRX[i+16]));
                    _eui64[2] = asciiToHexU8(&(_pBufRX[i+18]));
                    _eui64[1] = asciiToHexU8(&(_pBufRX[i+20]));
                    _eui64[0] = asciiToHexU8(&(_pBufRX[i+22]));

                    debugPrintBuffer("  |~> ", &(_pBufRX[i]), 32);
                    found = true;
                  }
                }
          }
          i++;
        }
      }
      if (found == false)
        debugPrintBuffer("->| IGNORING :: ", _pBufRX, _nBufRX);
    }
    _nBufRX = 0;  
  }
}
  //*****************************************************************************
  void SmartThingsMega::_shieldGetNetworkInfo(void)
  {
    _prt->print("custom netinfo");
    _prt->print('\n');

    if(_isDebugEnabled)
    {
      Serial.print("  |<~ ");
      Serial.print("custom netinfo");
      Serial.print('\n');
    }
  }


  //*****************************************************************************
  // Thing API  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
  //            V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V V
  //*****************************************************************************
  SmartThingsMega::SmartThingsMega(HardwareSerial *prt, SmartThingsCallout_t *callout, String shieldType, bool enableDebug) :
    _prt(prt),
    _calloutFunction(callout), 
    _isDebugEnabled(enableDebug), 
    _nBufRX(0), 
    _lastPingMS(0xFFFFFF00),
    _lastShieldMS(0xFFFFFF00),
    _networkState(STATE_UNKNOWN)
  {
    uint_fast8_t i;

    i = shieldType.length();
    if (i > 32) 
      i = 32;
    _shieldTypeLen = i;

    while (i--)
    {
      _shieldTypeBuf[i] = (uint8_t)shieldType[i];
    } 

    _prt->begin(2400);
  }

  //*****************************************************************************
  //SmartThings::~SmartThings() : ~SoftwareSerial()
  SmartThingsMega::~SmartThingsMega() 
  {
  }

  //*****************************************************************************
  void SmartThingsMega::run(void)
  { 
    while((_nBufRX < SMARTTHINGS_RX_BUFFER_SIZE) && _prt->available())
    {
#if 0 
      _pBufRX[_nBufRX++] = read();
#else
      uint8_t readByte = _prt->read();
      if((readByte == 0x0D) || (readByte == 0x0A))
      { // handle data from SmartThing line-by-line 
        handleLine();
      }
      else
      { 
        // keep track of everything that comes in until we reach a newline
        // TODO(cwvh): validate bufferlength 1988-10-19
        //if (_nBufRX > 200)
        //  panic("too many characters!"); 
        _pBufRX[_nBufRX++] = readByte;
      }
#endif 
    }
    _process();
  }

  //*****************************************************************************
  void SmartThingsMega::send(String message)
  {
    // e.g. thing.print("raw 0x0 {00 00 0A 0A 62 75 74 74 6f 6e 20 64 6f 77 6e }");
    _prt->print("raw 0x0 { 00 00 0A 0A ");
    if(_isDebugEnabled)
    {
      Serial.print("<-| ");
      Serial.print("raw 0x0 { 00 00 0A 0A ");
    }

    for(int i = 0; i < message.length(); i++)
    {
      char c = message[i];

      _prt->print(c, HEX);
      _prt->print(' ');

      if(_isDebugEnabled)
      {
        Serial.print(c, HEX);
        Serial.print(' ');
      }
    }

    _prt->print('}');
    _prt->print('\n');
    if(_isDebugEnabled)
    {
      Serial.print('}');
      Serial.print('\n');
    }

    _prt->print("send 0x0 1 1");
    _prt->print('\n');
  }

  //*****************************************************************************
  void SmartThingsMega::shieldSetLED(uint8_t red, uint8_t green, uint8_t blue)
  {
    if (red > 9)    red = 9;
    if (green > 9)  green = 9;
    if (blue > 9)   blue = 9;

    _prt->print("custom rgb ");
    _prt->write( (red+'0') );
    _prt->print(' ');
    _prt->write( (green+'0') );
    _prt->print(' ');
    _prt->write( (blue+'0') );
    _prt->print(' ');
    _prt->print('\n');

    if(_isDebugEnabled)
    {
      Serial.print("  |<~ ");
      Serial.print("custom rgb ");
      Serial.write(red+'0');
      Serial.print(' ');
      Serial.write(green+'0');
      Serial.print(' ');
      Serial.write(blue+'0');
      Serial.print(' ');
      Serial.print('\n');
    }
  }


  //*****************************************************************************
  SmartThingsNetworkState_t SmartThingsMega::shieldGetLastNetworkState(void)
  {
    return _networkState;
  }

  //*****************************************************************************
  SmartThingsNetworkState_t SmartThingsMega::shieldGetNetworkState(void)
  {
    _shieldGetNetworkInfo();
    return _networkState;
  }

  //*****************************************************************************
  uint16_t SmartThingsMega::shieldGetNodeID(void)
  { 
    _shieldGetNetworkInfo();
    return _nodeID;
  }

  //*****************************************************************************
  void SmartThingsMega::shieldGetEUI64(uint8_t eui[8])
  {
    _shieldGetNetworkInfo();
    {
      uint_fast8_t i = 7;
      do 
      {
        eui[i] = _eui64[i];
      }
      while (i--);
    }
  }

  //*****************************************************************************
  void SmartThingsMega::shieldFindNetwork(void)
  {
    _prt->print("custom find");
    _prt->print('\n');

    if(_isDebugEnabled)
    {
      Serial.print("  |<~ ");
      Serial.print("custom find");
      Serial.print('\n');
    }
  }

  //*****************************************************************************
  void SmartThingsMega::shieldLeaveNetwork(void)
  {
    _prt->print("custom leave");
    _prt->print('\n');

    if(_isDebugEnabled)
    {
      Serial.print("  |<~ ");
      Serial.print("custom leave");
      Serial.print('\n');
    }
  }
