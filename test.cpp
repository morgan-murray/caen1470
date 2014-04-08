#include <stdlib.h>

#include "N1470.h"


int main(int argc, char **argv){

  N1470 *hv = new N1470();

  if (!(hv->isConnected())) fprintf(stderr,"HV unit not connected\n");
  
  fprintf(stderr,"Values associated with test HV module:\n");
  
  for(int ii = 0; ii < CH_MAX; ii++){
    fprintf(stderr,"\t vmon[%d]: %lf\n",ii,hv->getVmon(ii));
    fprintf(stderr,"\t vset[%d]: %lf\n",ii,hv->getVset(ii));
    fprintf(stderr,"\t imon[%d]: %lf\n",ii,hv->getImon(ii));
    fprintf(stderr,"\n");
    fprintf(stderr,"\t VMax[%d]: %lf\n",ii,hv->getMaxVoltage(ii));
    fprintf(stderr,"\t IMax[%d]: %lf\n",ii,hv->getMaxCurrent(ii));
    fprintf(stderr,"\n");
    fprintf(stderr,"\t Rup[%d]: %d\n",ii,hv->getRampUp(ii));
    fprintf(stderr,"\t Rdn[%d]: %d\n",ii,hv->getRampDown(ii));
    fprintf(stderr,"\n");
    fprintf(stderr,"\t Ttm[%d]: %lf\n",ii,hv->getTripTime(ii));
    fprintf(stderr,"\t Tmd[%d]: %d\n",ii,hv->getTripMode(ii));
    fprintf(stderr,"\n");    
  }    
  fprintf(stderr,"\t Interlock: %d\n",hv->getInterlock());


  hv->makeConnection();
  for(int ii = 0; ii <=3; ii++){
   
    hv->switchOn(ii);

  }
  hv->dropConnection();

  delete hv;
  return 0;

}
