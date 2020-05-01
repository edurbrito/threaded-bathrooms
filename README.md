SOPE T02G05
----------

###### TRABALHO PRÁTICO Nº 2
# Acesso informático aos Quartos de Banho

### Descrição
Pretende-se obter uma aplicação do tipo cliente-servidor capaz de lidar com situações de conflito no acesso a zonas partilhadas. A zona partilhada é um Quarto de Banho com vários lugares unisexo, controlado por um processo Q ao qual se dirigem pedidos de acesso de utentes. Os pedidos de acesso são enviados por intermédio de um processo multithread U (cliente), e neles se indica o tempo que o interessado deseja estar num lugar das instalações sanitárias. Os pedidos ficarão numa fila de atendimento até terem vez; nessa altura, o utente respetivo acede a um lugar nas instalações durante o tempo pedido, sob o controlo do servidor Q; depois o recurso é libertado para outro utente. 

Com este projeto, demonstramos conhecer e saber utilizar a interface programática de UNIX em C para conseguir:

- criar programas multithread;

- promover a intercomunicação entre processos através de canais com nome (named pipes ou FIFOs);

- evitar conflitos entre entidades concorrentes, por via de mecanismos de sincronização.

### Relatório

## Estrutura das mensagens trocadas

Relativamente à estrutura das mensagens trocadas entre e o cliente e o servidor, optamos por utilizar a `struct message` de modo a //TODO.

O cliente irá guardar nessa `struct` o seu *PID* e *TID*, necessário para criar o canal privado onde recebe a resposta do servidor, a duração de tempo requisitado, e o seu número sequêncial. Por fim, envia ao servidor um pedido através do canal público recebido como parâmetro. 

Já no servidor, ao receber o pedido do cliente por esse canal público, deve alterar a struct, atualizando com o seu o PID e TID. No caso de o servidor já não estar em funcionamento, altera o pl e dur para -1, para indicar o encerramento do serviço. Assim envia a sua resposta ao cliente, pelo canal privado criado.

```c
typedef struct message {
    int i;
    pid_t pid;
    pthread_t tid;
    int dur;
    int pl;
    char fifoName[FIFONAME_SIZE];
} message;
```

## Intercomunicação entre processos através de canais com nome

>A utilização de FIFOs e de funções que permitem proceder à sua leitura e escrita permitiu proceder à troca de mensagens entre os processos do Cliente e do Servidor (Quarto de Banho).

### *Criação, Abertura e Leitura dos pedidos no Servidor* 

No Servidor, depois de lidos os argumentos e após a criação do FIFO com o nome neles indicado ter sido executada com sucesso, procede-se à abertura do FIFO em modo de escrita, através da função `mkfifo(..)`. 

Seguidamente, na abertura do FIFO em modo de leitura, `O_RDONLY`, optou-se por ativar a flag `O_NONBLOCK`, pois, para podermos controlar o tempo de execução do servidor, independentemente de este ter ou não solicitações nesse período, é preciso evitar que abertura do ficheiro com a função `open(...)` bloqueie pelo facto de não existir ainda nenhum processo com o FIFO aberto para escrita. Assim, o Servidor não fica durante tempo indeterminado à espera de Clientes - `open(...)` é bem sucedida e retorna imediatamente
mesmo que o FIFO ainda não tenha sido aberto para escrita por nenhum processo, que será o caso.

Já no ciclo que recebe pedidos, vai ser feita a cada iteração uma tentativa de leitura do FIFO, utilizando a função `read(...)` para proceder à tentativa de leitura do número de bytes ocupados pela struct `message` e verificando o seu valor de retorno:
- No caso de, entretanto, os Clientes começarem a fazer os seus pedidos, isto é, o FIFO já ter sido aberto para escrita pelo processo *U1*, `read(...)` poderá:
    - retornar um valor inteiro maior do que zero, quando efetivamente foram lidos dados do FIFO - sendo devolvidos o número de bytes lidos; , caso no qual se procede à criação de uma thread para processar o pedido; 
    - retornar `EAGAIN` pelo facto de o FIFO ter sido aberto em modo `O_NONBLOCK` e não existir nada no FIFO para ler naquele instante, caso no qual se avança para a próxima leitura, libertando o espaço anteriormente alocado para a mensagem do Cliente. 
- No caso de ainda não ter sido feito nenhum pedido, isto é, de o FIFO não se encontrar ainda aberto para escrita, a leitura vai retornar `EOF` pelo que o espaço previamente alocado para receber a mensagem do Cliente é libertado, passando-se para a próxima iteração do ciclo de receção de pedidos.

Um procedimento semelhante para leitura dos pedidos do FIFO público é adotado na função de início de thread responsável por gerar as threads que recusam os pedidos quando o servidor está a encerrar, `void * server_closing(void * arg)`.

### *Abertura e escrita de pedidos no processo Cliente*

No Cliente, é feita a abertura para escrita, `O_WRONLY`, do FIFO passado nos argumentos. Seguidamente, no ciclo de geração de pedidos são criadas threads, ficando cada thread encarregue da geração de um pedido. É de notar que cada thread terá acesso ao descritor do FIFO criado na thread inicial, tal como é sugerido pela seguinte afirmação: ["threads share the same global memory (data and heap segments)"](http://man7.org/linux/man-pages/man7/pthreads.7.html).
Na função de início da thread `void * client_request(int * arg)`, é criado o FIFO com a identificação do cliente (*PID* e *TID*), usando `mkfifo(..)` mais uma vez.
Segue-se a escrita da mensagem através da função `write(...)`. No caso de o Servidor ter o FIFO aberto para escrita, a chamada desta função bloqueia a thread até se conseguir escrever a mensagem, retornando com sucesso quando termina a escrita.
Já no caso em que o Servidor não tem o FIFO aberto para escrita, que acontece quando se força a terminação do Servidor abruptamente com um *CTRL-C*, 'SIGINT', a chamada retornará um erro, que levará à escrita da mensagem *FAILD* na saída padrão.
No caso de o envio do pedido ser bem sucedido, é aberto o FIFO criado pelo cliente em modo de leitura e é lida a resposta ao pedido realizado, sendo que a função `read(...)` bloqueia até que seja escrita a resposta ou até que o FIFO seja fechado para escrita.
Após interpretar a resposta do Servidor, o Cliente fecha(`close(...)`) e destrói (`unlink(...)`) o seu FIFO privado.

### *Receção dos pedidos nas threads do Servidor*

Finalmente, tanto na função de início de thread que tem a responsabilidade de aceitar pedidos - `void * handle_request(void *arg)`; como na que é responsável por os recusar - `void * refuse_request(void *arg)`; procede-se à abertura do FIFO privado do Cliente com a função `open(...)` e, após processado o pedido recebido, escreve-se a respetiva resposta utilizando a função `write(...)`.  // TODO (a terminar)

## Tratemneto do sinal SIGPIPE

>blablabla já estou cansada de escrever (a terminar)

### Autores

Diana Freitas - [up201806230](mailto:up201806230@fe.up.pt)
Eduardo Brito - [up201806271](mailto:up201806271@fe.up.pt)
Maria Baía - [up201704951](mailto:up201704951@fe.up.pt)