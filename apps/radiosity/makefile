all:
	gcc -I./glibdumb/ -I glibps/ *c glibdumb/*c glibps/*c -lm -lpthread
	./a.out -p 2 -tq 2000 -ae 2000.0 -pr 5 -pv 500 -bf 0.0015 -en 0.005 -batch
