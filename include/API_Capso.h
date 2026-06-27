#pragma once

#ifdef CAPSOLLD_EXPORTS
#define CAPSO_API __declspec(dllexport)
#else
#define CAPSO_API __declspec(dllimport)
#endif

#pragma pack(1)

typedef struct _DEVICELIST
{
    BYTE	iCount;
    char    SN_DockingSystem[255][32];
} DEVICELIST;

#pragma pack()

extern "C" {

    /**
    *   operate flow,
    *      AVCapso_Initialize
    *      AVCapso_DeviceList  <= get device list and SN
    *      .
    *      AVCapso_IOLock      <= lock I/O to prevent concurrent user access.
    *      .
    *      AVCapso_Read / AVCapso_Write
    *      .
    *      AVCapso_IOUnlock    <= release lock
    *      .
    *      AVCapso_Terminate
    */

    CAPSO_API BOOL AVCapso_Initialize();
    CAPSO_API BOOL AVCapso_Terminate();
    CAPSO_API BOOL AVCapso_DeviceList(DEVICELIST* pDeviceList);

    /*
    *   Arguments:
    *       szDockingSystemSN   <= device SN from  AVCapso_DeviceList
    */
    CAPSO_API BOOL AVCapso_IOLock(char* szDockingSystemSN);

    /*
    *   Arguments:
    *       szDockingSystemSN   <= device SN from  AVCapso_DeviceList
    */
    CAPSO_API BOOL AVCapso_IOUnlock(char* szDockingSystemSN);

    /*
    *   Arguments:
    *       szDockingSystemSN       <= device SN from  AVCapso_DeviceList
    *   other arguments:            <= same as winapi WinUsb_ReadPipe
    *       lpBuffer                <= A pointer to the buffer that receives the data read from a file or device
    *       nNumberOfBytesToWrite   <= The maximum number of bytes to be read from the file or device
    *       lpNumberOfBytesWritten  <= A pointer to the variable that receives the number of bytes read
    *       lpOverlapped            <= An optional pointer to an OVERLAPPED structure, which is used for asynchronous operations
    */
	CAPSO_API BOOL AVCapso_Read(char* szDockingSystemSN,
        LPVOID lpBuffer,
        DWORD nNumberOfBytesToWrite,
        LPDWORD lpNumberOfBytesWritten,
        LPOVERLAPPED lpOverlapped);

    /*
    *   Arguments:
    *       szDockingSystemSN       <= device SN from  AVCapso_DeviceList
    *   other arguments:            <= same as winapi WinUsb_WriteFile
    *       lpBuffer                <= A pointer to the buffer that receives the data read from a file or device
    *       nNumberOfBytesToWrite   <= The maximum number of bytes to be read from the file or device
    *       lpNumberOfBytesWritten  <= A pointer to the variable that receives the number of bytes read
    *       lpOverlapped            <= An optional pointer to an OVERLAPPED structure, which is used for asynchronous operations
    */
    CAPSO_API BOOL AVCapso_Write(char* szDockingSystemSN,
        LPCVOID lpBuffer,
        DWORD nNumberOfBytesToWrite,
        LPDWORD lpNumberOfBytesWritten,
        LPOVERLAPPED lpOverlapped);
}
