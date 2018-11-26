// WRGuard.h: interface for the CWRGuard class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WRGUARD_H__5FDE1951_C178_11D3_BD36_0080C88C130F__INCLUDED_)
#define AFX_WRGUARD_H__5FDE1951_C178_11D3_BD36_0080C88C130F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CWRGuard  
{
	HANDLE		m_hNoWriter;		// auto reset event
	HANDLE		m_hNoReader;		// manual reset event
	long		m_lReaderCount;
public:
	CWRGuard();
	virtual ~CWRGuard();

	DWORD		BeginWrite(DWORD);
	void		EndWrite();

	DWORD		BeginRead(DWORD);
	void		EndRead();
};

#endif // !defined(AFX_WRGUARD_H__5FDE1951_C178_11D3_BD36_0080C88C130F__INCLUDED_)
