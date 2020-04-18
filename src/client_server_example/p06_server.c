/* 
 Arquitectura cliente-servidor
 Programa servidor
 O cliente envia os números lidos do ecrã ao servidor (processo‐calculador)
 e o servidor devolve-lhe os resultados.

O processo‐servidor deve manter‐se em funcionamento até que os números a processar
sejam ambos iguais a zero, situação em que não deve efectuar qualquer cálculo, mas apenas deve destruir
os dois FIFOS
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <signal.h>
#include <errno.h>
#include <string.h> 
#include <pthread.h>

#define MAX_FIFO_NAME 25
#define MAX_RES 40

struct data{
   int operands[2];
   pid_t pid;
};

void *sum(void *arg)
{
   float * nums = (float *) arg;
   void *ret = malloc(MAX_RES * sizeof(char)); 
   char res[MAX_RES];

   sprintf(res, "Sum: %f",nums[0] + nums[1]); 

   memcpy((char *)ret, &res, MAX_RES);
   return ret;
}

void *sub(void *arg)
{
   float * nums = (float *) arg;
   void *ret = malloc(MAX_RES * sizeof(char)); 
   char res[MAX_RES];

   sprintf(res, "Subtraction: %f",nums[0] - nums[1]); 

   memcpy((char *)ret, &res, MAX_RES);
   return ret;
}

void *prod(void *arg)
{
   float * nums = (float *) arg;
   void *ret = malloc(MAX_RES * sizeof(char)); 
   char res[MAX_RES];

   sprintf(res, "Product: %f",nums[0] * nums[1]); 
   
   memcpy((char *)ret, &res, MAX_RES);
   return ret;
}

void *quoc(void *arg)
{
   float * nums = (float*) arg;
   void *ret = malloc(MAX_RES * sizeof(char)); 
   char res[MAX_RES];

   if(nums[1] != 0)
      sprintf(res,"Quocient: %f",nums[0] / (float)nums[1]); 
   else
      sprintf(res,"Quocient: inf");    
   
   memcpy((char *)ret, &res, MAX_RES);
   return ret;
}

int main(){
   int fd1, fd2;
   struct data * clientData = (struct data *) malloc(sizeof(struct data));
   pthread_t threads[4];

   if(mkfifo("/tmp/fifo_req",0660) < 0){
      if (errno == EEXIST)
         printf("FIFO '/tmp/fifo_req' already exists\n");
      else printf("Can't create FIFO\n"); 
   }
   else 
      printf("FIFO '/tmp/fifo_req' sucessfully created\n"); 

   do{
      if ((fd1 = open("/tmp/fifo_req",O_RDONLY)) != -1)
         printf("FIFO '/tmp/fifo_req' openned in READONLY mode\n");

      read(fd1,clientData, sizeof(struct data));
      close(fd1);

      if (clientData->operands[0] != 0 || clientData->operands[1] != 0) {
         char results[4][MAX_RES];
      
         char fifoName[MAX_FIFO_NAME];
         sprintf(fifoName,"/tmp/fifo_ans_%d",clientData->pid);

         float * operands = (float*)malloc(sizeof(float)*2);
         for(int i = 0; i<2; i++){
            operands[i] = (float)(clientData->operands[i]);
         }

         pthread_create(&threads[0], NULL, sum, operands);
         pthread_create(&threads[1], NULL, sub, operands);
         pthread_create(&threads[2], NULL, prod, operands);         
         pthread_create(&threads[3], NULL, quoc, operands);

         for(int i = 0; i < 4; i++){
            void * r;
            pthread_join(threads[i],&r);
            memcpy(results[i], (char *)r, MAX_RES);
            free(r);
         }

         free(operands);

         if ((fd2 = open(fifoName,O_WRONLY)) != -1)
            printf("FIFO '%s' openned in WRITEONLY mode\n",fifoName);

         write(fd2, results, 4 * MAX_RES *sizeof(char));
         close(fd2);
      }

   } while (clientData->operands[0] != 0 || clientData->operands[1] != 0); 

   close(fd1);
   close(fd2); 

   if (unlink("/tmp/fifo_req") < 0)
      printf("Error when destroying fifo_req'\n");
   else
      printf("fifo_req destroyed\n");

   exit(0); 
}