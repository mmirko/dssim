#include <iostream>
#include <cstdlib>
#include <cmath>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <mpi.h>

using namespace std;

void slave()
{
   int rank, Nprocessi, icontinue;
   double x;
   MPI_Status status;

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);  // numero di questo processo
   MPI_Comm_size(MPI_COMM_WORLD, &Nprocessi);  // numero totale di processi

   cout<<"Sono il processo "<<rank<<endl;

   for(unsigned int j=0; j < 20; j++) {
      // manda un messaggio al master process
      cout<<"Sono il processo "<<rank<<" e mando un messaggio"<<endl;
      MPI_Send(&x,1,MPI_DOUBLE,0,rank,MPI_COMM_WORLD);
      // ricevi un intero che ti dica se continuare
      cout<<"Sono il processo "<<rank<<" e ricevo un messaggio\n"<<endl;
      MPI_Recv(&icontinue,1,MPI_INT,0,MPI_ANY_TAG,MPI_COMM_WORLD,&status);
      if(icontinue == 0) break;
   }

   return;
}

void master()
{
   int jproc, j, Nprocessi, icontinue, Ntot;
   double x;
   MPI_Status status;

   // questo e' il processo zero
   MPI_Comm_size(MPI_COMM_WORLD, &Nprocessi);  // numero totale di processi

   icontinue = 1;    // 1 se deve continuare, 0 per fermare un task
   Ntot = 0; 

   while(Ntot < 20) {
      MPI_Recv(&x,1,MPI_DOUBLE,MPI_ANY_SOURCE,MPI_ANY_TAG,
               MPI_COMM_WORLD,&status);
      jproc = status.MPI_SOURCE;
      cout<<"Sono il master process e ho ricevuto un messaggio da "<<jproc<<endl; 

      Ntot++;

      // invia un intero che controlli l'esecuzione dei processi
      cout<<"Sono il master process e ho inviato un messaggio a "<<jproc<<endl; 
      MPI_Send(&icontinue,1,MPI_INT,jproc,0,MPI_COMM_WORLD);
   }

   icontinue = 0;
   for(int j=1; j<Nprocessi; j++) {
     cout<<"Sono il master process e ho ricevuto un messaggio da "<<jproc<<endl; 
      MPI_Recv(&x,1,MPI_DOUBLE,MPI_ANY_SOURCE,MPI_ANY_TAG,
               MPI_COMM_WORLD,&status);
      jproc = status.MPI_SOURCE;
      // invia un intero che controlli l'esecuzione dei processi
     cout<<"Sono il master process e ho inviato un messaggio a "<<jproc<<endl; 
      MPI_Send(&icontinue,1,MPI_INT,jproc,0,MPI_COMM_WORLD);
   }

   return;
}

int
main(int argc, char *argv[])
{
   int rank;
   int Nprocessi;

   MPI_Init(&argc, &argv);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);

   fflush(stdout);

   if(rank == 0) {
      cout<<"\n";
      MPI_Comm_size(MPI_COMM_WORLD, &Nprocessi);  // numero totale di processi
      if(Nprocessi == 1) {
         cout<<"Ci vogliono almeno due processi\n";
         cout<<"Usa mpirun -np Numero_processi file_eseguibile\n";
         exit(1);
      }
   }

   if(rank==0) master();
   else slave();

   MPI_Finalize();

   return 0;
}



