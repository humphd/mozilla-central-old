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

#include "nsDOMGamepad.h"
#include "nsDOMClassInfoID.h"
#include "nsIClassInfo.h"
#include "nsIXPCScriptable.h"
#include "jstypedarray.h"
#include "nsTArray.h"
#include "nsContentUtils.h"

DOMCI_DATA(Gamepad, nsDOMGamepad)

NS_IMPL_ADDREF(nsDOMGamepad)
NS_IMPL_RELEASE(nsDOMGamepad)

NS_INTERFACE_MAP_BEGIN(nsDOMGamepad)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGamepad)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Gamepad)
NS_INTERFACE_MAP_END

nsDOMGamepad::nsDOMGamepad(const nsAString &aID, PRUint32 aIndex,
                           PRUint32 aNumButtons, PRUint32 aNumAxes)
  : mID(aID),
    mIndex(aIndex),
    mConnected(true)
{
  mButtons.InsertElementsAt(0, aNumButtons, 0);
  mAxes.InsertElementsAt(0, aNumAxes, 0.0f);
}

nsDOMGamepad::~nsDOMGamepad()
{
}

/* readonly attribute DOMString id; */
NS_IMETHODIMP nsDOMGamepad::GetId(nsAString & aID)
{
  aID = mID;
  return NS_OK;
}

NS_IMETHODIMP nsDOMGamepad::GetIndex(PRUint32* aIndex)
{
  *aIndex = mIndex;
  return NS_OK;
}

void nsDOMGamepad::SetIndex(PRUint32 aIndex)
{
  mIndex = aIndex;
}

void nsDOMGamepad::SetConnected(bool aConnected)
{
  mConnected = aConnected;
}

void nsDOMGamepad::SetButton(PRUint32 aButton, PRUint8 aValue)
{
  mButtons[aButton] = aValue;
}

void nsDOMGamepad::SetAxis(PRUint32 axis, float value)
{
  mAxes[axis] = value;
}

/* readonly attribute boolean connected; */
NS_IMETHODIMP nsDOMGamepad::GetConnected(bool* aConnected)
{
  *aConnected = mConnected;
  return NS_OK;
}

/* readonly attribute nsIVariant buttons; */
NS_IMETHODIMP nsDOMGamepad::GetButtons(nsIVariant** aButtons)
{
  nsresult rv;
  nsCOMPtr<nsIWritableVariant> out = do_CreateInstance(NS_VARIANT_CONTRACTID,
                                                       &rv);
  if (NS_FAILED(rv))
    return rv;

  if (mButtons.Length() == 0) {
    rv = out->SetAsEmptyArray();
  } else {
    // Note: The resulting nsIVariant dupes both the array and its elements.
    PRUint8 *array = reinterpret_cast<PRUint8*>
                              (NS_Alloc(mButtons.Length() * sizeof(PRUint8)));
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 i = 0; i < mButtons.Length(); ++i) {
      array[i] = mButtons[i];
    }

    rv = out->SetAsArray(nsIDataType::VTYPE_UINT8,
                         nsnull,
                         mButtons.Length(),
                         reinterpret_cast<void*>(array));
    NS_Free(array);
  }
  if (NS_FAILED(rv))
    return rv;

  
  return CallQueryInterface(out, aButtons);
}

/* readonly attribute nsIVariant axes; */
NS_IMETHODIMP nsDOMGamepad::GetAxes(nsIVariant** aAxes)
{
  nsresult rv;
  nsCOMPtr<nsIWritableVariant> out = do_CreateInstance(NS_VARIANT_CONTRACTID,
                                                       &rv);
  if (NS_FAILED(rv))
    return rv;

  if (mAxes.Length() == 0) {
    rv = out->SetAsEmptyArray();
  } else {
    // Note: The resulting nsIVariant dupes both the array and its elements.
    float *array = reinterpret_cast<float*>
                              (NS_Alloc(mAxes.Length() * sizeof(float)));
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 i = 0; i < mAxes.Length(); ++i) {
      array[i] = mAxes[i];
    }

    rv = out->SetAsArray(nsIDataType::VTYPE_FLOAT,
                         nsnull,
                         mAxes.Length(),
                         reinterpret_cast<void*>(array));
    NS_Free(array);
  }
  if (NS_FAILED(rv))
    return rv;

  return CallQueryInterface(out, aAxes);
}

already_AddRefed<nsDOMGamepad>
nsDOMGamepad::Clone() {
  nsRefPtr<nsDOMGamepad> out =
    new nsDOMGamepad(mID, mIndex, mButtons.Length(), mAxes.Length());
  out->mConnected = mConnected;
  for (PRUint32 i = 0; i < mButtons.Length(); ++i) {
    out->mButtons[i] = mButtons[i];
  }
  for (PRUint32 i = 0; i < mAxes.Length(); ++i) {
    out->mAxes[i] = mAxes[i];
  }
  return out.forget();
}
