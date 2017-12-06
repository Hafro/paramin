#include <fstream>
#include <time.h>
#include <assert.h>


#include "vector.h"
#include "slavecommunication.h"


double Rosenbrock(double* x, int numVar);

main()
{
  int i;
  int canSend = 1;
  int canReceive = 1;
  double result;
  double* x_var;
  
  cout << "starting rosenbrock..." << endl;
  SlaveCommunication* slave;
  slave = new SlaveCommunication();
  int numberOfVar = slave->startNetCommunication();
  if (numberOfVar > 0) {
    x_var = new double[numberOfVar];
    //vector x_var(numberOfVar);
    // get first data from parent
    canReceive = slave->receiveFromMaster();
    slave->getVector(x_var);
    while (canSend && canReceive) {
      // received data from master
      result = Rosenbrock(x_var, numberOfVar);
      canSend = slave->sendToMaster(result);
      if (canSend)  {
	canReceive = slave->receiveFromMaster();
	slave->getVector(x_var);
      };
    };
    delete [] x_var;
  };
  slave->stopNetCommunication();
  delete slave;

}
      
    
double Rosenbrock(double* data, int numVar) {

  int i = 0;
  double f = 0;
  for (i = 0; i < (numVar-1); i++){
    f += 100.*(data[i+1]-data[i]*data[i])*(data[i+1]-data[i]*data[i])+(1.-data[i])*(1.-data[i]);
  };
  return f;

}
