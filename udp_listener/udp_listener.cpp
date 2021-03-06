/*
* DAQ Project
* Software Component: UDP Data Recorder
* Ronnie Wong
* Date: Jan 2018 (initial commit)
* udp_server_handler.cpp : Defines the entry point for the console application.
*/

#include "stdafx.h"

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif	// requirement for WINSOCK2: place this block after stdafx.h

#include <iostream>			// std:: cpp of stdio
#include <stdio.h>			// standard input/output	  : file io
#include <stdlib.h>			// standard lib - malloc/free : memory allocation
#include <WinSock2.h>		// for ETH control
#include <Windows.h>		// Windows API port to C/C++
#include <string.h>			// character array manipulation
#include <fstream>			// std:: input/output stream class
#include <winioctl.h>		// windows specific input/output control
#include <iphlpapi.h>		// IP(UDP/TCP) helper functions
#include <time.h>			// timer functions

// unlock headers if needed
//#include <tchar.h>			// for unicode single/multibyte character sets
//#include <strsafe.h>			// implements HRESULT return value

#pragma comment(lib,"ws2_32.lib")		// Winsock library

// defines for PROGRAM - change parameters as needed:
#define BUFLEN			512		// Max length of recording buffer
#define SEND_PORT		11		// port for sending data to picozed
#define LISTEN_PORT		12		// port for listening to data from picozed
#define LOG_ADDRESS		L"\\\\.\\I:\\BinaryDataRecord_02152018.bin"														// Location for file storage
#define TEST_FOLDER		L"\\\\.\\C:\\Users\\nocturnalhippo\\Desktop\\testFolder\\BinaryDataRecord_02152018.bin"			// Location for file storage
#define DATA_FOLDER		L"\\\\.\\C:\\Users\\ronny.wong\\Desktop\\data_view\\dataForward.bin"							// redirection of data
#define CLIENT_ADDRESS	"192.168.1.10"
#define SERVER_ADDRESS	"192.168.1.11"
#define DATA_ADDRESS	"192.168.1.11"
#define DATA_PORT		14
#define HANDSHAKE		0x40		// Handshake value to Picozed to initiate UDP transmission
#define PACKETS2RECORD	60000		// Num of packets to record in FOR-loop
#define TEST_PORT		14

using namespace std;

/* Escape thread for killing app */
DWORD WINAPI EscapeKeyPressed(LPVOID lpParam);

int main()
{
	// for Escape Thread
	CreateThread(NULL, 0, EscapeKeyPressed, NULL, 0, NULL);

	WSADATA WSAData;
	SOCKET server, client;
	SOCKADDR_IN serverAddr, clientAddr;

	char startByte[1] = { HANDSHAKE };
	char randomPacket[20] = { 0x00, 0x0A, 0xFF, 0xFA, 0xBB,
		0x00, 0xFA, 0xFF, 0x0A, 0xCC,
		0x00, 0x0A, 0xFF, 0xFA, 0xDD,
		0x00, 0xFA, 0xFF, 0x0A, 0xEE };
	int startByteSent;
	int serverBind;
	DWORD BytesToWrite;				// Bytes to be written -> 512 Bytes
	DWORD BytesWritten;				// Bytes Written counter
	BOOL bErrorFlag = FALSE;
	char buffer[BUFLEN];
	int recordCount0 = 0;
	int recordCount25 = (int)((PACKETS2RECORD * 25) / 100);
	int recordCount50 = (int)((PACKETS2RECORD * 50) / 100);
	int recordCount75 = (int)((PACKETS2RECORD * 75) / 100);
	int recordCount90 = (int)((PACKETS2RECORD * 90) / 100);

	/* Notify user for process cancellation */
	cout << "Hit escape key anytime to kill the current process" << endl;

	/* Create/Open Data File to be Written */
	wcout << "Creating file at: " << LOG_ADDRESS << endl;
	HANDLE hFile;
	hFile = CreateFile(DATA_FOLDER,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		cout << "Terminal failure: Unable to open file for writing." << endl;
		system("pause");
		return 1;
	}


	/* Initialize Winsock */
	cout << "Initializing Winsock2..." << endl;
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
		printf("Failed. Error Code: %d", WSAGetLastError());
		system("pause");
		return 1;
	}
	
	/* Open server side socket */
	cout << "Opening server socket" << endl;
	system("pause");
	server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (server == INVALID_SOCKET) {
		printf("Could not create RAW socket: %d\r\n", WSAGetLastError());
		system("pause");
		WSACleanup();
		return 1;
	}
	else {
		cout << "Server socket opened" << endl;
	}

	/* Define server sock structure */
	serverAddr.sin_addr.s_addr = inet_addr(DATA_ADDRESS);
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(TEST_PORT);
	/* Bind server socket to address */
	serverBind = bind(server, (SOCKADDR *)&serverAddr, sizeof(serverAddr));
	if (serverBind) {
		printf("Bind failed: %d\n\r", WSAGetLastError());
		system("pause");
		return 1;
	}
	else {
		cout << "Bind successful." << endl;
		system("pause");
	}


	/* Recording Loop */
	cout << "Listening..." << endl;
	BytesToWrite = (DWORD)BUFLEN;
	clock_t begin = clock();
	for (int recordIndex = 0; recordIndex < PACKETS2RECORD; recordIndex++) {
		BytesWritten = 0;
		if (recv(server, buffer, sizeof(buffer), 0) > 0) {
			// output tracker
			if (recordIndex == recordCount0) printf("Record 0 saved.\n\r");
			if (recordIndex == recordCount25) printf("25%% complete\n\r");
			if (recordIndex == recordCount50) printf("50%% complete\n\r");
			if (recordIndex == recordCount75) printf("75%% complete\n\r");
			if (recordIndex == recordCount90) printf("90%% complete\n\r");

			bErrorFlag = WriteFile(hFile, buffer, BytesToWrite, &BytesWritten, NULL);
			if (bErrorFlag == FALSE) {
				printf("Terminal failure: Unable to write to file.\n\r");
			}

			// clear buffer
			for (int clearIndex = 0; clearIndex < BUFLEN; clearIndex++) {
				buffer[clearIndex] = 0;
			}
		}
		else {
			printf("SOCKET ERROR: %d\n\r", WSAGetLastError());
			system("pause");
			WSACleanup();
			return 1;
		}
	}

	clock_t end = clock();
	closesocket(server);
	WSACleanup();

	if (!CloseHandle(hFile)) {
		cout << "CloseHandle failed." << endl;
	}
	else {
		cout << "Successfully closed record file." << endl;
	}

	cout << "Arrived at end of program." << endl;
	system("pause");

	return 0;
}


// Functions: TO-DO (Move to header .h and function .cpp files)
DWORD WINAPI EscapeKeyPressed(LPVOID lpParam)
{
	while (GetAsyncKeyState(VK_ESCAPE) == 0) {
		// sleep
		Sleep(10);
	}
	exit(0);
}