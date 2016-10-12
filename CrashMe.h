// CrashMe.h : main header file for the CRASHME application
//

#if !defined(AFX_CRASHME_H__CAE42F2D_70AC_4699_9834_A7687D53FCAB__INCLUDED_)
#define AFX_CRASHME_H__CAE42F2D_70AC_4699_9834_A7687D53FCAB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCrashMeApp:
// See CrashMe.cpp for the implementation of this class
//

class CCrashMeApp : public CWinApp
{
public:
	CCrashMeApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCrashMeApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCrashMeApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CRASHME_H__CAE42F2D_70AC_4699_9834_A7687D53FCAB__INCLUDED_)
