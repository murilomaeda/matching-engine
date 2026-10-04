# Matching Engine
## Como compilar e rodar

```
g++ engine.cpp -o engine
./engine
```

Os comandos são lidos da entrada padrão, então também dá para passar um arquivo: `./engine < in.txt`.

## Comandos

| Comando | Exemplo |
|---|---|
| `limit <buy\|sell> <preço> <qtd>` | `limit buy 10 100` |
| `market <buy\|sell> <qtd>` | `market sell 200` |
| `peg bid buy <qtd>` / `peg offer sell <qtd>` | `peg bid buy 150` |
| `cancel order <id>` | `cancel order identificador_1` |
| `modify order <id> <preço> <qtd>` | `modify order identificador_1 9.98 200` |
| `print book` | `print book` |
| `exit` | `exit` |

## Estrutura

- **`Order`**: struct com `price`, `id`, `qtd`, `type` (0 limit, 1 market, 2 peg), `side` (0 buy, 1 sell) e `time`.
- **`BookBuy` / `BookSell`**: `set<Order,Cmp>`, um para cada lado. O `set` mantém o book sempre ordenado, então a melhor ordem é sempre o `begin()`. Inserir e remover custa O(log n).
- **`Cmp`**: um único comparador para os dois sets. Como cada set só tem ordens de um lado, o `side` diz a direção: no buy os maiores preços vêm primeiro, no sell os menores. Empate de preço é desempatado pelo `time`, ou seja, quem chegou antes (prioridade preço-tempo).
- **`orders`**: vetor indexado pelo `id`, guarda o estado atual de cada ordem. É por ele que o `cancel` e o `modify` acham a ordem no book. Como o `Cmp` só olha `price`, `side` e `time`, `BookBuy.erase(orders[id])` encontra a ordem direto. `qtd = 0` significa ordem executada ou cancelada.
- **`match`**: executa uma ordem que chega contra o lado oposto do book e retorna a quantidade que sobrou. É usada pelo `limit`, pelo `market` e pelo `modify`.
- **`reprice`**: atualiza o preço das pegs depois de cada comando.

## Decisões

### Preços como inteiros
Os preços são multiplicados por 100 e guardados como `int` (10.5 vira 1050), e divididos por 100 na hora de imprimir. Assim as comparações e os trades não sofrem com erro de ponto flutuante. Os preços têm no máximo 2 casas decimais.

### Identificadores
Toda ordem limit ou peg recebe um id em ordem de chegada, impresso como `identificador_N`, começando em 1. Ordens market não recebem id, porque nunca ficam no book.

### Limit que sobra
O que sobra de uma ordem fica no book no preço da limit.

### Preço do trade
O trade sai sempre pelo preço da ordem que já estava no book, não da ordem que chegou.

### Uma linha de trade por nível de preço
Se uma ordem consome várias ordens no mesmo preço, imprimo uma linha só com a quantidade somada (`Trade, price: 20, qty: 150` consumindo 100 + 50). Quando o preço muda, sai uma nova linha.

### Market
Uma ordem market passa pelo lado oposto até ser totalmente executada ou o book acabar. O que sobrar é descartado, porque market nunca fica no book. Se nada foi executado (book vazio), imprime `Market order discarded`.

### Execução parcial
Uma ordem parcialmente executada mantém sua posição na fila: o preço e o tempo não mudam, só a quantidade.

### Cancel
`cancel order <id>` tira a ordem do book e marca `qtd = 0`. Ids no formato `identificador_N` que não existem, já foram executados ou já foram cancelados respondem `Order not found`.

### Modify
- Formato: `modify order <id> <preço> <qtd>`.
- Qualquer alteração perde a prioridade, tanto de preço quanto só de quantidade, para ninguém poder guardar lugar na fila.
- Para isso a `Order` tem um campo `time` separado do `id`: o `id` não pode mudar, porque é o identificador que o usuário conhece e o índice em `orders`. O modify dá um `time` novo para a ordem, que vai para o fim da fila no novo preço.
- Pegs não podem ser modificadas, porque não têm um preço delas.
### Peg
- Só são aceitas `peg bid buy` e `peg offer sell`.
- A peg fica nos mesmos sets que as limits, com um preço real, para podermos usar o `match`, `print book` e `cancel` normalmente.
- Os ids das pegs ativas ficam em `pegBuy` ou `pegSell`. Depois de cada comando que pode mudar o book, `reprice()` recalcula o preço de referência e, se mudou, tira a peg do set e coloca de volta com o preço novo.
- **Referência**: a melhor ordem do mesmo lado que *não* é peg. Senão a peg poderia seguir a si mesma e nunca descer quando a ordem que ela seguia é executada ou cancelada.
- **Prioridade**: a peg mantém o `time` de quando foi criada, mesmo quando muda de preço.
- **Sem referência**: se não sobrar nenhuma ordem não-peg no lado, a peg sai do book (fica com `price = 0`) mas continua ativa na lista. Ela não pode ser executada enquanto estiver fora, e volta para o book assim que aparecer uma ordem para seguir. Isso também vale para uma peg criada com o lado vazio. A peg não fica parada num preço antigo e o usuário não perde a ordem.

### Entradas inválidas
Lado inválido, preço ou quantidade ≤ 0, combinação de peg não permitida e comando desconhecido imprimem uma mensagem e o resto da linha é descartado. O programa continua rodando.

### Print book
`print book` mostra o book lado a lado, compras à esquerda e vendas à direita, cada lado na ordem de prioridade, como no enunciado.

## Limitações

- Um único ativo, tudo em memória, sem persistência.
- Preços com no máximo 2 casas decimais, e até cerca de 2*10^7 (limite de `int` com o preço × 100).
- O `reprice` passa por todas as pegs ativas a cada comando: O(P log N) por comando, onde P é o número de pegs ativas.
- O vetor `orders` guarda todas as ordens já criadas, incluindo aquelas que já foram executadas ou canceladas.
- O `cancel` e o `modify` esperam um id no formato `identificador_N` com N numérico. Um id como `identificador_abc` faz o `stoi` lançar uma exceção e encerra o programa.
