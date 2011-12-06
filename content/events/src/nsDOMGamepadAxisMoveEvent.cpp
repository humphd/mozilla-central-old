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

#include "nsDOMGamepadAxisMoveEvent.h"

#include "nsDOMClassInfoID.h"
#include "nsIDOMGamepad.h"
#include "nsIPrivateDOMEvent.h"
#include "nsGUIEvent.h"

NS_IMPL_ADDREF_INHERITED(nsDOMGamepadAxisMoveEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(nsDOMGamepadAxisMoveEvent, nsDOMEvent)

DOMCI_DATA(GamepadAxisMoveEvent, nsDOMGamepadAxisMoveEvent)

NS_INTERFACE_MAP_BEGIN(nsDOMGamepadAxisMoveEvent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGamepadAxisMoveEvent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GamepadAxisMoveEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

nsDOMGamepadAxisMoveEvent::nsDOMGamepadAxisMoveEvent(nsPresContext* aPresContext,
                                                     nsEvent* aEvent,
                                                     nsIDOMGamepad* aGamepad,
                                                     PRUint32 aAxis,
                                                     float aValue)
: nsDOMEvent(aPresContext, aEvent),
  mGamepad(aGamepad),
  mAxis(aAxis),
  mValue(aValue)
{
}

nsDOMGamepadAxisMoveEvent::~nsDOMGamepadAxisMoveEvent()
{
}

/* readonly attribute unsigned long axis; */
NS_IMETHODIMP nsDOMGamepadAxisMoveEvent::GetAxis(PRUint32* aAxis)
{
  *aAxis = mAxis;
  return NS_OK;
}

/* readonly attribute float value; */
NS_IMETHODIMP nsDOMGamepadAxisMoveEvent::GetValue(float* aValue)
{
  *aValue = mValue;
  return NS_OK;
}

/* readonly attribute nsIDOMGamepad gamepad; */
NS_IMETHODIMP nsDOMGamepadAxisMoveEvent::GetGamepad(nsIDOMGamepad** aGamepad)
{
  nsCOMPtr<nsIDOMGamepad> gamepad = mGamepad;
  gamepad.forget(aGamepad);
  return NS_OK;
}

/* void initGamepadAxisMoveEvent (in DOMString typeArg, in boolean canBubbleArg, in boolean cancelableArg, in nsIDOMGamepad gamepad, in unsigned long axis, in float value); */
NS_IMETHODIMP
nsDOMGamepadAxisMoveEvent::InitGamepadAxisMoveEvent(const nsAString& typeArg,
                                                    bool canBubbleArg,
                                                    bool cancelableArg,
                                                    nsIDOMGamepad* aGamepad,
                                                    PRUint32 aAxis,
                                                    float aValue)
{
  nsresult rv = nsDOMEvent::InitEvent(typeArg, canBubbleArg, cancelableArg);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mGamepad = aGamepad;
  mAxis = aAxis;
  mValue = aValue;

  return NS_OK;
}

nsresult
NS_NewDOMGamepadAxisMoveEvent(nsIDOMEvent** aInstancePtrResult,
                              nsPresContext* aPresContext,
                              class nsEvent* aEvent,
                              nsIDOMGamepad* aGamepad,
                              PRUint32 aAxis,
                              float aValue)
{
  nsDOMGamepadAxisMoveEvent* event = new nsDOMGamepadAxisMoveEvent(aPresContext,
                                                                   aEvent,
                                                                   aGamepad,
                                                                   aAxis,
                                                                   aValue);
  return CallQueryInterface(event, aInstancePtrResult);
}
