#pragma once

// Generic container that holds information 
// for each "button".
struct GENBTNDATA
{
    wchar_t     szText[100];
    wchar_t*    pszAccDesc; // optional description element for accessibility.
    wchar_t*    pszAccName; // optional accessible name element for accessibility.
    BOOL        bPushed;
    BOOL        bHasKbFocus;
    RECT        rcBounds;
};

/**
* This structure holds on to the data that determines the information
* for a given instance of an ugly button.
*/
typedef struct _UGLYBUTTONDATA
{
    HWND        hWnd;
    GENBTNDATA  btnOne;
    GENBTNDATA  btnTwo;
    GENBTNDATA  btnSelf;

} UGLYBUTTONDATA, *LPUGLYBUTTONDATA;

/**
 * Registers our ugly button window class so it can be used
 * in our main window.
 *
 * @param hInstance - module instance handle
 * @return - atom representation of our wnd class name.
 */
ATOM RegisterUglyButtonClass(
    HINSTANCE hInstance
);

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
BOOL SetUglyBtnText(
    HWND    hWnd,
    LPCWSTR lpText,
    LPCWSTR lpDesc,
    UINT    nIndex
);