// CrashMeDlg.h : header file
//

#include "afxcmn.h"
#if !defined(AFX_CRASHMEDLG_H__FF7AFC6F_FF8A_4333_B6BA_B5F53686E0DC__INCLUDED_)
#define AFX_CRASHMEDLG_H__FF7AFC6F_FF8A_4333_B6BA_B5F53686E0DC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CCrashMeDlg dialog

class CCrashMeDlg : public CDialog
{
// Construction
public:
	CCrashMeDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CCrashMeDlg)
	enum { IDD = IDD_CRASHME_DIALOG };
	CButton	m_btnStressMemAlloc;
	CButton	m_btnVanish;
  CButton	m_btnStressCPURun;
	CString	m_csTempFileDir;
	DWORD	m_dwTempFileSize;
	DWORD	m_dwStressCPU;
	DWORD	m_dwStressMem;
	CString	m_csSnmpTrapMsg;
	CString	m_csTimeText;
	long	m_lTimeC;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCrashMeDlg)
	public:
	virtual BOOL DestroyWindow();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

	CString	m_csLastFile;

// Implementation
protected:
	HICON m_hIcon;
  void CleanUp();
  void SwitchDesktop(LPCSTR szDesktopName);

	// Generated message map functions
	//{{AFX_MSG(CCrashMeDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnAccessViolation();
	afx_msg void OnUnHandledException();
	afx_msg void OnVanish();
	afx_msg void OnButtontest();
	afx_msg void OnTempFileAlloc();
	afx_msg void OnDeleteLast();
	afx_msg void OnStressCPURun();
	afx_msg void OnStressMemAlloc();
	afx_msg void OnAssert();
	afx_msg void OnDeskInteractive();
	afx_msg void OnDeskHidden();
	afx_msg void OnSnmpTrap();
	afx_msg void OnSnmpTrap2();
	afx_msg void OnTimeConvert();
	afx_msg void OnTimeConvert2();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
  CString m_csStressMemStatus;
  CListCtrl m_cStressCPUList;
  afx_msg void OnBnClickedAssertdump();
  afx_msg void OnBnClickedSeterrormode();
  afx_msg void OnBnClickedInstallbugtrap();
  afx_msg void OnBnClickedThrowbadalloc();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRASHMEDLG_H__FF7AFC6F_FF8A_4333_B6BA_B5F53686E0DC__INCLUDED_)
