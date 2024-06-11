#include "VPN.h"

int main(int argc, char* argv[]) {
	/*HWND hwnd = GetConsoleWindow();
	if (hwnd != NULL) {
		ShowWindow(hwnd, SW_HIDE);
	}*/
	VPN vpn(argc, argv);
	vpn.startVPN(argc, argv);
	
	vpn.communicationLoop();
	return 0;
}