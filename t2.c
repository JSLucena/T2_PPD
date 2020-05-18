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
        printf("Eh necessario no minimo 4 processos para funcionar, desligando\n");
        MPI_Finalize();
        return 0;
    }
    message = malloc((proc_n+1) * sizeof(int));
    for(i = 0;i <= proc_n;i++)
        message[i]= -1;
    

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


            printf("solicitando eleicoes concorrentes, a primeira derrubando o processo com maior PID e a segunda com todos\n");
            message[0] = NEW_ELECTION;
            message[proc_n] = proc_n-1;
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
                 
                if(my_rank == proc_n - 1) //se eu for o maior PID
                {
                    if(message[proc_n] == 1) // e o proceso 1 estiver fora do ar
                        dest = 2; //mando pro 2
                    else
                        dest = 1;     // senao mando pro 1
                }
                else
                {
                    if(my_rank == proc_n-2) // se eu for o segundo maior PID
                    {
                        if(message[proc_n] == my_rank+1) // e o maior PID estiver fora do ar
                            dest = 1; // mando pro 1
                        else
                            dest = my_rank+1; // senao mando normalmente
                        
                    }
                    else
                    {
                        if(message[proc_n] == my_rank+1) // se o processo fora do ar eh o proximo a mim
                            dest = my_rank+2; // eu pulo ele
                        else
                            dest = my_rank+1;
                        
                    }
                }
                if(status.MPI_TAG == CONTROL_TAG)
                {
                    if(message[0] == NEW_ELECTION)
                    {
                        message[0] = -1; // removo a mensagem do processo de controle
                        message[my_rank] = my_rank; // adiciono meu ID 
                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_TAG, MPI_COMM_WORLD); // e mando pro meu destino
                    }
                    else if(message[0] == STOP) // se eu recebi as ordens de parar
                    {
                        stop = 1; // eu paro e mando pro proximo
                        if(my_rank == proc_n-1)
                            MPI_Send(message, proc_n+1, MPI_INT, 1, CONTROL_TAG, MPI_COMM_WORLD);
                        else
                            MPI_Send(message, proc_n+1, MPI_INT, my_rank+1, CONTROL_TAG, MPI_COMM_WORLD);           
                    }
                }
                else if(status.MPI_TAG == ELECTION_TAG)
                {
                    if(message[my_rank] == my_rank) // procuro se ja participei da eleicao
                    {
                        coordPID = searchWinner(message, proc_n); // se sim procuro o vencedor
                        for(i = 0;i < proc_n;i++)
                                message[i]= -1; //limpar a mensagem
                        message[0] = coordPID; // mandar o novo coordenador
                        message[my_rank] = my_rank; //mandar o processo que iniciou a cadeia
                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD);
                    }
                    else //se nao participei
                    {                       
                        message[my_rank] = my_rank;  //coloco meu ID
                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_TAG, MPI_COMM_WORLD); //e mando pro prixmo
                    }                   
                }
                else if(status.MPI_TAG == ELECTION_CONFIRMATION_TAG)
                {
                    if(message[my_rank] == my_rank) // se todos ja receberam o resultado
                        MPI_Send(message, proc_n+1, MPI_INT, 0, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD); //mando de volta pro processo de controle
                    else
                    {
                        coordPID = message[0]; //atualizo informacoes do novo coordenador
                        MPI_Send(message, proc_n+1, MPI_INT, dest, ELECTION_CONFIRMATION_TAG, MPI_COMM_WORLD); //mando pro proximo
                    }    
                }
            }
            
        }

        
    free(message);
    MPI_Finalize();
}
