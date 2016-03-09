
#include "stdafx.h"

#include <windows.h>
#include <windowsx.h>
#include <strsafe.h>

#include "UglyButton.h"
#include "AccProxyObj.h"
#include "resource.h"

// Yes, yes i know i am tructating pointers :(
#pragma warning (disable: 4311)

// this is used to reference our button data as stored
// it is stored as a property of the window.
ATOM UBPROPATOM_DATA = 0;
ATOM UBPROPATOM_CACC = 0;

wchar_t szUglyClass[100]; // our ugly button class name

//////////////////////////////////////////////////////////////
// MESSAGE HANDLERS

// WndProc
LRESULT CALLBACK UglyButtonWndProc(
    HWND    hWnd, 
    UINT    nMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
);

// WM_LBUTTONUP
VOID OnLButtonUp(
    HWND    hWnd, 
    INT     x, 
    INT     y
);

// WM_LBUTTONDOWN
VOID OnLButtonDown(
    HWND    hWnd, 
    INT     x, 
    INT     y
);

// WM_PAINT
VOID OnPaint(
    HWND            hWnd, 
    LPPAINTSTRUCT   lpPaint
);

// WM_CREATE
VOID OnCreate(
    HWND            hWnd,
    LPCREATESTRUCT  lpcs
);

// WM_DESTROY
VOID OnDestroy(HWND hWnd);

// WM_KEYDOWN
BOOL OnKeyDown(
    HWND    hWnd,
    DWORD   wVirKey
);

// WM_KEYUP
BOOL OnKeyUp(
    HWND    hWnd,
    DWORD   wVirKey
);

// ************************************************************************************************

/**
 * Registers our ugly button window class so it can be used
 * in our main window.
 *
 * @param hInstance - module instance handle
 * @return - atom representation of our wnd class name.
 */
ATOM RegisterUglyButtonClass(HINSTANCE hInstance)
{
    WNDCLASS wndCls;
    LoadString(hInstance, IDC_UGBTNCLASS, szUglyClass, 100);
    ATOM retAtom = 0;

    if (!GetClassInfo(hInstance, szUglyClass, &wndCls))
    {
        ZeroMemory(&wndCls, sizeof(WNDCLASS));

        wndCls.style            = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
        wndCls.lpfnWndProc      = (WNDPROC)UglyButtonWndProc;
        wndCls.cbClsExtra       = 0;
        wndCls.cbWndExtra       = 0;
        wndCls.hInstance        = hInstance;
        wndCls.hIcon            = NULL;
        wndCls.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wndCls.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wndCls.lpszMenuName     = NULL;
        wndCls.lpszClassName    = szUglyClass;

        retAtom = RegisterClass(&wndCls);

        UBPROPATOM_DATA = GlobalAddAtom(_T("UGLY_BUTTON_DATA_PROP"));
        UBPROPATOM_CACC = GlobalAddAtom(_T("UGLY_BUTTON_CACC_PROP"));
    }

    return retAtom;
}

// ************************************************************************************************

/**
 * Sets the text for a given ugly button. 
 *
 * @param hWnd - the window to be updated.
 * @param lpText - the new text value.
 * @param lpDesc - desciptive text for accessibility.
 * @param nIndex - the index of the button. 
 *                  0 = outter button.
 *                  1 = first inner button.
 *                  2 = second inner button.
 * @return - TRUE if updated, else false.
 */
BOOL SetUglyBtnText(HWND hWnd, LPCWSTR lpText, LPCWSTR lpDesc, UINT nIndex)
{
    BOOL bUpdated = FALSE;
    RECT rcUpdate;
    LPUGLYBUTTONDATA lpData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));

    if (lpData)
    {
        GENBTNDATA* pBtnData = NULL;
        switch(nIndex)
        {
            case 0:
                pBtnData = &lpData->btnSelf;
                GetClientRect(hWnd, &rcUpdate);
                bUpdated = TRUE;
                break;
            case 1:
                pBtnData = &lpData->btnOne;
                rcUpdate = lpData->btnOne.rcBounds;
                bUpdated = TRUE;
                break;
            case 2:   
                pBtnData = &lpData->btnTwo;
                rcUpdate = lpData->btnTwo.rcBounds;
                bUpdated = TRUE;
                break;
        };

        if (pBtnData)
        {
            if (!lpText)
                ZeroMemory(pBtnData->szText, 100 * sizeof(wchar_t));
            else
                StringCchCopy(pBtnData->szText, 100, lpText);

            // This block of code will handle the accessible description 
            // buffer allocation / deallocated. Since most of the time
            // we probably don't have this we don't keep a static buffer
            // around eating up memory.

            if (pBtnData->pszAccDesc)
            {
                delete [] pBtnData->pszAccDesc;
                pBtnData->pszAccDesc = NULL;
            }

            if (lpDesc)
            {
                UINT nLen = 0;
                StringCchLength(lpDesc, STRSAFE_MAX_CCH, &nLen);
                pBtnData->pszAccDesc = new wchar_t[nLen + 1];
                ZeroMemory(pBtnData->pszAccDesc, (nLen + 1) * sizeof(wchar_t));
                StringCchCopy(pBtnData->pszAccDesc, STRSAFE_MAX_CCH, lpDesc);
            }                

            NotifyWinEvent(EVENT_OBJECT_DESCRIPTIONCHANGE, hWnd, (LONG)pBtnData, nIndex);
        }
    }

    if (bUpdated)
        InvalidateRect(hWnd, &rcUpdate, TRUE);

    return bUpdated;
}

// ************************************************************************************************

/**
 * Message handler function for our ugly button. 
 *
 * @param hWnd - handle to our button control.
 * @param nMsg - the message to be processed.
 * @param wParam - message specific
 * @param lParam - message specific
 * @return - 0 if processed, else 1.
 */
LRESULT CALLBACK UglyButtonWndProc(HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{	
    switch(nMsg)
    {
        case WM_CREATE:
            OnCreate(hWnd, (LPCREATESTRUCT)lParam);
            break;
        case WM_DESTROY:
            OnDestroy(hWnd);
            break;
        case WM_PAINT:
            {
                PAINTSTRUCT ps;
                ZeroMemory(&ps, sizeof(PAINTSTRUCT));
                BeginPaint(hWnd, &ps);
                OnPaint(hWnd, &ps);
                EndPaint(hWnd, &ps);
            }
            break;
        case WM_LBUTTONDOWN:
            OnLButtonDown(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        case WM_LBUTTONUP:
            OnLButtonUp(hWnd, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            break;
        case WM_KEYUP:
            {
                BOOL bHandled = OnKeyUp(hWnd, (DWORD)wParam);
                if (!bHandled)
                    return DefWindowProc(hWnd, nMsg, wParam, lParam);
            }
            break;
        case WM_KEYDOWN:
            {
                BOOL bHandled = OnKeyDown(hWnd, (DWORD)wParam);
                if (!bHandled)
                    return DefWindowProc(hWnd, nMsg, wParam, lParam);
            }
            break;
        case WM_SETFOCUS:
            {
                LPUGLYBUTTONDATA pData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));
                if (pData)
                {
                    if (!pData->btnSelf.bHasKbFocus &&
                        !pData->btnOne.bHasKbFocus &&
                        !pData->btnTwo.bHasKbFocus)
                    {
                        pData->btnSelf.bHasKbFocus = TRUE;
                    }

                    RECT rc;
                    GetClientRect(hWnd, &rc);
                    InvalidateRect(hWnd, &rc, FALSE);
                    
                }
                NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, (LONG)&pData->btnSelf, CHILDID_SELF);
            }
            break;
        case WM_GETOBJECT:
            {
                // This is the message that we must handle so that we can return the
                // IAccessible pointer that relates to this control.
                CAccProxyObject* pProxy = (CAccProxyObject*)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_CACC));
                if (pProxy)
                {
                    LRESULT lRes = LresultFromObject(IID_IAccessible, wParam, static_cast<IAccessible*>(pProxy));
                    return lRes;
                }
            }
            break;
        default:
            return DefWindowProc(hWnd, nMsg, wParam, lParam);
    }
    return 0;	
}

// ************************************************************************************************

/**
 * Create handler for the UglyButton control. This handles setting up our
 * internal buttons as well as our data structures and accessibility proxies.
 *
 * @param hWnd - window handle to our button
 * @param lpcs - pointer to a creation data structure.
 * @return - none
 */
VOID OnCreate(HWND hWnd, LPCREATESTRUCT lpcs)
{
    UINT nInnerWidth    = 0;
    UINT nInnerHeight   = 0;
    CONST UINT nMidSep  = 10;
    LPRECT pBtn         = NULL;
    CAccProxyObject* pProxy = NULL;
    LPUGLYBUTTONDATA lpBtnData = new UGLYBUTTONDATA;
    ZeroMemory(lpBtnData, sizeof(UGLYBUTTONDATA));
    
    lpBtnData->hWnd = hWnd;

    // Setup outter button 
    pBtn = &lpBtnData->btnSelf.rcBounds;
    SetRect(pBtn,
        lpcs->x, lpcs->y,
        lpcs->x + lpcs->cx,
        lpcs->y + lpcs->cy);
    lpBtnData->btnSelf.bPushed = FALSE;

    // Inner buttons Dimensions
    // Height = 1/4 of Outter Button
    // Width = 1/3 of Outter Button
    nInnerWidth = (UINT)(lpcs->cx / 3);
    nInnerHeight = (UINT)(lpcs->cy / 4);                

    // Setup first inner button 
    pBtn = &lpBtnData->btnOne.rcBounds;
    pBtn->right = (UINT)(lpcs->cx / 2) - (nMidSep  / 2);
    pBtn->left  = pBtn->right - nInnerWidth;
    pBtn->top   = (UINT)(lpcs->cy / 2) - (UINT)(nInnerHeight / 2);
    pBtn->bottom= pBtn->top + nInnerHeight;
    lpBtnData->btnOne.bPushed = FALSE;

    // Setup second inner button 
    pBtn = &lpBtnData->btnTwo.rcBounds;
    pBtn->left  = lpBtnData->btnOne.rcBounds.right + nMidSep;
    pBtn->right = pBtn->left + nInnerWidth;
    pBtn->top   = lpBtnData->btnOne.rcBounds.top;
    pBtn->bottom= lpBtnData->btnOne.rcBounds.bottom;
    lpBtnData->btnTwo.bPushed = FALSE;

    // store our data for this button as one of the window properties.
    SetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA), lpBtnData);    

    pProxy = new CAccProxyObject(lpBtnData);
    pProxy->AddRef(); // make sure we always have a copy.

    SetProp(hWnd, MAKEINTATOM(UBPROPATOM_CACC), pProxy);

    // This is done is reverse order as per-msdn instructions. THis should be send for child items
    // before the parent item.
    NotifyWinEvent(EVENT_OBJECT_CREATE, hWnd, OBJID_WINDOW, 2);
    NotifyWinEvent(EVENT_OBJECT_CREATE, hWnd, OBJID_WINDOW, 1);
    NotifyWinEvent(EVENT_OBJECT_CREATE, hWnd, OBJID_WINDOW, 0);    

    // END OF FUNCTION
}

// ************************************************************************************************

/**
 * Handles the destruction message for a given ugly button control. This handles memory 
 * cleanup as well as detaching this window from its accessible proxy.
 *
 * @param hWnd - handle to the window being destroyed.
 * @return - none 
 */
VOID OnDestroy(HWND hWnd)
{
    LPUGLYBUTTONDATA lpData = NULL;
    lpData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));
    
    NotifyWinEvent(EVENT_OBJECT_DESTROY, hWnd, OBJID_WINDOW, 0);    

    CAccProxyObject* pProxy = (CAccProxyObject*)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_CACC));
    if (pProxy)
    {
        pProxy->NullControlData();
        pProxy->Release();
    }

    if (lpData)
    {
        SetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA), NULL);

        if (lpData->btnSelf.pszAccDesc)
            delete [] lpData->btnSelf.pszAccDesc;
        if (lpData->btnSelf.pszAccName)
            delete [] lpData->btnSelf.pszAccName;

        if (lpData->btnOne.pszAccDesc)
            delete [] lpData->btnOne.pszAccDesc;
        if (lpData->btnOne.pszAccName)
            delete [] lpData->btnOne.pszAccName;

        if (lpData->btnTwo.pszAccDesc)
            delete [] lpData->btnTwo.pszAccDesc;
        if (lpData->btnTwo.pszAccName)
            delete [] lpData->btnTwo.pszAccName;

        delete lpData;
        lpData = NULL;
    }

    // END OF FUNCTION
}

// ************************************************************************************************

/**
 * Message handler for when a left button mouse up message is detected
 * on the ugly button control. This handler takes care of changing states
 * of the button.
 *
 * @param hWnd - handle to the button that got clicked.
 * @param x - x-coord of the mouse click
 * @param y - y-coord of the mouse click
 * @return - none
 */
VOID OnLButtonUp(HWND hWnd, INT x, INT y)
{
    LPUGLYBUTTONDATA lpData = NULL;
    lpData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));
    WORD wCtrlID = 0;

    POINT point = {x, y};
    if (lpData)
    {
        if (PtInRect(&lpData->btnOne.rcBounds, point))
        {
            lpData->btnOne.bPushed = FALSE;
            InvalidateRect(hWnd, &lpData->btnOne.rcBounds, TRUE);
            wCtrlID = 1;
            
            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hWnd, (LONG)&lpData->btnOne, 1);
        }
        else if(PtInRect(&lpData->btnTwo.rcBounds, point))
        {
            lpData->btnTwo.bPushed = FALSE;
            InvalidateRect(hWnd, &lpData->btnTwo.rcBounds, TRUE);
            wCtrlID = 2;

            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hWnd, (LONG)&lpData->btnTwo, 2);
        }
        else
        {
            lpData->btnSelf.bPushed = FALSE;
            RECT rc;
            GetClientRect(hWnd, &rc);
            InvalidateRect(hWnd, &rc, TRUE);
            wCtrlID = 0;

            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hWnd, (LONG)&lpData->btnSelf, CHILDID_SELF);
        }

        // notify our parent window that a button has been clicked.        
        SendMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(wCtrlID, BN_CLICKED), (LPARAM)hWnd);
    }

    // END OF FUNCTION
}

// ************************************************************************************************

/**
 * Message handler for when a left button mouse up message is detected
 * on the ugly button control. This handler takes care of changing states
 * of the button.
 *
 * @param hWnd - handle to the button that got clicked.
 * @param x - x-coord of the mouse click
 * @param y - y-coord of the mouse click
 * @return - none
 */
VOID OnLButtonDown(HWND hWnd, INT x, INT y)
{
    LPUGLYBUTTONDATA lpData = NULL;
    lpData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));

    POINT point = {x, y};
    if (lpData)
    {
        if (PtInRect(&lpData->btnOne.rcBounds, point))
        {
            lpData->btnOne.bPushed = TRUE;
            lpData->btnOne.bHasKbFocus = TRUE;
            lpData->btnTwo.bHasKbFocus = FALSE;            
            lpData->btnSelf.bHasKbFocus = FALSE;            
            
            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hWnd, (LONG)&lpData->btnOne, 1);

        }
        else if(PtInRect(&lpData->btnTwo.rcBounds, point))
        {
            lpData->btnTwo.bPushed = TRUE;            
            lpData->btnOne.bHasKbFocus = FALSE;
            lpData->btnTwo.bHasKbFocus = TRUE;
            lpData->btnSelf.bHasKbFocus = FALSE;            

            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hWnd, (LONG)&lpData->btnTwo, 2);
        }
        else
        {
            lpData->btnSelf.bPushed = TRUE;
            lpData->btnOne.bHasKbFocus = FALSE;
            lpData->btnTwo.bHasKbFocus = FALSE;
            lpData->btnSelf.bHasKbFocus = TRUE;            

            NotifyWinEvent(EVENT_OBJECT_STATECHANGE, hWnd, (LONG)&lpData->btnSelf, CHILDID_SELF);
        }   

        RECT rc;
        GetClientRect(hWnd, &rc);
        InvalidateRect(hWnd, &rc, TRUE);           
    }

    // END OF FUNCTION
}

// ************************************************************************************************

/**
 * Paint handler for the ugly button control.
 *
 * @param hWnd - handle to the button we are painting.
 * @param lpPaint - pointer to a paint struct that contains our drawing data including device context.
 * @return - none
 */
VOID OnPaint(HWND hWnd, LPPAINTSTRUCT lpPaint)
{
    LPUGLYBUTTONDATA lpData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));

    RECT rc;
    RECT rcWnd;
    RECT rcText;  
    RECT rcFocus;

    GetClientRect(hWnd, &rcWnd);

    HDC hMemDC = CreateCompatibleDC(lpPaint->hdc);
    HBITMAP hMemBmp = CreateCompatibleBitmap(lpPaint->hdc, rcWnd.right - rcWnd.left, rcWnd.bottom - rcWnd.top);
    HBITMAP hOld = (HBITMAP)SelectObject(hMemDC, hMemBmp);

    rc.top = rc.left = 0;
    rc.bottom = rcWnd.bottom - rcWnd.top;
    rc.right = rcWnd.right - rcWnd.left;

    COLORREF clrOld = GetBkColor(hMemDC);
    COLORREF clrBack = RGB(120, 120, 120);
    
    SetBkColor(hMemDC, clrBack);   
    ExtTextOut(hMemDC, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    SetBkColor(hMemDC, clrOld);

    SetBkMode(hMemDC, TRANSPARENT);
    DrawFrameControl(
        hMemDC, 
        &rc, 
        DFC_BUTTON, 
        DFCS_BUTTONPUSH | (lpData->btnSelf.bPushed ? DFCS_PUSHED : 0));

    if (lpData->btnSelf.bHasKbFocus)
    {
        rcFocus = rc;
        InflateRect(&rcFocus, -5, -5);
        DrawFocusRect(hMemDC, &rcFocus);
    }
    
    if (wcscmp(lpData->btnSelf.szText, _T("")) != 0)
    {
        UINT nStrLen = 0;
        StringCchLength(lpData->btnSelf.szText, 100, &nStrLen);
        rcText = rc;
        InflateRect(&rcText, -5, -5);
        rcText.bottom = lpData->btnOne.rcBounds.top - 5;  
        DrawText(hMemDC, lpData->btnSelf.szText, nStrLen, &rcText, DT_TOP | DT_CENTER);
    }

    DrawFrameControl(
        hMemDC,
        &lpData->btnOne.rcBounds,
        DFC_BUTTON, 
        DFCS_BUTTONPUSH | (lpData->btnOne.bPushed ? DFCS_PUSHED : 0));

    if (lpData->btnOne.bHasKbFocus)
    {
        rcFocus = lpData->btnOne.rcBounds;
        InflateRect(&rcFocus, -5, -5);
        DrawFocusRect(hMemDC, &rcFocus);
    }

    if (wcscmp(lpData->btnOne.szText, _T("")) != 0)
    {
        UINT nStrLen = 0;
        StringCchLength(lpData->btnOne.szText, 100, &nStrLen);
        rcText = lpData->btnOne.rcBounds;
        InflateRect(&rcText, -5, -5);
        DrawText(hMemDC, lpData->btnOne.szText, nStrLen, &rcText, DT_TOP | DT_CENTER | DT_VCENTER);
    }

    DrawFrameControl(
        hMemDC,
        &lpData->btnTwo.rcBounds,
        DFC_BUTTON, 
        DFCS_BUTTONPUSH | (lpData->btnTwo.bPushed ? DFCS_PUSHED : 0));

    if (lpData->btnTwo.bHasKbFocus)
    {
        rcFocus = lpData->btnTwo.rcBounds;
        InflateRect(&rcFocus, -5, -5);
        DrawFocusRect(hMemDC, &rcFocus);
    }

    if (wcscmp(lpData->btnTwo.szText, _T("")) != 0)
    {
        UINT nStrLen = 0;
        StringCchLength(lpData->btnTwo.szText, 100, &nStrLen);
        rcText = lpData->btnTwo.rcBounds;
        InflateRect(&rcText, -5, -5);
        DrawText(hMemDC, lpData->btnTwo.szText, nStrLen, &rcText, DT_TOP | DT_CENTER | DT_VCENTER);
    }

    BitBlt(lpPaint->hdc, 
        rcWnd.left, 
        rcWnd.top, 
        rcWnd.right - rcWnd.left,
        rcWnd.bottom - rcWnd.top,
        hMemDC,
        0, 
        0,
        SRCCOPY);

    SelectObject(hMemDC, hOld);
    DeleteObject(hMemBmp);
    DeleteDC(hMemDC);

    // END OF FUNCTION
}

// ************************************************************************************************

/**
 * Key down message handler. We use this to adjust focus within our control.
 * We handle the tab key to move focus to our internal buttons and the space key 
 * to activate our buttons.
 *
 * @param hWnd - handle to our button control.
 * @param wVirKey - virtual key code of the key that generated the event.
 * @return - true if handled, else false.
 */
BOOL OnKeyDown(HWND hWnd, DWORD wVirKey)
{
    BOOL bHandled = FALSE;    
    LPUGLYBUTTONDATA lpData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));

    if (lpData)
    {
        bHandled = TRUE;
        if (wVirKey == VK_TAB)
        {    
            if (GetKeyState(VK_SHIFT) < 0)
            {
                if (lpData->btnSelf.bHasKbFocus)
                {
                    lpData->btnSelf.bHasKbFocus = FALSE;
                    bHandled = FALSE;
                }
                else if (lpData->btnOne.bHasKbFocus)
                {
                    lpData->btnOne.bHasKbFocus = FALSE;
                    lpData->btnSelf.bHasKbFocus = TRUE;
                }
                else
                {
                    lpData->btnTwo.bHasKbFocus = FALSE;
                    lpData->btnOne.bHasKbFocus = TRUE;
                }
            }
            else
            {
                if (lpData->btnSelf.bHasKbFocus)
                {
                    lpData->btnSelf.bHasKbFocus = FALSE;
                    lpData->btnOne.bHasKbFocus = TRUE;
                    NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, (LONG)&lpData->btnOne, 1);
                }
                else if (lpData->btnOne.bHasKbFocus)
                {
                    lpData->btnOne.bHasKbFocus = FALSE;
                    lpData->btnTwo.bHasKbFocus = TRUE;
                    NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, (LONG)&lpData->btnTwo, 2);
                }
                else
                {
                    lpData->btnTwo.bHasKbFocus = FALSE;
                    bHandled = FALSE;
                    NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, (LONG)&lpData->btnSelf, CHILDID_SELF);
                }
            }
        }
        else if (wVirKey == VK_SPACE)
        {
            if (lpData->btnSelf.bHasKbFocus)
            {
                lpData->btnSelf.bPushed = TRUE;
                NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, (LONG)&lpData->btnSelf, CHILDID_SELF);
            }
            else if (lpData->btnOne.bHasKbFocus)
            {
                lpData->btnOne.bPushed = TRUE;
                NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, (LONG)&lpData->btnOne, 1);
            }
            else if (lpData->btnTwo.bHasKbFocus)
            {
                lpData->btnTwo.bPushed = TRUE;
                NotifyWinEvent(EVENT_OBJECT_FOCUS, hWnd, (LONG)&lpData->btnTwo, 2);
            }
            else
                bHandled = FALSE;
        }

        if (bHandled)
        {
            RECT rcWnd;
            GetClientRect(hWnd, &rcWnd);
            InvalidateRect(hWnd, &rcWnd, FALSE);
        }
    }

    return bHandled;
}

// ************************************************************************************************

/**
 * Handles the key up message. We only need this to handle the spacebar coming up at which
 * time we need to release the button.
 *
 * @param hWnd - handle to our control window.
 * @param wVirKey - virtual key code of the key that generated the event.
 * @return - true if handled, else false.
 */
BOOL OnKeyUp(HWND hWnd, DWORD wVirKey)
{
    BOOL bHandled = FALSE;
    LPUGLYBUTTONDATA lpData = (LPUGLYBUTTONDATA)GetProp(hWnd, MAKEINTATOM(UBPROPATOM_DATA));

    if (lpData && (wVirKey == VK_SPACE))
    {
        bHandled = TRUE;
        WORD wCtrlID = -1;
        if (lpData->btnSelf.bHasKbFocus)
        {
            lpData->btnSelf.bPushed = FALSE;
            wCtrlID = 0;
        }
        else if (lpData->btnOne.bHasKbFocus)
        {
            lpData->btnOne.bPushed = FALSE;
            wCtrlID = 1;
        }
        else if (lpData->btnTwo.bHasKbFocus)
        {
            lpData->btnTwo.bPushed = FALSE;
            wCtrlID = 2;
        }
        else
            bHandled = FALSE;

        if (wCtrlID != -1)
            PostMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(wCtrlID, BN_CLICKED), (LPARAM)hWnd);

        RECT rcWnd;
        GetClientRect(hWnd, &rcWnd);
        InvalidateRect(hWnd, &rcWnd, FALSE);
    }

    return bHandled;
}