#include "ClientManager.h"

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    ClientManager App;
    return App.Run(hInstance, nCmdShow);
}
