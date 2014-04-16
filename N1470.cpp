
#include "N1470.h"

// Default constructor. Make everything apart from the board number zero and get actual values in initialize function
// String commands are taken from the N1470 manual available at caen.it
N1470::N1470(int boardNumber) : 
  BD_(boardNumber),
  connected_(false), 
  interlock_(0), 
  mon_cmd_("$BD:XX,CMD:MON,CH:X,PAR:XX"),
  vset_cmd_("$BD:XX,CMD:SET,CH:X,PAR:VSET,VAL:XX"),
  iset_cmd_("$BD:XX,CMD:SET,CH:X,PAR:ISET,VAL:XX"),
  vmax_cmd_("$BD:XX,CMD:SET,CH:X,PAR:VMAX,VAL:XX"),
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
  
  for (int cmd = 0; cmd < CMD_LIST_LEN; cmd++){

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

  int bufLen, bufWrit;
  char * cmd;
  std::string target = "CH:X";
  std::ostringstream replacement;

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

  
#ifdef DEBUG
  // Write the actual command unless there's no device presenta s defined
  // at compile time, in which case we fake the connection
  // and pretend everything is OK
  fprintf(stderr,"Writing command to switch state on N1470 module channel %d: ",channel);
  fputs(cmd,stderr);
#endif

  bufLen = std::strlen(cmd);
#ifdef NO_DEVICE

  fprintf(stderr,"Faking switch state of channel %d\n",channel);
  bufWrit = bufLen;
  
#else
  
  if ((ret = FT_Write(dev_,cmd,bufLen,(LPDWORD) &bufWrit)) != FT_OK){

    PRINT_ERR("FT_Write",ret);
    return -1;
  }

#endif

  if(bufWrit != bufLen){
    fprintf(stderr, "Buffersize mismatch: bufLen %u \t bufWrit %u\n",bufLen,bufWrit);
  }

  // No memory leaks!
  free(cmd);

  return 0;
}
