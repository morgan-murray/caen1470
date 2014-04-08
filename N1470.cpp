#include "N1470.h"
// Default constructor. Make everything zero and get actual values in initialize function
// String commands are taken from the N1470 manual available at caen.it
N1470::N1470() : 
  BD_(0),
  connected_(false), 
  interlock_(0), 
  mon_cmd_("$BD:XX,CMD:MON,CH:%1d,PAR:%s"),
  vset_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:VSET,VAL:%04.1f"),
  iset_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:ISET,VAL:%04.2f"),
  vmax_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:VMAX,VAL:%04d"),
  rampup_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:RUP,VAL:%03d"),
  rampdown_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:RDW,VAL:%03d"),
  triptime_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:TRIP,VAL:%04.1f"),
  tripmode_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:PDWN,VAL:%s"),
  channel_on_cmd_("$BD:XX,CMD:SET,CH:X,PAR:ON"),
  channel_off_cmd_("$BD:XX,CMD:SET,CH:%01d,PAR:OFF"),
  interlock_cmd_("$BD:XX,CMD:SET,PAR:BDILKM,VAL:%s"),
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

  unsigned long ret;
  std::string command;
  std::string target;
  std::ostringstream replacement;
  int position;

  char * cmd;
  unsigned int bufLen, bufWrit;

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
  
  bufLen = command.size() + 3;
  bufWrit = 0;
  cmd = (char *)malloc(bufLen);
  snprintf(cmd,bufLen,command.c_str());

  if (cmd == NULL){
    fprintf(stderr,"No memory allocation possible!\n");
    return -9999;
  }
  
  strncat(cmd,"\r\n",2);
  
  fprintf(stderr,"Writing command to switch on N1470 module channel %d: ",channel);fprintf(stderr,cmd);

#ifdef NO_DEVICE

  fprintf(stderr,"Faking switch on of channel %d\n",channel);
  bufWrit = bufLen;
  
#else
  
  if ((ret = FT_Write(dev_,cmd,bufLen,(LPDWORD) &bufWrit)) != FT_OK){

    PRINT_ERR("FT_Write_Data",ret);
    return -1;
  }

#endif

  if(bufWrit != bufLen){
    fprintf(stderr, "Buffersize mismatch: bufLen %u \t bufWrit %u\n",bufLen,bufWrit);
  }

  free(cmd);

  return 0;
}
