/* 
 Arquitectura cliente-servidor
 Programa cliente
 O cliente lê os números do ecrã, informa o sevidor e apresenta os resultados dos cálculos, fornecidos pelo servidor após o cálculo.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <errno.h>
#include <string.h> 

#define MAX_FIFO_NAME 25
#define MAX_RES 40

struct data{
   int operands[2];
   pid_t pid;
};

int main(){

   int fd1, fd2;
   struct data clientData;
   clientData.pid = getpid();
   char fifoName[MAX_FIFO_NAME];
   sprintf(fifoName,"/tmp/fifo_ans_%d",clientData.pid);

   if(mkfifo(fifoName,0660) < 0){
      if (errno == EEXIST)
         printf("FIFO '%s' already exists\n",fifoName);
      else printf("Can't create FIFO\n"); 
   }
   else 
      printf("FIFO '/tmp/fifo_ans' sucessfully created\n");

   do{

      printf("x y ? ");
      scanf("%d %d", &clientData.operands[0],&clientData.operands[1]);
      getchar();

      fd1 = open("/tmp/fifo_req",O_WRONLY);
      if (fd1 == -1) {
         printf("Oops !!! Service is closed !!!\n");
         exit(1);
      }

      write(fd1,&clientData,sizeof(struct data));
      close(fd1); 

      if(clientData.operands[0] != 0 || clientData.operands[1] != 0){

         if ((fd2 = open(fifoName,O_RDONLY)) != -1)
            printf("FIFO '%s' openned in READONLY mode\n", fifoName);

         char line[MAX_RES]; 
         while(read(fd2,line,MAX_RES) != 0) printf("%s\n",line); 
         close(fd2);
         
      }

      if(clientData.operands[0] != 0 || clientData.operands[1] != 0){
         printf("Request another operation (Y/N) ?");
         char ans = getchar();
         if(ans == 'N' || ans == 'n')
            break;
      }

   } while(clientData.operands[0] != 0 || clientData.operands[1] != 0);

   if (unlink(fifoName) < 0)
      printf("Error when destroying fifo_ans'\n");
   else
      printf("fifo_ans destroyed\n");
   
   exit(0);
}