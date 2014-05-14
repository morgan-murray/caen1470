
#include "N1470.h"

// // Default constructor. Make everything apart from the board number zero and get
// actual values in initialize function
// String commands are taken from the N1470 manual available at caen.it
N1470::N1470(int boardNumber) : 
  BD_(boardNumber),
  connected_(false), 
  interlock_(0), 
  board_name_("$BD:XX,CMD:MON,PAR:BDNAME\r\n"),
  mon_cmd_("$BD:XX,CMD:MON,CH:X,PAR:XX"),
  vset_cmd_("$BD:XX,CMD:SET,CH:X,PAR:VSET,VAL:XX"),
  iset_cmd_("$BD:XX,CMD:SET,CH:X,PAR:ISET,VAL:XX"),
  vmax_cmd_("$BD:XX,CMD:SET,CH:X,PAR:MAXV,VAL:XX"),
  rampup_cmd_("$BD:XX,CMD:SET,CH:X,PAR:RUP,VAL:XX"),
  rampdown_cmd_("$BD:XX,CMD:SET,CH:X,PAR:RDW,VAL:XX"),
  triptime_cmd_("$BD:XX,CMD:SET,CH:X,PAR:TRIP,VAL:XX"),
  tripmode_cmd_("$BD:XX,CMD:SET,CH:X,PAR:PDWN,VAL:XX"),
  channel_on_cmd_("$BD:XX,CMD:SET,CH:X,PAR:ON"),
  channel_off_cmd_("$BD:XX,CMD:SET,CH:X,PAR:OFF"),
  interlock_cmd_("$BD:XX,CMD:SET,PAR:BDILKM,VAL:XX"),
  clearalarm_cmd_("$BD:XX,CMD:SET,PAR:BDCLR"){

  std::string * cmd_list_[] = {&mon_cmd_, &vset_cmd_, &iset_cmd_, &vmax_cmd_, 
		 &rampup_cmd_, &rampdown_cmd_, &triptime_cmd_, 
		 &tripmode_cmd_, &channel_on_cmd_, &channel_off_cmd_,
		 &interlock_cmd_, &clearalarm_cmd_};  
  
  // set the board number in the command strings above
  int pos;
  std::ostringstream replacement;
  std::string target = "$BD:XX";
  
  replacement << "$BD:" << BD_;
  
  for (int cmd = 0; cmd < CMD_LIST_LEN_N1470; cmd++){

    pos = cmd_list_[cmd]->find(target);
    cmd_list_[cmd]->replace(pos,target.size(),replacement.str());

  }
  
  // Set all the intial values to 0
  for (int ch = 0; ch <= 3; ch++){
    
    vmon_[ch] = 0.0;
    imon_[ch] = 0.0;
    vset_[ch] = 0.0;
    iset_[ch] = 0.0;
    vmax_[ch] = 0.0;
    
    rampup_[ch] = 0;
    rampdown_[ch] = 0;
    
    triptime_[ch] = 0.0;
    
    tripmode_[ch] = 1; // By default have trip mean kill
  }
  
};


char * N1470::formCommand(std::string command, std::string target, std::string replacement){

  int position;
  char * cmd;
  unsigned int bufLen;

  std::string newCommand;

  position = command.find(target);
  if (position < 0){
    std::cerr << "Target string " << target << " not found" << std::endl;
    exit(1);
  }
  newCommand = command.replace(position, target.size(), replacement);
  
  // N1470 accepts C-style strings, now convert from std::string to c_str style
  // need to append a Windows-style line ending (\<cr>\<lf>) to make N1470 realise it's the end of the command
  bufLen = newCommand.size() + 3;
  cmd = (char *)malloc(bufLen);

  if (cmd == NULL){
    fprintf(stderr,"No memory allocation possible!\n");
    return NULL;
  }
  
  std::strcpy(cmd,command.c_str());
  strncat(cmd,"\r\n",2);
  
  return cmd;
 
} 


int N1470::makeConnection(){

#ifndef NO_DEVICE
  
 
  unsigned long ret;
  
  if ((ret = FT_Open(0, &dev_)) != FT_OK){
    
    PRINT_ERR("FT_Open", ret);
    return -1;
    
  }

  if ((ret = FT_SetBaudRate(dev_, FT_BAUD_9600)) != FT_OK){
    
    PRINT_ERR("FT_SetBaudRate",ret);
    return -2;
    
  }
  
  
  if ((ret = FT_SetDataCharacteristics(dev_, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE )) != FT_OK){
    
    PRINT_ERR("FT_SetDataCharacteristics",ret);
    return -3;
    
  }
  
  
  if ((ret = FT_Purge(dev_, (FT_PURGE_RX & FT_PURGE_TX))) != FT_OK){
    
    PRINT_ERR("FT_Purge", ret);
    return -4;
    
  }
  
#endif
  connected_ = true;



#ifdef DEBUG
  fprintf(stderr,"Connected to the device with Board ID: %d\n",BD_);
#endif
	
  return 0;
  
};	

int N1470::readBoardName(){
  
  std::string * response = new std::string();

  char cmd[80];

  strcpy(cmd,board_name_.c_str());

  if(writeCommand(cmd)!=0){

    std::cerr << "Problem writing command to get board name!\n" << std::endl;
    delete(response);
    return -1;
  }

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    delete(response);
    exit(1);
  }

#ifdef DEBUG
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

  int loc = response->find("N1470");
  delete(response);

  if (loc >= 0){
    return 0;
  }
  else{
    return -1;
  }
}


int N1470::dropConnection(){

#ifndef NO_DEVICE

	unsigned long ret;

	if ((ret = FT_Purge(dev_, (FT_PURGE_RX & FT_PURGE_TX))) != 0){

		PRINT_ERR("FT_Purge", ret);
		return -1;

	}


	if ((ret = FT_Close(dev_)) != FT_OK){

		PRINT_ERR("FT_Close", ret);
		return -2;
	}

#endif
	connected_ = false;


#ifdef DEBUG
	fprintf(stderr,"Disconnected from the device with Board ID: %d\n",BD_);
#endif
	return 0;
};	


int N1470::switchState(int channel, bool state){

  char * cmd;
  std::string target = "CH:X";
  std::ostringstream replacement;
  std::string *response = new std::string();
  // Make sure connected
  if (!connected_){
  	
  	fprintf(stderr,"Cannot switch on a channel on a module that is not connected\n");
  	PRINT_ERR("switchState",(unsigned long) channel);
  	return -1;
  	
  }
  
  // Check input
  if ((channel < 0) || (channel >=4)){

    PRINT_ERR("switchState",(unsigned long)channel);
    return -channel;
  }

  // Form command properly
  replacement << "CH:" << channel;
  
  if (state){
    cmd = this->formCommand(channel_on_cmd_, target, replacement.str());
  } else{
    cmd = this->formCommand(channel_off_cmd_, target, replacement.str());
  }

  
#ifdef DEBUG_MAX
  // Write the actual command unless there's no device presenta s defined
  // at compile time, in which case we fake the connection
  // and pretend everything is OK
  fprintf(stderr,"Writing command to switch state on N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  
  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem switching state on channel" << channel << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,1,NULL) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }


  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return 0;
}


void N1470::channelCheck(int channel){

  if (channel < 0 || channel >=4){
    std::cerr << "Channel call of " << channel << " not understood" << std::endl;
    exit(1);
  }

}
    
int N1470::writeCommand(char *cmd){

  int bufLen, bufWrit, ret;
  bufLen = strlen(cmd); 

#ifdef DEBUG_MAX

  std::cerr << "Writing the following command to the device: " << cmd << std::endl;

#endif

#ifndef NO_DEVICE

  if ((ret = FT_Write(dev_,cmd,bufLen,(LPDWORD) &bufWrit)) != FT_OK){

    PRINT_ERR("FT_Write", (long unsigned)ret);
    free(cmd);	
    return -1;
  }

#else
  std::cerr << "Faking successful write" << std::endl;
  bufWrit = bufLen;
#endif
  
  if(bufWrit != bufLen){
    fprintf(stderr, "Buffersize mismatch: bufLen %u \t bufWrit %u\n",bufLen,bufWrit);
    return -1;
  }

 return 0;

}

double N1470::printStatus(int channel){
  
  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;

  double status;
 
  channelCheck(channel);

  replacement << "CH:" << channel << ",PAR:STAT";
  cmd = this->formCommand(mon_cmd_,target,replacement.str());

#ifdef DEBUG_MAX
  std::cerr << "Getting the status of channel " << channel << std::endl;
#endif

 if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the voltage" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif

  if (parseResponse(response,2,&status) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }

#ifdef DEBUG
  fprintf(stderr,"Status was %x\n",(unsigned)status);
#endif

  // No memory leaks!                                                      
  free(cmd);
  delete(response);
  parseChannelStatus(status);
  return status;

}

double N1470::getActualVoltage(int channel){

  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
  double voltage;
 
  channelCheck(channel);

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:VMON";
  cmd = this->formCommand(mon_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to get actual value of voltage N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the voltage" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,2,&voltage) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Voltage was " << voltage << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return voltage;

}

double N1470::getActualCurrent(int channel){

  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
  double current;
 
  channelCheck(channel);

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:IMON";
  cmd = this->formCommand(mon_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to get actual current N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the current" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,2,&current) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Current was " << current << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return current;

}


double N1470::getTripTime(int channel){

  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
  double tripTime;
 
  channelCheck(channel);

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:TRIP";
  cmd = this->formCommand(mon_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to get trip time N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the trip time" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,2,&tripTime) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Trip Time is " << tripTime << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return tripTime;

}

double N1470::getPolarity(int channel){

  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
  double polarity;
 
  channelCheck(channel);

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:POL";
  cmd = this->formCommand(mon_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to get polarity N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the polarity" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,2,&polarity) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Polarity is " << polarity << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return polarity;

}

double N1470::getMaxVoltage(int channel){

  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
  double voltage;
 
  channelCheck(channel);

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:MAXV";
  cmd = this->formCommand(mon_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to get trip time N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the trip time" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,2,&voltage) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Max voltage is " << voltage << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return voltage;

}

double N1470::getRampUpRate(int channel){

  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
  double rate;
 
  channelCheck(channel);

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:RUP";
  cmd = this->formCommand(mon_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to get ramp up rate N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the ramp up rate" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,2,&rate) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Ramp up rate is " << rate << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return rate;

}

double N1470::getRampDownRate(int channel){

  char * cmd;
  std::string target = "CH:X,PAR:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
  double rate;
 
  channelCheck(channel);

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:RDW";
  cmd = this->formCommand(mon_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to get ramp down rate N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to read out the ramp down rate" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,2,&rate) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Ramp down rate is " << rate << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return rate;

}

double N1470::setRampUpRate(int channel, double rate){

  char * cmd;
  std::string target = "CH:X,PAR:RUP,VAL:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
 
  channelCheck(channel);
  if (rate < 0 || rate > 500){
    std::cerr << "Rate is outside of limits" << std::endl;
    return -1;
  }

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:RUP,VAL:" << rate;
  cmd = this->formCommand(rampup_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to set ramp up rate N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to set the ramp up rate" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,1,&rate) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Ramp up rate was set to " << rate << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return rate;	
}

double N1470::setRampDownRate(int channel, double rate){

  char * cmd;
  std::string target = "CH:X,PAR:RDW,VAL:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
 
  channelCheck(channel);
  if (rate < 0 || rate > 500){
    std::cerr << "Rate is outside of limits" << std::endl;
    return -1;
  }

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:RDW,VAL:" << rate;
  cmd = this->formCommand(rampup_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to set ramp down rate N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to set the ramp down rate" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,1,&rate) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Ramp down rate was set to " << rate << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return rate;	
}

double N1470::setVoltage(int channel, double voltage){

  char * cmd;
  std::string target = "CH:X,PAR:VSET,VAL:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
 
  channelCheck(channel);
  if (voltage < 0 || voltage > 1500){
    std::cerr << "Voltage is outside of limits" << std::endl;
    return -1;
  }

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:VSET,VAL:" << voltage;
  cmd = this->formCommand(vset_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to set voltage N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to set the voltage" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,1,&voltage) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Voltage was set to " << voltage << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return voltage;	
}

	
double N1470::setMaxVoltage(int channel, double voltage){

  char * cmd;
  std::string target = "CH:X,PAR:MAXV,VAL:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
 
  channelCheck(channel);
  if (voltage < 0 || voltage > 1500){
    std::cerr << "Voltage is outside of limits" << std::endl;
    return -1;
  }

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:MAXV,VAL:" << voltage;
  cmd = this->formCommand(vmax_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to set voltage N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to set the voltage" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,1,&voltage) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Max voltage was set to " << voltage << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return voltage;	
}	

double N1470::setCurrent(int channel, double current){

  char * cmd;
  std::string target = "CH:X,PAR:ISET,VAL:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
 
  channelCheck(channel);
  if (current < 0 || current > 3000){
    std::cerr << "Current is outside of limits" << std::endl;
    return -1;
  }

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:ISET,VAL:" << current;
  cmd = this->formCommand(iset_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to set current N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to set the current" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,1,&current) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Current was set to " << current << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return current;

}

double N1470::setTripTime(int channel, double tripTime){

  char * cmd;
  std::string target = "CH:X,PAR:TRIP,VAL:XX";
  std::string *response = new std::string();
  std::ostringstream replacement;
 
  channelCheck(channel);
  if (tripTime < 0 || tripTime > 25){
    std::cerr << "tripTime is outside of limits" << std::endl;
    return -1;
  }

  // Form command properly                                                                                
  replacement << "CH:" << channel << ",PAR:TRIP,VAL:" << tripTime;
  cmd = this->formCommand(triptime_cmd_, target, replacement.str());

#ifdef DEBUG_MAX
  // Write the actual command unless there's no device present as defined                                       
  // at compile time, in which case we fake the connection                                                      
  // and pretend everything is OK                                                                                                     
  fprintf(stderr,"Writing command to set trip time N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  if (writeCommand(cmd) != 0){

    std::cerr << "There was a problem writing the command to set the trip time" << std::endl;
    free(cmd);
    delete(response);
    exit(1);
  }
    

  if (getResponse(response) != 0){
    
    fprintf(stderr,"Could not get response\n");
    free(cmd);
    delete(response);
    exit(1);
  }

#ifdef DEBUG_MAX
  std::cout << "Printing response:" << std::endl;
  std::cout << (*response) << std::endl;
#endif

#ifdef DEBUG_MAX
  std::cout << "Parsing response:" << std::endl;
#endif
  if (parseResponse(response,1,&tripTime) != 0){
    std::cerr << "Could not parse response" << std::endl;
  }
#ifdef DEBUG
  std::cout << "Trip Time was set to " << tripTime << std::endl;
#endif

  // No memory leaks!                                                      
  delete(response);
  free(cmd);
  return tripTime;

}

int N1470::getResponse(std::string * accumulator){
  
  char * buf; 
  
  DWORD bufLenWd, bufWrtWd, status, bufRead;
  int ret;

  buf = (char *)calloc(BUFFER_SIZE, sizeof(char));

  bufLenWd = 0;
  bufWrtWd = 0;
  status = 0;
  bufRead = 0;
  ret = 0;

  sleep(RESPONSE_TIME_N1470);
  
#ifdef DEBUG_MAX
  std::cout << "bufLenWd BufWrtWd Status" << std::endl;
  std::cout << bufLenWd << " " << bufWrtWd << " " << status << std::endl;
#endif

  do {

    if((ret = FT_GetStatus(dev_, &bufLenWd, &bufWrtWd, &status)) != FT_OK){
      PRINT_ERR("FT_GetStatus",(long unsigned)ret);
      return 1;
    }
      
    if (bufLenWd > BUFFER_SIZE){

      buf = (char *)realloc(buf,bufLenWd);
      
      if (buf == NULL){

	fprintf(stderr,"OOM ERROR on buffer length %ul\n",bufLenWd);
	return 1;
      }
    }      
    
    if((ret = FT_Read(dev_,buf, bufLenWd, &bufRead))!=FT_OK){
      PRINT_ERR("FT_Read",(long unsigned)ret);
      return 1;
    }

    // Null-terminate the response
    buf[bufLenWd] = '\0';

#ifdef DEBUG_MAX
    std::cerr << "Accumulating buffer" << std::endl;	    
    puts(buf);
#endif
    accumulator->append(buf);
  }  while (bufLenWd!=0);
  
  return 0; 
}

int N1470::parseResponse(std::string *response, int type, double *value){

    int loc, loc2;	
    loc = response->find("OK");
    if (loc == -1)
      {	
    std::cerr << loc << std::endl;
    std::cerr << "Something has gone wrong. Command failed!" << std::endl;
    return -9;
  }		

    if(type == 2){
    
    loc2 = response->find("VAL:+\r\n",0,5);

    if ( loc2  != -1){   
#ifdef DEBUG_MAX
    std::cerr << *response;
#endif
    *value = 1;	
    return 0;
  }				
    else if ((loc2 = response->find("VAL:-\r\n",0,5)) != -1){
#ifdef DEBUG_MAX
    std::cerr << *response;
#endif
    *value = -1;
    return 0;
  }			
    
    
    
    if (sscanf(response->substr(loc).c_str(),"OK,VAL:%lf",value) != 1){
    
    std::cerr << "Could not interpret a value from the response: " << *response << std::endl;
    return -2;
  }			
  }
  
  return 0;

  }	

void N1470::parseChannelStatus(double statusDb){
    
    int status = (int)statusDb;

    if ((status>>0) & 0x1)
      std::cerr << "Channel is on" << std::endl;
    else
      std::cerr << "Channel is off" << std::endl;

    if ((status>>1) & 0x1)
      std::cerr << "Channel is ramping up" << std::endl;

    if ((status>>2) & 0x1)
      std::cerr << "Channel is ramping down" << std::endl;

    if ((status>>3) & 0x1)
      std::cerr << "IMON > ISET" << std::endl;

    if ((status>>4) & 0x1)
      std::cerr << "VMON > VSET + 250V Tolerance" << std::endl;

    if ((status>>5) & 0x1)
      std::cerr << "VMON < VSET - 250V Tolerance" << std::endl;

    if ((status>>6) & 0x1)
      std::cerr << "VOUT AT MAXV" << std::endl;

    if ((status>>7) & 0x1)
      std::cerr << "CHANNEL HAS TRIPPED" << std::endl;
 
    if ((status>>8) & 0x1)
      std::cerr << "MAX POWER IS EXCEEDED" << std::endl;

    if ((status>>9) & 0x1)
      std::cerr << "TEMP > 105c MAXIMUM" << std::endl;

    if ((status>>10) & 0x1)
      std::cerr << "CHANNEL IS DISABLED" << std::endl;
    
    if ((status>>11) & 0x1)
      std::cerr << "CHANNEL KILLED BY FRONT PANEL" << std::endl;

    if ((status>>12) & 0x1)
      std::cerr << "CHANNEL INTERLOCKED BY FRONT PANEL" << std::endl;

    if ((status>>13) & 0x1)
      std::cerr << "CHANNEL CALIBRATION ERROR" << std::endl;

  }
