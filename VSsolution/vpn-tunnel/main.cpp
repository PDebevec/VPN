#include "VPN.h"

int main(int argc, char* argv[]) {
	/*HWND hwnd = GetConsoleWindow();
	if (hwnd != NULL) {
		ShowWindow(hwnd, SW_HIDE);
	}*/
	VPN vpn(argc, argv); // dekleracija in inicializacija
	
	vpn.startVPN(argc, argv); // začetek strani odjemalca ali strežnika
	
	vpn.communicationLoop(); // zanka komunikacije z GUI
	return 0;
}