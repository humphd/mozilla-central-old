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

#ifndef nsDomGamepad_h
#define nsDomGamepad_h

#include "nsIDOMGamepad.h"
#include "nsString.h"
#include "nsCOMPtr.h"

class nsDOMGamepad : public nsIDOMGamepad
{
public:
  nsDOMGamepad(const nsAString &aID, PRUint32 aIndex,
               PRUint32 aNumButtons, PRUint32 aNumAxes);
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGAMEPAD

  nsDOMGamepad();
  void SetConnected(bool aConnected);
  void SetButton(PRUint32 aButton, PRUint8 aValue);
  void SetAxis(PRUint32 aAxis, float aValue);
  void SetIndex(PRUint32 aIndex);

  // Return a new nsDOMGamepad containing the same data as this object.
  already_AddRefed<nsDOMGamepad> Clone();

private:
  ~nsDOMGamepad();

protected:
  nsString mID;
  PRUint32 mIndex;

  // true if this gamepad is currently connected.
  bool mConnected;

  // Current state of buttons, axes.
  nsTArray<PRUint8> mButtons;
  nsTArray<float> mAxes;
};

#endif // nsDomGamepad_h
