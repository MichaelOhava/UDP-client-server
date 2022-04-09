#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h> 
#include <string.h>
#include <time.h>
#include <conio.h>

#define TIME_PORT	27015
#define TICKS 2000
#define REQUESTS_IN_A_ROW 2000 //this needs to match the server-side

void display_menu(); //main UI
void MeasureRTT(char* Msg_rcvd, int* index); // client side - case 5
void GetClientToServerDelayEstimation(SOCKET connSocket, sockaddr_in server); //client side - case 4
void MeasureTimeLap(SOCKET connSocket, sockaddr_in server); // client side - case  13

void main()
{   //INITIALIZATION:
	int choice = 0, temp, i = 0, bytesSent = 0, bytesRecv = 0;
	char sendBuff[255] = "", recvBuff[255] = "";
	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Client: Error at WSAStartup()\n";
		return;
	}
	SOCKET connSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == connSocket)
	{
		cout << "Time Client: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}
	sockaddr_in server;
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	server.sin_port = htons(TIME_PORT);
	//COMMUNICATION:
	display_menu();
	cin >> choice;
	while (choice < 1 || choice > 14)
	{
		getchar();
		cout << "invalid choice - pick an action from 1 - > 14\n";
		cin >> choice;
	}
	printf("\n");
	while (choice != 14)
	{
		_itoa_s(choice, sendBuff, 10); //choice to string into sendBuff, decimal number format

		bytesSent = sendto(connSocket, sendBuff, (int)strlen(sendBuff), 0, (const sockaddr*)&server, sizeof(server));
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "Time Client: Error at sendto(): " << WSAGetLastError() << endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}
		if (choice != 5)
			cout << "Time Client: Sent: " << bytesSent << "/" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";

		// Gets the server's answer using simple recieve (no need to hold the server's address).
		bytesRecv = recv(connSocket, recvBuff, 255, 0);
		if (SOCKET_ERROR == bytesRecv)
		{
			cout << "Time Client: Error at recv(): " << WSAGetLastError() << endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}

		if (choice != 5 && choice != 4 && choice != 13) {
			recvBuff[bytesRecv] = '\0'; //add the null-terminating to make it a string
			cout << "\nThe Answer Is: \n" << recvBuff << endl;
			//cout << "Time Client: Recieved: " << bytesRecv << " bytes of \"" << recvBuff << "\" message.\n";
			cout << "\n------\n\n";
		}
		if (choice == 5)
			MeasureRTT(recvBuff, &i);
		if (choice == 4)
			GetClientToServerDelayEstimation(connSocket, server);
		if (choice == 13)
			MeasureTimeLap(connSocket, server);

		if ((choice != 5) || (choice == 5 && i >= TICKS))
		{
			while ((getchar()) != '\n');
			cout << "press anything to continue" << endl;
			while (!_kbhit())
			{
			}
			system("cls");


			display_menu();
			cin >> choice;
			while (choice < 1 || choice > 14)
			{
				while ((getchar()) != '\n');
				cout << "invalid choice - pick an action from 1 - > 14\n";
				cin >> choice;
				while ((getchar()) != '\n');
			}
			if (choice == 5 && i >= TICKS) i = 0;
		}
	}

	// Closing connections and Winsock.
	cout << "Thanks for using our services!\n";
	cout << "Time Client: Closing Connection.\n";
	closesocket(connSocket);

	system("pause");
}

void display_menu()
{
	printf("---Hello!---\nChoose the action you would like:\n");
	printf("1.Get the time (Y/M/D/H/Min/Sec) format\n");
	printf("2.Get the time without the date\n");
	printf("3.Get the time in seconds format since 1.1.1970\n");
	printf("4.Get client to server delay estimation\n");
	printf("5.MeasureRTT - Get Round Trip Time\n");
	printf("6.Get the time withoud date or seconds\n");
	printf("7.Get the current year\n");
	printf("8.Get the current day and month\n");
	printf("9.Get seconds since the start of the current month\n");
	printf("10.Get the current week number since the start of the year\n");
	printf("11.Get 1 if its Summer clock right now, 0 if it's Winter clock\n");
	printf("12.Get H/M/S time from JP, AUS, USA, PT.\n");
	printf("13.Measure Time Lap - get the time past between the times using this action (max amount 3 min)\n");
	printf("14.Exit\n");
	printf("Desired Action: ");
}


void MeasureRTT(char* Msg_rcvd, int* index)
{
	DWORD serverTime = atoi(Msg_rcvd);

	static DWORD total = 0;

	// to increase the accuracy of the measured time increase the TICKS in the definition above.
	if (*index < TICKS)
	{
		total += ((DWORD)GetTickCount64() - serverTime);
		*index += 1;
	}
	if (*index >= TICKS) {
		cout << "RTT is: " << total / 100 << " milliseconds. " << endl;
		*index += 1;
		total = 0;
	}
}

void GetClientToServerDelayEstimation(SOCKET connSocket, sockaddr_in server)
{
	static double total = 0; //float represntation for better accuracy.
	int bytesSent = -1, bytesRecv = 0, i;
	double current = 0, last = 0;
	char sendBuff[255] = "4", recvBuff[255] = ""; //spams server with REQUESTS_IN_A_ROW amount of requests, to estimate delay

	for (i = 0; i < REQUESTS_IN_A_ROW; i++)//here we need to send #TICKS msg one after another
	{//sending the X messages in order to receive tick counts from server
		bytesSent = sendto(connSocket, (sendBuff), (int)strlen(sendBuff), 0, (const sockaddr*)&server, sizeof(server));
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "Time Client: Error at sendto(): " << WSAGetLastError() << endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}
	}

	for (i = 0; i < REQUESTS_IN_A_ROW; i++)//here we need to send #TICKS msg one after another
	{//receiving back the X messages and calculating.
		bytesRecv = recv(connSocket, recvBuff, 255, 0);
		current = atoi(recvBuff);
		if (SOCKET_ERROR == bytesRecv)
		{
			cout << "Time Client: Error at recv(): " << WSAGetLastError() << endl;
			closesocket(connSocket);
			WSACleanup();
			return;
		}
		if (last != 0) //calculating differences requires at least 2 tick counts.
			total += current - last;
		last = current;
	}//gettickcount() measurements are in milliseconds

	if (total != 0)                                              //differences calculated minus 1st one
		cout << "Estimated client-to-server delay is: " << total / (REQUESTS_IN_A_ROW - 1) << endl;
	else cout << "Esimation calculation failed.." << endl;
	total = 0;
}

void MeasureTimeLap(SOCKET connSocket, sockaddr_in server)
{
	int bytesSent = 0, bytesRecv = 0, time_passed = 0;
	char sendBuff[255] = "13", recvBuff[255] = "";
	time_t begin;
	cout << "Timer has activated.. \n" << "enter anything and then press enter (timeout after 3 min) " << endl;
	begin = clock();
	//timer for the 2nd call to go out within 3 minutes:
	for (; ((clock() - begin) / CLOCKS_PER_SEC) < 180;)
	{
		if (_kbhit()) //if any key is hit, send another message
		{//2nd msg in time
			bytesSent = sendto(connSocket, (sendBuff), (int)strlen(sendBuff), 0, (const sockaddr*)&server, sizeof(server));
			if (SOCKET_ERROR == bytesSent)
			{
				cout << "Time Client: Error at sendto(): " << WSAGetLastError() << endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}
			while (getchar() != '\n');

			bytesRecv = recv(connSocket, recvBuff, 255, 0);
			time_passed = atoi(recvBuff);
			if (SOCKET_ERROR == bytesRecv)
			{
				cout << "Time Client: Error at recv(): " << WSAGetLastError() << endl;
				closesocket(connSocket);
				WSACleanup();
				return;
			}
			break;
		}
	}//if no key was hit during 3 minutes, received message will be "timeout"

	if (strcmp(recvBuff, "") == 0)
		cout << "Server timed out..\n";
	else cout << "Time passed since 1st call is: " << time_passed << " seconds " << endl;
}

//void send_msg(SOCKET connSocket, char* sendBuff, sockaddr_in server)
//{
//	int bytesSent = 0;
//	bytesSent = sendto(connSocket, (sendBuff), (int)strlen(sendBuff), 0, (const sockaddr*)&server, sizeof(server));
//	if (SOCKET_ERROR == bytesSent)
//	{
//		cout << "Time Client: Error at sendto(): " << WSAGetLastError() << endl;
//		closesocket(connSocket);
//		WSACleanup();
//		return;
//	}
//}
//
//void get_msg(SOCKET connSocket,char* recvBuff )
//{
//	int bytesRecv = 0;
//	char temp[255] = "";
//	bytesRecv = recv(connSocket, temp, (int)strlen(temp), 0); //recieves timeout or seconds.
//	strcpy(recvBuff, temp);
//	if (SOCKET_ERROR == bytesRecv)
//	{
//		cout << "Time Client: Error at recv(): " << WSAGetLastError() << endl;
//		closesocket(connSocket);
//		WSACleanup();
//		return;
//	}
//}


////////////////////




