#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

#define CONTROL_TAG = 0
#define ELECTION_TAG = 1
#define ELECTION_CONFIRMATION_TAG = 2

#define NEW_ELECTION = 10
#define ENABLE_PROC = 20
#define DISABLE_PROC = 30
#define STOP = 90

main(int argc, char** argv)
  {
    int my_rank;       // Identificador deste processo
    int proc_n;        // Numero de processos disparados pelo usuario na linha de comando (np)  
    int *message;       // Buffer para as mensagens                    
    MPI_Status status; // estrutura que guarda o estado de retorno          
    int dest;
    int coordPID;

    int i;

    MPI_Init(&argc , &argv); // funcao que inicializa o MPI, todo o codigo paralelo estah abaixo

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // pega pega o numero do processo atual (rank)
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);  // pega informacao do numero de processos (quantidade total)

    int *enabled = malloc(proc_n * sizeof(int));
    for(i = 0;i < proc_n;i++)
    {
        enabled[i]= 1;
    }

////////Processo de Controle/////////////////////////////
    if ( my_rank == 0 )
        {
            &message = NEW_ELECTION;
            MPI_Send(message, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(message, 1, MPI_INT, 1, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD, &status); 
        }
    else
        {
            MPI_Recv(message, 1, MPI_INT, my_rank-1, MPI_ANY_TAG, MPI_COMM_WORLD, &status); 
            if(my_rank == proc_n - 1)
            {
                if(enabled[1] == 1)
                    dest = 1;
                else
                    dest = 2;
                
            }
            else
            {
                if(enabled[my_rank+1] == 1)
                    dest = my_rank+1;
                else
                {
                    if(my_rank+2 >= proc_n)
                        dest = 1;
                    else
                        dest = my_rank+2;
                    
                }
            }  
            if(status.MPI_TAG == CONTROL_TAG)
            {
                if(message == NEW_ELECTION)
                {
                    message = malloc(sizeof(int));
                    message[0] = my_rank;

                    MPI_Send(message, 1, MPI_INT, dest, ELECTION_TAG, MPI_COMM_WORLD);
                }
            }
            else if(status.MPI_TAG == ELECTION_TAG)
            {

            }
            else if(status.MPI_TAG == ELECTION_CONFIRMATION_TAG)
            {

            }
            
            
        }

        
    free(enabled);
    MPI_Finalize();
}
