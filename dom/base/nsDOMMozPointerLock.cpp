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
 * The Original Code is Mouse Lock.
 *
 * The Initial Developer of the Original Code is Mozilla Foundation
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   David Humphrey <david.humphrey@senecac.on.ca>
 *   Diogo Golovanevsky <diogo.gmt@gmail.com>
 *   Raymond Hung <hung.raymond@gmail.com>
 *   Jesse Silver <jasilver1@learn.senecac.on.ca>
 *   Matthew Schranz <schranz.m@gmail.com>
 *   Joseph Hughes <CloudScorpion@gmail.com>
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

#include "nsDOMMozPointerLock.h"
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
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIServiceManager.h"
#include "nsContentUtils.h"

DOMCI_DATA(MozPointerLock, nsDOMMozPointerLock)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMMozPointerLock)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMMozPointerLock)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMozPointerLock)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMMozPointerLock)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(MozPointerLock)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMMozPointerLock)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMMozPointerLock)

NS_IMPL_CYCLE_COLLECTION_2(nsDOMMozPointerLock,
                           mWindow,
                           mPointerLockedElement)

nsCOMPtr<nsIDOMElement> nsDOMMozPointerLock::mPointerLockedElement =  nsnull;

static void
DispatchPointerLockLost(nsINode* aTarget)
{
  nsRefPtr<nsPLDOMEvent> e =
    new nsPLDOMEvent(aTarget,
                     NS_LITERAL_STRING("mozpointerlocklost"),
                     true,
                     false);
  e->PostDOMEvent();
}

nsDOMMozPointerLock::nsDOMMozPointerLock() :
  mWindow(nsnull)
{
}

nsDOMMozPointerLock::~nsDOMMozPointerLock()
{
}

nsresult
nsDOMMozPointerLock::Init(nsIDOMWindow* aContentWindow)
{
  NS_ENSURE_ARG_POINTER(aContentWindow);
  // Hang on to the window so we can check for fullscreen
  mWindow = aContentWindow;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMMozPointerLock::Unlock()
{
  if (!mPointerLockedElement) {
    return NS_OK;
  }

  nsCOMPtr<nsINode> node = do_QueryInterface(mPointerLockedElement);
  if (!node) {
    NS_ERROR("Unlock(): unable to get nsINode for locked element.");
    return NS_ERROR_UNEXPECTED;
  }
  DispatchPointerLockLost(node);
  node->RemoveMutationObserver(this);

  // Making the pointer reappear
  nsCOMPtr<nsPIDOMWindow> domWindow( do_QueryInterface( mWindow ) );
  if (!domWindow) {
    NS_ERROR("Unlock(): No DOM found in nsCOMPtr<nsPIDOMWindow>");
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<nsPresContext> presContext;
  domWindow->GetDocShell()->GetPresContext(getter_AddRefs(presContext));
  if (!presContext)	{
    NS_ERROR("Unlock(): Unable to get presContext in \
              domWindow->GetDocShell()->GetPresContext()");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
  if (!shell)	{
    NS_ERROR("Unlock(): Unable to find presContext->PresShell()");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIWidget> widget = shell->GetRootFrame()->GetNearestWidget();
  if (!widget) {
    NS_ERROR("Unlock(): Unable to find widget in \
              shell->GetRootFrame()->GetNearestWidget();");
    return NS_ERROR_UNEXPECTED;
  }

  nsRefPtr<nsEventStateManager> esm = presContext->EventStateManager();
  esm->SetCursor(NS_STYLE_CURSOR_AUTO, nsnull, false, 0.0f,
                 0.0f, widget, true);
  esm->SetPointerLock(widget, nsnull);

  mPointerLockedElement = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsDOMMozPointerLock::IsLocked(bool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mPointerLockedElement ? true : false;
  return NS_OK;
}

bool
nsDOMMozPointerLock::ShouldLock(nsIDOMElement* aTarget)
{
  // Check if fullscreen pointer lock pref is enabled
  nsCOMPtr<nsIPrefBranch> prefs(do_GetService("@mozilla.org/preferences-service;1"));
  bool pointerLockEnabled;
  prefs->GetBoolPref(PREF_POINTER_LOCK_ENABLED, &pointerLockEnabled);
  if (!pointerLockEnabled) {
    return false;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  mWindow->GetDocument(getter_AddRefs(domDoc));
  if (!domDoc) {
    NS_ERROR("ShouldLock(): Unable to get document");
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsINode> targetNode(do_QueryInterface(aTarget));
  if (!targetNode) {
    return false;
  }

  // Check if the element is in a DOM tree and also this DOM.
  nsCOMPtr<nsIDocument> targetDoc = targetNode->GetCurrentDoc();
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  if (!targetDoc || targetDoc != doc) {
    return false;
  }

  // Check if element is in fullscreen mode
  nsCOMPtr<nsIDOMHTMLElement> lockedElement;
  domDoc->GetMozFullScreenElement(getter_AddRefs(lockedElement));
  if (lockedElement != aTarget) {
    return false;
  }

  return true;
}

NS_IMETHODIMP
nsDOMMozPointerLock::Lock(nsIDOMElement* aTarget,
                          nsIDOMMozPointerLockSuccessCallback* aSuccessCallback,
                          nsIDOMMozPointerLockFailureCallback* aFailureCallback)
{
  NS_ENSURE_ARG_POINTER(aTarget);

  nsCOMPtr<nsIContent> element = do_QueryInterface(aTarget);
  if (!element) {
    NS_ERROR("Lock: Unable to get nsIContent for locked element");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsPointerLockRequest> request =
    new nsPointerLockRequest(element, aSuccessCallback, aFailureCallback);
  nsCOMPtr<nsIRunnable> ev;

  // If we're already locked to this target, re-call success callback
  if (mPointerLockedElement && mPointerLockedElement == aTarget){
    ev = new nsRequestPointerLockEvent(true, request);
  } else if (ShouldLock(aTarget)) {
    mPointerLockedElement = aTarget;

    // TODO: should these throw or cause the error callback?
    nsCOMPtr<nsPIDOMWindow> domWindow = do_QueryInterface(mWindow);
    if (!domWindow) {
      NS_ERROR("Lock(): No DOM found in nsCOMPtr<nsPIDOMWindow>");
      return NS_ERROR_FAILURE;
    }

    nsRefPtr<nsPresContext> presContext;
    domWindow->GetDocShell()->GetPresContext(getter_AddRefs(presContext));
    if (!presContext)	{
      NS_ERROR("Lock(): Unable to get presContext in \
                domWindow->GetDocShell()->GetPresContext()");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIPresShell> shell = presContext->PresShell();
    if (!shell) {
      NS_ERROR("Lock(): Unable to find presContext->PresShell()");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIWidget> widget = shell->GetRootFrame()->GetNearestWidget();
    if (!widget) {
      NS_ERROR("Lock(): Unable to find widget in \
                shell->GetRootFrame()->GetNearestWidget();");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsINode> node = do_QueryInterface(element);
    if (!node) {
      NS_ERROR("Lock(): unable to get nsINode for locked element.");
      return NS_ERROR_UNEXPECTED;
    }
    node->AddMutationObserver(this);

    // Hide the cursor and set pointer lock for future mouse events
    nsRefPtr<nsEventStateManager> esm = presContext->EventStateManager();
    esm->SetCursor(NS_STYLE_CURSOR_NONE, nsnull, false,
                   0.0f, 0.0f, widget, true);
    esm->SetPointerLock(widget, element);

    ev = new nsRequestPointerLockEvent(true, request);
  } else {
    ev = new nsRequestPointerLockEvent(false, request);
  }

  NS_DispatchToMainThread(ev);

  return NS_OK;
}

/**
 * nsIMutationObserver
 **/
void
nsDOMMozPointerLock::AttributeChanged(nsIDocument* aDocument,
                                      mozilla::dom::Element* aElement,
                                      PRInt32 aNameSpaceID,
                                      nsIAtom* aAttribute, PRInt32 aModType)
{
}

void
nsDOMMozPointerLock::ContentAppended(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aChild,
                                     PRInt32 aIndexInContainer)
{
}

void
nsDOMMozPointerLock::ContentInserted(nsIDocument* aDocument,
                                     nsIContent* aContainer,
                                     nsIContent* aChild,
                                     PRInt32 aIndexInContainer)
{
}

void
nsDOMMozPointerLock::ContentRemoved(nsIDocument* aDocument,
                                    nsIContent* aContainer,
                                    nsIContent* aChild,
                                    PRInt32 aIndexInContainer,
                                    nsIContent* aPreviousSibling)
{
}

void
nsDOMMozPointerLock::CharacterDataWillChange(nsIDocument* aDocument,
                                             nsIContent* aContent,
                                             CharacterDataChangeInfo* aInfo)
{
}

void
nsDOMMozPointerLock::CharacterDataChanged(nsIDocument* aDocument,
                                          nsIContent* aContent,
                                          CharacterDataChangeInfo* aInfo)
{
}

void
nsDOMMozPointerLock::AttributeWillChange(nsIDocument* aDocument,
                                         mozilla::dom::Element* aElement,
                                         PRInt32 aNameSpaceID,
                                         nsIAtom* aAttribute, PRInt32 aModType)
{
}

void
nsDOMMozPointerLock::ParentChainChanged(nsIContent* aContent)
{
  Unlock();
}

void
nsDOMMozPointerLock::NodeWillBeDestroyed(const nsINode* aNode)
{
}

// nsPointerLockRequest
NS_IMPL_THREADSAFE_ISUPPORTS0(nsPointerLockRequest)

nsPointerLockRequest::nsPointerLockRequest(
  nsIContent* aContent,
  nsIDOMMozPointerLockSuccessCallback* aSuccessCallback,
  nsIDOMMozPointerLockFailureCallback* aFailureCallback)
  : mContent(aContent),
    mSuccessCallback(aSuccessCallback),
    mFailureCallback(aFailureCallback)
{
}

void
nsPointerLockRequest::SendSuccess()
{
  if (mSuccessCallback) {
    nsCxPusher pusher;
    if (pusher.Push(mContent)) {
      mSuccessCallback->HandleEvent();
    }
  }
}

void
nsPointerLockRequest::SendFailure()
{
  if (mFailureCallback) {
    nsCxPusher pusher;
    if (pusher.Push(mContent)) {
      mFailureCallback->HandleEvent();
    }
  }
}

// nsRequestPointerLockEvent
NS_IMETHODIMP
nsRequestPointerLockEvent::Run()
{
  if (mAllow) {
    mRequest->SendSuccess();
  } else {
    mRequest->SendFailure();
  }
  return NS_OK;
}
