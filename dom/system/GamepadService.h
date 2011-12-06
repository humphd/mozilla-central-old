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

#ifndef mozilla_dom_GamepadService_h_
#define mozilla_dom_GamepadService_h_

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsDataHashtable.h"
#include "nsDOMGamepad.h"
#include "nsGlobalWindow.h"
#include "nsIFocusManager.h"
#include "nsIObserver.h"
#include "nsITimer.h"

class nsIDOMDocument;
class nsIDOMEventTarget;

namespace mozilla {
namespace dom {

class GamepadService {
 public:
  // Get the singleton service
  static NS_EXPORT_(GamepadService*) GetService();
  // Destroy the singleton.
  static void DestroyService();

  void BeginShutdown();

  // Indicate that |aWindow| wants to receive gamepad events.
  void AddListener(nsGlobalWindow* aWindow);
  // Indicate that |aWindow| should no longer receive gamepad events.
  void RemoveListener(nsGlobalWindow* aWindow);

  // Add a gamepad to the list of known gamepads, and return its index.
  NS_EXPORT_(PRUint32) AddGamepad(const char* id, PRUint32 numButtons, PRUint32 numAxes);
  // Remove the gamepad at |index| from the list of known gamepads.
  NS_EXPORT_(void) RemoveGamepad(PRUint32 index);

  NS_EXPORT_(void) NewButtonEvent(PRUint32 index, PRUint32 button, bool pressed);
  NS_EXPORT_(void) NewAxisMoveEvent(PRUint32  index, PRUint32 axis, float value);

 protected:
  GamepadService();
  virtual ~GamepadService() {};
  virtual void Startup() = 0;
  virtual void Shutdown() = 0;
  void StartCleanupTimer();

  void NewConnectionEvent(PRUint32 index, bool connected);
  void FireAxisMoveEvent(nsIDOMDocument* domdoc,
                         nsIDOMEventTarget* target,
                         nsDOMGamepad* gamepad,
                         PRUint32 axis,
                         float value);
  void FireButtonEvent(nsIDOMDocument* domdoc,
                       nsIDOMEventTarget* target,
                       nsDOMGamepad* gamepad,
                       PRUint32 button,
                       bool pressed);
  void FireConnectionEvent(nsIDOMDocument* domdoc,
                           nsIDOMEventTarget* target,
                           nsDOMGamepad* gamepad,
                           bool connected);

  // true if the platform-specific backend has started work
  bool mStarted;
  // true when shutdown has begun
  bool mShuttingDown;

 private:
  // Returns true if we have already sent data from this gamepad
  // to this window. This should only return true if the user
  // explicitly interacted with a gamepad while this window
  // was focused, by pressing buttons or similar actions.
  bool WindowHasSeenGamepad(nsGlobalWindow* window, PRUint32 index);
  // Indicate that a window has recieved data from a gamepad.
  void SetWindowHasSeenGamepad(nsGlobalWindow* window, PRUint32 index,
                               bool hasSeen = true);

  bool GetFocusedWindow(nsGlobalWindow** window);
  static PLDHashOperator EnumerateForDisconnect(nsGlobalWindow* aKey,
                                                bool& aData,
                                                void* userArg);

  static void TimeoutHandler(nsITimer* aTimer, void* aClosure);
  static GamepadService* sSingleton;
  static bool sShutdown;

  // Gamepads connected to the system. Copies of these are handed out
  // to each window.
  nsTArray<nsRefPtr<nsDOMGamepad> > mGamepads;
  // This hashtable is keyed by nsGlobalWindows that are listening
  // for gamepad events. The bool indicates whether gamepad data
  // has been sent to that window.
  nsDataHashtable<nsRefPtrHashKey<nsGlobalWindow>, bool> mListeners;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsIFocusManager> mFocusManager;
  nsCOMPtr<nsIObserver> mObserver;
  // Used for convenience when enumerating mListeners entries.
  int mDisconnectingGamepad;
};

}
}

#endif // mozilla_dom_GamepadService_h_
