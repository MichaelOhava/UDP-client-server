#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include <chrono>

#define TIME_PORT	27015
#define REQUESTS_IN_A_ROW 2000 //needs to match client-side

char* call_func(char* action_choice, const sockaddr* client_addr, int* client_addr_line, SOCKET m_socket);
char* GetTime();
char* GetTimeWithoutDate();
char* GetTimeSinceEpoch();
char* GetClientToServerDelayEstimation(const sockaddr* client_addr, int* client_addr_len, SOCKET m_socket);
char* MeasureRTT();
char* GetTimeWithoutDateOrSeconds();
char* GetYear();
char* GetMonthAndDay();
char* GetSecondsSinceBeginningOfMonth();
char* GetWeekOfYear();
char* GetDaylightSavings();
char* GetTimeWithoutDateInCity();
char* MeasureTimeLap();
char* GetTimeWithDifference(int time_diff);
int next_day(int hour);

void main()
{
	WSAData wsaData;
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Time Server: Error at WSAStartup()\n";
		return;
	}

	SOCKET m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (INVALID_SOCKET == m_socket)
	{
		cout << "Time Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	sockaddr_in serverService;
	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;	//inet_addr("127.0.0.1");
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order (which is big-endian).
	serverService.sin_port = htons(TIME_PORT);
	// Bind the socket for client's requests.
	if (SOCKET_ERROR == bind(m_socket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "Time Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(m_socket);
		WSACleanup();
		return;
	}
	sockaddr client_addr;
	int client_addr_len = sizeof(client_addr), bytesSent = 0, bytesRecv = 0;
	char sendBuff[255], recvBuff[255];

	cout << "Time Server: Wait for clients' requests.\n";

	while (true)
	{
		bytesRecv = recvfrom(m_socket, recvBuff, 255, 0, &client_addr, &client_addr_len);
		if (SOCKET_ERROR == bytesRecv)
		{
			cout << "Time Server: Error at recvfrom(): " << WSAGetLastError() << endl;
			closesocket(m_socket);
			WSACleanup();
			return;
		}

		recvBuff[bytesRecv] = '\0'; //add the null-terminating to make it a string
		cout << "Time Server: Recieved: " << bytesRecv << " bytes of \"" << recvBuff << "\" message.\n";

		//call_func is main response layer, for different functionalities required
		strcpy(sendBuff, call_func(recvBuff, (const sockaddr*)&client_addr, &client_addr_len, m_socket)); //call_func is main response action caller
		sendBuff[strlen(sendBuff)] = '\0'; //to remove the new-line from the created string
		bytesSent = sendto(m_socket, sendBuff, (int)strlen(sendBuff), 0, (const sockaddr*)&client_addr, client_addr_len);
		if (SOCKET_ERROR == bytesSent)
		{
			cout << "Time Server: Error at sendto(): " << WSAGetLastError() << endl;
			closesocket(m_socket);
			WSACleanup();
			return;
		}

		cout << "Time Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << "\" message.\n";
	}

	// Closing connections and Winsock.
	cout << "Time Server: Closing Connection.\n";
	closesocket(m_socket);
	WSACleanup();
}

char* call_func(char* action_choice, const sockaddr* client_addr, int* client_addr_len, SOCKET m_socket)
{//master func
	int choice = atoi(action_choice);
	switch (choice)
	{
	case 1:
		return GetTime();
		break;
	case 2:
		return GetTimeWithoutDate();
		break;
	case 3:
		return GetTimeSinceEpoch();
		break;
	case 4:
		return GetClientToServerDelayEstimation(client_addr, client_addr_len, m_socket);
		break; //requires the ability to send back an X amount of messages back.
	case 5:
		return MeasureRTT();
		break;
	case 6:
		return GetTimeWithoutDateOrSeconds();
		break;
	case 7:
		return GetYear();
		break;
	case 8:
		return GetMonthAndDay();
		break;
	case 9:
		return GetSecondsSinceBeginningOfMonth();
		break;
	case 10:
		return GetWeekOfYear();
		break;
	case 11:
		return GetDaylightSavings();
		break;
	case 12:
		return GetTimeWithoutDateInCity();
		break;
	case 13:
		return MeasureTimeLap();
		break;
	}
}

char* GetTime()
{
	char sendBuff[255] = "";
	// Get the current time.
	time_t timer;
	time(&timer);
	// Parse the current time to printable string.
	strcpy(sendBuff, ctime(&timer));
	sendBuff[strlen(sendBuff) - 1] = '\0'; //to remove the new-line from the created string

	return sendBuff;
}

char* GetTimeWithoutDate()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s(ltm->tm_hour, sendBuff, 10);
	strcat(sendBuff, ":");
	_itoa_s(ltm->tm_min, temp, 10);
	sendBuff[strlen(sendBuff)] = '\0';
	strcat(sendBuff, temp);
	strcat(sendBuff, ":");
	sendBuff[strlen(sendBuff)] = '\0';
	_itoa_s(ltm->tm_sec, temp, 10);
	strcat(sendBuff, temp);

	return sendBuff;
}
char* GetTimeSinceEpoch()
{

	char sendBuff[255] = "";
	// Get the current time.
	time_t timer;
	time(&timer); //time returns the seconds passed since epoch

	_itoa_s(timer, sendBuff, 10);
	strcat(sendBuff, " seconds have passed since 1.1.1970");

	return sendBuff;

}
char* GetClientToServerDelayEstimation(const sockaddr* client_addr, int* client_addr_len, SOCKET m_socket)
{//counts it has sent back the amount of messages received when estimating time server delay.
	char sendBuff[255] = "";
	static short counter = 0;

	double currentTime = GetTickCount();
	_itoa_s(currentTime, sendBuff, 10);
	sendBuff[9] = '\0';

	counter++; //count till all of messages have been received..
	if (counter == REQUESTS_IN_A_ROW)
		counter = 0; //initialize 0 for next time 4. is requested.

	//bytesSent = sendto(m_socket, sendBuff, (int)strlen(sendBuff), 0, client_addr, *client_addr_len);

	return sendBuff;
}
char* MeasureRTT()
{//the client asks X times , and receives X answers
 //it goes in a request , answer , request , answer manner...
	char sendBuff[255] = "";

	DWORD currentTime = GetTickCount();
	_itoa_s(currentTime, sendBuff, 10);

	return sendBuff;
}
char* GetTimeWithoutDateOrSeconds()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s(ltm->tm_hour, sendBuff, 10);
	strcat(sendBuff, ":");
	_itoa_s(ltm->tm_min, temp, 10);
	sendBuff[strlen(sendBuff)] = '\0';
	strcat(sendBuff, temp);
	sendBuff[strlen(sendBuff)] = '\0';

	return sendBuff;
}
char* GetYear()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s((ltm->tm_year + 1900), sendBuff, 10);
	sendBuff[strlen(sendBuff)] = '\0';

	return sendBuff;
}
char* GetMonthAndDay()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s(ltm->tm_mday, sendBuff, 10);
	strcat(sendBuff, "/");
	_itoa_s(ltm->tm_mon + 1, temp, 10);
	sendBuff[strlen(sendBuff)] = '\0';
	strcat(sendBuff, temp);
	sendBuff[strlen(sendBuff)] = '\0';

	return sendBuff;
}
char* GetSecondsSinceBeginningOfMonth()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s(((ltm->tm_mday) * (86400)) - 86400 + (ltm->tm_hour * 3600) + (ltm->tm_min * 60) + (ltm->tm_sec), sendBuff, 10);
	//DAYS SO FAR IN MONTH * SECS IN A DAY, MINUS TODAY + HOURS SO FAR TODAY * HOURS IN SEC + MINS THIS HOUR 
	// * SEC IN MINS + SEC SO FAR.
	sendBuff[strlen(sendBuff)] = '\0';

	return sendBuff;
}
char* GetWeekOfYear()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s(ltm->tm_yday / 7, sendBuff, 10); //yday is amount of days since january 1st
	sendBuff[strlen(sendBuff)] = '\0';

	return sendBuff;
}
char* GetDaylightSavings()
{
	time_t now = time(0);
	tm* ltm = localtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s(ltm->tm_isdst, sendBuff, 10);
	sendBuff[strlen(sendBuff)] = '\0';

	return sendBuff;

}
char* GetTimeWithoutDateInCity()
{
	time_t now = time(0);
	tm* ltm = gmtime(&now);
	// ltm is the struct containing the information of current time
	char sendBuff[255] = "", temp[255] = "";
	//Tokyo, japan:
	strcat(sendBuff, "TKY: ");
	strcat(sendBuff, GetTimeWithDifference(9));
	strcat(sendBuff, "\n");
	//Melbourne, Australia:
	strcat(sendBuff, "Melbourne: ");
	strcat(sendBuff, GetTimeWithDifference(11));
	strcat(sendBuff, "\n");
	//Sanfrancisco, USA
	strcat(sendBuff, "San Francisco: ");
	strcat(sendBuff, GetTimeWithDifference(-7));
	strcat(sendBuff, "\n");
	//Sanfrancisco, USA
	strcat(sendBuff, "Porto: ");
	strcat(sendBuff, GetTimeWithDifference(1));
	strcat(sendBuff, "\n");
	//UTC:
	strcat(sendBuff, "UTC: ");
	strcat(sendBuff, GetTimeWithDifference(0));
	strcat(sendBuff, "\n");

	return sendBuff;

}
char* MeasureTimeLap()
{
	char sendBuff[255] = "";
	int diff;
	static clock_t begin;
	static bool _1st_call = true;

	if (_1st_call == false) //this represents the 2nd call to measureTimeLap
	{
		diff = (clock() - begin) / CLOCKS_PER_SEC;
		if (diff < 180)
		{
			_itoa_s(diff, sendBuff, 10); //returns the time in seconds elapsed since 1st call.
		}
		else strcat(sendBuff, "timeout");

		_1st_call = true;
	}
	else if (_1st_call) //here we start measurement ,1st call to func
	{
		_1st_call = false;
		begin = clock();
		strcat(sendBuff, "start");
	}


	return sendBuff;
}

int next_day(int hour)
{
	if (hour >= 24)
		return hour - 24;
	if (hour <= 0)
		return 24 + hour;
	return hour;
}

char* GetTimeWithDifference(int time_diff)
{
	time_t now = time(0);
	tm* ltm = gmtime(&now);
	// ltm is the struct containing the information of current time
	// gmtime stands for UTC time.
	char sendBuff[255] = "", temp[255] = "";
	_itoa_s(next_day(ltm->tm_hour + time_diff), sendBuff, 10);
	strcat(sendBuff, ":");
	_itoa_s(ltm->tm_min, temp, 10);
	sendBuff[strlen(sendBuff)] = '\0';
	strcat(sendBuff, temp);
	strcat(sendBuff, ":");
	sendBuff[strlen(sendBuff)] = '\0';
	_itoa_s(ltm->tm_sec, temp, 10);
	strcat(sendBuff, temp);
	sendBuff[strlen(sendBuff)] = '\0';

	return sendBuff;
}
