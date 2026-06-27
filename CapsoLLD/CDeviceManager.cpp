#include "pch.h"

#pragma comment(lib, "winusb.lib")

using namespace std;

DEFINE_GUID(GUID_CapsoDockingSystem, 0xCC0EDA17, 0x920D, 0x4F8B, 0xA0, 0x11, 0x72, 0x40, 0x65, 0x8B, 0xF2, 0xF9);

const GUID* pGUID_CapsoDockingSystem = &GUID_CapsoDockingSystem;

const unsigned short	CDAS2_VID = 0x0638;
const unsigned short	CDAS2_PID = 0x0931;

//=============================================================================

CDeviceManager::CDeviceManager()
{
	iDevCount = 0;
	memset(&devs, 0, sizeof(devs));

	RefreshDeviceList();
}

CDeviceManager::~CDeviceManager()
{
	for (int i = 0; i < iDevCount; i++)
	{
		if (devs[i].hLock)
		{
			CloseHandle(devs[i].hLock);
			devs[i].hLock = NULL;
		}
	}
}

bool CDeviceManager::FindEndpoints(int iHandleDev)
{
	bool						blRet = true;
	HANDLE						hDeviceHandle = INVALID_HANDLE_VALUE;
	WINUSB_INTERFACE_HANDLE		hWinUsb = NULL;
	USB_INTERFACE_DESCRIPTOR	DescInterface;
	WINUSB_PIPE_INFORMATION		InfoPipe;


	try
	{
		hDeviceHandle = CreateFile(devs[iHandleDev].szPortName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (hDeviceHandle == INVALID_HANDLE_VALUE)
			throw false;

		if (!WinUsb_Initialize(hDeviceHandle, &hWinUsb))
			throw false;

		if (!WinUsb_QueryInterfaceSettings(hWinUsb, 0, &DescInterface))
			throw false;

		for (BYTE i = 0; i < DescInterface.bNumEndpoints; i++)
		{
			if (WinUsb_QueryPipe(hWinUsb, 0, i, &InfoPipe))
			{
				if (InfoPipe.PipeType == UsbdPipeTypeBulk)
				{
					if (USB_ENDPOINT_DIRECTION_IN(InfoPipe.PipeId))
					{
						devs[iHandleDev].bEndpointBulkIn = InfoPipe.PipeId;
					}
					else
					{
						devs[iHandleDev].bEndpointBulkOut = InfoPipe.PipeId;
					}
				}
			}
		}
	}
	catch (bool)
	{
		blRet = FALSE;
	}

	if (hWinUsb != NULL)
		WinUsb_Free(hWinUsb);

	if (hDeviceHandle != INVALID_HANDLE_VALUE)
		CloseHandle(hDeviceHandle);

	return blRet;
}

bool CDeviceManager::RefreshDeviceList()
{
	HDEVINFO							hDeviceInfo;
	SP_DEVICE_INTERFACE_DATA			deviceInterfaceData;
	BYTE								bTmpStringData[260];
	SP_DEVICE_INTERFACE_DETAIL_DATA* pDeviceDetailData;
	SP_DEVINFO_DATA						deviceInfoData;

	SECURITY_ATTRIBUTES	SecAttrib;
	SECURITY_DESCRIPTOR	mySecurityDescriptor;

	DWORD					i;
	WORD					wVID, wPID;
	char					devicePath[MAX_PATH], szTargetPortName[256] = { 0 }, szEventName[256] = { 0 };
	bool					blAddList = false;


	try
	{
		// clear previous device list
		for (int i = 0; i < iDevCount; i++)
		{
			if (devs[i].hLock)
			{
				CloseHandle(devs[i].hLock);
				devs[i].hLock = NULL;
			}
			memset(&devs[i], 0, sizeof(DEVINFO));
		}
		memset(&devs, 0, sizeof(devs));
		pDeviceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*)bTmpStringData;

		// Enum Scanner devices
		hDeviceInfo = SetupDiGetClassDevs(pGUID_CapsoDockingSystem, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
		if (INVALID_HANDLE_VALUE != hDeviceInfo)
		{
			i = 0;
			while (true)
			{
				// get device interface data
				deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
				if (!SetupDiEnumDeviceInterfaces(hDeviceInfo, 0, pGUID_CapsoDockingSystem, i, &deviceInterfaceData))
					break;
				// get device data
				deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
				if (!SetupDiEnumDeviceInfo(hDeviceInfo, i, &deviceInfoData))
					break;
				i++;

				//get device instance name
				pDeviceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
				if (SetupDiGetDeviceInterfaceDetail(hDeviceInfo, &deviceInterfaceData, pDeviceDetailData, 256, NULL, NULL))
				{
					if (_strnicmp("\\\\?\\usb#", pDeviceDetailData->DevicePath, 8) == 0)
					{
						strcpy_s(szTargetPortName, sizeof(szTargetPortName), pDeviceDetailData->DevicePath);
					}
					else
					{
						szTargetPortName[0] = '\0';
					}
				}
				else
				{
					szTargetPortName[0] = '\0';
				}

				// check VID + PID
				if (0 != szTargetPortName[0])
				{
					strcpy_s(devicePath, sizeof(devicePath), pDeviceDetailData->DevicePath);
					_strlwr_s(devicePath, strlen(devicePath) + 1);
					sscanf_s(strstr(devicePath, "vid_"), "vid_%04hx", &wVID);
					sscanf_s(strstr(devicePath, "pid_"), "pid_%04hx", &wPID);
					if (wVID == CDAS2_VID && wPID == CDAS2_PID)
					{
						blAddList = false;
						ZeroMemory(&mySecurityDescriptor, sizeof(SECURITY_DESCRIPTOR));
						if (InitializeSecurityDescriptor(&mySecurityDescriptor, SECURITY_DESCRIPTOR_REVISION))
						{
							if (SetSecurityDescriptorDacl(&mySecurityDescriptor, TRUE, NULL, FALSE))
							{
								// normalize event name by replacing '\' and '#' with '_'
								snprintf(szEventName, sizeof(szEventName), "AVCAPSO_%s", pDeviceDetailData->DevicePath);
								for (size_t j = 0; szEventName[j] != '\0'; j++)
								{
									if (szEventName[j] == '\\' || szEventName[j] == '#')
									{
										szEventName[j] = '_';
									}
								}

								SecAttrib.nLength = sizeof(SecAttrib);
								SecAttrib.lpSecurityDescriptor = &mySecurityDescriptor;
								SecAttrib.bInheritHandle = FALSE;
								devs[iDevCount].hLock = CreateEvent(&SecAttrib, FALSE, TRUE, szEventName);
								if (devs[iDevCount].hLock)
								{
									DWORD dwWaitResult;
									dwWaitResult = WaitForSingleObject(devs[iDevCount].hLock, IOLOCK_EVENT_WAIT_INTERVAL);
									if (dwWaitResult == WAIT_OBJECT_0)
									{
										strcpy_s(devs[iDevCount].szPortName, sizeof(devs[iDevCount].szPortName), szTargetPortName);
										if (FindEndpoints(iDevCount))
										{
											if (LaunchInstruction_Sector(iDevCount, CMD_READ_DOCKING_SYSTEM_SN, 0, 1, InstructionData::Read, (BYTE*)devs[iDevCount].SN_DockingSystem, DOCKING_SYSTEM_SERIAL_NUM_LENGTH) == 0x00)
												blAddList = true;
										}
										SetEvent(devs[iDevCount].hLock);
									}
								}
							}
						}
					}

					if (blAddList)
					{
						iDevCount++;
						if (iDevCount >= 255)
						{
							break;
						}
					}
					else
					{
						if (devs[iDevCount].hLock)
						{
							CloseHandle(devs[iDevCount].hLock);
							devs[iDevCount].hLock = NULL;
						}
						memset(&devs[iDevCount], 0, sizeof(DEVINFO));
					}
				}
			}

			SetupDiDestroyDeviceInfoList(hDeviceInfo);
		}
	}
	catch (...)
	{

	}

	return true;
}

void CDeviceManager::GetDeviceList(DEVICELIST* pDeviceList)
{
	if (pDeviceList)
	{
		memset(pDeviceList, 0, sizeof(DEVICELIST));
		pDeviceList->iCount = iDevCount;
		for (int i = 0; i < iDevCount; i++)
		{
			strncpy_s(pDeviceList->SN_DockingSystem[i], devs[i].SN_DockingSystem, sizeof(pDeviceList->SN_DockingSystem) - 1);
		}
	}
}

int CDeviceManager::GetTargetDevice(const char* szSN)
{
	int	iHandle = -1;


	if (szSN == NULL)
	{
		if (iDevCount > 0)
			iHandle = 0;
	}
	else
	{
		for (int i = 0; i < iDevCount; i++)
		{
			if (strcmp(szSN, devs[i].SN_DockingSystem) == 0)
			{
				iHandle = i;
				break;
			}
		}
	}
	
	return iHandle;
}

//=============================================================================
// IO related functions

void CommandConstruct(BYTE* pCmd, BYTE bCmdType)
{
	memset(pCmd, 0, CMD_LENGTH);
	// "USBC", 4 bytes
	pCmd[0] = 0x55;
	pCmd[1] = 0x53;
	pCmd[2] = 0x42;
	pCmd[3] = 0x43;
	// cmd
	pCmd[8] = bCmdType;
}

bool VerifyStatus(BYTE* pStatus, DWORD dwTag)
{
	bool	blRet = true;


	// "USBC", 4 bytes
	if (pStatus[0] != 0x55 && pStatus[1] != 0x53 && pStatus[2] != 0x42 && pStatus[3] != 0x43)
		blRet = false;

	if (*((DWORD*)(pStatus + 4)) != dwTag)
		blRet = false;

	return blRet;
}

int CDeviceManager::LaunchInstruction(int iHandleDev, BYTE* pCmd, InstructionData iDirect, BYTE* pData, DWORD dwDataSize)
{
	int						iError = 0;
	HANDLE					hDeviceHandle = INVALID_HANDLE_VALUE;
	WINUSB_INTERFACE_HANDLE	hWinUsb = NULL;
	DWORD					dwTag, dwTransferedSize = 0;
	BYTE					bStatus[STATUS_LENGTH] = { 0 };


	try
	{
		hDeviceHandle = CreateFile(devs[iHandleDev].szPortName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (hDeviceHandle == INVALID_HANDLE_VALUE)
			throw false;

		if (!WinUsb_Initialize(hDeviceHandle, &hWinUsb))
			throw false;

		srand(static_cast<unsigned int>(time(NULL)));
		dwTag = (DWORD)rand();
		*((DWORD*)(pCmd + 4)) = dwTag;

		if (!WinUsb_WritePipe(hWinUsb, devs[iHandleDev].bEndpointBulkOut, pCmd, CMD_LENGTH, &dwTransferedSize, NULL) || dwTransferedSize != CMD_LENGTH)	throw false;
		if (iDirect == InstructionData::Read)
		{
			if (!WinUsb_ReadPipe(hWinUsb, devs[iHandleDev].bEndpointBulkIn, pData, dwDataSize, &dwTransferedSize, NULL) || dwTransferedSize != dwDataSize)	throw false;
		}
		else if (iDirect == InstructionData::Write)
		{
			if (!WinUsb_WritePipe(hWinUsb, devs[iHandleDev].bEndpointBulkOut, pData, dwDataSize, &dwTransferedSize, NULL) || dwTransferedSize != dwDataSize)	throw false;
		}
		if (!WinUsb_ReadPipe(hWinUsb, devs[iHandleDev].bEndpointBulkIn, bStatus, STATUS_LENGTH, &dwTransferedSize, NULL) || dwTransferedSize != STATUS_LENGTH)	throw false;
		if (!VerifyStatus(bStatus, dwTag))	throw false;
	}
	catch (...)
	{
		iError = -1;
	}

	if (hWinUsb != NULL)
		WinUsb_Free(hWinUsb);

	if (hDeviceHandle != INVALID_HANDLE_VALUE)
		CloseHandle(hDeviceHandle);

	return iError;
}

int CDeviceManager::LaunchInstruction_Sector(int iHandleDev, BYTE bCmdType, DWORD dwPageStart, WORD wPageBytes,
	InstructionData iDirect, BYTE* pData, DWORD dwDataSize)
{
	BYTE	bCmd[CMD_LENGTH] = { 0 };


	CommandConstruct(bCmd, bCmdType);
	*((DWORD*)(bCmd + 12)) = dwPageStart;
	*((WORD*)(bCmd + 16)) = (WORD)wPageBytes;
	return LaunchInstruction(iHandleDev, bCmd, iDirect, pData, dwDataSize);
}

BOOL CDeviceManager::IOLock(int iHandleDev)
{
	BOOL	blRet = FALSE;
	DWORD	dwWaitResult;


	if (iHandleDev >= 0 && iHandleDev < iDevCount)
	{
		dwWaitResult = WaitForSingleObject(devs[iHandleDev].hLock, IOLOCK_EVENT_WAIT_INTERVAL);
		if (dwWaitResult == WAIT_OBJECT_0)
		{
			devs[iHandleDev].blLocked = true;
			blRet = TRUE;
		}
		else
		{
			blRet = FALSE;
		}
	}
	return blRet;
}

BOOL CDeviceManager::IOUnlock(int iHandleDev)
{
	if (iHandleDev >= 0 && iHandleDev < iDevCount)
	{
		if (devs[iHandleDev].blLocked)
		{
			devs[iHandleDev].blLocked = false;
			SetEvent(devs[iHandleDev].hLock);
		}
	}

	return TRUE;
}

BOOL CDeviceManager::Read_Sync(int iHandleDev,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToRead,
	LPDWORD lpNumberOfBytesRead,
	LPOVERLAPPED lpOverlapped)
{
	BOOL					blRet = TRUE;
	HANDLE					hDeviceHandle = INVALID_HANDLE_VALUE;
	WINUSB_INTERFACE_HANDLE	hWinUsb = NULL;


	try
	{
		hDeviceHandle = CreateFile(devs[iHandleDev].szPortName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (hDeviceHandle == INVALID_HANDLE_VALUE)
			throw FALSE;

		if (!WinUsb_Initialize(hDeviceHandle, &hWinUsb))
			throw FALSE;

		if (!WinUsb_ReadPipe(hWinUsb, devs[iHandleDev].bEndpointBulkIn, (BYTE*)lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped))
			throw FALSE;
	}
	catch (BOOL)
	{
		blRet = FALSE;
	}

	if (hWinUsb != NULL)
		WinUsb_Free(hWinUsb);

	if (hDeviceHandle != INVALID_HANDLE_VALUE)
		CloseHandle(hDeviceHandle);

	return blRet;
}

BOOL CDeviceManager::Write_Sync(int iHandleDev,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped)
{
	BOOL					blRet = TRUE;
	HANDLE					hDeviceHandle = INVALID_HANDLE_VALUE;
	WINUSB_INTERFACE_HANDLE	hWinUsb = NULL;


	try
	{
		hDeviceHandle = CreateFile(devs[iHandleDev].szPortName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		if (hDeviceHandle == INVALID_HANDLE_VALUE)
			throw FALSE;

		if (!WinUsb_Initialize(hDeviceHandle, &hWinUsb))
			throw FALSE;

		if (!WinUsb_WritePipe(hWinUsb, devs[iHandleDev].bEndpointBulkOut, (BYTE*)lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped))
			throw FALSE;
	}
	catch (BOOL)
	{
		blRet = FALSE;
	}

	if (hWinUsb != NULL)
		WinUsb_Free(hWinUsb);

	if (hDeviceHandle != INVALID_HANDLE_VALUE)
		CloseHandle(hDeviceHandle);

	return blRet;
}