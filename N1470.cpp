#include "N1470.h"
// Default constructor. Make everything zero and get actual values in initialize function
// String commands are taken from the N1470 manual available at caen.it
N1470::N1470() : 
  BD_(0),
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


int N1470::switchOn(int channel){

  // Check input
  if ((channel < 0) || (channel >=4)){

    PRINT_ERR("switchOn",(unsigned long)channel);
    return -channel;
  }

  // Variables for string-munging
  unsigned long ret;
  std::string command;
  std::string target;
  std::ostringstream replacement;
  int position;

  char * cmd;
  unsigned int bufLen, bufWrit;

  // Get the correct character string to sewitch the associated channel on
  command = channel_on_cmd_;
  target = "$BD:XX";
  position = command.find(target);
  replacement << "$BD:" << BD_;
  command = command.replace(position,target.size(),replacement.str());

  replacement.clear();
  replacement.str("");
  target = "CH:X";
  
  position = command.find(target);
  replacement << "CH:" << channel;
  command = command.replace(position, target.size(), replacement.str());
  
  // N1470 accepts C-style strings, now convert from std::string to c_str style
  // need to append a Windows-style line ending (\<cr>\<lf>) to make N1470 realise it's the end of the command
  bufLen = command.size() + 3;
  bufWrit = 0;
  cmd = (char *)malloc(bufLen);
  snprintf(cmd,bufLen,command.c_str());

  if (cmd == NULL){
    fprintf(stderr,"No memory allocation possible!\n");
    return -9999;
  }
  
  strncat(cmd,"\r\n",2);
  

  // Write the actual command unless there's no device presenta s defined
  // at compile time, in which case we fake the connection
  // and pretend everything is OK
  fprintf(stderr,"Writing command to switch on N1470 module channel %d: ",channel);fprintf(stderr,cmd);

#ifdef NO_DEVICE

  fprintf(stderr,"Faking switch on of channel %d\n",channel);
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
