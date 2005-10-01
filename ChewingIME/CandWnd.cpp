#include "candwnd.h"
#include "ChewingIME.h"
#include "DrawUtil.h"
#include "CandList.h"
#include "imm.h"
#include ".\candwnd.h"
#include <tchar.h>

static CandWnd* g_thisCandWnd = NULL;
CandWnd::CandWnd( HWND imeUIWnd ) : IMEWnd(imeUIWnd, g_cand_wnd_class)
{
	g_thisCandWnd = this;
	font = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
}

CandWnd::~CandWnd(void)
{
}


BOOL CandWnd::RegisterClass(void)
{
	WNDCLASSEX wc;
	wc.cbSize         = sizeof(WNDCLASSEX);
	wc.style          = CS_IME;
	wc.lpfnWndProc    = (WNDPROC)CandWnd::WndProc;
	wc.cbClsExtra     = 0;
	wc.cbWndExtra     = 0;
	wc.hInstance      = g_dllInst;
	wc.hCursor        = LoadCursor( NULL, IDC_ARROW );
	wc.hIcon          = NULL;
	wc.lpszMenuName   = (LPTSTR)NULL;
	wc.lpszClassName  = g_cand_wnd_class;
	wc.hbrBackground  = NULL;
	wc.hIconSm        = NULL;

	if( !RegisterClassEx( (LPWNDCLASSEX)&wc ) )
		return FALSE;

	return TRUE;
}

LRESULT CandWnd::WndProc(HWND hwnd , UINT msg, WPARAM wp , LPARAM lp)
{
	switch (msg)
	{
		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				BeginPaint( hwnd, &ps );
				g_thisCandWnd->OnPaint(ps);
				EndPaint(hwnd, &ps);
				break;
			}
		case WM_ERASEBKGND:
			return TRUE;
			break;
		case WM_LBUTTONDOWN:
			g_thisCandWnd->OnLButtonDown(wp, lp);
			break;
		case WM_MOUSEMOVE:
			g_thisCandWnd->OnMouseMove(wp, lp);
			break;
		case WM_LBUTTONUP:
			g_thisCandWnd->OnLButtonUp(wp, lp);
			break;
		case WM_MOUSEACTIVATE:
			return MA_NOACTIVATE;
		default:
			if (!IsImeMessage(msg))
				return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 0;
}

void CandWnd::OnPaint(PAINTSTRUCT& ps)
{
	HIMC hIMC = getIMC();
	INPUTCONTEXT* ic = ImmLockIMC(hIMC);
	CandList* candList = (CandList*)ImmLockIMCC(ic->hCandInfo);

// Begin paint

	HDC hDC = ps.hdc;
	HFONT oldFont;
	RECT rc;
	DWORD i;

	oldFont = (HFONT)SelectObject(hDC, font);

	GetClientRect(hwnd,&rc);

	SetTextColor( hDC, GetSysColor( COLOR_WINDOWTEXT ) );
	SetBkColor( hDC, GetSysColor( COLOR_WINDOW ) );

	int items_per_row =  g_candPerRow;

	RECT cand_rc;	cand_rc.left = 1;	cand_rc.top = 1;	cand_rc.right = 1;

	int pageEnd = candList->getPageStart() + candList->getPageSize();
	if( pageEnd > candList->getTotalCount() )
		pageEnd = candList->getTotalCount();
	int numCand = pageEnd - candList->getPageStart();
	int num = 0;
	for( int i = candList->getPageStart(); i < pageEnd; ++i )
	{
		++num;

		TCHAR cand[64];
		wsprintf ( cand, _T("%d.%s"), (i - candList->getPageStart() + 1), candList->getCand(i) );

		int len = _tcslen( cand );
		SIZE candsz;
		GetTextExtentPoint32(hDC, cand, len, &candsz);
		candsz.cx += 4;
		candsz.cy += 2;

		cand_rc.right = cand_rc.left + candsz.cx;
		cand_rc.bottom = cand_rc.top + candsz.cy;

		ExtTextOut( hDC, cand_rc.left + 2, cand_rc.top, ETO_OPAQUE, &cand_rc, cand, 
			len, NULL);

		if( num >= items_per_row && (i + 1) <= pageEnd )
		{
			cand_rc.left = 1;
			cand_rc.top += candsz.cy;
			num = 0;
		}
		else
			cand_rc.left = cand_rc.right;

	}

	cand_rc.left = cand_rc.right;
	cand_rc.right = rc.right;
	ExtTextOut( hDC, cand_rc.left, cand_rc.top, ETO_OPAQUE, &cand_rc, NULL, 0, NULL);

	Draw3DBorder( hDC, &rc, GetSysColor(COLOR_3DFACE), 0);
	SelectObject( hDC, oldFont );

// End paint

	ImmUnlockIMCC(ic->hCandInfo);
	ImmUnlockIMC(hIMC);
}

void CandWnd::getSize(int* w, int* h)
{
	*w = 0; *h = 0;
	HIMC hIMC = getIMC();
	INPUTCONTEXT* ic = ImmLockIMC(hIMC);
	CandList* candList = (CandList*)ImmLockIMCC(ic->hCandInfo);

	HDC hDC = GetDC(hwnd);
	HFONT oldFont;
	RECT rc;
	DWORD i;

	oldFont = (HFONT)SelectObject(hDC, font);

	int items_per_row =  g_candPerRow;

	int pageEnd = candList->getPageStart() + candList->getPageSize();
	if( pageEnd > candList->getTotalCount() )
		pageEnd = candList->getTotalCount();
	int numCand = pageEnd - candList->getPageStart();
	int num = 0;
	int width = 0, height = 0;
	for( int i = candList->getPageStart(); i < pageEnd; ++i )
	{
		++num;
		TCHAR cand[64];
		wsprintf ( cand, _T("%d.%s"), (i - candList->getPageStart() + 1), candList->getCand(i) );
		int len = _tcslen( cand );
		SIZE candsz;
		GetTextExtentPoint32(hDC, cand, len, &candsz);
		width += candsz.cx + 4;
		if( candsz.cy > height )
			height = candsz.cy;
        if( num >= items_per_row && (i + 1) <= pageEnd )
		{
			if( width > *w )
				*w = width;
			*h += height + 2;
			num = 0;
			width = 0;
		}
	}
	if( width > *w )
		*w = width;
	if( num > 0 && num < items_per_row )
		*h += height;

	SelectObject(hDC, oldFont );
// End paint

	ImmUnlockIMCC(ic->hCandInfo);
	ImmUnlockIMC(hIMC);

	ReleaseDC(hwnd, hDC);
	*w += 2;
	*h += 4;
}

void CandWnd::updateSize(void)
{
	int w, h;
	getSize(&w, &h);
	SetWindowPos( hwnd, NULL, 0, 0, w, h, SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOZORDER);
}