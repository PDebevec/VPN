#include "VPN.h"

int main(int argc, char* argv[]) {

	VPN vpn(argc, argv);
	vpn.startVPN(argc, argv);

	/*HWND hwnd = GetConsoleWindow();
	if (hwnd != NULL) {
		ShowWindow(hwnd, SW_HIDE);
	}*/

	vpn.communicationLoop();
	system("pause");
	return 0;
}