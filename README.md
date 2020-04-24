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


### Autores

Diana Freitas - [up201806230](mailto:up201806230@fe.up.pt)
Eduardo Brito - [up201806271](mailto:up201806271@fe.up.pt)
Maria Baía - [up201704951](mailto:up201704951@fe.up.pt)