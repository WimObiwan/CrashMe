// CrashMeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "CrashMe.h"
#include "CrashMeDlg.h"
#include ".\crashmedlg.h"
#include "rtcapi.h"

#include "BugTrap.h"
#include "DbgHelp.h"
#include <new>

using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void *g_pVirtual = NULL;
DWORD g_dwMemAllocated = 0;
bool g_bStressCPURunning[32] = {false};
HANDLE g_hStressCPU[32] = {NULL};
//DWORD dwCPUState[32] = 

BOOL GetDLLVersion(const CString& sFileName, VS_FIXEDFILEINFO& fi)
{
	BOOL CheckFlag=FALSE;
	//Standaard versie string op onbekend
	DWORD Handle;
  char szFileName[MAX_PATH];
  strcpy_s(szFileName, sFileName);

	DWORD Size=GetFileVersionInfoSize(szFileName,&Handle);
	//Indien geslaagd (versie informatie bekend)
	if (Size)
	{
		//Buffer aanmaken met de grootte van dit versie informatie blok
		char* Data=new char[Size];
    BOOL b = GetFileVersionInfo(szFileName,Handle,Size,Data);
    if (b)
		{
			void* Location=NULL;
			//Pointer naar de grootte van de gevraagde informatie
			UINT Size;
			//Ophalen van de pointer naar en de grootte van de gevraagde informatie ('\\'=Default version info)
			if (VerQueryValue(Data,"\\",&Location,&Size))
			{
				memcpy(&fi,Location,min(Size,sizeof(fi)));
    		delete Data;
        return TRUE;
      }
    }
 		delete Data;
    Data = NULL;
  }
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CCrashMeDlg dialog

CCrashMeDlg::CCrashMeDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CCrashMeDlg::IDD, pParent)
  , m_csStressMemStatus(_T(""))
  {
	//{{AFX_DATA_INIT(CCrashMeDlg)
	m_csTempFileDir = _T("C:\\");
	m_dwTempFileSize = 0;
	m_dwStressCPU = 50;
	m_dwStressMem = 25;
	m_csSnmpTrapMsg = _T("");
	m_csTimeText = _T("");
	m_lTimeC = 0;
  m_csStressMemStatus = "(No memory allocated)";
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32

	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CCrashMeDlg::DoDataExchange(CDataExchange* pDX)
{
CDialog::DoDataExchange(pDX);
//{{AFX_DATA_MAP(CCrashMeDlg)
DDX_Control(pDX, IDC_STRESSMEMALLOC, m_btnStressMemAlloc);
DDX_Control(pDX, IDC_Vanish, m_btnVanish);
DDX_Control(pDX, IDC_STRESSCPURUN, m_btnStressCPURun);
DDX_Text(pDX, IDC_TEMPFILEDIR, m_csTempFileDir);
DDV_MaxChars(pDX, m_csTempFileDir, 256);
DDX_Text(pDX, IDC_TEMPFILESIZE, m_dwTempFileSize);
DDV_MinMaxDWord(pDX, m_dwTempFileSize, 0, 1024);
DDX_Text(pDX, IDC_STRESSCPU, m_dwStressCPU);
DDV_MinMaxDWord(pDX, m_dwStressCPU, 0, 100);
DDX_Text(pDX, IDC_STRESSMEM, m_dwStressMem);
DDX_Text(pDX, IDC_SNMPTRAPMSG, m_csSnmpTrapMsg);
DDX_Text(pDX, IDC_TIMETEXT, m_csTimeText);
DDX_Text(pDX, IDC_TIMEC, m_lTimeC);
//}}AFX_DATA_MAP
DDX_Text(pDX, IDC_STRESSMEMSTATUS, m_csStressMemStatus);
DDX_Control(pDX, IDC_STRESSCPULIST, m_cStressCPUList);
}

BEGIN_MESSAGE_MAP(CCrashMeDlg, CDialog)
	//{{AFX_MSG_MAP(CCrashMeDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_AccessViolation, OnAccessViolation)
	ON_BN_CLICKED(IDC_UnHandledException, OnUnHandledException)
	ON_BN_CLICKED(IDC_Vanish, OnVanish)
	ON_BN_CLICKED(IDC_TEMPFILEALLOC, OnTempFileAlloc)
	ON_BN_CLICKED(IDC_DELETELAST, OnDeleteLast)
	ON_BN_CLICKED(IDC_STRESSCPURUN, OnStressCPURun)
	ON_BN_CLICKED(IDC_STRESSMEMALLOC, OnStressMemAlloc)
	ON_BN_CLICKED(IDC_Assert, OnAssert)
	ON_BN_CLICKED(IDC_DESKINTERACTIVE, OnDeskInteractive)
	ON_BN_CLICKED(IDC_DESKHIDDEN, OnDeskHidden)
	ON_BN_CLICKED(IDC_SNMPTRAP, OnSnmpTrap)
	ON_BN_CLICKED(IDC_SNMPTRAP2, OnSnmpTrap2)
	ON_BN_CLICKED(IDC_TIMECONVERT, OnTimeConvert)
	ON_BN_CLICKED(IDC_TIMECONVERT2, OnTimeConvert2)
	//}}AFX_MSG_MAP
  ON_BN_CLICKED(IDC_AssertDump, OnBnClickedAssertdump)
  ON_BN_CLICKED(IDC_SetErrorMode, &CCrashMeDlg::OnBnClickedSeterrormode)
  ON_BN_CLICKED(IDC_InstallBugTrap, &CCrashMeDlg::OnBnClickedInstallbugtrap)
  ON_BN_CLICKED(IDC_ThrowBadAlloc, &CCrashMeDlg::OnBnClickedThrowbadalloc)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCrashMeDlg message handlers

BOOL CCrashMeDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
  m_cStressCPUList.InsertColumn(0, "CPU");
  m_cStressCPUList.InsertColumn(1, "Target");
  m_cStressCPUList.InsertColumn(2, "Current");
	
  SYSTEM_INFO SysInfo;
  GetSystemInfo(&SysInfo);

  for (int i = 0; i < (int)SysInfo.dwNumberOfProcessors; i++)
    {
    CString cStr;
    cStr.Format("%d", i);
    m_cStressCPUList.InsertItem(i, cStr);
    }
  
  m_cStressCPUList.SetView(LV_VIEW_DETAILS);

  return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCrashMeDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCrashMeDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CCrashMeDlg::OnAccessViolation() 
{
  long *ptr = 0;
  *ptr = 15;
}

void CCrashMeDlg::OnUnHandledException() 
{
  throw 0xffffffff;
}

void CCrashMeDlg::OnAssert() 
{
#ifdef _DEBUG
  ASSERT(FALSE);
#else
  VS_FIXEDFILEINFO fi;
  if (GetDLLVersion("MFC42D.DLL", fi))
    {
    HMODULE hMfcDebug = LoadLibrary("MFC42D.DLL");
    if (hMfcDebug)
      {
      typedef BOOL (AFXAPI * TAfxAssertFailedLine)(LPCSTR lpszFileName, int nLine);
      TAfxAssertFailedLine AfxAssertFailedLine = NULL;
      if (fi.dwFileVersionMS == 0x00060000)
        {
        // Visual C++ 6.0
        AfxAssertFailedLine = (TAfxAssertFailedLine) GetProcAddress(hMfcDebug, (LPCSTR)1041); // VC 6.0!
        }
      else
        {
        CString cStr;
        cStr.Format("MFC42D.DLL version not compatible (%08x)", fi.dwFileVersionMS);
        MessageBox(cStr);
        }
      if (AfxAssertFailedLine)
        {
        AfxAssertFailedLine(__FILE__, __LINE__);
        }
      else
        {
        MessageBox("Could not find 'AfxAssertFailedLine' in MFC42D.DLL");
        }
      }
    else
      {
      MessageBox("Could not load MFC42D.DLL");
      }
    }
  else
    {
    MessageBox("Could not determine version of MFC42D.DLL");
    }
#endif
}

void CCrashMeDlg::OnVanish() 
{
  try
    {
    //void (*p)() = (void (*)())65548;
    //p();
    ((void (*)())65548)();
    }
  catch(...)
    {
    }

  /*int i = 65548;
  while (true) 
    {
    try 
      {
        // i= 65548
        void (*p)() = (void (*)())i;
        p();
      }
    catch(...)
      {
      }
    i++;
    char sz[10];
    itoa(i, sz, 10);
    m_btnVanish.SetWindowText(sz);
    }*/
  MessageBox("OnVanish done");
}

void CCrashMeDlg::OnButtontest() 
{
  DWORD dwId = 0;
  DWORD WINAPI FileTest(VOID *pVoid);
  HANDLE h = INVALID_HANDLE_VALUE;
  for (int i = 0; i < 10; i++)
    {
    CreateThread(NULL, 0, FileTest, NULL, 0, &dwId);
    CloseHandle(h);
    }
}

DWORD WINAPI FileTest(VOID *pVoid)
{
  DWORD dwStart = GetTickCount();
  while (GetTickCount() - dwStart < 30000)
    {
    FILE *f;
    fopen_s(&f, "c:\\filetest.txt", "a+w");
    fputs("fputs\r\n", f);
    fprintf(f, "fprintf %s%d\r\n", "GetTickCount", GetTickCount());
    fclose(f);
    }
  return 0;
}

void CCrashMeDlg::OnTempFileAlloc() 
{
  UpdateData();
  CString csFileName;
  if (!GetTempFileName(m_csTempFileDir, "CrashMe_", 0, csFileName.GetBuffer(MAX_PATH)))
    {
    CString cStr;
    cStr.Format("Could not construct unique temporary file name in directory\n"
                "%s\n"
                "GetLastError=%d", m_csTempFileDir, GetLastError());
    MessageBox(cStr, NULL, MB_ICONERROR);
    return;
    }
  csFileName.ReleaseBuffer();
  ULARGE_INTEGER uliFree;
  if (m_dwTempFileSize == 0)
    {
    ULARGE_INTEGER uliTotal, uliDummy;
    if (!GetDiskFreeSpaceEx(m_csTempFileDir, &uliDummy, &uliTotal, &uliFree))
      {
      }
    }
  else
    {
    uliFree.QuadPart = unsigned __int64(m_dwTempFileSize) * 1024ui64 * 1024ui64;
    }

  // Display message to user
    {
    CString cStr;
    cStr.Format("Do you want to allocate the temporary file\n"
              "%s\n"
              "with size %I64u?", (LPCSTR)csFileName, uliFree.QuadPart);
    if (MessageBox(cStr, NULL, MB_YESNOCANCEL | MB_ICONQUESTION) == IDYES)
      {
      HANDLE hFile = CreateFile(csFileName, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        CREATE_ALWAYS, 0, NULL);
      if (hFile != INVALID_HANDLE_VALUE)
        {
        CWaitCursor cWaitCursor;
        BOOL bOK = SetFilePointer(hFile, (LONG)uliFree.LowPart, (LONG*)&uliFree.HighPart, FILE_BEGIN);
        if (bOK) bOK = SetEndOfFile(hFile);
        if (!bOK && !m_dwTempFileSize)
          {
          ULARGE_INTEGER uli;
          uli.QuadPart = 1024;
          for (int i = 0; i < 16 && uli.QuadPart < uliFree.QuadPart && !bOK; i++)
            {
            ULARGE_INTEGER uli2;
            uli2.QuadPart = uliFree.QuadPart - uli.QuadPart;
            bOK = SetFilePointer(hFile, (LONG)uli2.LowPart, (LONG*)&uli2.HighPart, FILE_BEGIN);
            if (bOK) bOK = SetEndOfFile(hFile);
            uli.QuadPart *= 2;
            }
          }
        CloseHandle(hFile);
        cWaitCursor.Restore();
        if (bOK)
          {
          MessageBox("Temporary file created", NULL, 0);
          m_csLastFile = csFileName;
          }
        else
          {
          DeleteFile(csFileName);
          CString cStr;
          cStr.Format("Could not set filepointer in the temporary file\n"
                      "%s\n"
                      "GetLastError=%d\n"
                      "The remains of the temporary file are removed after the error", 
                      csFileName, GetLastError());
          MessageBox(cStr, NULL, MB_ICONERROR);
          }
        }
      else
        {
        CString cStr;
        cStr.Format("Could not open the temporary file\n"
                    "%s\n"
                    "GetLastError=%d", csFileName, GetLastError());
        MessageBox(cStr, NULL, MB_ICONERROR);
        }
      }
    }
}

void CCrashMeDlg::OnDeleteLast() 
{
  if (!m_csLastFile.GetLength())
    {
    MessageBox("Not yet a temporary file allocated", NULL, MB_ICONERROR);
    }
  else
    {
    WIN32_FIND_DATA FindData;
    HANDLE hFile = FindFirstFile(m_csLastFile, &FindData);
    if (hFile != INVALID_HANDLE_VALUE)
      {
      FindClose(hFile);
      CWaitCursor cWaitCursor;
      BOOL bOK = DeleteFile(m_csLastFile);
      cWaitCursor.Restore();
      if (bOK)
        {
        CString cStr;
        cStr.Format("The last temporary file\n"
                    "%s\n"
                    "was succesfully deleted", m_csLastFile);
        MessageBox(cStr, NULL, 0);
        m_csLastFile.Empty();
        }
      else
        {
        CString cStr;
        cStr.Format("The last temporary file\n"
                    "%s\n"
                    "could not be deleted, GetLastError=%d", 
                    m_csLastFile, GetLastError());
        MessageBox(cStr, NULL, MB_ICONERROR);
        }
      }
    else
      {
      CString cStr;
      cStr.Format("The last temporary file\n"
                  "%s\n"
                  "was not found", m_csLastFile);
      MessageBox(cStr, NULL, MB_ICONERROR);
      m_csLastFile.Empty();
      }
    }
	
}

void CCrashMeDlg::OnStressCPURun() 
{
  UpdateData(TRUE);
  if (g_hStressCPU[0])
    {
    g_bStressCPURunning[0] = false;
    WaitForSingleObject(g_hStressCPU[0], INFINITE);
    CloseHandle(g_hStressCPU[0]);
    g_hStressCPU[0] = NULL;
    m_btnStressCPURun.SetWindowText("Run!");
    }
  else
    {
    DWORD dwID;
    g_bStressCPURunning[0] = true;
    DWORD WINAPI StressCPU(LPVOID);
    DWORD dwParam = MAKEWPARAM((WORD)m_dwStressCPU, 0);
    g_hStressCPU[0] = CreateThread(NULL, 0, StressCPU, (LPVOID)dwParam, 0, &dwID);
    m_btnStressCPURun.SetWindowText("STOP");
    }
}

DWORD WINAPI StressCPU(LPVOID pVoid)
{
  DWORD dwParam = (DWORD)pVoid;
  const int iCPU = HIWORD(dwParam);
  DWORD dwTickCount = GetTickCount();
  DWORD dwTargetCPU = LOWORD(dwParam);
  if (dwTargetCPU > 100) dwTargetCPU = 50;
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
  SetThreadAffinityMask(GetCurrentThread(), 1 << iCPU);

  DWORD dwTest = 0;
  const DWORD dwThreadId = GetCurrentThreadId();
  DWORD dwOffset = (4096 * dwTargetCPU) / 100;
  const int iArraySize = 10;
  DWORD dwArrayOffset[iArraySize];
  for (int i = 0; i < sizeof(dwArrayOffset) / sizeof(dwArrayOffset[0]); i++)
    {
    dwArrayOffset[i] = dwOffset;
    }

  while (g_bStressCPURunning[iCPU])
    {
    ULARGE_INTEGER uliLastKernelTime, uliLastUserTime;
      {
      FILETIME ftKernel, ftUser;
      FILETIME ftDummy1, ftDummy2;
      VERIFY(GetProcessTimes(GetCurrentProcess(), &ftDummy1, &ftDummy2, 
        &ftKernel, &ftUser));
      uliLastKernelTime.LowPart = ftKernel.dwLowDateTime;
      uliLastKernelTime.HighPart = ftKernel.dwHighDateTime;
      uliLastUserTime.LowPart = ftUser.dwLowDateTime;
      uliLastUserTime.HighPart = ftUser.dwHighDateTime;
      }

    DWORD dwTotal = GetTickCount();

    for (int j = 0; j < 8; j++)
      {
      for (DWORD i = 0; i < dwOffset * 1024; i++)
        {
        dwTest *= dwThreadId;
        }
      Sleep(1);
      }

    ULARGE_INTEGER uliKernelTime, uliUserTime;
      {
      FILETIME ftDummy1, ftDummy2;
      VERIFY(GetProcessTimes(GetCurrentProcess(), &ftDummy1, &ftDummy2, 
        (FILETIME*) &uliKernelTime, (FILETIME*) &uliUserTime));
      }

    dwTotal = GetTickCount() - dwTotal;
    if (dwTotal > 2000000) dwTotal = MAXDWORD - dwTotal + 1;

    DWORD dwCPU = (DWORD) ((uliKernelTime.QuadPart + uliUserTime.QuadPart 
      - uliLastKernelTime.QuadPart - uliLastUserTime.QuadPart) / 100i64) / dwTotal;

    DWORD dwSum = 0;
    for (int i = 0; i < iArraySize - 1; i++)
      {
      dwArrayOffset[i] = dwArrayOffset[i + 1];
      dwSum += dwArrayOffset[i];
      }
    int iDiff = (int)dwTargetCPU - (int)dwCPU;
    if (iDiff >= 0)
      {
      dwOffset += (dwOffset * DWORD(iDiff)) / 100;
      }
    else
      {
      dwOffset -= (dwOffset * DWORD(-iDiff)) / 100;
      }
    dwArrayOffset[iArraySize - 1] = dwOffset;
    dwSum += dwOffset;
    dwOffset = dwSum / iArraySize;
    }
  return 0;
}

void CCrashMeDlg::OnStressMemAlloc() 
{
  UpdateData(TRUE);
  if (!g_pVirtual)
    {
    CWaitCursor cWaitCursor;
    MEMORYSTATUS MemoryStatus;
    GlobalMemoryStatus(&MemoryStatus);
    DWORD dwBytesToAlloc = (DWORD)((((unsigned __int64)MemoryStatus.dwTotalPhys) * m_dwStressMem) / 100);
    DWORD dwBytesToAlloc1 = (DWORD)(((unsigned __int64)dwBytesToAlloc * 11) / 10);
    DWORD dwBytesToAlloc2 = (DWORD)(((unsigned __int64)dwBytesToAlloc * 13) / 10);
    if (!SetProcessWorkingSetSize(GetCurrentProcess(), dwBytesToAlloc1, dwBytesToAlloc2))
      {
      cWaitCursor.Restore();
      CString cStr;
      cStr.Format("Unable to set workingset size! GetLastError=%d", GetLastError());
      MessageBox(cStr);
      return;
      }

    g_pVirtual = VirtualAlloc(NULL, dwBytesToAlloc, MEM_COMMIT, PAGE_READWRITE);
    if (!g_pVirtual)
      {
      cWaitCursor.Restore();
      CString cStr;
      cStr.Format("VirtualAlloc failed, GetLastError=%d", GetLastError());
      MessageBox(cStr);
      return;
      }
    if (!VirtualLock(g_pVirtual, dwBytesToAlloc))
      {
      cWaitCursor.Restore();
      CString cStr;
      cStr.Format("VirtualLock failed, GetLastError=%d", GetLastError());
      MessageBox(cStr);
      VirtualFree(g_pVirtual, 0, MEM_RELEASE);
      return;
      }
    g_dwMemAllocated = dwBytesToAlloc;
    m_btnStressMemAlloc.SetWindowText("DEALLOCATE");
    m_csStressMemStatus.Format("%d bytes (%d MB)", dwBytesToAlloc, dwBytesToAlloc / (1024 * 1024));
    }
  else
    {
    CWaitCursor cWaitCursor;
    if (!VirtualUnlock(g_pVirtual, g_dwMemAllocated))
      {
      cWaitCursor.Restore();
      CString cStr;
      cStr.Format("VirtualUnlock failed, GetLastError=%d", GetLastError());
      MessageBox(cStr);
      }
    if (!VirtualFree(g_pVirtual, 0, MEM_RELEASE))
      {
      cWaitCursor.Restore();
      CString cStr;
      cStr.Format("VirtualAlloc failed, GetLastError=%d", GetLastError());
      MessageBox(cStr);
      }
    g_pVirtual = NULL;
    m_btnStressMemAlloc.SetWindowText("Allocate!");
    m_csStressMemStatus = "(No memory allocated)";
    }
  UpdateData(FALSE);
}

void CCrashMeDlg::CleanUp()
{
  for (int i = 0; i < 32; i++) if (g_hStressCPU[i]) OnStressCPURun();
  if (g_pVirtual) OnStressMemAlloc();
}

BOOL CCrashMeDlg::DestroyWindow() 
{
  CleanUp();
	
	return CDialog::DestroyWindow();
}



void CCrashMeDlg::OnDeskInteractive() 
{
  SwitchDesktop("Default");
}

void CCrashMeDlg::OnDeskHidden() 
{
  SwitchDesktop("HiddenDesktop");
}

void CCrashMeDlg::SwitchDesktop(LPCSTR szDesktopName)
{
  BOOL bRet;

  HDESK hDesk = ::OpenDesktop((char*)szDesktopName, 0, FALSE, DESKTOP_SWITCHDESKTOP);
  if (!hDesk)
    {
    CString cStr;
    cStr.Format("OpenDesktop failed, GetLastError=%d", GetLastError());
    MessageBox(cStr);
    return;
    }

  bRet = ::SwitchDesktop(hDesk);
  if (!bRet)
    {
    CString cStr;
    cStr.Format("SwitchDesktop failed, GetLastError=%d", GetLastError());
    MessageBox(cStr);
    }

  bRet = ::CloseDesktop(hDesk);
  if (!bRet)
    {
    CString cStr;
    cStr.Format("CloseDesktop failed, GetLastError=%d", GetLastError());
    MessageBox(cStr);
    }
}


void CCrashMeDlg::OnSnmpTrap() 
{
  UpdateData();

  SYSTEM_INFO SysInfo;
  GetSystemInfo(&SysInfo);
  const DWORD dwGranularity = SysInfo.dwAllocationGranularity;  
  const DWORD dwSize = ((1 * 1024 * 1024 + dwGranularity - 1) / dwGranularity) * dwGranularity; // 1MB

  HANDLE hMapping = OpenFileMapping(FILE_MAP_WRITE, FALSE, "VLSNMPSHAREDMEM");

  if (!hMapping)
    {
    CString cStr;
    cStr.Format("Creating shared memory block 'VLSNMPSHAREDMEM' failed, GetLastError=%d", GetLastError());
    MessageBox(cStr);
    return;
    }

  HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, "VLSNMPTRAPAVAILABLE");
  if (!hEvent)
    {
    CString cStr;
    cStr.Format("Creating event 'VLSNMPTRAPAVAILABLE' failed, GetLastError=%d", GetLastError());
    MessageBox(cStr);
    CloseHandle(hMapping);
    return;
    }

  typedef struct
    {
    DWORD m_dwMessageLen;
    DWORD m_dwPrevMessage;
    DWORD m_dwType;
    DWORD m_dwSubType;
    char  m_szMessage[1];
    } TSNMPMessage;
  typedef struct
    {
    DWORD m_dwOffsetLastMessage;
    } TSNMPSharedMem;

  BYTE *pSharedMem = (BYTE *)MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);
  if (!pSharedMem)
    {
    CString cStr;
    cStr.Format("Could not map shared memory block 'VLSNMPSHAREDMEM', GetLastError=%d", GetLastError());
    MessageBox(cStr);
    }
  else
    {
    HANDLE hMutex = OpenMutex(SYNCHRONIZE, FALSE, "VLSNMPSHAREDMEMMUTEX");
    if (!hMutex)
      {
      CString cStr;
      cStr.Format("Open mutex VLSNMPSHAREDMEMMUTEX failed, GetLastError=%d", GetLastError());
      MessageBox(cStr);
      }
    else
      {
      if (WaitForSingleObject(hMutex, 60 * 60 * 1000) != WAIT_OBJECT_0)
        {
        CString cStr;
        cStr.Format("Locking VLSNMPSHAREDMEMMUTEX failed, GetLastError=%d", GetLastError());
        MessageBox(cStr);
        }
      else
        {
        TSNMPSharedMem *pSNMPSharedMem = (TSNMPSharedMem *)pSharedMem;
        DWORD dwPrevMessage = pSNMPSharedMem->m_dwOffsetLastMessage;
        if (pSNMPSharedMem->m_dwOffsetLastMessage == 0)
          {
          pSNMPSharedMem->m_dwOffsetLastMessage = sizeof(DWORD);
          }
        else
          {
          const TSNMPMessage *const pMessage = (TSNMPMessage *)(pSharedMem + pSNMPSharedMem->m_dwOffsetLastMessage);
          pSNMPSharedMem->m_dwOffsetLastMessage += pMessage->m_dwMessageLen;
          }

        TSNMPMessage *const pMessage = (TSNMPMessage *)(pSharedMem + pSNMPSharedMem->m_dwOffsetLastMessage);
        pMessage->m_dwPrevMessage = dwPrevMessage;
        int iBytesToCopy = strlen(m_csSnmpTrapMsg) + 1;
        memcpy(pMessage->m_szMessage, m_csSnmpTrapMsg, iBytesToCopy);
        pMessage->m_dwMessageLen = iBytesToCopy + sizeof(*pMessage) - sizeof(pMessage->m_szMessage);

        SetEvent(hEvent);
        ReleaseMutex(hMutex);
        }
      CloseHandle(hMutex);
      }
    UnmapViewOfFile(pSharedMem);
    }
  
  CloseHandle(hEvent);
  CloseHandle(hMapping);
}

void OpenHandlesAsEveryone(HANDLE &hSNMPTrapAvailable, HANDLE &hSNMPSharedMem, HANDLE &hSNMPSharedMemMutex)
{
    SYSTEM_INFO SysInfo;
    GetSystemInfo(&SysInfo);
    const DWORD dwGranularity = SysInfo.dwAllocationGranularity;
    const DWORD dwSize = ((1 * 1024 * 1024 + dwGranularity - 1) / dwGranularity) * dwGranularity; // 1MB
    char sz[15];
    sprintf_s(sz, "%d", dwSize);
    MessageBox(NULL, sz, "VLSNMP", 0);


    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_WORLD_SID_AUTHORITY;
    PSID psidEveryone = NULL; 
    int nSidSize ; 
    int nAclSize ;
    PACL paclNewDacl = NULL; 
    SECURITY_DESCRIPTOR sd ;
    SECURITY_ATTRIBUTES sa ; 
    
    __try{
        // Create the everyone sid
        if (!AllocateAndInitializeSid(&siaWorld, 1, SECURITY_WORLD_RID, 0,
                                           0, 0, 0, 0, 0, 0, &psidEveryone))
        {            
            psidEveryone = NULL ; 
            __leave;
        }
 
        nSidSize = GetLengthSid(psidEveryone) ;
        nAclSize = nSidSize * 2 + sizeof(ACCESS_ALLOWED_ACE) + sizeof(ACCESS_DENIED_ACE) + sizeof(ACL) ;
        paclNewDacl = (PACL) LocalAlloc( LPTR, nAclSize ) ;
        if( !paclNewDacl )
            __leave ; 
        if(!InitializeAcl( paclNewDacl, nAclSize, ACL_REVISION ))
            __leave ; 
        if(!AddAccessDeniedAce( paclNewDacl, ACL_REVISION, WRITE_DAC | WRITE_OWNER, psidEveryone ))
            __leave ; 
        // I am using GENERIC_ALL here so that this very code can be applied to 
        // other objects.  Specific access should be applied when possible.
        if(!AddAccessAllowedAce( paclNewDacl, ACL_REVISION, GENERIC_ALL, psidEveryone ))
            __leave ; 
        if(!InitializeSecurityDescriptor( &sd, SECURITY_DESCRIPTOR_REVISION ))
            __leave ; 
        if(!SetSecurityDescriptorDacl( &sd, TRUE, paclNewDacl, FALSE ))
            __leave ; 
        sa.nLength = sizeof( sa ) ;
        sa.bInheritHandle = FALSE ; 
        sa.lpSecurityDescriptor = &sd ;
 
        hSNMPTrapAvailable = CreateEvent(&sa, TRUE, FALSE, "VLSNMPTRAPAVAILABLE");

        hSNMPSharedMem = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE | SEC_COMMIT, 0, dwSize,
          "VLSNMPSHAREDMEM");

        hSNMPSharedMemMutex = CreateMutex(&sa, FALSE, "VLSNMPSHAREDMEMMUTEX");

    }__finally{
        if( !paclNewDacl )
            LocalFree( paclNewDacl ) ;
        if( !psidEveryone )
            FreeSid( psidEveryone ) ;

    }
 
    return; 
}

HANDLE g_hSNMPSharedMem = NULL, g_hSNMPTrapAvailable = NULL, g_hSNMPSharedMemMutex = NULL;
void CCrashMeDlg::OnSnmpTrap2() 
{
    if (!g_hSNMPSharedMem || !g_hSNMPTrapAvailable || !g_hSNMPSharedMemMutex)
        OpenHandlesAsEveryone(g_hSNMPTrapAvailable, g_hSNMPSharedMem, g_hSNMPSharedMemMutex);

  typedef struct
    {
    DWORD m_dwMessageLen;
    DWORD m_dwPrevMessage;
    DWORD m_dwSource;
    DWORD m_dwType;
    DWORD m_dwSubType;
    DWORD m_dwPriority;
    char  m_szMessage[1];
    } TSNMPMessage;
  typedef struct
    {
    DWORD m_dwOffsetLastMessage;
    } TSNMPSharedMem;


    BYTE *pSharedMem = (BYTE *)MapViewOfFile(g_hSNMPSharedMem, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    if (!pSharedMem)
      {
      CString cStr;
      cStr.Format("Could not map shared memory block 'VLSNMPSHAREDMEM', err=%d", GetLastError());
      MessageBox(cStr);
      }
    else
      {
      if (WaitForSingleObject(g_hSNMPSharedMemMutex, 1000) != WAIT_OBJECT_0)
        {
        MessageBox("VLSNMPSHAREDMEMMUTEX Lock failed", "VLSNMP", 0);
        }
      else
        {
        TSNMPSharedMem *pSNMPSharedMem = (TSNMPSharedMem *) pSharedMem;
        if (pSNMPSharedMem->m_dwOffsetLastMessage > 0)
          {
          TSNMPMessage *pSNMPMessage = (TSNMPMessage*) (pSharedMem + pSNMPSharedMem->m_dwOffsetLastMessage);           
          pSNMPSharedMem->m_dwOffsetLastMessage = pSNMPMessage->m_dwPrevMessage;
          MessageBox(pSNMPMessage->m_szMessage, "VLWNMP", 0);
          }
        else
          {
          MessageBox("No message available", "VLWNMP", 0);
          }
                        
        ResetEvent(g_hSNMPTrapAvailable);
        ReleaseMutex(g_hSNMPSharedMemMutex);
        }
      UnmapViewOfFile(pSharedMem);
      }
  
    return;
}


void CCrashMeDlg::OnTimeConvert() 
{
  UpdateData(TRUE);

  CTime cTime = m_lTimeC;
  m_csTimeText = cTime.Format("%Y-%m-%d %H:%M:%S");

  UpdateData(FALSE);
}

void CCrashMeDlg::OnTimeConvert2() 
{
  UpdateData(TRUE);

  // 0123456789012345678
  // 2007-04-30 00:00:00

  if (m_csTimeText.GetLength() != 19)
    MessageBox("Length of DateTime string should be 19 characters\n(Format expected: '2007-04-30 00:00:00')");
  else if (m_csTimeText[4] != '-' && m_csTimeText[4] != '/')
    MessageBox("Character 5 of DateTime string should be '-' or '/'\n(Format expected: '2007-04-30 00:00:00')");
  else if (m_csTimeText[7] != '-' && m_csTimeText[7] != '/')
    MessageBox("Character 8 of DateTime string should be '-' or '/'\n(Format expected: '2007-04-30 00:00:00')");
  else if (m_csTimeText[10] != ' ')
    MessageBox("Character 11 of DateTime string should be ' '\n(Format expected: '2007-04-30 00:00:00')");
  else if (m_csTimeText[13] != ':')
    MessageBox("Character 14 of DateTime string should be ':'\n(Format expected: '2007-04-30 00:00:00')");
  else if (m_csTimeText[16] != ':')
    MessageBox("Character 17 of DateTime string should be ':'\n(Format expected: '2007-04-30 00:00:00')");
  else
  {
    int iYear = atoi(m_csTimeText.Mid(0, 4));
    int iMonth = atoi(m_csTimeText.Mid(5, 2));
    int iDay = atoi(m_csTimeText.Mid(8, 2));
    int iHour = atoi(m_csTimeText.Mid(11, 2));
    int iMinute = atoi(m_csTimeText.Mid(14, 2));
    int iSecond = atoi(m_csTimeText.Mid(17, 2));

    CTime cTime(iYear, iMonth, iDay, iHour, iMinute, iSecond);
    m_lTimeC = (long)cTime.GetTime();
  }

  UpdateData(FALSE);
}

int MyErrorFunc(int errorType, const char *filename, int linenumber, const char *moduleName, const char *format, ...)
{
  // Prevent re-entrancy
  static long running = 0;
  while (InterlockedExchange(&running, 1))
    Sleep(0);
  // Now, disable all RTC failures
  int numErrors = _RTC_NumErrors();
  int *errors=(int*)_alloca(numErrors);
  for (int i = 0; i < numErrors; i++)
  errors[i] = _RTC_SetErrorType((_RTC_ErrorNumber)i, _RTC_ERRTYPE_IGNORE);

  // First, let's get the rtc error number from the var-arg list...
  va_list vl;
  va_start(vl, format);
  _RTC_ErrorNumber rtc_errnum = va_arg(vl, _RTC_ErrorNumber);
  va_end(vl);

  char buf[512];
  const char *err = _RTC_GetErrDesc(rtc_errnum);
  _snprintf_s(buf, sizeof(buf), "%s\nLine #%d\nFile:%s\nModule:%s",
    err,
    linenumber,
    filename ? filename : "Unknown",
    moduleName ? moduleName : "Unknown");
  int res = (MessageBox(NULL, buf, "RTC Failed...", MB_YESNO) == IDYES) ? 1 : 0;
  // Now, restore the RTC errortypes
  for(int i = 0; i < numErrors; i++)
    _RTC_SetErrorType((_RTC_ErrorNumber)i, errors[i]);
  running = 0;
  return res;
}

int MyReportHook(int reportType, char *message, int *returnValue)
{
  MessageBox(NULL, message, "", MB_OK);
  DebugBreak();
  return TRUE;
}

void CCrashMeDlg::OnBnClickedAssertdump()
{
  //_RTC_Initialize();
  _CRT_REPORT_HOOK fnOldFunction = _CrtSetReportHook(MyReportHook);
  ASSERT(FALSE);
  _CrtSetReportHook(fnOldFunction);
}

void CCrashMeDlg::OnBnClickedSeterrormode()
{
  // SEM_NOGPFAULTERRORBOX 0x0002
  // The system does not display the general-protection-fault message box.
  // This flag should only be set by debugging applications that handle general protection (GP) faults themselves 
  // with an exception handler.
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOALIGNMENTFAULTEXCEPT | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);

  // result: dialog box "Runtime Error" still displayed, but debugger (WinDbg, drwtsn32) is not called anymore!
}

void CCrashMeDlg::OnBnClickedInstallbugtrap()
{
  BT_InstallSehFilter();
  BT_SetAppName("crashme.exe");
  BT_SetAppVersion("1.0.0.1");
  BT_SetFlags(BTF_DETAILEDMODE);
  BT_SetActivityType(BTA_SAVEREPORT);
  BT_SetReportFilePath(".");    // required
  BT_SetDumpType(MiniDumpWithFullMemory | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo);
  BT_SetReportFormat(BTRF_TEXT);
}

void CCrashMeDlg::OnBnClickedThrowbadalloc()
{
  throw bad_alloc();
}
