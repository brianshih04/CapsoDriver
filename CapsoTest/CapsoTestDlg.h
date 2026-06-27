
// CapsoTestDlg.h : header file
//

#pragma once

#pragma pack(1)

typedef struct _DEVICELIST
{
	BYTE	iCount;
	char    SN_DockingSystem[255][32];
} DEVICELIST;

#pragma pack()

// CCapsoTestDlg dialog
class CCapsoTestDlg : public CDialogEx
{
// Construction
public:
	CCapsoTestDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CAPSOTEST_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedButtonSnRead1();
	afx_msg void OnBnClickedButtonSnWrite1();
	afx_msg void OnBnClickedButtonSnRead2();
	afx_msg void OnBnClickedButtonSnWrite2();
	afx_msg void OnDestroy();

private:
	HMODULE	hCapsoLLD;
	DEVICELIST deviceList;
};
