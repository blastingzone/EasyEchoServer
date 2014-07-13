#pragma once
#include <winsock2.h>

const unsigned int MAX_BUFFER_SIZE = 4096;

struct Session
{
	SOCKET m_Socket;
	char recvBuffer[MAX_BUFFER_SIZE + 1];
	char sendBuffer[MAX_BUFFER_SIZE + 1];
};

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );
void SocketEventProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam );