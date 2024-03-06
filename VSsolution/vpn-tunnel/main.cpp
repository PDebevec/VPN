#include "VPN.h"

int main(int argc, char* argv[]) {

	VPN vpn(argc, argv);
	vpn.startVPN(argc, argv);
	vpn.communicationLoop();
	system("pause");
	return 0;
}