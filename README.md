SOPE T02G05
----------

###### TRABALHO PRÁTICO Nº 2
# Acesso informático aos Quartos de Banho

### Descrição
Pretende-se obter uma aplicação do tipo cliente-servidor capaz de lidar com situações de conflito no acesso a zonas partilhadas. A zona partilhada é um Quarto de Banho com vários lugares unisexo, controlado por um processo Q ao qual se dirigem pedidos de acesso de utentes. Os pedidos de acesso são enviados por intermédio de um processo multithread U (cliente), e neles se indica o tempo que o interessado deseja estar num lugar das instalações sanitárias. Os pedidos ficarão numa fila de atendimento até terem vez; nessa altura, o utente respetivo acede a um lugar nas instalações durante o tempo pedido, sob o controlo do servidor Q; depois o recurso é libertado para outro utente. 

Com este projeto, demonstramos conhecer e saber utilizar a interface programática de UNIX em C para conseguir:
● criar programas multithread;
● promover a intercomunicação entre processos através de canais com nome (named pipes ou FIFOs);
● evitar conflitos entre entidades concorrentes, por via de mecanismos de sincronização.

### Relatório

## Estrutura das mensagens trocadas

Relativamente à estrutura das mensagens trocadas entre e o cliente e o servidor, utilizamos uma struct 'message' para estruturar a mensagem de pedido do cliente e a resposta do servidor. 

O cliente irá guardar nessa struct o seu PID e TID, necessário para criar o canal privado onde recebe a resposta do servidor, a duração de tempo requesitado, e o seu número sequêncial. Por fim, envia ao servidor um pedido através do canal público recebido como parâmetro. 

Já no servidor, ao receber o pedido do cliente por esse canal público, deve alterar a struct, atualizando com o seu o PID e TID. No caso de o servidor já não estar em funcionamente, altera o pl e dur para -1, para indicar o encerramento do serviço. Assim envia a sua resposta ao cliente, pelo canal privado criado.

typedef struct message {
    int i;
    pid_t pid;
    pthread_t tid;
    int dur;
    int pl;
    char fifoName[FIFONAME_SIZE];
} message;


### Autores

Diana Freitas - [up201806230](mailto:up201806230@fe.up.pt)
Eduardo Brito - [up201806271](mailto:up201806271@fe.up.pt)
Maria Baía - [up201704951](mailto:up201704951@fe.up.pt)