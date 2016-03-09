
#include "stdafx.h"
#include <strsafe.h>

#include "AccProxyObj.h"
#include "UglyButton.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif


// CO_E_OBJNOTCONNECTED is the error code that should always be returned in the case
// where the proxy accessible object still exists but the window and/or object it 
// references has been destroyed.
#define DATACHECK(pData) (pData) ? S_OK : CO_E_OBJNOTCONNECTED

// This just checks to make sure the child id passed into any of our functions
// is valid for an ugly button control.
#define VALID_CHILDID(varChild) ((varChild.vt == VT_I4) && (varChild.lVal >= CHILDID_SELF) && (varChild.lVal < 3))

// ************************************************************************************************

/**
 * Helper function to convert a rectangle from client coordinates to 
 * screen coordinates. Win32 API only provides the call for a point.
 *
 * @param hWnd - window that the rect is in the coordinate space of.
 * @param lpRect - contains the source coords in client space coming in and
 *                 then contains the coords in screen space going out.
 * @return - none
 */
static void ClientToScreen(HWND hWnd, LPRECT lpRect)
{
    if (lpRect)
    {
        POINT ptTL = {lpRect->left, lpRect->top};
        POINT ptBR = {lpRect->right, lpRect->bottom};
        
        ClientToScreen(hWnd, &ptTL);         
        ClientToScreen(hWnd, &ptBR); 

        SetRect(lpRect, ptTL.x, ptTL.y, ptBR.x, ptBR.y);
    }
    // END OF FUNCTION
}

// ************************************************************************************************

/**
 * Constructor. Initializes internal data members.
 */
CAccProxyObject::CAccProxyObject(LPUGLYBUTTONDATA pButtonData) : mRefCount(0), mData(pButtonData)
{
    // END OF FUNCTION
}

// ************************************************************************************************

/**
 * Destruction. Just generally turns out the lightes for us. 
 * @return - none
 */
CAccProxyObject::~CAccProxyObject()
{
    // just a saftey to let anyone know if they are manually deleteing this object
    // that they shouldn't be. Ref count will always be zero if managed by
    // IUnknown ref counting mechanism.
    _ASSERT(mRefCount == 0);

    // END OF FUNCTION
}

//////////////////////////////////////////////////////////////////////////
// Implement IUnknown
//  This is a base under IDispatch and handles ref counting and querying
//  for other supported interfaces. We only support, IUnknown, IDispatch
//  and IAccessible.
//////////////////////////////////////////////////////////////////////////

// ************************************************************************************************

/**
 * This function is used for interface discovery and acquisition. We only
 * implement IAccessible, IDispatch (not really, but we can provide the interface) 
 * and IUnknown.
 *
 * @param riid - the GUID of the interface the caller would like returned.
 * @param ppvObject - the storage location for the interface.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::QueryInterface(REFIID riid, void** ppvObject)
{
    if (riid == IID_IAccessible)
        *ppvObject = static_cast<IAccessible*>(this);
    else if (riid == IID_IDispatch) 
        *ppvObject = static_cast<IDispatch*>(this);
    else if (riid == IID_IUnknown)
        *ppvObject = static_cast<IUnknown*>(this);
    else
        *ppvObject = NULL;

    if (*ppvObject)
        reinterpret_cast<IUnknown*>(*ppvObject)->AddRef();

    return (*ppvObject) ? S_OK : E_NOINTERFACE;    
}

// ************************************************************************************************

/**
 * Increments the COM objects refcount. Used for lifetime
 * management.
 *
  * @return - new ref count.
 */
STDMETHODIMP_(ULONG) CAccProxyObject::AddRef()
{
    return InterlockedIncrement((LONG volatile*)&mRefCount);
}

// ************************************************************************************************

/**
 * IUnknown required function for reference counting. We delete
 * ourself upon the last release being made.
 *
  * @return - remaining refcount.
 */
STDMETHODIMP_(ULONG) CAccProxyObject::Release()
{
    ULONG ulRefCnt = InterlockedDecrement((LONG volatile*)&mRefCount);
    if (ulRefCnt == 0)
        delete this;

    return ulRefCnt;
}

//////////////////////////////////////////////////////////////////////////
// Implement IAccessible
//////////////////////////////////////////////////////////////////////////

// ************************************************************************************************

/**
 * Returns the parent IAccessible in the form of an IDispatch 
 * interface. Since our interfaces here are per-HWND we look
 * to our Parent window and give back that HWND.
 *
 * @param ppdispParent - storage location for the returned ptr.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accParent(IDispatch **ppdispParent)
{
    HRESULT retCode  = DATACHECK(mData);
    if (ppdispParent)
    {
        // Find our parent window
        HWND hWnd = ::GetParent(mData->hWnd);
        if (hWnd)
        {
            // if we have a window attempt to get its IAccessible
            // pointer if available.
            AccessibleObjectFromWindow(hWnd , (DWORD) OBJID_CLIENT,
                IID_IAccessible, (void**) ppdispParent);

            retCode  = (*ppdispParent) ? S_OK : S_FALSE;
        }
    }
    else
    {
        // no storage location
        retCode  = E_INVALIDARG;
    }
           

    return retCode ;
}

// ************************************************************************************************

/**
 * Returns the number of children we have for this control.
 * In our "uglybutton" case we know that this is always 2.
 * We have our button itself and its 2 inner buttons. 
 *
 * NOTE: If you were implementing a more complex control
 *       with a dynamic number of children a discovery 
 *       process would have to go here instead of hardcoding.
 *
 * @param pcountChildren - storage location for the child count.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accChildCount(long *pcountChildren)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pcountChildren)
            *pcountChildren = 2; // 2 inner buttons are our children.
        else            
            retCode = E_INVALIDARG; // no valid storage location, this is invalid.
    }

    return retCode;
}

// ************************************************************************************************

/**
 * Returns a child object. Since our button has a static number of children
 * there are 3 things we can return.
 *  varChild = CHILDID_SELF - returns the main button.
 *  varChild = 1            - no interface because this is a simple element.
 *  varChild = 2            - no interface because this is a simple element.
 *
 *
 * @param varChild - child id of the child to be returned.
 * @param ppdispChild - storage location for the interface.
 * @return - HRESULT error code,
 */
STDMETHODIMP CAccProxyObject::get_accChild(VARIANT varChild, IDispatch **ppdispChild)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (ppdispChild && VALID_CHILDID(varChild))
        {
            switch (varChild.lVal)
            {
                case CHILDID_SELF:            
                    *ppdispChild = static_cast<IDispatch*>(this);
                    break;
                case 1:
                case 2:
                    // We do not return an interface for our children elements
                    // since they are simple elements. They have no interface.
                    *ppdispChild = NULL;
                    retCode = S_FALSE;
                    break;
            };
        }
        else
            // if we don't have a integer type for the child id, we
            // don't have a place to store the interface or the child
            // id is out of bounds we need to return invalidarg.
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * Returns the accessible name for each part of the control. The accessible
 * name will be the text on each control. 
 *
 * NOTE: While we are using the text of the control it is NOT required. You
 * can use the name that you feel best describes your control to a blind
 * user. Just be careful to ensure that a sighted user and the blind user
 * are able to get the same information.
 *
 * @param varChild - the child of the item to be returned.
 * @param pszName - storage location for the string to be returned.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accName(VARIANT varChild, BSTR *pszName)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pszName && VALID_CHILDID(varChild))
        {
            GENBTNDATA* pData = NULL;
            switch (varChild.lVal)
            {
                case CHILDID_SELF:
                    pData = &mData->btnSelf;
                    break;
                case 1:
                    pData = &mData->btnOne;
                    break;
                case 2:
                    pData = &mData->btnTwo;
                    break;
            };
            
            if (pData)
            {
                // First preference goes to the set accessible name. If no accessible
                // name is available 
                if (pData->pszAccName)
                    *pszName = ::SysAllocString(pData->pszAccName);
                else if (wcscmp(L"", pData->szText) != 0)
                    *pszName = ::SysAllocString(pData->szText);
            }
        }
        else
        {
            retCode = E_INVALIDARG;
        }
    }

    return retCode;
}

// ************************************************************************************************

/**
 * Returns the value of our ugly button control or one of its
 * children. 
 *
 * NOTE: Buttons do NOT have values. Since our button can not 
 * have a value we just return null if everything else 
 * (data members and parmaters) checks out.
 *
 * @param varChild - child id of the item to get the value for.
 * @param pszValue - storage location for the value string.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accValue(VARIANT varChild, BSTR *pszValue)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pszValue && VALID_CHILDID(varChild))
            // Button objects to not have values. 
            *pszValue = NULL;
        else
            // Even though buttons do not actually have values, we still need
            // to be diligent in checking of parameters.
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * Returns the accessible description for our button. Descriptions of items
 * may not be visible to normal users at all. Instead, they detail the purpose
 * of the button for a blind user. 
 *
 * @param varChild - child id to get the description for.
 * @param pszDescription - storage location for the return string.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accDescription(VARIANT varChild, BSTR *pszDescription)
{
    HRESULT retCode = DATACHECK(mData);
    
    if (SUCCEEDED(retCode))
    {
        if (pszDescription && VALID_CHILDID(varChild))
        {
            GENBTNDATA* pData = NULL;
            switch (varChild.lVal)
            {
                case CHILDID_SELF:
                    pData = &mData->btnSelf;
                    break;
                case 1:
                    pData = &mData->btnOne;
                    break;
                case 2:
                    pData = &mData->btnTwo;
                    break;
            };

            if (pData && pData->pszAccDesc)
                *pszDescription = ::SysAllocString(pData->pszAccDesc);
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * Returns the role of our control. The role is what type of control are
 * custom control is similar to on the system. ie. list, button, tree, etc..
 * This is how ATs know how to represent the control to the user.
 *
 * @param varChild - id of the child we are to return the role for.
 * @param pvarRole - storage location for the role constant.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accRole(VARIANT varChild, VARIANT *pvarRole)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        // we don't have to do any work here because our button's two children
        // elements are also both buttons. Since this is the case we just return
        // ROLE_SYSTEM_PUSHBUTTON for all.
        if (pvarRole)
        {
            pvarRole->vt    = VT_I4;
            pvarRole->lVal  = ROLE_SYSTEM_PUSHBUTTON;
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This function returns the state of our button to the caller.
 * Our button can have states such as focused, focusable, disabled,
 * etc...
 *
 * @param varChild - child id of the item we are to retrieve the state for.
 * @param pvarState - place to store the state data.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accState(VARIANT varChild, VARIANT *pvarState)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pvarState)
        {
            GENBTNDATA* pData = NULL;
            switch (varChild.lVal)
            {
                case CHILDID_SELF:
                    pData = &mData->btnSelf;
                    break;
                case 1:
                    pData = &mData->btnOne;
                    break;
                case 2:
                    pData = &mData->btnTwo;
                    break;
            };


            pvarState->vt   = VT_I4;            
            pvarState->lVal = (GetFocus() == mData->hWnd) && pData->bHasKbFocus ? STATE_SYSTEM_FOCUSED : 0;
            pvarState->lVal |= (pData->bPushed) ? STATE_SYSTEM_PRESSED : 0;
            pvarState->lVal |= STATE_SYSTEM_FOCUSABLE;

            // these final two states are only possible on the main control itself. We did not build in complex
            // functionality into the subbuttons so they can not have these final 2 abilities.
            if (varChild.lVal == CHILDID_SELF)
            {
                DWORD dwStyle = GetWindowLong(mData->hWnd, GWL_STYLE);                
                pvarState->lVal |= ((dwStyle & WS_VISIBLE) == 0) ? STATE_SYSTEM_INVISIBLE : 0;                        
                pvarState->lVal |= ((dwStyle & WS_DISABLED) > 0) ? STATE_SYSTEM_UNAVAILABLE : 0;
            }
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This function returns the keyboard shortcut that can be used to activate
 * the control. Our simple button does not implmenet accelerators (because i'm lazy) 
 * so we just always return "None". In practice this should a) not happen and b) strings
 * should be localized.
 *
 * @param varChild - id of the child item to return the shortcut for.
 * @param pszKeyboardShortcut - place to store the shortcut string.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accKeyboardShortcut(VARIANT varChild, BSTR *pszKeyboardShortcut)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pszKeyboardShortcut && VALID_CHILDID(varChild))
            *pszKeyboardShortcut = ::SysAllocString(L"None");
        else
            retCode = E_INVALIDARG;
    }    

    return retCode;
}

// ************************************************************************************************

/**
 * This method will return a child id if one of our children (or us)
 * has the keyboard focus. If none does, then we find the dispatch
 * pointer if available of the item that does have the focus.
 *
 * @param pvarChild - storage location for the interface or child id.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accFocus(VARIANT *pvarChild)
{

    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pvarChild)
        {
            // if we are the focused window then we return this interface.
            HWND hFocused = GetFocus();
            if (mData->hWnd == hFocused)
            {
                pvarChild->vt = VT_I4;
                if (mData->btnOne.bHasKbFocus)
                    pvarChild->lVal = 1;
                else if (mData->btnTwo.bHasKbFocus)
                    pvarChild->lVal = 2;
                else if (mData->btnSelf.bHasKbFocus)
                    pvarChild->lVal = CHILDID_SELF;
            }
            else
            {
                // If we are not the focused window then we need to attempt to get the 
                // IDispatch pointer for the currently focused window from the window itself.
                //
                // Note, we can not always get an interface back. In the cases we can't, we
                // just return S_FALSE;
                pvarChild->vt = VT_DISPATCH;                
                retCode = AccessibleObjectFromWindow(hFocused, OBJID_WINDOW, 
                            IID_IAccessible, (void**)&pvarChild->pdispVal);                
                
            }
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This method would normally return a selection list. For something such as a
 * custom list we would return the various items in the list that are selected.
 * As a button, we don't have a selection.
 *
 * @param pvarChildren - storage location for selected items. (we won't have any)
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accSelection(VARIANT *pvarChildren)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pvarChildren)
            pvarChildren->vt = VT_EMPTY;
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This method returns a string describing the default action of our control.
 * Since we are a button with two inner buttons we will always return back
 * a string saying "push".
 *
 * @param varChild - child to get the default action for.
 * @param pszDefaultAction - place to store the action string.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::get_accDefaultAction(VARIANT varChild, BSTR *pszDefaultAction)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pszDefaultAction && VALID_CHILDID(varChild))
            *pszDefaultAction = ::SysAllocString(L"Push");
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This method has to do with child item selection. This can be used to force
 * an item to take focus. It also has more complex details when applied to items
 * that support selection such as list or tree items within a list/tree control.
 *
 * @param flagsSelect - determines what action we take.
 * @param varChild - the child the action is to be applied to.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::accSelect(long flagsSelect, VARIANT varChild)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (VALID_CHILDID(varChild))
        {
            // since we are a button with no "selectable" items the only flag
            // we support is the SELFLAG_TAKEFOCUS.
            if (((flagsSelect & SELFLAG_TAKEFOCUS) > 0) && (GetFocus() == mData->hWnd))
            {
                RECT rcWnd;

                switch (varChild.lVal)
                {
                    case CHILDID_SELF:
                        mData->btnSelf.bHasKbFocus = TRUE;
                        mData->btnOne.bHasKbFocus = FALSE;
                        mData->btnTwo.bHasKbFocus = FALSE;                        
                        break;
                    case 1:
                        mData->btnSelf.bHasKbFocus = FALSE;
                        mData->btnOne.bHasKbFocus = TRUE;
                        mData->btnTwo.bHasKbFocus = FALSE;
                        break;
                    case 2:
                        mData->btnSelf.bHasKbFocus = FALSE;
                        mData->btnOne.bHasKbFocus = FALSE;
                        mData->btnTwo.bHasKbFocus = TRUE;
                        break;
                };

                GetClientRect(mData->hWnd, &rcWnd);
                InvalidateRect(mData->hWnd, &rcWnd, FALSE);
            }
            else
                retCode = S_FALSE;
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This returns back the screen coordinates of our control or one of its
 * child items. 
 *
 * @param pxLeft - left coordinate.
 * @param pyTop - top coordinate.
 * @param pcxWidth - width of the item.
 * @param pcyHeight - height of the item.
 * @param varChild - child of the item to get the location for.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::accLocation(long *pxLeft, long *pyTop,long *pcxWidth,long *pcyHeight, VARIANT varChild)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pxLeft && pyTop && pcxWidth && pcyHeight && VALID_CHILDID(varChild))
        {
            RECT rcItem;
            switch(varChild.lVal)
            {
                case CHILDID_SELF:
                    {
                        rcItem = mData->btnSelf.rcBounds;                    
                        ClientToScreen(GetParent(mData->hWnd), &rcItem);
                    }
            	    break;
                case 1:
                    {
                        rcItem = mData->btnOne.rcBounds;
                        ClientToScreen(mData->hWnd, &rcItem);
                    }
                    break;
                case 2:
                    {
                        rcItem = mData->btnTwo.rcBounds;
                        ClientToScreen(mData->hWnd, &rcItem);
                    }
                    break;
            }

            *pxLeft     = rcItem.left;
            *pyTop      = rcItem.top;
            *pcxWidth   = rcItem.right - rcItem.left;
            *pcyHeight  = rcItem.bottom - rcItem.top;
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This method allows clients to move the keyboard focus within the control 
 *
 * @param navDir - determines how we move the focus.
 * @param varStart - the child we start to move from.
 * @param pVarEndUpAt - the child we land on after the move.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::accNavigate(long navDir, VARIANT varStart, VARIANT *pvarEndUpAt)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pvarEndUpAt && VALID_CHILDID(varStart))
        {
            pvarEndUpAt->vt = VT_EMPTY;
            switch(navDir)
            {
                case NAVDIR_DOWN:
                    // Do nothing. No items in our control have items that we implement
                    // as siblings below us.
                    break;
                case NAVDIR_FIRSTCHILD:
                    {
                        // the only element that has children is the self control. If we are
                        // not that then we just do nothing.
                        if (varStart.lVal == CHILDID_SELF)
                        {
                            pvarEndUpAt->vt = VT_I4;
                            pvarEndUpAt->lVal = 1; // this is our first child.
                        }
                    }
            	    break;                
                case NAVDIR_LASTCHILD:
                    {
                        // the only element that has children is the self control. If we are
                        // not that then we just do nothing.
                        if (varStart.lVal == CHILDID_SELF)
                        {
                            pvarEndUpAt->vt = VT_I4;
                            pvarEndUpAt->lVal = 2;
                        }
                    }
                    break;
                case NAVDIR_PREVIOUS:
                case NAVDIR_LEFT:
                    // the only element that has something to the left of it is the 
                    // second inner button. 
                    if (varStart.lVal == 2)
                    {
                        pvarEndUpAt->vt = VT_I4;
                        pvarEndUpAt->lVal = 1; 
                    }
                    break;            	
                case NAVDIR_NEXT:
                case NAVDIR_RIGHT:
                    // the only element that has something to the right of it is the 
                    // first inner button. 
                    if (varStart.lVal == 1)
                    {
                        pvarEndUpAt->vt = VT_I4;
                        pvarEndUpAt->lVal = 2; 
                    }
                    break;
                case NAVDIR_UP:
                    // Do nothing. No items in our control have items that we implement
                    // as siblings above us.
                    break;
                default:
                    retCode = E_INVALIDARG;
                    break;
            }
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * This allows a client to check if the coordinates provided are within our
 * control or child items.
 *
 * @param xLeft - x coordinate of the point to test.
 * @param yTop - y coordinate of the point to test.
 * @param pvarChild - place to store the item that was located via the hit test check.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::accHitTest(long xLeft, long yTop, VARIANT *pvarChild)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (pvarChild)
        {
            pvarChild->vt = VT_EMPTY;

            RECT rcItem;
            POINT pt = {xLeft, yTop};
            rcItem = mData->btnOne.rcBounds;
            ClientToScreen(mData->hWnd, &rcItem);
            
            if (PtInRect(&rcItem, pt))
            {
                pvarChild->vt = VT_I4;
                pvarChild->lVal = 1;
            }
            else
            {
                rcItem = mData->btnTwo.rcBounds;
                ClientToScreen(mData->hWnd, &rcItem);

                if (PtInRect(&rcItem, pt))
                {
                    pvarChild->vt = VT_I4;
                    pvarChild->lVal = 2;
                }
                else 
                {
                    rcItem = mData->btnSelf.rcBounds;
                    ClientToScreen(GetParent(mData->hWnd), &rcItem);
                    if (PtInRect(&rcItem, pt))
                    {
                        pvarChild->vt = VT_I4;
                        pvarChild->lVal = CHILDID_SELF;
                    }
                }
            }
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * Forces the default action of our button to happen. In the case 
 * of a button this is simply a click.
 *
 * @param varChild - the child item to do the default action for.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::accDoDefaultAction(VARIANT varChild)
{
    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (VALID_CHILDID(varChild))
        {
            // doing our default action for out button is to simply click the button.
            // to do this we just need to send a message we the right ids up to 
            // our parent window.
            SendMessage(GetParent(mData->hWnd), WM_COMMAND, 
                MAKEWPARAM(varChild.lVal, BN_CLICKED),
                (LPARAM)mData->hWnd);
        }
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * Changes the accessible name of an item within our control.
 *
 * @param varChild - the child to change the accessible name for.
 * @param szName - the new accessible name.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::put_accName(VARIANT varChild, BSTR szName)
{

    HRESULT retCode = DATACHECK(mData);
    if (SUCCEEDED(retCode))
    {
        if (VALID_CHILDID(varChild) && szName)
        {
            UINT nLen = 0;
            GENBTNDATA* pData = NULL;
            switch (varChild.lVal)
            {
                case CHILDID_SELF:
                    pData = &mData->btnSelf;
                    break;
                case 1:
                    pData = &mData->btnOne;
                    break;
                case 2:
                    pData = &mData->btnTwo;
                    break;
            };

            if (pData->pszAccName)
            {
                delete [] pData->pszAccName;
                pData->pszAccName = NULL;
            }

            StringCchLength(szName, STRSAFE_MAX_CCH, &nLen);
            pData->pszAccName = new wchar_t[nLen + 1];
            ZeroMemory(pData->pszAccName, (nLen + 1) * sizeof(wchar_t));
            StringCchCopy(pData->pszAccName, STRSAFE_MAX_CCH, szName);

            NotifyWinEvent(EVENT_OBJECT_NAMECHANGE, mData->hWnd, (LONG)pData, varChild.lVal);
        }          
        else
            retCode = E_INVALIDARG;
    }

    return retCode;
}

// ************************************************************************************************

/**
 * You can not change the accessible value of a button since it does not have one.
 * This being the case, we return back not implemented.
 *
 * @param varChild - child to change the value of.
 * @param szValue - the new accessible value of the button.
 * @return - HRESULT error code.
 */
STDMETHODIMP CAccProxyObject::put_accValue(VARIANT varChild, BSTR szValue)
{
    return E_NOTIMPL;
} 

// ************************************************************************************************

/**
 * Because there is not help file for this application or any real help
 * to be had for this button we are not going to implement this function.
 *
 * We just need to return E_NOTIMPL;
 */
STDMETHODIMP CAccProxyObject::get_accHelp(VARIANT varChild, BSTR *pszHelp)
{
    return E_NOTIMPL;
}

// ************************************************************************************************

/**
 * Because there is not help file for this application or any real help
 * to be had for this button we are not going to implement this function.
 *
 * We just need to return E_NOTIMPL;
*/
STDMETHODIMP CAccProxyObject::get_accHelpTopic(BSTR *pszHelpFile, VARIANT varChild,long *pidTopic)
{
    return E_NOTIMPL;
}

// ************************************************************************************************

//////////////////////////////////////////////////////////////////////////
// Implement IDispatch
//
// NOTE: We are not going to implement IDispatch. MSDN Notes that:
//
//       "With Active Accessibility 2.0, servers can return E_NOTIMPL from 
//        IDispatch methods and Active Accessibility will implement the 
//        IAccessible interface for them."
//
//////////////////////////////////////////////////////////////////////////
STDMETHODIMP CAccProxyObject::GetTypeInfoCount(unsigned int FAR* pctinfo)
{
    return E_NOTIMPL;
}

// ************************************************************************************************

STDMETHODIMP CAccProxyObject::GetTypeInfo(unsigned int iTInfo, LCID lcid, ITypeInfo FAR* FAR* ppTInfo)
{
    return E_NOTIMPL;
}

// ************************************************************************************************

STDMETHODIMP CAccProxyObject::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, unsigned int cNames, LCID lcid, DISPID FAR* rgDispId)
{
    return E_NOTIMPL;
}

// ************************************************************************************************

STDMETHODIMP CAccProxyObject::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS FAR* pDispParams, 
    VARIANT FAR* pVarResult, EXCEPINFO FAR* pExcepInfo, unsigned int FAR* puArgErr)
{
    return E_NOTIMPL;
}
