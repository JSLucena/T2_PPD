#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

//###TAGS#############################
#define CONTROL_TAG  0
#define ELECTION_TAG  1
#define ELECTION_CONFIRMATION_TAG  2
//######################################

//##Control Commands###################
#define NEW_ELECTION  10
#define STOP  90
//######################################

int searchWinner(int *message, int size)
{
    int biggest = 0;
    for(int i = 0;i < size;i++)
    {
        if(message[i] > biggest)
            biggest = message[i];
    }
    return biggest;
}


main(int argc, char** argv)
  {
    int my_rank;       // Identificador deste processo
    int proc_n;        // Numero de processos disparados pelo usuario na linha de comando (np)  
    int *message;       // Buffer para as mensagens                    
    MPI_Status status; // estrutura que guarda o estado de retorno          
    int dest;
    int coordPID;
    int failed;

    int i;
    int stop = 0;
    MPI_Init(&argc , &argv); // funcao que inicializa o MPI, todo o codigo paralelo estah abaixo

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank); // pega pega o numero do processo atual (rank)
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);  // pega informacao do numero de processos (quantidade total)

    if(proc_n < 4)
    {
        printf("Eh necessario mais que no minimo 4 processos para funcionar, desligando\n");
        MPI_Finalize();
        return 0;
    }


    message = malloc((proc_n+1) * sizeof(int));
    for(i = 0;i <= proc_n;i++)
    {
        message[i]= -1;
    }
    

////////Processo de Controle/////////////////////////////
    if ( my_rank == 0 )
        {
            printf("iniciando sistema\n");
            message[0] = NEW_ELECTION;
            message[proc_n] = -1;
            printf("solicitando eleicao para o processo 1\n");
            MPI_Send(message, proc_n+1, MPI_INT, 1, CONTROL_TAG, MPI_COMM_WORLD);
            printf("Esperando termino de eleicao....\n");
            MPI_Recv(message, proc_n+1, MPI_INT, 1, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD, &status); 
            printf("Eleicao concluida\n");
            coordPID = message[0];
            printf("Novo coordenador eh o processo %d\n",coordPID);

            printf("solicitando nova eleicao com o coordenador atual desabilitado\n");
            message[0] = NEW_ELECTION;
            message[proc_n] = coordPID;
            MPI_Send(message, proc_n+1, MPI_INT, 1, CONTROL_TAG, MPI_COMM_WORLD);
            printf("Esperando termino de eleicao....\n");
            MPI_Recv(message, proc_n+1, MPI_INT, 1, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD, &status); 
            printf("Eleicao concluida\n");
            coordPID = message[0];
            printf("Novo coordenador eh o processo %d\n",coordPID);


            printf("solicitando eleicoes concorrentes, a primeira derrubando o atual coordenador e a ultima com todos\n");
            message[0] = NEW_ELECTION;
            message[proc_n] = coordPID;
            MPI_Send(message, proc_n+1, MPI_INT, 1, CONTROL_TAG, MPI_COMM_WORLD);
            message[0] = NEW_ELECTION;
            message[proc_n] = -1;
            MPI_Send(message, proc_n+1, MPI_INT, 2, CONTROL_TAG, MPI_COMM_WORLD);
            printf("esperando termino de eleicoes\n");
            MPI_Recv(message, proc_n+1, MPI_INT, MPI_ANY_SOURCE, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD, &status);
            coordPID = message[0];
            printf("Coordenador da primeira eleicao: %d\n",coordPID);
            MPI_Recv(message, proc_n+1, MPI_INT, MPI_ANY_SOURCE, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD, &status);  
            coordPID = message[0];
            printf("Novo coordenador eh o processo %d\n",coordPID);


            message[0] = STOP;
            printf("Terminando teste\n");
            MPI_Send(message, proc_n+1, MPI_INT, coordPID, CONTROL_TAG, MPI_COMM_WORLD);
        }
////////////////////////////////////////////////////////
    else
        {
            while(stop != 1)
            {
                MPI_Recv(message, proc_n+1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status); 
                 
                if(my_rank == proc_n - 1)
                {
                    if(message[proc_n] == 1)
                        dest = 2;
                    else
                        dest = 1;     
                }
                else
                {
                    if(my_rank == proc_n-2)
                    {
                        if(message[proc_n] == my_rank+1)
                            dest = 1;
                        else
                            dest = my_rank+1;
                        
                    }
                    else
                    {
                        if(message[proc_n] == my_rank+1)
                            dest = my_rank+2;
                        else
                            dest = my_rank+1;
                        
                    }
                }

                if(status.MPI_TAG == CONTROL_TAG)
                {
                    if(message[0] == NEW_ELECTION)
                    {
                        message[0] = -1;
                        message[my_rank] = my_rank;
                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_TAG, MPI_COMM_WORLD);
                    }
                    else if(message[0] == STOP)
                    {
                        stop = 1;
                        if(my_rank == proc_n-1)
                            MPI_Send(message, proc_n+1, MPI_INT, 1, CONTROL_TAG, MPI_COMM_WORLD);
                        else
                        {
                            MPI_Send(message, proc_n+1, MPI_INT, my_rank+1, CONTROL_TAG, MPI_COMM_WORLD);
                        }
                        
                    }
                }
                else if(status.MPI_TAG == ELECTION_TAG)
                {
                    if(message[my_rank] == my_rank)
                    {
                        coordPID = searchWinner(message, proc_n);

                        for(i = 0;i < proc_n;i++)
                            {
                                message[i]= -1;
                            }

                        
                        message[0] = coordPID; // mandar o novo coordenador
                        message[my_rank] = my_rank; //mandar o processo que iniciou a cadeia
                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD);
                    }
                    else
                    {
                        
                        message[my_rank] = my_rank;

                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_TAG, MPI_COMM_WORLD);
                    }
                    
                }
                else if(status.MPI_TAG == ELECTION_CONFIRMATION_TAG)
                {
                    if(message[my_rank] == my_rank)
                    {
                        MPI_Send(message, proc_n+1, MPI_INT, 0, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD);
                    }
                    else
                    {
                        coordPID = message[0];
                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD);
                    }
                    
                    
                }
            }
            
        }

        
    free(message);
    MPI_Finalize();
}
