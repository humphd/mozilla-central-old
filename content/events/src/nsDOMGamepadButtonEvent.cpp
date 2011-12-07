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

#include "nsDOMGamepadButtonEvent.h"
#include "nsIDOMGamepad.h"

#include "nsDOMClassInfoID.h"
#include "nsIPrivateDOMEvent.h"
#include "nsGUIEvent.h"


NS_IMPL_ADDREF_INHERITED(nsDOMGamepadButtonEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMGamepadButtonEvent, nsDOMEvent)

DOMCI_DATA(GamepadButtonEvent, nsDOMGamepadButtonEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMGamepadButtonEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGamepadButtonEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GamepadButtonEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsDOMGamepadButtonEvent::nsDOMGamepadButtonEvent(nsPresContext* aPresContext,
                                                 nsEvent* aEvent,
                                                 nsIDOMGamepad* aGamepad,
                                                 PRUint32 aButton)
: nsDOMEvent(aPresContext, aEvent),
  mGamepad(aGamepad),
  mButton(aButton)
{
}

nsDOMGamepadButtonEvent::~nsDOMGamepadButtonEvent()
{
}

/* readonly attribute unsigned long button; */
NS_IMETHODIMP nsDOMGamepadButtonEvent::GetButton(PRUint32* aButton)
{
  *aButton = mButton;
  return NS_OK;
}

/* readonly attribute nsIDOMGamepad gamepad; */
NS_IMETHODIMP nsDOMGamepadButtonEvent::GetGamepad(nsIDOMGamepad** aGamepad)
{
  nsCOMPtr<nsIDOMGamepad> gamepad = mGamepad;
  gamepad.forget(aGamepad);
  return NS_OK;
}

/* void initGamepadButtonEvent (in DOMString typeArg, in boolean canBubbleArg, in boolean cancelableArg, in nsIDOMGamepad gamepad, in unsigned long button); */
NS_IMETHODIMP
nsDOMGamepadButtonEvent::InitGamepadButtonEvent(const nsAString& typeArg,
                                                bool canBubbleArg,
                                                bool cancelableArg,
                                                nsIDOMGamepad *aGamepad,
                                                PRUint32 aButton)
{
  nsresult rv = nsDOMEvent::InitEvent(typeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mGamepad = aGamepad;
  mButton = aButton;

  return NS_OK;
}

nsresult
NS_NewDOMGamepadButtonEvent(nsIDOMEvent** aInstancePtrResult,
                            nsPresContext* aPresContext,
                            class nsEvent *aEvent,
                            nsIDOMGamepad *aGamepad,
                            PRUint32 aButtonID)
{
  nsDOMGamepadButtonEvent* event = new nsDOMGamepadButtonEvent(aPresContext,
                                                               aEvent,
                                                               aGamepad,
                                                               aButtonID);
  return CallQueryInterface(event, aInstancePtrResult);
}
