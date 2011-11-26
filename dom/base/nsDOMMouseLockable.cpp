/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et cindent: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla code.
 *
 * The Initial Developer of the Original Code is the Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  David Humphrey <david.humphrey@senecac.on.ca>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsDOMMouseLockable.h"
#include "nsContentUtils.h"
#include "nsEventStateManager.h"
#include "nsIWidget.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsAutoPtr.h"
#include "nsIDOMHTMLElement.h"
#include "nsINode.h"
#include "nsPLDOMEvent.h"
#include "nsIInterfaceRequestorUtils.h"

DOMCI_DATA(MouseLockable, nsDOMMouseLockable)

NS_INTERFACE_MAP_BEGIN(nsDOMMouseLockable)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseLockable)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MouseLockable)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDOMMouseLockable)
NS_IMPL_RELEASE(nsDOMMouseLockable)

nsDOMMouseLockable::nsDOMMouseLockable() :
  mIsLocked(PR_FALSE)
{
}

nsDOMMouseLockable::~nsDOMMouseLockable()
{
}

/* void unlock (); */
NS_IMETHODIMP nsDOMMouseLockable::Unlock()
{
  // No point doing this if mIslocked is already false,
  // plus it won't crash this way.
  if (mIsLocked) {
    
    //Not working!
    //nsCOMPtr<nsINode> node = do_GetInterface(mTarget);
    //DispatchMouseLockLost(node);
    
    // Making the mouse reappear
    nsCOMPtr<nsPIDOMWindow> domWindow( do_QueryInterface( mWindow ) );
    if (!domWindow)
	{
		NS_ERROR("Unlock(): No DOM found in nsCOMPtr<nsPIDOMWindow>");
		return NS_ERROR_UNEXPECTED;
	}
        
    nsRefPtr<nsPresContext> presContext;
    domWindow->GetDocShell()->GetPresContext(getter_AddRefs(presContext));
    if (!presContext)
	{
		NS_ERROR("Unlock(): Unable to get presContext in \
				 domWindow->GetDocShell()->GetPresContext()");
		return NS_ERROR_UNEXPECTED;
	}

    nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
    if (!shell)
	{
		NS_ERROR("Unlock(): Unable to find presContext->PresShell()");
		return NS_ERROR_UNEXPECTED;
	}

    nsCOMPtr<nsIWidget> widget = shell->GetRootFrame()->GetNearestWidget();
    if (!widget)
	{
		NS_ERROR("Unlock(): Unable to find widget in \
				 shell->GetRootFrame()->GetNearestWidget();");
		return NS_ERROR_UNEXPECTED;
	}
        
    presContext->EventStateManager()->SetCursor(NS_STYLE_CURSOR_AUTO, 
                                                nsnull, false, 0.0f, 
                                                0.0f, widget, true);
                                                
    mIsLocked = PR_FALSE;                                            
  }
  return NS_OK;
}

/* bool islocked (); */
NS_IMETHODIMP nsDOMMouseLockable::Islocked(bool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mIsLocked;
  return NS_OK;
}

nsresult
nsDOMMouseLockable::Init(nsIDOMWindow* aContentWindow)
{
  NS_ENSURE_ARG_POINTER(aContentWindow);
  // Hang on to the window so we can check for fullscreen
  mWindow = aContentWindow;
  mWindow = aContentWindow;
  return NS_OK;
}

NS_IMETHODIMP nsDOMMouseLockable::Lock(nsIDOMElement* aTarget)
{
  // Decided to use isFullScreen instead of mIsLocked
  // because I assume we should try to keep the values
  // of mIsLocked to PR_FALSE and PR_TRUE
  bool isFullScreen;
  mWindow->GetFullScreen(&isFullScreen);
  if(isFullScreen)
  {
    mIsLocked = PR_TRUE;
    mTarget = aTarget;

    nsCOMPtr<nsPIDOMWindow> domWindow( do_QueryInterface( mWindow ) );
    if (!domWindow)
	{
		NS_ERROR("Unlock(): No DOM found in nsCOMPtr<nsPIDOMWindow>");
		return NS_ERROR_UNEXPECTED;
	}
        
    nsRefPtr<nsPresContext> presContext;
    domWindow->GetDocShell()->GetPresContext(getter_AddRefs(presContext));
    if (!presContext)
	{
		NS_ERROR("Unlock(): Unable to get presContext in \
				 domWindow->GetDocShell()->GetPresContext()");
		return NS_ERROR_UNEXPECTED;
	}

    nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
    if (!shell)
	{
		NS_ERROR("Unlock(): Unable to find presContext->PresShell()");
		return NS_ERROR_UNEXPECTED;
	}

    nsCOMPtr<nsIWidget> widget = shell->GetRootFrame()->GetNearestWidget();
    if (!widget)
	{
		NS_ERROR("Unlock(): Unable to find widget in \
				 shell->GetRootFrame()->GetNearestWidget();");
		return NS_ERROR_UNEXPECTED;
	}
        
    presContext->EventStateManager()->SetCursor(NS_STYLE_CURSOR_NONE, 
                                                nsnull, false, 0.0f, 
                                                0.0f, widget, true);
  }
  return NS_OK;
}

// Not working!
static void
DispatchMouseLockLost(nsINode* aTarget)
{
    printf("\nDispatchMouseLockLost\n");
    nsRefPtr<nsPLDOMEvent> e = new nsPLDOMEvent(aTarget,
      NS_LITERAL_STRING("mouselocklost"), true, false);
    e->PostDOMEvent();
}

