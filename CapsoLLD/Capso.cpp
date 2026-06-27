#include "pch.h"

using namespace std;

CDeviceManager	myDeviceManager;

//=============================================================================

extern "C"
{

CAPSO_API BOOL AVCapso_Initialize()
{
	return TRUE;
}

CAPSO_API BOOL AVCapso_Terminate()
{
	return TRUE;
}

CAPSO_API BOOL AVCapso_DeviceList(DEVICELIST* pDeviceList)
{
	if (pDeviceList)
	{
		memset(pDeviceList, 0, sizeof(DEVICELIST));
		myDeviceManager.RefreshDeviceList();
		myDeviceManager.GetDeviceList(pDeviceList);
	}

	return TRUE;
}

CAPSO_API BOOL AVCapso_IOLock(char* szDockingSystemSN)
{
	int iHandle = myDeviceManager.GetTargetDevice(szDockingSystemSN);

	if (iHandle < 0)
		return FALSE;
    return myDeviceManager.IOLock(iHandle);
}

CAPSO_API BOOL AVCapso_IOUnlock(char* szDockingSystemSN)
{
	int iHandle = myDeviceManager.GetTargetDevice(szDockingSystemSN);

	if (iHandle < 0)
		return FALSE;
    return myDeviceManager.IOUnlock(iHandle);
}

CAPSO_API BOOL AVCapso_Read(char* szDockingSystemSN,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	int iHandleDev = myDeviceManager.GetTargetDevice(szDockingSystemSN);

	if (iHandleDev < 0)
		return FALSE;
	return myDeviceManager.Read_Sync(iHandleDev, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

CAPSO_API BOOL AVCapso_Write(char* szDockingSystemSN,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	int iHandleDev = myDeviceManager.GetTargetDevice(szDockingSystemSN);

	if (iHandleDev < 0)
		return FALSE;
	return myDeviceManager.Write_Sync(iHandleDev, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

}