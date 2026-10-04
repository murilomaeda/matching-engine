#include<bits/stdc++.h>
using namespace std;

int id = 0;



struct Order
{
	//Tipo:
	//0 - limit
	//1 - market
	//2 - peg

	//Side:
	//0 - Buy
	//1 - Sell

	//id aumenta com o tempo, então serve para identificar qual veio antes e qual veio depois

	//Preços são guardados como inteiros multiplicados por 100 (10.5 -> 1050) para evitar erro de ponto flutuante,
	//e divididos por 100 na hora de imprimir

	//Peg com price = 0 está fora do book (não tem preço para seguir)
	int price, id, qtd, type, side;
};

//Uso o side pra saber se é para usar maior ou menor
//Desempate pelo menor id
struct Cmp
{
	bool operator()(const Order &a, const Order &b) const
	{
		if(a.price != b.price)
		{
			if(a.side == 0) return a.price > b.price;
			return a.price < b.price;
		}
		return a.id < b.id;
	}
};

//orders[id] guarda a ordem
//Utilizo orders para achar/apagar a ordem no book
//qtd = 0 significa ordem executada ou cancelada.
vector<Order> orders;

set<Order,Cmp> BookBuy, BookSell;

//ids das pegs ativas de cada lado
vector<int> pegBuy, pegSell;


//preço digitado -> inteiro (10.5 -> 1050)
int toInt(double price)
{
	return (int)llround(price*100);
}

//inteiro -> preço (só para imprimir)
double toPrice(int price)
{
	return price/100.0;
}

//descarta o resto da linha depois de uma entrada inválida
void skipLine()
{
	cin.clear();
	cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

string prefix = "identificador_";

//"identificador_x" -> x, retorna -1 se não existir ordem ativa com esse id
int getId(const string &s)
{
	if(s.compare(0, prefix.size(), prefix) != 0) return -1;
	int x = stoi(s.substr(prefix.size()));
	if(x <= 0 || x >= (int)orders.size() || orders[x].qtd == 0) return -1;
	return x;
}

void printTrade(int price, int qtd)
{
	cout << "Trade, price: " << toPrice(price) << ", qty: " << qtd << "\n";
}

//Executa uma ordem que chega (side) contra o lado oposto do book.
//Se type == 0 (limit), só cruza enquanto o preço do book for aceitável.
//O trade sai pelo preço da ordem que já estava no book, e é impresso
//Retorna a qtd que sobrou

//Tenta fazer o match de uma ordem que chegou com o lado oposto (buy com sell, sell com buy)
//Se é uma ordem de limit, só faço match enquanto o preço do book estiver bom
int match(int side, int qtd, int type, int price)
{
	//Escolho só o set que vou utilizar
	set<Order,Cmp> &book = (side == 0 ? BookSell : BookBuy);

	int tradePrice = -1, tradeQtd = 0;

	while(qtd > 0 && !book.empty())
	{
		Order best = *book.begin();

		//Se for limit
		if(type == 0)
		{	
			//Se for buy e o preço for alto demais, eu paro
			if(side == 0 && best.price > price) break;

			//Se for sell e o preço for baixo demais, eu paro
			if(side == 1 && best.price < price) break;
		}

		//Aqui eu já sei que o preço está válido, posso executar a trade

		//Aqui é se a minha ordem está passando por vários outros pedidos que estavam no book
		//Aí eu tenho que imprimir que executei o pedido anterior e continuar fazendo outros
		if(best.price != tradePrice)
		{
			if(tradeQtd > 0) printTrade(tradePrice, tradeQtd);
			tradePrice = best.price;
			tradeQtd = 0;
		}

		int q = min(qtd, best.qtd);
		qtd -= q;
		tradeQtd += q;

		//preço e id não mudam, então a ordem volta para a mesma posição (não perde prioridade)
		book.erase(book.begin());
		best.qtd -= q;
		orders[best.id] = best;
		if(best.qtd > 0) book.insert(best);
	}

	if(tradeQtd > 0) printTrade(tradePrice, tradeQtd);

	return qtd;
}


//Preço que as pegs de um lado seguem: a melhor ordem desse lado que não é peg
//Retorna 0 se não tiver nenhuma
int reference(int side)
{
	set<Order,Cmp> &book = (side == 0 ? BookBuy : BookSell);

	for(const Order &o : book)
		if(o.type != 2) return o.price;

	return 0;
}

//Atualiza o preço das pegs depois de cada comando
//O id não muda, então a peg mantém a prioridade de quando foi criada
void reprice()
{
	for(int side = 0; side < 2; side++)
	{
		set<Order,Cmp> &book = (side == 0 ? BookBuy : BookSell);
		vector<int> &pegs = (side == 0 ? pegBuy : pegSell);

		int ref = reference(side);

		vector<int> active;
		for(int x : pegs)
		{
			//peg executada ou cancelada sai da lista
			if(orders[x].qtd == 0) continue;
			active.push_back(x);

			if(orders[x].price == ref) continue;

			//tiro do book e coloco de volta com o preço novo
			//se ref == 0 a peg fica fora do book até aparecer um preço para seguir
			if(orders[x].price != 0) book.erase(orders[x]);
			orders[x].price = ref;
			if(ref != 0) book.insert(orders[x]);
		}
		pegs = active;
	}
}

//Imprime o Book
void printBook()
{
	cout << left << setw(20) << "Buy" << "| Sell\n";

	auto b = BookBuy.begin();
	auto s = BookSell.begin();

	while(b != BookBuy.end() || s != BookSell.end())
	{
		if(b != BookBuy.end())
		{
			cout << right << setw(8) << b->qtd << " @ " << left << setw(9) << toPrice(b->price);
			b++;
		}
		else cout << setw(20) << "";

		cout << "| ";

		if(s != BookSell.end())
		{
			cout << s->qtd << " @ " << toPrice(s->price);
			s++;
		}
		cout << "\n";
	}
}

int main()
{
	cout << setprecision(15);

	//para os ids começarem em 1;
	orders.push_back({0,0,0,0,0});
	id = 1;

	string command;
	while(cin >> command)
	{
		if(command == "limit")
		{
			string type; double p; int qtd;
			cin >> type >> p >> qtd;

			if((type != "buy" && type != "sell") || p <= 0 || qtd <= 0)
			{
				cout << "Ordem Invalida. Usage: limit <buy|sell> <price> <qty>\n";
				skipLine();
				continue;
			}

			int price = toInt(p);
			int side = (type == "buy" ? 0 : 1);

			cout << "Order created: " << type << " " << qtd << " @ " << toPrice(price) << " " << prefix << id << "\n";

			//Tento dar match nos meus pedidos
			qtd = match(side, qtd, 0, price);

			orders.push_back({price,id,qtd,0,side});
			if(qtd > 0)
			{
				if(side == 0) BookBuy.insert(orders[id]);
				else BookSell.insert(orders[id]);
			}
			id++;

			reprice();
		}
		else if(command == "market")
		{
			string type; int qtd;
			cin >> type >> qtd;

			if((type != "buy" && type != "sell") || qtd <= 0)
			{
				cout << "Invalid order. Usage: market <buy|sell> <qty>\n";
				skipLine();
				continue;
			}

			int side = (type == "buy" ? 0 : 1);

			int left = match(side, qtd, 1, 0);
			if(left == qtd) cout << "Market order discarded\n";

			reprice();
		}
		else if(command == "peg")
		{
			string ref, type; int qtd;
			cin >> ref >> type >> qtd;

			//Só aceito peg bid buy e peg offer sell
			if(!((ref == "bid" && type == "buy") || (ref == "offer" && type == "sell")) || qtd <= 0)
			{
				cout << "Invalid order. Usage: peg bid buy <qty> | peg offer sell <qty>\n";
				skipLine();
				continue;
			}

			int side = (type == "buy" ? 0 : 1);

			cout << "Order created: peg " << ref << " " << type << " " << qtd << " " << prefix << id << "\n";

			//Começa fora do book e o reprice ajusta isso
			orders.push_back({0,id,qtd,2,side});
			if(side == 0) pegBuy.push_back(id);
			else pegSell.push_back(id);
			id++;

			reprice();
		}
		else if(command == "cancel")
		{
			string word, sid; cin >> word >> sid;

			int x = getId(sid);
			if(word != "order" || x == -1)
			{
				cout << "Order not found\n";
				continue;
			}

			if(orders[x].side == 0) BookBuy.erase(orders[x]);
			else BookSell.erase(orders[x]);
			orders[x].qtd = 0;

			cout << "Order cancelled\n";

			reprice();
		}
		else if(command == "print")
		{
			string word; cin >> word;
			printBook();
		}
		else if(command == "exit")
		{
			break;
		}
		else
		{
			cout << "Unknown command\n";
			skipLine();
		}
	}

}
