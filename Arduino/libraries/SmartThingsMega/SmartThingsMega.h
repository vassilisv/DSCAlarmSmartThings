//*******************************************************************************
/// @file
/// @brief
///   SmartThings Arduino Library 
/// @section License
///   (C) Copyright 2013 Physical Graph
//*******************************************************************************
#ifndef __SMARTTHINGS_H__
#define __SMARTTHINGS_H__

#include <inttypes.h>
#include <Arduino.h>

//*******************************************************************************
/// @brief SmartThings Arduino Library Version
//*******************************************************************************
#define SMARTTHINGSLIB_VERSION_MAJOR    0
#define SMARTTHINGSLIB_VERSION_MINOR    0
#define SMARTTHINGSLIB_VERSION_BUILD    5

#define SMARTTHINGSLIB_VERSION \
  (((SMARTTHINGSLIB_VERSION_MAJOR & 0xFF) << 24) |\
   ((SMARTTHINGSLIB_VERSION_MINOR & 0xFF) << 16) |\
   (SMARTTHINGSLIB_VERSION_BUILD & 0xFFFF))

//*******************************************************************************
#define SMARTTHINGS_RX_BUFFER_SIZE 256 // if > 255: change _nBufRX to u16 
#define SMARTTHINGS_SHIELDTYPE_SIZE 32 // if > 255: change _shieldTypeLen to u16 

//*******************************************************************************
/// @brief Callout Function Definition for Messages Received over ZigBee Network 
//*******************************************************************************
typedef void SmartThingsCallout_t(String message);

//*******************************************************************************
/// @brief ZigBee Network State Definition
//*******************************************************************************
typedef enum 
{
  STATE_NO_NETWORK, 
  STATE_JOINING,
  STATE_JOINED,
  STATE_JOINED_NOPARENT,
  STATE_LEAVING,
  STATE_UNKNOWN
} SmartThingsNetworkState_t;

//*******************************************************************************
//
class SmartThingsMega 
{
  private:
    HardwareSerial *prt;
    SmartThingsCallout_t *_calloutFunction;
    bool _isDebugEnabled;
    uint32_t _lastPingMS;
    uint32_t _lastShieldMS;

    SmartThingsNetworkState_t _networkState;
    uint8_t _eui64[8];
    uint16_t _nodeID;

    uint8_t _shieldTypeBuf[SMARTTHINGS_SHIELDTYPE_SIZE]; 
    uint8_t _shieldTypeLen; 

    uint8_t _pBufRX[SMARTTHINGS_RX_BUFFER_SIZE]; 
    uint_fast8_t _nBufRX;

    //void debugPrintString(String prefix, String s);
    //void debugPrintBuffer(String prefix, byte* b, int len);

    void _shieldGetNetworkInfo(void);
    void _process(void);

    void debugPrintBuffer(String prefix, uint8_t * pBuf, uint_fast8_t nBuf);
    bool isRxLine(uint8_t * pLine);
    bool isAsciiHex(uint8_t ascii);
    uint8_t asciiToHexU8(uint8_t pAscii[2]);
    uint_fast8_t translatePayload(uint8_t *pBuf, uint_fast8_t nBuf, char* payloadString);
    void handleLine(void);

  public:
    //*******************************************************************************
    /// @brief  SmartThings Constructor 
    /// @param[in] prt - Hardwre serial port
    /// @param[in] callout - Set the Callout Function that is called on Msg Reception
    /// @param[in] shieldType (optional) - Set the Reported SheildType to the Server 
    /// @param[in] enableDebug (optional) - Enable internal Library debug
    //*******************************************************************************
    SmartThingsMega(HardwareSerial *prt, SmartThingsCallout_t *callout, String shieldType="GenericShield", bool enableDebug=false);
    
    //*******************************************************************************
    /// @brief  Run SmartThings Library 
    //*******************************************************************************
    void run(void);

    //*******************************************************************************
    /// @brief Send Message out over ZigBee to the Hub 
    /// @param[in] message to send
    //*******************************************************************************
    void send(String message);

    //*******************************************************************************
    /// @brief  Set SmartThings Shield MultiColor LED
    /// @param[in]  red: intensity {0=off to 9=max}
    /// @param[in]  green: intensity {0=off to 9=max}
    /// @param[in]  blue: intensity {0=off to 9=max}
    //*******************************************************************************
    void shieldSetLED(uint8_t red, uint8_t green, uint8_t blue);

    //*******************************************************************************
    /// @brief  Get Last Read Shield State
    /// @return Last Read Network State
    //*******************************************************************************
    SmartThingsNetworkState_t shieldGetLastNetworkState(void);
    
    //*******************************************************************************
    /// @brief  Get Last Read Shield State and Trigger Refresh of Network Info 
    /// @return Last Read Network State
    //*******************************************************************************
    SmartThingsNetworkState_t shieldGetNetworkState(void);

    //*******************************************************************************
    /// @brief  Get Last Read NodeID and Trigger Refresh of Network Info
    /// @return Last Read NodeID 
    //*******************************************************************************
    uint16_t shieldGetNodeID(void);

    //*******************************************************************************
    /// @brief  Get Last Read EUI64 and Trigger Refresh of Network Info
    /// @return Last Read EUI64 
    //*******************************************************************************
    void shieldGetEUI64(uint8_t eui[8]);

    //*******************************************************************************
    /// @brief Find and Join a Network 
    //*******************************************************************************
    void shieldFindNetwork(void);

    //*******************************************************************************
    /// @brief Leave the Current ZigBee Network 
    //*******************************************************************************
    void shieldLeaveNetwork(void);

    //*******************************************************************************
    /// @brief Descructor 
    //*******************************************************************************
    ~SmartThingsMega();
};

#endif
