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

#ifndef nsDOMMozPointerLock_h___
#define nsDOMMozPointerLock_h___

#define PREF_POINTER_LOCK_ENABLED "full-screen-api.pointer-lock.enabled"

#include "nsIDOMMozPointerLock.h"
#include "nsIDOMMozPointerLockSuccessCallback.h"
#include "nsIDOMMozPointerLockFailureCallback.h"
#include "nsWeakPtr.h"
#include "nsINode.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "nsIContent.h"
#include "nsIMutationObserver.h"
#include "nsCycleCollectionParticipant.h"

class nsPointerLockRequest : public nsISupports
{
public:
  NS_DECL_ISUPPORTS

  nsPointerLockRequest(nsIContent* aContent,
                       nsIDOMMozPointerLockSuccessCallback* aSuccessCallback,
                       nsIDOMMozPointerLockFailureCallback* aFailureCallback);
  void SendSuccess();
  void SendFailure();

private:
  nsCOMPtr<nsIContent>                          mContent;
  nsCOMPtr<nsIDOMMozPointerLockSuccessCallback> mSuccessCallback;
  nsCOMPtr<nsIDOMMozPointerLockFailureCallback> mFailureCallback;
};


class nsRequestPointerLockEvent : public nsRunnable
{
public:
  nsRequestPointerLockEvent(bool aAllow,
                            nsPointerLockRequest* aRequest)
    : mRequest(aRequest),
      mAllow(aAllow)
  {}

  NS_IMETHOD Run();

private:
  nsRefPtr<nsPointerLockRequest> mRequest;
  bool mAllow;
};


class nsDOMMozPointerLock : public nsIDOMMozPointerLock,
                            public nsIMutationObserver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMMozPointerLock,
                                           nsIDOMMozPointerLock)
  NS_DECL_NSIDOMMOZPOINTERLOCK
  NS_DECL_NSIMUTATIONOBSERVER

  nsDOMMozPointerLock();
  nsresult Init(nsIDOMWindow*);

private:
  ~nsDOMMozPointerLock();
  bool ShouldLock(nsIDOMElement*);

  nsCOMPtr<nsIDOMWindow>  mWindow;
  static nsCOMPtr<nsIDOMElement> mPointerLockedElement;
};

#endif /* nsDOMMozPointerLock_h___ */
