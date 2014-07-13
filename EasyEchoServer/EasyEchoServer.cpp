// EasyEchoServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "EasyEchoServer.h"

#pragma comment(lib,"ws2_32.lib")

#define WM_RECV (WM_APP + 1)
#define MY_PORT 9001

SOCKET g_ListenSocket = NULL;
std::list<Session*> g_SessionList;

int _tmain( int argc, _TCHAR* argv[] )
{
	HWND hWnd;
	WNDCLASS wc;
	CHAR* ClassName = "EasyEchoServer";

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = (WNDPROC)WndProc;
	wc.cbClsExtra = NULL;
	wc.cbWndExtra = NULL;
	wc.hInstance = NULL;
	wc.hIcon = LoadIcon( NULL, IDI_APPLICATION );
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = (HBRUSH)( COLOR_WINDOW + 1 );
	wc.lpszMenuName = NULL;
	wc.lpszClassName = (LPCWSTR)ClassName;

	if ( !RegisterClass( &wc ) )
		return -1;

	hWnd = CreateWindow(
		(LPCWSTR)ClassName,
		L"EasyEchoServer",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		NULL,
		CW_USEDEFAULT,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL );

	WSADATA wsa;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa ) != 0 )
	{
		WSACleanup();
		return -1;
	}

	g_ListenSocket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( INVALID_SOCKET == g_ListenSocket )
		return -1;

	int opt = 1;
	setsockopt( g_ListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof( int ) );


	SOCKADDR_IN serverAddr;
	ZeroMemory( &serverAddr, sizeof( serverAddr ) );
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons( MY_PORT );
	serverAddr.sin_addr.s_addr = htonl( INADDR_ANY );
	int ret = bind( g_ListenSocket, (SOCKADDR*)&serverAddr, sizeof( serverAddr ) );
	if ( ret == SOCKET_ERROR )
	{
		return -1;
	}

	ret = listen( g_ListenSocket, SOMAXCONN );
	if ( ret == SOCKET_ERROR )
	{
		return -1;
	}

	ret = WSAAsyncSelect( g_ListenSocket, hWnd, WM_RECV, FD_ACCEPT | FD_CLOSE );
	if ( ret == SOCKET_ERROR )
	{
		return -1;
	}

	printf( "Server Start \n" );

	MSG msg = { 0, };
	DWORD result = 0;

	while ( result = GetMessage( &msg, NULL, 0, 0 ) )
	{
		if ( result == -1 )
		{
			return -2;
		}

		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}

	for ( auto iter : g_SessionList )
	{
		closesocket( iter->m_Socket );
		delete iter;
	}

	g_SessionList.clear();
	closesocket( g_ListenSocket );
	WSACleanup();

	return 0;
}

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch ( msg )
	{
	case WM_RECV:
		if ( WSAGETSELECTERROR( lParam ) )
		{
			return -1;
		}

		SocketEventProc( hWnd, msg, wParam, lParam );

		break;
	default:
		break;
	}
	
	return DefWindowProc( hWnd, msg, wParam, lParam );
}

void SocketEventProc( HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam )
{
	SOCKET socket = NULL;
	SOCKADDR_IN clientSockAddr;
	Session* session = nullptr;

	int ret = 0;
	int addressLength = 0;

	switch ( WSAGETSELECTEVENT( lParam ) )
	{
	case FD_ACCEPT:
		addressLength = sizeof( clientSockAddr );
		socket = accept( wParam, (SOCKADDR*)&clientSockAddr, &addressLength );

		if ( INVALID_SOCKET == socket )
		{
			return;
		}

		session = new Session();
		session->m_Socket = socket;

		ret = WSAAsyncSelect( socket, hWnd, WM_RECV, FD_READ | FD_WRITE | FD_CLOSE );

		if ( ret == SOCKET_ERROR )
		{
			closesocket( socket );
			return;
		}

		printf( "Accept : %s \n", inet_ntoa( clientSockAddr.sin_addr) );

		g_SessionList.push_back( session );

		break;
	case FD_READ:
		for ( std::list<Session*>::iterator iter = g_SessionList.begin(); iter != g_SessionList.end(); ++iter )
		{
			if ( (*iter)->m_Socket == wParam )
			{
				session = nullptr;
				session = *iter;
				ret = recv( session->m_Socket, session->recvBuffer, MAX_BUFFER_SIZE, NULL );

				if ( ret == SOCKET_ERROR )
				{
					closesocket( ( *iter )->m_Socket );
					g_SessionList.erase( iter );
					return;
				}

				ret = send( session->m_Socket, session->recvBuffer, ret, NULL );
				if ( ret == SOCKET_ERROR )
				{
					closesocket( ( *iter )->m_Socket );
					g_SessionList.erase( iter );
					return;
				}

				printf( "Send Echo Message \n" );

				break;
			}
		}
		break;
	case FD_WRITE:
		break;
	case FD_CLOSE:
		closesocket( wParam );
		break;
	}
}