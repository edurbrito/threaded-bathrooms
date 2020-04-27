## Limite de threads por processo

Neste momento conseguimos no Servidor ter um limite de threads. Quando atingido tem que se esperar que haja threads disponiveis. O problema é ao tentar reutilizar os espaços do array de threads que vão ficando livres ao terminar de responder aos pedidos.
Da forma como estou a atualizar a posição do array a ocupar, é possível que estejamos a incrementar a posição a ocupar no main para uma que tem uma thread que ainda não terminou.
Assim, tenho 2 ideias para resolver isso mas não sei se são boas:

1 - Criar arrays de posições livres(int) para cada array de threads. Começava todo a 0 (todo livre). À medida que se criava threads ia-se, no main, se ainda existirem threads disponiveis, ocupar(colocar a 1 na posição respetiva do array de posições livres) a 1ª posição livre no array de threads - implica um loop de pesquisa. Nas threads, a posição atual da thread era colocada a 0 quando esta termina - fica livre;

2 - Ter um Fifo auxiliar que começa com indices de 0 até ao numero maximo de threads. A posição a ocupar é dada fazendo 1 read do Fifo. Uma posição que fique livre é colocada no Fifo pela thread que a liberta.

Resolvendo o problema no servidor diria para aplicar a mesma estratégia no Cliente e assim ficamos com um limite de threads por processo
