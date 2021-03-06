// PTI2Staubli.cpp : main

#include "stdafx.h"
#include "TX60L.h"
#include <string>
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <stdio.h>  
#include "colorFinder.hpp"
#ifndef SERIALCLASS_H_INCLUDED
#define SERIALCLASS_H_INCLUDED

#define ARDUINO_WAIT_TIME 2000

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

#define Joint     0
#define Cartesian 1
#define max(a,b)	 (a>b?a:b)
#define min(a,b)	 (a<b?a:b)
#define range(x,a,b) (min(max(x,a),b))//a is min, b is max
#define Xmax (600)
#define Xmin (-600)
#define Ymax 600
#define Ymin (-600)
#define Zmax 100
#define Zmin -180
#define HZmax 240
#define HZmin -180
double lastB=-123000;

class Serial
{
private:
	//Serial comm handler
	HANDLE hSerial;
	//Connection status
	bool connected;
	//Get various information about the connection
	COMSTAT status;
	//Keep track of last error
	DWORD errors;

public:
	//Initialize Serial communication with the given COM port
	Serial(const char *portName);
	//Close the connection
	~Serial();
	//Read data in a buffer, if nbChar is greater than the
	//maximum number of bytes available, it will return only the
	//bytes available. The function return -1 when nothing could
	//be read, the number of bytes actually read.
	int ReadData(char *buffer, unsigned int nbChar);
	//Writes data from a buffer through the Serial connection
	//return true on success.
	bool WriteData(const char *buffer, unsigned int nbChar);
	//Check if we are actually connected
	bool IsConnected();


};

#endif // SERIALCLASS_H_INCLUDED

Serial::Serial(const char *portName)
{
	//We're not yet connected
	this->connected = false;

	//Try to connect to the given port throuh CreateFile
	WCHAR wszClassName[1024];
	memset(wszClassName, 0, sizeof(wszClassName));
	MultiByteToWideChar(CP_ACP, 0, portName, strlen(portName) + 1, wszClassName, sizeof(wszClassName) / sizeof(wszClassName[0]));
	this->hSerial = CreateFile(wszClassName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	//Check if the connection was successfull
	if (this->hSerial == INVALID_HANDLE_VALUE)
	{
		//If not success full display an Error
		if (GetLastError() == ERROR_FILE_NOT_FOUND){

			//Print Error if neccessary
			printf("ERROR: Handle was not attached. Reason: %s not available.\n", portName);

		}
		else
		{
			printf("ERROR!!!");
		}
	}
	else
	{
		//If connected we try to set the comm parameters
		DCB dcbSerialParams = { 0 };

		//Try to get the current
		if (!GetCommState(this->hSerial, &dcbSerialParams))
		{
			//If impossible, show an error
			printf("failed to get current serial parameters!");
		}
		else
		{
			//Define serial connection parameters for the arduino board
			dcbSerialParams.BaudRate = CBR_9600;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			//Setting the DTR to Control_Enable ensures that the Arduino is properly
			//reset upon establishing a connection
			dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

			//Set the parameters and check for their proper application
			if (!SetCommState(hSerial, &dcbSerialParams))
			{
				printf("ALERT: Could not set Serial Port parameters");
			}
			else
			{
				//If everything went fine we're connected
				this->connected = true;
				//Flush any remaining characters in the buffers
				PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
				//We wait 2s as the arduino board will be reseting
				Sleep(ARDUINO_WAIT_TIME);
			}
		}
	}

}

Serial::~Serial()
{
	//Check if we are connected before trying to disconnect
	if (this->connected)
	{
		//We're no longer connected
		this->connected = false;
		//Close the serial handler
		CloseHandle(this->hSerial);
	}
}

int Serial::ReadData(char *buffer, unsigned int nbChar)
{
	//Number of bytes we'll have read
	DWORD bytesRead;
	//Number of bytes we'll really ask to read
	unsigned int toRead;

	//Use the ClearCommError function to get status info on the Serial port
	ClearCommError(this->hSerial, &this->errors, &this->status);

	//Check if there is something to read
	if (this->status.cbInQue>0)
	{
		//If there is we check if there is enough data to read the required number
		//of characters, if not we'll read only the available characters to prevent
		//locking of the application.
		if (this->status.cbInQue>nbChar)
		{
			toRead = nbChar;
		}
		else
		{
			toRead = this->status.cbInQue;
		}

		//Try to read the require number of chars, and return the number of read bytes on success
		if (ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL))
		{
			return bytesRead;
		}

	}

	//If nothing has been read, or that an error was detected return 0
	return 0;

}


bool Serial::WriteData(const char *buffer, unsigned int nbChar)
{
	DWORD bytesSend;

	//Try to write the buffer on the Serial port
	if (!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0))
	{
		//In case it don't work get comm error and return false
		ClearCommError(this->hSerial, &this->errors, &this->status);

		return false;
	}
	else
		return true;
}

bool Serial::IsConnected()
{
	//Simply return the connection status
	return this->connected;
}

Serial* SP;


double unify(double x)
{
	
	double P = asin(1)*2;
//    cout <<"x-"<< x << endl;
//	cout <<"p-"<< P << endl;
	while (x > P-0.001)
	{
		x -= P;
	}
	return abs(x);
}

bool IsArmReach(TX60L* TX60LObj, double epsilon, std::vector<double> target, bool type)
{
	double err = 0;
	if (type)//CartesianPosition type
	{
		std::vector<double> pos;
		TX60LObj->GetRobotCartesianPosition(pos);
		for (int i = 0; i < 3; i++)
			err += fabs(pos[i] - target[i]) * 1000;//For advance use, user can define err by himself
		if (!(abs(pos[3])<epsilon) )

		for (int i = 3; i < 6; i++)
		{
			
			err += unify(fabs(pos[i] - target[i])) * 1000;//For advance use, user can define err by himself
		}
	}
	else
	{
		std::vector<double> joint;
		TX60LObj->GetRobotJoints(joint);
		for (int i = 0; i < 6; i++)
			err += fabs(joint[i] - target[i]) * 1000;//For advance use, user can define err by himself
	}
	//cout << "err:" << err << endl;
	if (err < epsilon)
		return true;
	else
		return false;
}
/*
This function make sure the arm operating in safe area
return value stands for whether arm have reached the target position with target error
*/
bool Move2XYZ(TX60L* TX60LObj, double * XYZ, double epsilon)
{
	std::vector<double> targetPos;
	targetPos.clear();
	targetPos.push_back(range(-XYZ[0], Xmin, Xmax) / 1000);
	targetPos.push_back(range(-XYZ[1], Ymin, Ymax) / 1000); 
	targetPos.push_back(range(XYZ[2], Zmin, Zmax) / 1000);
	targetPos.push_back(DEGREE_2_RADIAN(0));
	targetPos.push_back(DEGREE_2_RADIAN(-90));
	targetPos.push_back(DEGREE_2_RADIAN(180));
	if (!TX60LObj->MoveCartesian(targetPos)) return false;
	return IsArmReach(TX60LObj, epsilon, targetPos, Cartesian);
}

bool Move2XYZr(TX60L* TX60LObj, double * XYZ, double epsilon, double rx, double ry, double rz)
{
	std::vector<double> targetPos;
	targetPos.clear();
	targetPos.push_back(range(-XYZ[0], Xmin, Xmax) / 1000);
	targetPos.push_back(range(-XYZ[1], Ymin, Ymax) / 1000);
	targetPos.push_back(range(XYZ[2], Zmin, Zmax) / 1000);
	double a, b, c;
	a = -90; c = -90;
	b = atan2(-XYZ[0], -XYZ[1]);
	targetPos.push_back(DEGREE_2_RADIAN(a));
	targetPos.push_back(b);
	targetPos.push_back(DEGREE_2_RADIAN(c));
	bool first = TX60LObj->MoveCartesian(targetPos);
	bool second = IsArmReach(TX60LObj, epsilon, targetPos, Cartesian);
//	cout << "first " << first << " second " << second << endl;
	//cout << -XYZ[0] << " " << -XYZ[1] << " " << XYZ[2] << endl;
	if (!first) return false;
	return second;
}

bool Move2XYZD(TX60L* TX60LObj, double * XYZ, double epsilon, double rx, double ry, double rz)
{
	std::vector<double> targetPos;
	targetPos.clear();
	targetPos.push_back(range(-XYZ[0], Xmin, Xmax) / 1000);
	targetPos.push_back(range(-XYZ[1], Ymin, Ymax) / 1000);
	targetPos.push_back(range(XYZ[2], Zmin, 5*Zmax) / 1000);
	double a, b, c;
	a = -90; c = -90;
	b = atan2(-XYZ[0], -XYZ[1]);
	targetPos.push_back(DEGREE_2_RADIAN(a));
	targetPos.push_back(b);
	targetPos.push_back(DEGREE_2_RADIAN(c));
	bool first = TX60LObj->MoveCartesian(targetPos);
	bool second = IsArmReach(TX60LObj, epsilon, targetPos, Cartesian);
	//	cout << "first " << first << " second " << second << endl;
	//cout << -XYZ[0] << " " << -XYZ[1] << " " << XYZ[2] << endl;
	if (!first) return false;
	return second;
}

bool Move2XYZRR(TX60L* TX60LObj, double * XYZ, double epsilon, double rx, double ry, double rz)
{
	std::vector<double> targetPos;
	targetPos.clear();
	targetPos.push_back(range(-XYZ[0], Xmin, Xmax) / 1000);
	targetPos.push_back(range(-XYZ[1], Ymin, Ymax) / 1000);
	targetPos.push_back(range(XYZ[2], Zmin, 5 * Zmax) / 1000);
	double a, b, c;
	a = -90; c = 90;
	b = atan2(-XYZ[0], -XYZ[1]);
	targetPos.push_back(DEGREE_2_RADIAN(a));
	targetPos.push_back(b);
	targetPos.push_back(DEGREE_2_RADIAN(c));
	bool first = TX60LObj->MoveCartesian(targetPos);
	bool second = IsArmReach(TX60LObj, epsilon, targetPos, Cartesian);
	//	cout << "first " << first << " second " << second << endl;
	//cout << -XYZ[0] << " " << -XYZ[1] << " " << XYZ[2] << endl;
	if (!first) return false;
	return second;
}


bool Move2XYZc(TX60L* TX60LObj, double * XYZ, double epsilon, double rx, double ry, double rz)
{
	std::vector<double> targetPos;
	targetPos.clear();
	targetPos.push_back(range(-XYZ[0], Xmin, Xmax) / 1000);
	targetPos.push_back(range(-XYZ[1], Ymin, Ymax) / 1000);
	targetPos.push_back(range(XYZ[2], Zmin-70, Zmax) / 1000);
	double a, b, c;
	a = -90; c = 90;
	b = atan2(-XYZ[0], -XYZ[1]);
	//cout << "rx " << DEGREE_2_RADIAN(a) << "ry " << b << "rz " << DEGREE_2_RADIAN(c) << endl;
	targetPos.push_back(DEGREE_2_RADIAN(a));
	targetPos.push_back(b);
	targetPos.push_back(DEGREE_2_RADIAN(c));

	if (!TX60LObj->MoveCartesian(targetPos)) return false;
	return IsArmReach(TX60LObj, epsilon, targetPos, Cartesian);
}

bool Move2XYZdefine(TX60L* TX60LObj, double * XYZ, double epsilon, double rx, double ry, double rz)
{
	std::vector<double> targetPos;
	targetPos.clear();
	targetPos.push_back(range(-XYZ[0], Xmin, Xmax) / 1000);
	targetPos.push_back(range(-XYZ[1], Ymin, Ymax) / 1000);
	targetPos.push_back(range(XYZ[2], Zmin, Zmax) / 1000);
	double a, b, c;
	a = -90; c = -90;
	b = atan2(-XYZ[0], -XYZ[1]);
	//cout << "rx " << DEGREE_2_RADIAN(a) << "ry " << b << "rz " << DEGREE_2_RADIAN(c) << endl;
	targetPos.push_back(DEGREE_2_RADIAN(0));
	targetPos.push_back(DEGREE_2_RADIAN(-90));
	targetPos.push_back(DEGREE_2_RADIAN(180));

	if (!TX60LObj->MoveCartesian(targetPos)) return false;
	return IsArmReach(TX60LObj, epsilon, targetPos, Cartesian);
}

void Move2ZeroJoint(TX60L* TX60LObj)
{
	std::vector<double> ZeroJoint;
	//Reset ARM to ZeroJoint Pose
	ZeroJoint.clear();
	ZeroJoint.push_back(0.0);
	ZeroJoint.push_back(0.0);
	ZeroJoint.push_back(0.0);
	ZeroJoint.push_back(0.0);
	ZeroJoint.push_back(0.0);
	ZeroJoint.push_back(0.0);
	TX60LObj->MoveJoints(ZeroJoint);
	while (!(IsArmReach(TX60LObj, 0.1, ZeroJoint, Joint)));
	cout << "Zero Joint Pose reached!" << endl;
}
void ShowLimit(TX60L* TX60LObj )
{
	std::vector<double> pos;
	double xyz[3];
	xyz[0] = Xmax;
	xyz[1] = Ymax;
	xyz[2] = Zmax;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmax Ymax Zmax" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");

	xyz[0] = Xmin;
	xyz[1] = Ymax;
	xyz[2] = Zmax;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmin Ymax Zmax" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");

	xyz[0] = Xmin;
	xyz[1] = Ymin;
	xyz[2] = Zmax;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmin Ymin Zmax" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");

	xyz[0] = Xmax;
	xyz[1] = Ymin;
	xyz[2] = Zmax;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmax Ymin Zmax" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");

	xyz[0] = Xmax;
	xyz[1] = Ymax;
	xyz[2] = Zmin;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmax Ymax Zmin" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");

	xyz[0] = Xmin;
	xyz[1] = Ymax;
	xyz[2] = Zmin;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmin Ymax Zmin" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");

	xyz[0] = Xmin;
	xyz[1] = Ymin;
	xyz[2] = Zmin;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmin Ymin Zmin" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");

	xyz[0] = Xmax;
	xyz[1] = Ymin;
	xyz[2] = Zmin;
	while (!Move2XYZ(TX60LObj, xyz, 0.1));
	cout << "Here is Xmax Ymin Zmin" << endl;
	TX60LObj->GetRobotCartesianPosition(pos);
	cout << "x:" << pos[0] * 1000 << endl;
	cout << "y:" << pos[1] * 1000 << endl;
	cout << "z:" << pos[2] * 1000 << endl;
	system("pause");
}

bool Release_Q()
{
	bool readResult = SP->WriteData("AAAAAAAAAA", 10);
	Sleep(60000);
	// printf("Bytes read: (0 means no data available) %i\n",readResult);
	return readResult;
}

bool Catch_Q()
{
	//SP->flush
	bool readResult = SP->WriteData("BBBBBBBBBB", 10);
	Sleep(7500);
	// printf("Bytes read: (0 means no data available) %i\n",readResult);
	return readResult;
}

int _tmain(int argc, _TCHAR* argv[])
{
	
	SP = new Serial("COM4");    // adjust as needed

	if (SP->IsConnected())
		cout << "We're connected" << endl;
	else
	{
		cout << "Serial ERROR" << endl;
	}
	//Release_Q();
	//while (1) Catch_Q();
	TX60L* g_pRobot = g_pRobot = new TX60L;
	if (!g_pRobot->IsLoggedIn())
	{
		//int ret = g_pRobot->Login("http://192.168.31.55:5653/", "default", "");
		//Robot 2
		int ret = g_pRobot->Login("http://192.168.1.100:5653/", "default", "");
		//Robot 1
		//int ret = g_pRobot->Login("http://localhost:851/", "default", "");
		//Simulator
		if (ret <= 0)
		{
			getchar();
			return -1;
		}
		Sleep(1000);
		g_pRobot->Power(true);
		//Power on the TX60
		g_pRobot->ResetMotion();
	}
	double xyz[3];//xyz
	std::vector<double> pos, joint, targetPos, targetJoint;
	/////secure /////secure /////secure /////secure /////secure 
	joint.resize(6);

	while (1
		)
	{
		xyz[0] = 0;
		xyz[1] = -400;
		xyz[2] = 500;
		while (!Move2XYZRR(g_pRobot, xyz, 0.001, 0, -90, 0));
	//	system("pause");
//		while (!Move2XYZc(g_pRobot, xyz, 0.001, 0, -90, 0));
		system("pause");
		//while (!Move2XYZ(g_pRobot, xyz, 0.001));
	}
	
	joint[0] = 0.0702332;
	joint[1] = -0.296897;
	joint[2] = -2.34575;
	joint[3] = -3.04995;
	joint[4] = -0.873924;
	joint[5] = 3.08268;

	joint[4] -= 0.9;
	//g_pRobot->MoveJoints(joint);
	//while (!(IsArmReach(g_pRobot, 0.1, joint, Joint)));
	system("pause");
	ColorFinder* finderTB = new ColorFinder();
	for (double jd = 2.5; jd < 0.0702332 + 3.1415 / 6 * 5.7; jd += 0.5)
	{
		joint[0] = jd;
		g_pRobot->MoveJoints(joint);
		while (!(IsArmReach(g_pRobot, 0.1, joint, Joint)));
		finderTB->init_image_process(1);
		break;
	}
	ColorFinder* finderTR = new ColorFinder();
	for (double jd = 2.5; jd < 0.0702332 + 3.1415 / 6 * 5.7; jd += 0.5)
	{
		joint[0] = jd;
		g_pRobot->MoveJoints(joint);
		while (!(IsArmReach(g_pRobot, 0.1, joint, Joint)));
		finderTR->init_image_process(1);
		int cmd;
		//cin >> cmd;
		//if (cmd == 2017) 
			break;
	}
	ColorFinder* finderTG = new ColorFinder();
	for (double jd = 2.5; jd < 0.0702332 + 3.1415 / 6 * 5.7; jd += 0.5)
	{
		joint[0] = jd;
		g_pRobot->MoveJoints(joint);
		while (!(IsArmReach(g_pRobot, 0.1, joint, Joint)));
		finderTG->init_image_process(1);
		//int cmd;
	//	cin >> cmd;
	//	if (cmd == 2017)
			break;
	}
	ColorFinder* finder = new ColorFinder();
	finder->mul = 3;
	finder->init_image_process(1);
	joint[0] = 0.0702332+1.8;
	joint[1] = -0.296897;
	joint[2] = -2.34575;
	joint[3] = -3.04995;
	joint[4] = -0.873924;
	joint[5] = 3.08268;
	joint[4] -= 0.9;
	g_pRobot->MoveJoints(joint);
	while (!(IsArmReach(g_pRobot, 0.1, joint, Joint)));
	while (1)
	{
		system("pause");
		cout << "Confirming" << endl;
		for (double jd = 1.8; jd < 0.0702332 + 3.1415 / 6 * 5.8; jd += 0.15)
		{
			finderTB->color_area = 0;
			finderTR->color_area = 0;
			finderTG->color_area = 0;
			joint[0] = jd;
			g_pRobot->MoveJoints(joint);
			while (!(IsArmReach(g_pRobot, 0.1, joint, Joint)));
			cout << "checking Blue" << endl;
			finderTR->process_once(40);
			cout << "checking Yellow" << endl;
			finderTB->process_once(40);
			cout << "checking Red" << endl;
			finderTG->process_once(40);
			//system("pause");
			int tj = (finderTG->color_area > 1500) + (finderTR->color_area > 1500) + (finderTB->color_area > 1500);
			if ((tj >= 3) && (finderTB->color_area + finderTG->color_area + finderTR->color_area > 7000) && (finderTB->xc < finderTR->xc&&finderTR->xc < finderTG->xc)) break;

		}
		cout << "Confirmed" << endl;
		/////secure /////secure /////secure /////secure /////secure 
		g_pRobot->GetRobotCartesianPosition(pos);
		cout << pos[0] << " " << pos[1] << " " << pos[2] << " " << pos[3] << " " << pos[4] << " " << pos[5] << " " << endl;
		//Reset ARM to ZeroJoint Pose
		//Move2ZeroJoint(g_pRobot);
		//system("pause");
		//ShowLimit(g_pRobot);
		//Move ARM to initial position

		xyz[0] = -pos[0] * 1000 * 1.6;
		xyz[1] = -pos[1] * 1000 * 1.6;
		xyz[2] = pos[2] * 1000 - 20;

		/*xyz[0] = 355;
		xyz[1] = 0;
		xyz[2] = 40;*/
		//xyz[0] = -( Xmax + Xmin ) / 2;
		//xyz[1] = -( Ymax + Ymin ) / 2;
		//xyz[2] = Zmax;
		cout << "Searching Camera!" << endl;
		//	ColorFinder* finder = new ColorFinder();
		double theta = 1.57, z = 200;
		cout << "Setting Recoginition!" << endl;
		while (!Move2XYZr(g_pRobot, xyz, 0.001, 0, -90, 0));
		g_pRobot->GetRobotJoints(joint);
		for (int uuu = 0; uuu <= 5; uuu++)cout << "joint[" << uuu << "]=" << joint[uuu] << endl;
		//finder->init_image_process(1);
		cout << "Now the Arm is ready!" << endl;

		g_pRobot->GetRobotCartesianPosition(pos);
		//while (1);
		//system("pause");
		const char* wndname = "Square Detection Demo"; //窗口名称
		destroyWindow("Original");
		double ccxx, ccyy;
		while (1)
		{
			if (finder->process_once(xyz[2]))
			{
				imshow("After process", finder->Small_image);
				/*add your code here*/
				/*add your code here*/
				g_pRobot->GetRobotCartesianPosition(pos);
				printf("The 3D coordinate of the box is \nxc:%f\nyc:%f\nzc:%f\r\n", finder->xc, finder->yc, finder->zc);
				//if (factor < 0.6)  factor = 0.6;
				if (pos[1] * pos[1] + pos[0] * pos[0] > 80000)
				{
					cout << "dangerous area: " << pos[0] << " " << pos[1] << endl;
				}
				if (xyz[1] * xyz[1] + xyz[0] * xyz[0] > 80000)
				{
					cout << "current XYZ" << " " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
					theta = atan2(xyz[1], xyz[0]);
					cout << "theta" << theta << endl;
					ccxx = (-cos(theta)*finder->yc + sin(theta)*finder->xc);
					ccyy = -(sin(theta)*finder->yc + cos(theta)*finder->xc);
					if (xyz[2] <= Zmin + 1)
					{
						if (abs(finder->yc) < 0.01&&abs(finder->xc) < 0.01)
							break;
					}
					cout << "ccxx " << ccxx << endl;
					cout << "ccyy " << ccyy << endl;
					if (xyz[2] > -100)
					{
						xyz[0] += ccxx * (200);//x
						xyz[1] += ccyy * (200);//y		
					}
					else
					{
						xyz[0] += ccxx * (10);//x
						xyz[1] += ccyy * (10);//y		
					}
				}
				else
				{
					if (xyz[0] > 0) xyz[0] += 100; else  xyz[0] -= 100;
					if (xyz[1] > 0) xyz[1] += 100; else  xyz[1] -= 100;
					xyz[2] += 75;
				}
				if (abs(finder->yc) < 0.15&&abs(finder->xc) < 0.15)
				{
					if (xyz[2] < -150)
						xyz[2] -= 25;
					else
						xyz[2] -= 75;
				}
				if (xyz[2] < Zmin) xyz[2] = Zmin;
				if (xyz[2] > Zmax) xyz[2] = Zmax;
				if (xyz[1] < Ymin) xyz[1] = Ymin;
				if (xyz[1] > Ymax) xyz[1] = Ymax;
				if (xyz[0] < Xmin) xyz[0] = Xmin;
				if (xyz[0] > Xmax) xyz[0] = Xmax;
				cout << "target XYZ" << " " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
				while (!Move2XYZr(g_pRobot, xyz, 0.001, 0, -90, 0));
				waitKey(100);

			}
			else
			{
				cout << "===========lost the target==============" << endl;
				imshow("After process", finder->cv_image);
				waitKey(100);
				if (xyz[1] * xyz[1] + xyz[0] * xyz[0] < 80000)
				{
					if (xyz[0] > 0) xyz[0] += 100; else  xyz[0] -= 100;
					if (xyz[1] > 0) xyz[1] += 100; else  xyz[1] -= 100;
					xyz[2] += 75;
				}
				xyz[2] += 200;
				if (xyz[2] < Zmin) xyz[2] = Zmin;
				if (xyz[2] > Zmax) xyz[2] = Zmax;
				if (xyz[1] < Ymin) xyz[1] = Ymin;
				if (xyz[1] > Ymax) xyz[1] = Ymax;
				if (xyz[0] < Xmin) xyz[0] = Xmin;
				if (xyz[0] > Xmax) xyz[0] = Xmax;
				while (!Move2XYZr(g_pRobot, xyz, 0.001, 0, -90, 0));
			}
		}
		double rx, ry;
		rx = xyz[0];
		ry = xyz[1];
		cout << "box x " << rx << "box y " << ry << endl;
		//system("pause");
		while (!Move2XYZc(g_pRobot, xyz, 0.001, 0, -90, 0));
		xyz[2] -= 65;
		while (!Move2XYZc(g_pRobot, xyz, 0.001, 0, -90, 0));
		Catch_Q();
		xyz[2] += 65;
		while (!Move2XYZc(g_pRobot, xyz, 0.001, 0, -90, 0));
		xyz[0] = xyz[0] * 0.6; xyz[1] = xyz[1] * 0.6;
		while (!Move2XYZc(g_pRobot, xyz, 0.001, 0, -90, 0));
		double tx = xyz[0], ty=xyz[1];
		xyz[0] = tx * cos(-75.0 / 180.0 * 3.1415926) - ty * sin(-75.0 / 180.0 * 3.1415926);
		xyz[1] = tx * sin(-75.0 / 180.0 * 3.1415926) + ty * cos(-75.0 / 180.0 * 3.1415926);
		//xyz[0] = 300; xyz[1] = 0;
		while (!Move2XYZc(g_pRobot, xyz, 0.001, 0, -90, 0));
		xyz[2] = 400;
		while (!Move2XYZD(g_pRobot, xyz, 0.001, 0, -90, 0));
		xyz[0] = rx*0.8; xyz[1] = ry*0.8;
		while (!Move2XYZD(g_pRobot, xyz, 0.001, 0, -90, 0));
		//while (1);
		//while (!Move2XYZD(g_pRobot, xyz, 0.001, 0, -90, 0));
		g_pRobot->GetRobotJoints(joint);
		double std_pos = joint[0];
		finder->process_once(xyz[2] - 400); finder->process_once(xyz[2] - 400);
		finder->process_once(xyz[2] - 400);
		for (double jd = std_pos + 0.7; jd < std_pos - 0.6; jd += 0.1)
		{
			joint[0] = jd;
			g_pRobot->MoveJoints(joint);
			while (!(IsArmReach(g_pRobot, 0.1, joint, Joint)));
			if (finder->process_once(xyz[2] - 400))break;
		}
		g_pRobot->GetRobotCartesianPosition(pos);
		xyz[0] = -pos[0] * 1000;
		xyz[1] = -pos[1] * 1000;
		xyz[2] = pos[2] * 1000;
		while (1)
		{
			if (finder->process_once(xyz[2] - 500))
			{
				imshow("After process", finder->Small_image);
				/*add your code here*/
				/*add your code here*/
				g_pRobot->GetRobotCartesianPosition(pos);
				printf("The 3D coordinate of the box is \nxc:%f\nyc:%f\nzc:%f\r\n", finder->xc, finder->yc, finder->zc);
				//if (factor < 0.6)  factor = 0.6;
				if (pos[1] * pos[1] + pos[0] * pos[0] > 80000)
				{
					cout << "dangerous area: " << pos[0] << " " << pos[1] << endl;
				}
				if (xyz[1] * xyz[1] + xyz[0] * xyz[0] > 80000)
				{
					cout << "current XYZ" << " " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
					theta = atan2(xyz[1], xyz[0]);
					cout << "theta" << theta << endl;
					ccxx = (-cos(theta)*finder->yc + sin(theta)*finder->xc);
					ccyy = -(sin(theta)*finder->yc + cos(theta)*finder->xc);
					if (xyz[2] <= 410 + 1)
					{
						if (abs(finder->yc) < 0.01&&abs(finder->xc) < 0.01)
							break;
					}
					cout << "ccxx " << ccxx << endl;
					cout << "ccyy " << ccyy << endl;
					if (xyz[2] > 380)
					{
						xyz[0] += ccxx * (20);//x
						xyz[1] += ccyy * (20);//y		
					}
					else
					{
						xyz[0] += ccxx * (10);//x
						xyz[1] += ccyy * (10);//y		
					}
				}
				else
				{
					if (xyz[0] > 0) xyz[0] += 100; else  xyz[0] -= 100;
					if (xyz[1] > 0) xyz[1] += 100; else  xyz[1] -= 100;
					xyz[2] += 75;
				}
				if (abs(finder->yc) < 0.5&&abs(finder->xc) < 0.5)
				{
					xyz[2] -= 25;
				}
				if (xyz[2] < 410) xyz[2] = 410;
				if (xyz[2] > 420) xyz[2] = 420;
				cout << "target XYZ" << " " << xyz[0] << " " << xyz[1] << " " << xyz[2] << endl;
				while (!Move2XYZD(g_pRobot, xyz, 0.001, 0, -90, 0));
				waitKey(100);

			}
			else
			{
				cout << "===========lost the target==============" << endl;
				imshow("After process", finder->cv_image);
				waitKey(100);
				if (xyz[1] * xyz[1] + xyz[0] * xyz[0] < 80000)
				{
					if (xyz[0] > 0) xyz[0] += 100; else  xyz[0] -= 100;
					if (xyz[1] > 0) xyz[1] += 100; else  xyz[1] -= 100;
					xyz[2] += 25;
				}
				xyz[2] += 25;
				if (xyz[2] < 410) xyz[2] = 410;
				if (xyz[2] > 420) xyz[2] = 420;
				while (!Move2XYZD(g_pRobot, xyz, 0.001, 0, -90, 0));
			}
		}
		xyz[2] += 20;
		while (!Move2XYZD(g_pRobot, xyz, 0.001, 0, -90, 0));
		while (!Move2XYZRR(g_pRobot, xyz, 0.001, 0, -90, 0));
		Sleep(6000);
		Release_Q();
		system("pause");
	}
	delete finder;
	return 0;
}


