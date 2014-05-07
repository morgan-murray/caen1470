#include <stdlib.h>

#include "N1470.h"

// Set of tests for N1470 module
// Idea is to have this run even if there's no device connected, to check code
// Done by wrapping actual device connection lines in N1470 with 
// #ifdefs and defining NO_DEVICE at compile time.

int main(int argc, char **argv){

  N1470 *hv = new N1470(0);

  if (!(hv->isConnected())) fprintf(stderr,"HV unit not connected\n");
  
  fprintf(stderr,"\t Interlock: %d\n",hv->getInterlock());

  
  hv->makeConnection();

  
  for(int ii = 0; ii <=3; ii++){

    std::cerr << "Now on channel " << ii << std::endl;

    (void)hv->setMaxVoltage(ii,1000.0);
    (void)hv->setCurrent(ii,170.0);
    (void)hv->setVoltage(ii,900.0);
    (void)hv->setTripTime(ii,5);
    (void)hv->setRampUpRate(ii,100);

    (void)hv->getActualVoltage(ii);
    (void)hv->getActualCurrent(ii);
    (void)hv->getMaxVoltage(ii);
    (void)hv->getTripTime(ii);
    (void)hv->getRampUpRate(ii);
    (void)hv->getPolarity(ii);

    (void)hv->printStatus(ii);
    
    std::cerr << "========================================" << std::endl;
  }


  fprintf(stderr,"Switching channels on\n");

  for(int ii = 0; ii <=3; ii++){
   
    std::cerr << "Now switching on channel " << ii << std::endl;
    hv->switchState(ii, true);

  }
  std::cerr << "========================================" << std::endl;
  
  sleep(10);

  for(int ii = 0; ii <=3; ii++){
    std::cerr << "Now on channel " << ii << std::endl;	
    hv->getActualVoltage(ii);
    hv->getActualCurrent(ii);
    hv->printStatus(ii);
  }
  std::cerr << "========================================" << std::endl;	  
  sleep(10);

  fprintf(stderr,"Switching channels off\n");
  for(int ii = 0; ii <=3; ii++){
    std::cerr << "Now switching off channel " << ii << std::endl;	
    hv->switchState(ii, false);

  }
  std::cerr << "========================================" << std::endl;

  sleep(10);
 
  int stat_check[4] ={0,0,0,0};
  int sum;
 
  for(int ii = 0; ii <=3; ii++){
    std::cerr << "Now on channel " << ii << std::endl;	
    hv->getActualVoltage(ii);
    hv->getActualCurrent(ii);
    stat_check[ii] = ((int)hv->printStatus(ii) & 0x1);
  }
  std::cerr << "========================================" << std::endl;	  
  
  while (1){

    for(int ii = 0; ii <=3; ii++){
      std::cerr << "Now on channel " << ii << std::endl;	
      hv->getActualVoltage(ii);
      hv->getActualCurrent(ii);
      stat_check[ii] = ((int)hv->printStatus(ii) & 0x1);
    }	
    
    for(int ii=0;ii <=3; ii++)
      sum+=stat_check[ii];
      
    if (sum == 0)
      break;
    else
      sum = 0;

    sleep(5);	      
  }
   
  hv->dropConnection();

  // Not strictly necessary, but it makes valgrind happy :-)
  delete hv;
  return 0;

}
