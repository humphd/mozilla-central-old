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
 * The Original Code is Mozilla Gamepad API.
 *
 * The Initial Developer of the Original Code is
 * The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Ted Mielczarek <ted.mielczarek@gmail.com>
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

#include "GamepadService.h"

#include "nsAutoPtr.h"
#include "nsFocusManager.h"
#include "nsIDOMEvent.h"
#include "nsIDOMDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsDOMGamepad.h"
#include "nsIDOMGamepadButtonEvent.h"
#include "nsIDOMGamepadAxisMoveEvent.h"
#include "nsIDOMGamepadConnectionEvent.h"
#include "nsIDOMWindow.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIServiceManager.h"
#include "nsITimer.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"

#include <cstddef>

namespace mozilla {
namespace dom {

// This should be implemented per-platform and return an instance
// of the GamepadService subclass.
extern GamepadService* CreateGamepadService();

// Amount of time to wait before cleaning up gamepad resources
// when no pages are listening for events.
static const int kCleanupDelayMS = 2000;

namespace {

class DestroyGamepadServiceEvent : public nsRunnable {
public:
  DestroyGamepadServiceEvent() {}

  NS_IMETHOD Run() {
    GamepadService::DestroyService();
    return NS_OK;
  }
};

class ShutdownObserver : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  ShutdownObserver() {
    nsCOMPtr<nsIObserverService> observerService = 
      mozilla::services::GetObserverService();
    observerService->AddObserver(this,
                                 NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID,
                                 PR_FALSE);
  }
};

NS_IMPL_ISUPPORTS1(ShutdownObserver, nsIObserver)

NS_IMETHODIMP
ShutdownObserver::Observe(nsISupports* aSubject,
                          const char* aTopic,
                          const PRUnichar* aData) {
  // Shutdown the service.
  GamepadService::GetService()->BeginShutdown();

  // Unregister while we're here.
  nsCOMPtr<nsIObserverService> observerService = 
    mozilla::services::GetObserverService();
  observerService->RemoveObserver(this, NS_XPCOM_WILL_SHUTDOWN_OBSERVER_ID);

  // And delete it soon.
  nsRefPtr<DestroyGamepadServiceEvent> event =
    new DestroyGamepadServiceEvent();
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  return NS_OK;
}

} // namespace

GamepadService* GamepadService::sSingleton = NULL;
bool GamepadService::sShutdown = false;

GamepadService::GamepadService()
  : mStarted(false),
    mShuttingDown(false),
    mFocusManager(do_GetService(FOCUSMANAGER_CONTRACTID)),
    mObserver(new ShutdownObserver())
{
  mListeners.Init();
}

void
GamepadService::BeginShutdown() {
  mShuttingDown = true;
  Shutdown();
}

void
GamepadService::AddListener(nsGlobalWindow* aWindow)
{
  if (mShuttingDown) {
    return;
  }

  bool unused;
  if (mListeners.Get(aWindow, &unused)) {
    return; // already exists
  }

  if (!mStarted) {
    Startup();
  }

  mListeners.Put(aWindow, false);
}

void
GamepadService::RemoveListener(nsGlobalWindow* aWindow)
{
  if (mShuttingDown) {
    // Doesn't matter at this point. It's possible we're being called
    // as a result of our own destructor here, so just bail out.
    return;
  }

  bool unused;
  if (!mListeners.Get(aWindow, &unused)) {
    return; // doesn't exist
  }

  mListeners.Remove(aWindow);

  if (mListeners.Count() == 0 && !mShuttingDown) {
    StartCleanupTimer();
  }
}

PRUint32
GamepadService::AddGamepad(const char* id,
                           PRUint32 numButtons,
                           PRUint32 numAxes) {
  //TODO: get initial button/axis state
  nsRefPtr<nsDOMGamepad> gamepad =
    new nsDOMGamepad(NS_ConvertUTF8toUTF16(nsDependentCString(id)),
                     0,
                     numButtons,
                     numAxes);
  int index = -1;
  for (PRUint32 i = 0; i < mGamepads.Length(); i++) {
    if (!mGamepads[i]) {
      mGamepads[i] = gamepad;
      index = i;
      break;
    }
  }
  if (index == -1) {
    mGamepads.AppendElement(gamepad);
    index = mGamepads.Length() - 1;
  }

  gamepad->SetIndex(index);
  NewConnectionEvent(index, true);

  return index;
}

void
GamepadService::RemoveGamepad(PRUint32 index) {
  if (index < mGamepads.Length()) {
    mGamepads[index]->SetConnected(false);
    NewConnectionEvent(index, false);
    // If this is the last entry in the list, just remove it.
    if (index == mGamepads.Length() - 1) {
      mGamepads.RemoveElementAt(index);
    } else {
      // Otherwise just null it out and leave it, so the
      // indices of the following entries remain valid.
      mGamepads[index] = NULL;
    }
  }
}

void
GamepadService::NewButtonEvent(PRUint32 index, PRUint32 button, bool pressed) {
  if (mShuttingDown || index >= mGamepads.Length()) {
    return;
  }

  mGamepads[index]->SetButton(button, pressed ? 1 : 0);

  nsRefPtr<nsGlobalWindow> window;
  if (GetFocusedWindow(getter_AddRefs(window))) {
    if (!WindowHasSeenGamepad(window, index)) {
      SetWindowHasSeenGamepad(window, index);
      // This window hasn't seen this gamepad before, so
      // send a connection event first.
      NewConnectionEvent(index, true);
    }

    nsRefPtr<nsDOMGamepad> gamepad = window->GetGamepad(index);
    nsCOMPtr<nsIDOMDocument> domdoc;
    window->GetDocument(getter_AddRefs(domdoc));

    if (domdoc && gamepad) {
      gamepad->SetButton(button, pressed ? 1 : 0);
      // Fire event
      FireButtonEvent(domdoc, window, gamepad, button, pressed);
    }
  }
}

void
GamepadService::FireButtonEvent(nsIDOMDocument *domdoc,
                                nsIDOMEventTarget *target,
                                nsDOMGamepad* gamepad,
                                PRUint32 button,
                                bool pressed)
{
  nsCOMPtr<nsIDOMEvent> event;
  bool defaultActionEnabled = true;
  domdoc->CreateEvent(NS_LITERAL_STRING("MozGamepadButtonEvent"),
                      getter_AddRefs(event));
  if (!event) {
    return;
  }

  nsCOMPtr<nsIDOMGamepadButtonEvent> je = do_QueryInterface(event);

  if (!je) {
    return;
  }

  nsString name = pressed ? NS_LITERAL_STRING("MozGamepadButtonDown") :
                            NS_LITERAL_STRING("MozGamepadButtonUp");
  je->InitGamepadButtonEvent(name, false, false, gamepad, button);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  if (privateEvent) {
    privateEvent->SetTrusted(PR_TRUE);
  }

  target->DispatchEvent(event, &defaultActionEnabled);
}

void
GamepadService::NewAxisMoveEvent(PRUint32 index, PRUint32 axis, float value) {
  if (mShuttingDown || index >= mGamepads.Length()) {
    return;
  }
  mGamepads[index]->SetAxis(axis, value);

  nsRefPtr<nsGlobalWindow> window;
  if (GetFocusedWindow(getter_AddRefs(window))) {
    if (!WindowHasSeenGamepad(window, index)) {
      SetWindowHasSeenGamepad(window, index);
      // This window hasn't seen this gamepad before, so
      // send a connection event first.
      NewConnectionEvent(index, true);
    }

    nsRefPtr<nsDOMGamepad> gamepad = window->GetGamepad(index);
    nsCOMPtr<nsIDOMDocument> domdoc;
    window->GetDocument(getter_AddRefs(domdoc));

    if (domdoc && gamepad) {
      gamepad->SetAxis(axis, value);
      // Fire event
      FireAxisMoveEvent(domdoc, window, gamepad, axis, value);
    }
  }
}

void
GamepadService::FireAxisMoveEvent(nsIDOMDocument* domdoc,
                                  nsIDOMEventTarget* target,
                                  nsDOMGamepad* gamepad,
                                  PRUint32 axis,
                                  float value)
{
  nsCOMPtr<nsIDOMEvent> event;
  bool defaultActionEnabled = true;
  domdoc->CreateEvent(NS_LITERAL_STRING("MozGamepadAxisMoveEvent"),
                      getter_AddRefs(event));
  if (!event) {
    return;
  }

  nsCOMPtr<nsIDOMGamepadAxisMoveEvent> je = do_QueryInterface(event);

  if (!je) {
    return;
  }

  je->InitGamepadAxisMoveEvent(NS_LITERAL_STRING("MozGamepadAxisMove"),
                                false, false, gamepad, axis, value);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  if (privateEvent)
    privateEvent->SetTrusted(PR_TRUE);

  target->DispatchEvent(event, &defaultActionEnabled);
}

void
GamepadService::NewConnectionEvent(PRUint32 index, bool connected)
{
  if (mShuttingDown || index >= mGamepads.Length()) {
    return;
  }

  if (connected) {
    nsRefPtr<nsGlobalWindow> window;
    if (GetFocusedWindow(getter_AddRefs(window))) {
      // We don't fire a connected event here unless the window
      // has seen input from at least one device.
      bool hasSeenData;
      if (connected && !(mListeners.Get(window, &hasSeenData) && hasSeenData)) {
        return;
      }

      SetWindowHasSeenGamepad(window, index);

      nsRefPtr<nsDOMGamepad> gamepad = window->GetGamepad(index);
      nsCOMPtr<nsIDOMDocument> domdoc;
      window->GetDocument(getter_AddRefs(domdoc));

      if (domdoc && gamepad) {
        // Fire event
        FireConnectionEvent(domdoc, window, gamepad, connected);
      }
    }
  } else {
    // For disconnection events, fire one at every window that has received
    // data from this gamepad.
    mDisconnectingGamepad = index;
    mListeners.Enumerate(EnumerateForDisconnect, (void*)this);
    mDisconnectingGamepad = -1;
  }
}

// static
PLDHashOperator
GamepadService::EnumerateForDisconnect(nsGlobalWindow* aWindow,
                                       bool& aData,
                                       void *userArg)
{
  GamepadService* self = reinterpret_cast<GamepadService*>(userArg);

  if (aWindow &&
      self->WindowHasSeenGamepad(aWindow, self->mDisconnectingGamepad)) {

    nsRefPtr<nsDOMGamepad> gamepad = aWindow->GetGamepad(self->mDisconnectingGamepad);

    nsCOMPtr<nsIDOMDocument> domdoc;
    aWindow->GetDocument(getter_AddRefs(domdoc));

    if (domdoc && gamepad) {
      gamepad->SetConnected(false);
      // Fire event
      self->FireConnectionEvent(domdoc,
                                aWindow,
                                gamepad,
                                false);
    }

    if (gamepad) {
      aWindow->RemoveGamepad(self->mDisconnectingGamepad);
    }
  }
  return PL_DHASH_NEXT;
}

void
GamepadService::FireConnectionEvent(nsIDOMDocument *domdoc,
                                    nsIDOMEventTarget *target,
                                    nsDOMGamepad* gamepad,
                                    bool connected)
{
  nsCOMPtr<nsIDOMEvent> event;
  bool defaultActionEnabled = true;
  domdoc->CreateEvent(NS_LITERAL_STRING("MozGamepadConnectionEvent"),
                      getter_AddRefs(event));
  if (!event) {
    return;
  }

  nsCOMPtr<nsIDOMGamepadConnectionEvent> je = do_QueryInterface(event);

  if (!je) {
    return;
  }

  nsString name = connected ? NS_LITERAL_STRING("MozGamepadConnected") :
                              NS_LITERAL_STRING("MozGamepadDisconnected");
  je->InitGamepadConnectionEvent(name, false, false, gamepad);

  nsCOMPtr<nsIPrivateDOMEvent> privateEvent = do_QueryInterface(event);
  if (privateEvent) {
    privateEvent->SetTrusted(PR_TRUE);
  }

  target->DispatchEvent(event, &defaultActionEnabled);
}


// static
GamepadService* GamepadService::GetService() {
  NS_ASSERTION(!sShutdown, "Attempted to get GamepadService after shutdown!");
  if (sShutdown) {
    // Should crash safely in release builds.
    return NULL;
  }

  if (!sSingleton) {
    sSingleton = CreateGamepadService();
  }
  return sSingleton;
}

// static
void GamepadService::DestroyService() {
  delete sSingleton;
  sSingleton = NULL;
  sShutdown = true;
}

bool
GamepadService::WindowHasSeenGamepad(nsGlobalWindow* aWindow, PRUint32 index)
{
  nsRefPtr<nsDOMGamepad> gamepad = aWindow->GetGamepad(index);
  return gamepad != NULL;
}

void
GamepadService::SetWindowHasSeenGamepad(nsGlobalWindow* aWindow,
                                        PRUint32 index,
                                        bool hasSeen)
{
  bool hasSeenData;
  if (!mListeners.Get(aWindow, &hasSeenData)) {
    // This window isn't even listening for gamepad events.
    return;
  }

  if (hasSeen) {
    mListeners.Put(aWindow, true);
    nsRefPtr<nsDOMGamepad> gamepad = mGamepads[index]->Clone();
    aWindow->AddGamepad(index, gamepad);
  } else {
    aWindow->RemoveGamepad(index);
  }
}

bool
GamepadService::GetFocusedWindow(nsGlobalWindow** aWindow)
{
  nsCOMPtr<nsIDOMWindow> focusedWindow;
  if (NS_FAILED(mFocusManager->GetFocusedWindow(getter_AddRefs(focusedWindow)))) {
    return false;
  }

  nsCOMPtr<nsPIDOMWindow> outerWindow = do_QueryInterface(focusedWindow);
  if (!outerWindow) {
    return false;
  }
  nsCOMPtr<nsIDOMWindow> innerWindow = outerWindow->GetCurrentInnerWindow();

  nsCOMPtr<nsPIDOMWindow> innerPIWindow = do_QueryInterface(innerWindow);
  if (!innerPIWindow) {
    return false;
  }

  // Yeah, this sucks. I'm so going to replace this.
  nsPIDOMWindow* innerPIWindowPtr;
  innerPIWindow.forget(&innerPIWindowPtr);
  *aWindow = static_cast<nsGlobalWindow*>(innerPIWindowPtr);
  return *aWindow;
}

// static
void
GamepadService::TimeoutHandler(nsITimer *aTimer, void *aClosure)
{
  // the reason that we use self, instead of just using nsITimerCallback or nsIObserver
  // is so that subclasses are free to use timers without worry about the base classes's
  // usage.
  GamepadService* self = reinterpret_cast<GamepadService*>(aClosure);
  if (!self) {
    NS_ERROR("no self");
    return;
  }

  if (self->mShuttingDown) {
    return;
  }

  if (self->mListeners.Count() == 0) {
    self->Shutdown();
    if (!self->mGamepads.IsEmpty()) {
      self->mGamepads.Clear();
    }
  }
}

void
GamepadService::StartCleanupTimer()
{
  if (mTimer) {
    mTimer->Cancel();
  }

  mTimer = do_CreateInstance("@mozilla.org/timer;1");
  if (mTimer) {
    mTimer->InitWithFuncCallback(TimeoutHandler,
                                 this,
                                 kCleanupDelayMS,
                                 nsITimer::TYPE_ONE_SHOT);
  }
}

} // namespace dom
} // namespace mozilla
