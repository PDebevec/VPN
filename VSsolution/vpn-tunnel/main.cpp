#include "clientUDP.h"
#include "serverUDP.h"


int main(int argc, char* argv[]) {

	if (!strcmp(argv[1], "server"))
	{
		ServerUDP sudp;
		sudp.startLoop();
		system("pause");
	}
	else if(!strcmp(argv[1], "client"))
	{
		ClientUDP cudp;
		cudp.startLoop();
		system("pause");
	}
}