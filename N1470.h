#ifndef N1470_H
#define N1470_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ftd2xx.h"

#include <iostream>
#include <cstring>
#include <string>
#include <sstream>

#define CH_MAX 4 // number of channels on board
#define CMD_LIST_LEN 12
#define RESPONSE_TIME 1 // the number of seconds that the board needs
                        // to respond to a normal request
#define BUFFER_SIZE 512
#define PRINT_ERR(name, err) fprintf(stderr,"Function %s failed with error code %lu in line %d of file %s\n", name, err, __LINE__, __FILE__)

// Header file for C++ module related to CAEN N1470 4-channel HV NIM module
// Note that the documentation switches between iset and ilim for the same quantity
// We restrict ourselves to iset for consistency.

class N1470{
	
 private:

  // Board ID
  int BD_;
  // Device handle
  FT_HANDLE dev_; 

  // Is the module represented by this object connected?
  bool connected_;
			
  // Hardware settings
  double vmon_[4]; // Measured voltage in V
  double imon_[4]; // Measured current in uA

  double vset_[4]; // Set value in V
	
  double iset_[4]; // Limit value in uA

  double vmax_[4]; // Max voltage each channel can reach in V

  int rampup_[4]; // Ramp up rate in V/s
  int rampdown_[4]; // Ramp down rate in V/s

  double triptime_[4]; // Maximum time a current over iset is allowed in seconds before trip
  int tripmode_[4]; // if 0, trip means ramp down, if 1, trip means kill

  int interlock_; // 0 = OPEN, 1 = CLOSED

  // Commands

  std::string board_name_,mon_cmd_, vset_cmd_, iset_cmd_, vmax_cmd_, rampup_cmd_, rampdown_cmd_, triptime_cmd_, tripmode_cmd_;
  std::string channel_on_cmd_, channel_off_cmd_, interlock_cmd_, clearalarm_cmd_;
			
  // Forms a command to send to the module
  // Takes a name from the private list above, a target of what to replace and a replacement
  // Returns a char* to the command that should be sent to the module.
  char * formCommand(std::string, std::string, std::string);

  // Writes a command to the N1470 and checks to make sure that it is written.
  // Takes a command string and returns 0 if no problems.
  int writeCommand(char *);

  // Get response from the board following a command
  // Takes std::string pointer to store response string 
  // Return 0 if response arrives, non-zero if response failed. In that case, response string set to NULL
  int getResponse(std::string *);

  // Parse the response and determine if error
  // If error, call parseError()
  // If not error, fill the appropriate internal variables
  // Return 0 if OK, non-zero if error.
  int parseResponse(std::string *, int type, double *value);

  // Parse an error response
  // returns 0 if error string parsed correctly, non-zero if error string not parsed
  int parseError(std::string);


  // Takes a channel number as argument and checks that it is within [0,3]
  void channelCheck(int);

  

 public:

  N1470(int);

  // Prints the current status to stdout
  // Returns status field
  double printStatus(int);
  
  // Returns true if connected, false if not connected
  bool isConnected(){ return connected_; }

  // Make the connection to the physical module. Sets private connected variable.
  int makeConnection();
  int dropConnection();

  // Returns 0 on success, non-zero on failure. Takes a channel number [0->3]
  int switchState(int, bool);

  // Sets the voltage for a channel in Volts. Takes channel number [0-3] and value [0000.00 - 8000.00]. Returns correct value on success.
  double setVoltage(int, double);
  // Same for current
  double setCurrent(int, double);

  // Sets the maximum voltage for a channel. Takes channel number (0-3) and value (0000.00-8100.00). Returns correct value on success.
  double setMaxVoltage(int, double);

  // Sets the ramp up for a channel. Takes channel number (0-3) and value (000 - 999). Returns correct value on success.
  double setRampUpRate(int, double);
  // Sets the ramp down for a channel. Takes channel number (0-3) and value (000 - 999). Returns correct value on success
  double setRampDownRate(int, double);
		
  // Sets the trip time for a channel. Takes channel number (0-3) and value (0000.0 - 9999.9). Returns correct value on success, -9999 on error.
  double setTripTime(int, double);

  // Sets the power down mode for a channel. Takes a channel number and an integer: 0 = RAMP, 1 = KILL. Returns 0 on success.
  int setTripmode(int, int);

  // Sets the interlock mode. 0 = OPEN, 1 = CLOSED. Returns 0 on success, -9999 on error.
  int setInterlock(int);

  // Clears the board alarm. Returns 0 on success, -9999 on error.
  int clearAlarm();


  // Getters
		
  // returns the device handle of the current module if set. If not set, return NULL.
  FT_HANDLE getDeviceHandle(){ if (connected_) return dev_; else return NULL;}

  // Prints Board name to stdout
  // returns -1 in case of error
  int readBoardName();

  // gets the actual settings on the board
  double getActualVoltage(int channel);
  double getActualCurrent(int channel);
  // gets the maximum voltage for a channel. Takes channel number (0-3). Returns correct value on success.
  double getMaxVoltage(int ch);

  // gets the ramp up for a channel. Takes channel number (0-3). Returns correct value on success.
  double getRampUpRate(int ch);
  // gets the ramp down for a channel. Takes channel number (0-3). Returns correct value on success.
  double getRampDownRate(int ch);
		
  // gets the trip time for a channel. Takes channel number (0-3). Returns correct value on success, -9999 on error.
  double getTripTime(int ch);//{ if ((ch >= 0) && (ch < CH_MAX)) return triptime_[ch]; else return -9999; }
  double getPolarity(int);


  void parseChannelStatus(double);

};

#endif
