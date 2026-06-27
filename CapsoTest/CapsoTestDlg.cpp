
// CapsoTestDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "CapsoTest.h"
#include "CapsoTestDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//=============================================================================

typedef BOOL (*FuncAVCapso_Initialize)();
typedef BOOL (*FuncAVCapso_Terminate)();
typedef BOOL (*FuncAVCapso_DeviceList)(DEVICELIST* pDeviceList);

typedef BOOL (*FuncAVCapso_IOLock)(char* szDockingSystemSN);
typedef BOOL (*FuncAVCapso_IOUnlock)(char* szDockingSystemSN);
typedef BOOL (*FuncAVCapso_Read)(char* szDockingSystemSN,
	LPVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped);
typedef BOOL(*FuncAVCapso_Write)(char* szDockingSystemSN,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped);

FuncAVCapso_Initialize	AVCapso_Initialize = NULL;
FuncAVCapso_Terminate	AVCapso_Terminate = NULL;
FuncAVCapso_DeviceList	AVCapso_DeviceList = NULL;
FuncAVCapso_IOLock		AVCapso_IOLock = NULL;
FuncAVCapso_IOUnlock	AVCapso_IOUnlock = NULL;
FuncAVCapso_Read		AVCapso_Read = NULL;
FuncAVCapso_Write		AVCapso_Write = NULL;

//=============================================================================

static DWORD ReadLe32(const BYTE* pData)
{
	return ((DWORD)pData[0]) |
		((DWORD)pData[1] << 8) |
		((DWORD)pData[2] << 16) |
		((DWORD)pData[3] << 24);
}

static CString GetCapsoErrorName(BYTE bErrorCode)
{
	switch (bErrorCode)
	{
	case 0x00: return "No error";
	case 0x10: return "ERROR_Command";
	case 0x11: return "ERROR_Unknown_Command";
	case 0x20: return "ERROR_ReadPage";
	case 0x21: return "ERROR_Data";
	case 0x22: return "ERROR_Decode";
	case 0x23: return "ERROR_Failed_CRC";
	case 0x24: return "ERROR_FIFO_Overflow";
	case 0x30: return "ERROR_WritePage";
	case 0x31: return "ERROR_EraseBlock";
	case 0x40: return "ERROR_StrongSignal";
	case 0x41: return "ERROR_WeakSignal";
	case 0x42: return "ERROR_NoCapsule";
	case 0x50: return "ERROR_OverCurrent";
	case 0x51: return "ERROR_CoverOpen";
	case 0x70: return "ERROR_UpdateFx2";
	case 0x71: return "ERROR_UpdateFPGA";
	default: return "Unknown error";
	}
}

static BOOL VerifyCapsoStatus(const BYTE* pStatus, DWORD dwExpectedTag, CString& strError)
{
	if (pStatus[0] != 'U' || pStatus[1] != 'S' || pStatus[2] != 'B' || pStatus[3] != 'S')
	{
		strError = "Invalid status signature.";
		return FALSE;
	}

	DWORD dwStatusTag = ReadLe32(pStatus + 4);
	if (dwStatusTag != dwExpectedTag)
	{
		strError.Format("Invalid status tag. Expected 0x%08X, got 0x%08X.", dwExpectedTag, dwStatusTag);
		return FALSE;
	}

	BYTE bErrorCode = pStatus[8];
	if (bErrorCode != 0x00)
	{
		strError.Format("Device returned error code 0x%02X (%s).", bErrorCode, GetCapsoErrorName(bErrorCode));
		return FALSE;
	}

	strError.Empty();
	return TRUE;
}

//=============================================================================
// CCapsoTestDlg dialog

CCapsoTestDlg::CCapsoTestDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CAPSOTEST_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCapsoTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CCapsoTestDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_SN_READ1, &CCapsoTestDlg::OnBnClickedButtonSnRead1)
	ON_BN_CLICKED(IDC_BUTTON_SN_WRITE1, &CCapsoTestDlg::OnBnClickedButtonSnWrite1)
	ON_BN_CLICKED(IDC_BUTTON_SN_READ2, &CCapsoTestDlg::OnBnClickedButtonSnRead2)
	ON_BN_CLICKED(IDC_BUTTON_SN_WRITE2, &CCapsoTestDlg::OnBnClickedButtonSnWrite2)
END_MESSAGE_MAP()

BOOL CCapsoTestDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	hCapsoLLD = LoadLibrary("CapsoLLD.dll");
	if (!hCapsoLLD)
	{
		MessageBox("Failed to load CapsoLLD.dll.", "Error", MB_ICONERROR);
		return FALSE;
	}

	AVCapso_Initialize = (FuncAVCapso_Initialize)GetProcAddress(hCapsoLLD, "AVCapso_Initialize");
	AVCapso_Terminate = (FuncAVCapso_Terminate)GetProcAddress(hCapsoLLD, "AVCapso_Terminate");
	AVCapso_DeviceList = (FuncAVCapso_DeviceList)GetProcAddress(hCapsoLLD, "AVCapso_DeviceList");
	AVCapso_IOLock = (FuncAVCapso_IOLock)GetProcAddress(hCapsoLLD, "AVCapso_IOLock");
	AVCapso_IOUnlock = (FuncAVCapso_IOUnlock)GetProcAddress(hCapsoLLD, "AVCapso_IOUnlock");
	AVCapso_Read = (FuncAVCapso_Read)GetProcAddress(hCapsoLLD, "AVCapso_Read");
	AVCapso_Write = (FuncAVCapso_Write)GetProcAddress(hCapsoLLD, "AVCapso_Write");

	if (AVCapso_Initialize() == TRUE)
	{
		if (AVCapso_DeviceList(&deviceList) == TRUE)
		{
			if (deviceList.iCount > 0)
			{
				CString str;
				str.Format("Number of devices: %d", deviceList.iCount);
				MessageBox(str, "Device List", MB_ICONINFORMATION);

				CComboBox *pComboDev1, *pComboDev2;

				for (BYTE i = 0; i < deviceList.iCount; i++)
				{
					pComboDev1 = (CComboBox*)GetDlgItem(IDC_COMBO_DEV1);
					pComboDev1->AddString(deviceList.SN_DockingSystem[i]);

					pComboDev2 = (CComboBox*)GetDlgItem(IDC_COMBO_DEV2);
					pComboDev2->AddString(deviceList.SN_DockingSystem[i]);
				}
				
				
				
			}
			else
			{
				MessageBox("Device not found.", "Error", MB_ICONERROR);
				EndDialog(IDCANCEL);
				return FALSE;
			}
		}
		else
		{
			MessageBox("Failed to get device list.", "Error", MB_ICONERROR);
			EndDialog(IDCANCEL);
			return FALSE;
		}
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCapsoTestDlg::OnDestroy()
{
	if (AVCapso_Terminate)
	{
		AVCapso_Terminate();
	}

	if (hCapsoLLD)
	{
		FreeLibrary(hCapsoLLD);
		hCapsoLLD = NULL;
	}

	CDialogEx::OnDestroy();
}

void CCapsoTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CCapsoTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CCapsoTestDlg::OnBnClickedButtonSnRead1()
{
	BOOL	blRet = FALSE;
	BYTE	bCurrentSelect, bCmd[32] = { 0 }, bStatus[32] = { 0 }, bSN[32] = { 0 };
	DWORD	dwTag, dwTransferedSize;
	CString	strError;

	
	bCurrentSelect = ((CComboBox*)GetDlgItem(IDC_COMBO_DEV1))->GetCurSel();
	if (AVCapso_IOLock(deviceList.SN_DockingSystem[bCurrentSelect]) == TRUE)
	{
		srand(static_cast<unsigned int>(time(NULL)));
		dwTag = (DWORD)rand();

		memset(bCmd, 0, 32);
		// "USBC", 4 bytes
		bCmd[0] = 0x55;
		bCmd[1] = 0x53;
		bCmd[2] = 0x42;
		bCmd[3] = 0x43;
		// tag
		*((DWORD*)(bCmd + 4)) = dwTag;
		// cmd
		bCmd[8] = 0xab;
		//
		*((DWORD*)(bCmd + 12)) = 0;
		*((WORD*)(bCmd + 16)) = (WORD)1;
		if (AVCapso_Write(deviceList.SN_DockingSystem[bCurrentSelect], bCmd, 32, &dwTransferedSize, NULL))
		{
			if (AVCapso_Read(deviceList.SN_DockingSystem[bCurrentSelect], bSN, 16, &dwTransferedSize, NULL))
			{
				if (AVCapso_Read(deviceList.SN_DockingSystem[bCurrentSelect], bStatus, 32, &dwTransferedSize, NULL))
				{
					blRet = VerifyCapsoStatus(bStatus, dwTag, strError);
				}
				else
					strError = "Failed to read status.";
			}
			else
				strError = "Failed to read SN.";
		}
		else
			strError = "Failed to write command.";

		if (blRet)
		{
			SetDlgItemText(IDC_EDIT_SN1, (char*)bSN);
			MessageBox("Device 1 SN read success.", "Success", MB_USERICON);
		}
		else
		{
			if (strError.IsEmpty())
				strError = "Device 1 SN read fail.";
			MessageBox(strError, "Error", MB_ICONERROR);
		}

		AVCapso_IOUnlock(deviceList.SN_DockingSystem[bCurrentSelect]);
	}
	else
	{
		MessageBox("Device 1 busy.", "Error", MB_ICONERROR);
	}
}

void CCapsoTestDlg::OnBnClickedButtonSnWrite1()
{
	BOOL	blRet = FALSE;
	BYTE	bCurrentSelect, bCmd[32] = { 0 }, bStatus[32] = { 0 }, bSN[32] = { 0 };
	DWORD	dwTag, dwTransferedSize;
	CString	strError;


	bCurrentSelect = ((CComboBox*)GetDlgItem(IDC_COMBO_DEV1))->GetCurSel();
	if (AVCapso_IOLock(deviceList.SN_DockingSystem[bCurrentSelect]) == TRUE)
	{
		CString strSN;
		int nLen = GetDlgItemText(IDC_EDIT_SN1, strSN);
		if (nLen > 0 && nLen < 17)
		{
			strncpy_s((char*)bSN, sizeof(bSN), CT2A(strSN), _TRUNCATE);

			srand(static_cast<unsigned int>(time(NULL)));
			dwTag = (DWORD)rand();

			memset(bCmd, 0, 32);
			// "USBC", 4 bytes
			bCmd[0] = 0x55;
			bCmd[1] = 0x53;
			bCmd[2] = 0x42;
			bCmd[3] = 0x43;
			// tag
			*((DWORD*)(bCmd + 4)) = dwTag;
			// cmd
			bCmd[8] = 0xaa;
			//
			*((DWORD*)(bCmd + 12)) = 0;
			*((WORD*)(bCmd + 16)) = (WORD)1;
			if (AVCapso_Write(deviceList.SN_DockingSystem[bCurrentSelect], bCmd, 32, &dwTransferedSize, NULL))
			{
				if (AVCapso_Write(deviceList.SN_DockingSystem[bCurrentSelect], bSN, 16, &dwTransferedSize, NULL))
				{
					if (AVCapso_Read(deviceList.SN_DockingSystem[bCurrentSelect], bStatus, 32, &dwTransferedSize, NULL))
					{
						blRet = VerifyCapsoStatus(bStatus, dwTag, strError);
					}
					else
						strError = "Failed to read status.";
				}
				else
					strError = "Failed to write SN.";
			}
			else
				strError = "Failed to write command.";

			if (blRet)
				MessageBox("Device 1 SN write success.", "Success", MB_USERICON);
			else
			{
				if (strError.IsEmpty())
					strError = "Device 1 SN write fail.";
				MessageBox(strError, "Error", MB_ICONERROR);
			}
		}
		else
		{
			MessageBox("SN length must 1 ~ 16", "Error", MB_ICONERROR);
		}

		AVCapso_IOUnlock(deviceList.SN_DockingSystem[bCurrentSelect]);
	}
	else
	{
		MessageBox("Device 1 busy.", "Error", MB_ICONERROR);
	}
}


void CCapsoTestDlg::OnBnClickedButtonSnRead2()
{
	BOOL	blRet = FALSE;
	BYTE	bCurrentSelect, bCmd[32] = { 0 }, bStatus[32] = { 0 }, bSN[32] = { 0 };
	DWORD	dwTag, dwTransferedSize;
	CString	strError;


	bCurrentSelect = ((CComboBox*)GetDlgItem(IDC_COMBO_DEV2))->GetCurSel();
	if (AVCapso_IOLock(deviceList.SN_DockingSystem[bCurrentSelect]) == TRUE)
	{
		srand(static_cast<unsigned int>(time(NULL)));
		dwTag = (DWORD)rand();

		memset(bCmd, 0, 32);
		// "USBC", 4 bytes
		bCmd[0] = 0x55;
		bCmd[1] = 0x53;
		bCmd[2] = 0x42;
		bCmd[3] = 0x43;
		// tag
		*((DWORD*)(bCmd + 4)) = dwTag;
		// cmd
		bCmd[8] = 0xab;
		//
		*((DWORD*)(bCmd + 12)) = 0;
		*((WORD*)(bCmd + 16)) = (WORD)1;
		if (AVCapso_Write(deviceList.SN_DockingSystem[bCurrentSelect], bCmd, 32, &dwTransferedSize, NULL))
		{
			if (AVCapso_Read(deviceList.SN_DockingSystem[bCurrentSelect], bSN, 16, &dwTransferedSize, NULL))
			{
				if (AVCapso_Read(deviceList.SN_DockingSystem[bCurrentSelect], bStatus, 32, &dwTransferedSize, NULL))
				{
					blRet = VerifyCapsoStatus(bStatus, dwTag, strError);
				}
				else
					strError = "Failed to read status.";
			}
			else
				strError = "Failed to read SN.";
		}
		else
			strError = "Failed to write command.";

		if (blRet)
		{
			SetDlgItemText(IDC_EDIT_SN2, (char*)bSN);
			MessageBox("Device 2 SN read success.", "Success", MB_USERICON);
		}
		else
		{
			if (strError.IsEmpty())
				strError = "Device 2 SN read fail.";
			MessageBox(strError, "Error", MB_ICONERROR);
		}

		AVCapso_IOUnlock(deviceList.SN_DockingSystem[bCurrentSelect]);
	}
	else
	{
		MessageBox("Device 2 busy.", "Error", MB_ICONERROR);
	}
}

void CCapsoTestDlg::OnBnClickedButtonSnWrite2()
{
	BOOL	blRet = FALSE;
	BYTE	bCurrentSelect, bCmd[32] = { 0 }, bStatus[32] = { 0 }, bSN[32] = { 0 };
	DWORD	dwTag, dwTransferedSize;
	CString	strError;


	bCurrentSelect = ((CComboBox*)GetDlgItem(IDC_COMBO_DEV2))->GetCurSel();
	if (AVCapso_IOLock(deviceList.SN_DockingSystem[bCurrentSelect]) == TRUE)
	{
		CString strSN;
		int nLen = GetDlgItemText(IDC_EDIT_SN2, strSN);
		if (nLen > 0 && nLen < 17)
		{
			strncpy_s((char*)bSN, sizeof(bSN), CT2A(strSN), _TRUNCATE);

			srand(static_cast<unsigned int>(time(NULL)));
			dwTag = (DWORD)rand();

			memset(bCmd, 0, 32);
			// "USBC", 4 bytes
			bCmd[0] = 0x55;
			bCmd[1] = 0x53;
			bCmd[2] = 0x42;
			bCmd[3] = 0x43;
			// tag
			*((DWORD*)(bCmd + 4)) = dwTag;
			// cmd
			bCmd[8] = 0xaa;
			//
			*((DWORD*)(bCmd + 12)) = 0;
			*((WORD*)(bCmd + 16)) = (WORD)1;
			if (AVCapso_Write(deviceList.SN_DockingSystem[bCurrentSelect], bCmd, 32, &dwTransferedSize, NULL))
			{
				if (AVCapso_Write(deviceList.SN_DockingSystem[bCurrentSelect], bSN, 16, &dwTransferedSize, NULL))
				{
					if (AVCapso_Read(deviceList.SN_DockingSystem[bCurrentSelect], bStatus, 32, &dwTransferedSize, NULL))
					{
						blRet = VerifyCapsoStatus(bStatus, dwTag, strError);
					}
					else
						strError = "Failed to read status.";
				}
				else
					strError = "Failed to write SN.";
			}
			else
				strError = "Failed to write command.";

			if (blRet)
				MessageBox("Device 2 SN write success.", "Success", MB_USERICON);
			else
			{
				if (strError.IsEmpty())
					strError = "Device 2 SN write fail.";
				MessageBox(strError, "Error", MB_ICONERROR);
			}
		}
		else
		{
			MessageBox("SN length must 1 ~ 16", "Error", MB_ICONERROR);
		}

		AVCapso_IOUnlock(deviceList.SN_DockingSystem[bCurrentSelect]);
	}
	else
	{
		MessageBox("Device 2 busy.", "Error", MB_ICONERROR);
	}
}
