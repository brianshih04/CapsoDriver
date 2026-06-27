#pragma once

#define	IOLOCK_EVENT_WAIT_INTERVAL	5000	// in milliseconds

enum class InstructionData {
	None = 0,
	Read = 1,
	Write = 2
};

#define CMD_LENGTH					32			// this was set (incorrectly?) to 64 - corrected on 5/2/2012 - however, we do not see the impact of this change
#define STATUS_LENGTH				32

#define DOCKING_SYSTEM_SERIAL_NUM_LENGTH	16			// in bytes

#define CMD_WRITE_DOCKING_SYSTEM_SN			0xaa
#define CMD_READ_DOCKING_SYSTEM_SN			0xab

//=============================================================================

typedef struct _DEVINFO
{
	char	szPortName[MAX_PATH];
	char    SN_DockingSystem[32];

	HANDLE	hLock;
	bool	blLocked;

	BYTE	bEndpointBulkIn;
	BYTE	bEndpointBulkOut;

} DEVINFO;

class CDeviceManager
{
public:
	CDeviceManager();
	~CDeviceManager();

	bool RefreshDeviceList();
	void GetDeviceList(DEVICELIST* pDeviceList);
	int GetTargetDevice(const char* szSN);

	//=============================================================================
	// IO related functions
	int LaunchInstruction(int iHandleDev, BYTE* pCmd, InstructionData iDirect, BYTE* pData, DWORD dwDataSize);
	int LaunchInstruction_Sector(int iHandleDev, BYTE bCmdType, DWORD dwPageStart, WORD wPageBytes,
		InstructionData iDirect, BYTE* pData, DWORD dwDataSize);

	BOOL IOLock(int iHandleDev);
	BOOL IOUnlock(int iHandleDev);

	BOOL Read_Sync(int iHandleDev,
		LPVOID lpBuffer,
		DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten,
		LPOVERLAPPED lpOverlapped);

	BOOL Write_Sync(int iHandleDev,
		LPCVOID lpBuffer,
		DWORD nNumberOfBytesToWrite,
		LPDWORD lpNumberOfBytesWritten,
		LPOVERLAPPED lpOverlapped);

private:
	int		iDevCount;
	DEVINFO	devs[255];

	bool FindEndpoints(int iHandleDev);
};

