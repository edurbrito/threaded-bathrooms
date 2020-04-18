# Server Q1 (Quarto de Banho)

## Invocação: 

>  ### Qn <-t nsecs>  fifoname

- fifoname – nome do canal público (FIFO) a criar pelo servidor para atendimento de pedidos;
- nsecs - nº (aproximado) de segundos que o programa deve funcionar

## Estrutura:
- recolhe os argumentos de linha de comando;
- recebe ciclicamente pedidos de clientes

- <strong>se e só se não estiver a encerrar </strong> atende pedidos, sendo cada pedido atendido por um thread, que comunica com o thread cliente:
   > ### Cada Thread: 
   > *Um pedido de um cliente corresponde a uma thread. Como só são aceites pedidos se o Quarto de Banho não estiver a encerrar, só são criadas threads nesse caso.*
   >
   > <strong>Na 1ª entrega todos os restantes pedidos são sempre aceites, pois o número de lugares é ilimitado!</strong>
   > - é associada ao seu '*pl*'
   > - recebe o pedido do respetivo cliente - o tempo que quer utilizar a casa de banho e restantes elementos da *mensagem*
   > - ocupa o quarto de banho durante esse tempo
   > - devolver *mensagem* atualizada com o seu '*pid*' e '*tid*' e preenchendo '*pl*'

- após o tempo estipulado para o funcionamento do programa (nsecs)
   - espera que terminem utilizações atuais do quarto de banho;
   - não aceita novos pedidos de utilização - retorna -1 em *'pl'* e *'dur'* na mensagem
   - desativa FIFO fifoname.

___
# Client U1 (Utente)

## Invocação: 
> ### Un <-t nsecs> fifoname

- fifoname – nome do canal público (FIFO) para comunicação com o servidor;
- nsecs - nº (aproximado) de segundos que o programa deve funcionar;

## Objetivo:
Envia pedidos de acesso ao servidor de forma (pseudo)aleatória (em termos de duração do acesso) - intervalos curtos.

## Estrutura:
- recolhe os argumentos de linha de comando;
- lança continuamente (i.e. com intervalos de alguns milissegundos) threads, cada um ficando associado a um pedido:
> ### Cada Thread: 
> *Um cliente corresponde a uma thread*
> é associada ao seu '*id*'
> - cria um FIFO de nome "/tmp/pid.tid"
>     - pid - identificador de sistema do processo cliente
>     - tid - identificador no sistema do thread cliente
> -  gera aleatoriamente parâmetros do pedido pelo qual é responsável (especificamente, a duração do acesso)
> - comunica com o servidor enviando-lhe uma *mensagem* com o tempo que pretende utilizá-lo ('*dur*'), o seu *'pid'*/'*tid*' e um '*id*' sequencial (os pedidos são enviados ao servidor através do canal público (fifoname);
> - recebe resposta do servidor pelo canal privado com nome criado ("/tmp/pid.tid")
> - elimina canal "/tmp/pid.tid"
> - termina - o atendimento do pedido está completo

___
## Estrutura para a mensagem
> [ i, pid, tid, dur, pl ]

- *i* - o número sequencial do pedido (gerado por Un)
- *pid* - identificador de sistema do processo (cliente, no caso do pedido; servidor, no caso da resposta)
- *tid* - identificador no sistema do thread cliente (cliente, no caso do pedido; servidor, no caso da resposta)
- *dur* - duração, em milissegundos, de utilização (de um lugar) do Quarto de Banho (valor atribuído no pedido e repetido na resposta, se se der a ocupação; se não se der, por motivo de o Quarto de
Banho estar em vias de encerrar, o servidor responde aqui com o valor -1
- *pl* – nº de lugar que eventualmente lhe será atribuído no Quarto de Banho (no pedido, este campo é preenchido com o valor -1 e na resposta terá o valor do lugar efetivamente ocupado ou também -1,
na sequência de insucesso de ocupação, por motivo de encerramento)