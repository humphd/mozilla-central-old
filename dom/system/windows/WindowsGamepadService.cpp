#include <algorithm>
#include <cstddef>

#include <stdio.h>
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "nsIComponentManager.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsThreadUtils.h"
#include "mozilla/dom/GamepadService.h"
#include "mozilla/Services.h"

namespace mozilla {
namespace dom {

const LONG kMaxAxisValue = 65535;
const DWORD BUTTON_DOWN_MASK = 0x80;
// Multiple devices-changed notifications can be sent when a device
// is connected, because USB devices consist of multiple logical devices.
// Therefore, we wait a bit after receiving one before looking for
// device changes.
const PRUint32 kDevicesChangedStableDelay = 200;

typedef struct {
  float x,y;
} HatState;

struct Gamepad {
  // From DirectInput, unique to this device+computer combination.
  GUID guidInstance;
  // The ID assigned by the base GamepadService
  int id;
  // A somewhat unique string consisting of the USB vendor/product IDs,
  // and the controller name.
  char idstring[128];
  // USB vendor and product IDs
  int vendorID;
  int productID;
  // Information about the physical device.
  int numAxes;
  int numHats;
  int numButtons;
  // The human-readable device name.
  char name[128];
  // The DirectInput device.
  LPDIRECTINPUTDEVICE8 device;
  // A handle that DirectInput signals when there is new data from
  // the device.
  HANDLE event;
  // The state of any POV hats on the device.
  HatState hatState[4];
  // Used during rescan to find devices that were disconnected.
  bool present;
};

// Given DWORD |hatPos| representing the position of the POV hat per:
// http://msdn.microsoft.com/en-us/library/ee418260%28v=VS.85%29.aspx
// fill |axes| with the position of the x and y axes.
//
//XXX: ostensibly the values could be arbitrary degrees for a hat with
// full rotation, but we'll punt on that for now. This should handle
// 8-way D-pads exposed as POV hats.
static void
HatPosToAxes(DWORD hatPos, HatState& axes) {
  // hatPos is in hundredths of a degree clockwise from north.
  if (LOWORD(hatPos) == 0xFFFF) {
    // centered
    axes.x = axes.y = 0.0;
  }
  else if (hatPos == 0)  {
    // Up
    axes.x = 0.0;
    axes.y = -1.0;
  }
  else if (hatPos == 45 * DI_DEGREES) {
    // Up-right
    axes.x = 1.0;
    axes.y = -1.0;
  }
  else if (hatPos == 90 * DI_DEGREES) {
    // Right
    axes.x = 1.0;
    axes.y = 0.0;
  }
  else if (hatPos == 135 * DI_DEGREES) {
    // Down-right
    axes.x = 1.0;
    axes.y = 1.0;
  }
  else if (hatPos == 180 * DI_DEGREES) {
    // Down
    axes.x = 0.0;
    axes.y = 1.0;
  }
  else if (hatPos == 225 * DI_DEGREES) {
    // Down-left
    axes.x = -1.0;
    axes.y = 1.0;
  }
  else if (hatPos == 270 * DI_DEGREES) {
    // Left
    axes.x = -1.0;
    axes.y = 0.0;
  }
  else if (hatPos == 315 * DI_DEGREES) {
    // Up-left
    axes.x = -1.0;
    axes.y = -1.0;
  }
}

// Used to post events from the background thread to the foreground thread.
class GamepadEvent : public nsRunnable {
public:
  typedef enum {
    Axis,
    Button,
    HatX,
    HatY,
    HatXY,
    Unknown
  } Type;

  GamepadEvent(const Gamepad& gamepad,
               Type type,
               int which,
               DWORD data) : mGamepad(gamepad),
                             mType(type),
                             mWhich(which),
                             mData(data) {
  }

  NS_IMETHOD Run() {
    GamepadService* gamepadsvc = GamepadService::GetService();

    switch (mType) {
    case Button:
      gamepadsvc->NewButtonEvent(mGamepad.id, mWhich, mData & BUTTON_DOWN_MASK);
      break;
    case Axis: {
      float adjustedData = ((float)mData * 2.0f) / (float)kMaxAxisValue - 1.0f;
      gamepadsvc->NewAxisMoveEvent(mGamepad.id, mWhich, adjustedData);
    }
    case HatX:
    case HatY:
    case HatXY: {
      // Synthesize 2 axes per POV hat for convenience.
      HatState hatState;
      HatPosToAxes(mData, hatState);
      int xAxis = mGamepad.numAxes + 2 * mWhich;
      int yAxis = mGamepad.numAxes + 2 * mWhich + 1;
      //TODO: ostensibly we could not fire an event if one axis hasn't
      // changed, but it's a pain to track that.
      if (mType == HatX || mType == HatXY) {
        gamepadsvc->NewAxisMoveEvent(mGamepad.id, xAxis, hatState.x);
      }
      if (mType == HatY || mType == HatXY) {
        gamepadsvc->NewAxisMoveEvent(mGamepad.id, yAxis, hatState.y);
      }
      break;
    }
    case Unknown:
      break;
    }
    
    return NS_OK;
  }

  const Gamepad& mGamepad;
  // Type of event
  Type mType;
  // Which button/axis is involved
  int mWhich;
  // Data specific to event
  DWORD mData;
};

class GamepadChangeEvent : public nsRunnable {
public:
  enum Type {
    Added,
    Removed
  };
  GamepadChangeEvent(Gamepad& gamepad,
                     Type type) : mGamepad(gamepad),
                                  mID(gamepad.id),
                                  mType(type) {
  }

  NS_IMETHOD Run() {
    GamepadService* gamepadsvc = GamepadService::GetService();
    if (mType == Added) {
      mGamepad.id = gamepadsvc->AddGamepad(mGamepad.idstring,
                                           mGamepad.numButtons,
                                           mGamepad.numAxes +
                                           mGamepad.numHats*2);
    } else {
      gamepadsvc->RemoveGamepad(mID);
    }
    return NS_OK;
  }

private:
  Gamepad& mGamepad;
  PRUint32 mID;
  Type mType;
};

class WindowsGamepadService;

class Observer : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  Observer(WindowsGamepadService& svc) : mSvc(svc) {
    nsresult rv;
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
    nsCOMPtr<nsIObserverService> observerService = 
      mozilla::services::GetObserverService();
    observerService->AddObserver(this, "devices-changed", PR_FALSE);
  }
  virtual ~Observer() {
    nsCOMPtr<nsIObserverService> observerService = 
      mozilla::services::GetObserverService();
    observerService->RemoveObserver(this, "devices-changed");
  }

private:
  // Gamepad service owns us, we just hold a reference back to it.
  WindowsGamepadService& mSvc;
  nsCOMPtr<nsITimer> mTimer;
};

NS_IMPL_ISUPPORTS1(Observer, nsIObserver);
  
class WindowsGamepadService : public GamepadService {
public:
  WindowsGamepadService();
  virtual ~WindowsGamepadService() {
    Cleanup();
    CloseHandle(mThreadExitEvent);
    CloseHandle(mThreadRescanEvent);
    if (dinput) {
      dinput->Release();
      dinput = NULL;
    }
  }

  void DevicesChanged();

private:
  virtual void Startup();
  virtual void Shutdown();
  void ScanForDevices();
  void Cleanup();
  void CleanupGamepad(Gamepad& gamepad);
  // Callback for enumerating axes on a device
  static BOOL CALLBACK EnumObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,
                                           LPVOID pvRef);
  // Callback for enumerating devices via DInput
  static BOOL CALLBACK EnumCallback(LPCDIDEVICEINSTANCE lpddi, LPVOID pvRef);
  // Thread function to wait on device events
  static DWORD WINAPI DInputThread(LPVOID arg);

  // Used to signal the background thread to exit.
  HANDLE mThreadExitEvent;
  // Used to signal the background thread to rescan devices.
  HANDLE mThreadRescanEvent;
  HANDLE mThread;

  // List of connected devices.
  nsTArray<Gamepad> mGamepads;
  // List of event handles used for signaling.
  nsTArray<HANDLE> mEvents;

  LPDIRECTINPUT8 dinput;

  nsRefPtr<Observer> mObserver;
};

// Called by GamepadService to instantiate the singleton.
GamepadService* CreateGamepadService() {
  //TODO: prevent double-calling?
  return new WindowsGamepadService();
}

WindowsGamepadService::WindowsGamepadService()
  : mThreadExitEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
    mThreadRescanEvent(CreateEvent(NULL, FALSE, FALSE, NULL)),
    mThread(NULL),
    dinput(NULL) {
  mObserver = new Observer(*this);
  // Initialize DirectInput
  CoInitialize(NULL);
  if (CoCreateInstance(CLSID_DirectInput8,
                       NULL,
                       CLSCTX_INPROC_SERVER,
                       IID_IDirectInput8W,
                       (LPVOID*)&dinput) == S_OK) {
    if (dinput->Initialize(GetModuleHandle(NULL),
                           DIRECTINPUT_VERSION) != DI_OK) {
      dinput->Release();
      dinput = NULL;
    }
  }
}

// static
BOOL CALLBACK
WindowsGamepadService::EnumObjectsCallback(LPCDIDEVICEOBJECTINSTANCE lpddoi,
                                           LPVOID pvRef) {
  // Ensure that all axes are using the same range.
  Gamepad* gamepad = reinterpret_cast<Gamepad*>(pvRef);
  DIPROPRANGE dp;
  dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
  dp.diph.dwSize = sizeof(DIPROPRANGE);
  dp.diph.dwHow = DIPH_BYID;
  dp.diph.dwObj = lpddoi->dwType;
  dp.lMin = 0;
  dp.lMax = kMaxAxisValue;
  gamepad->device->SetProperty(DIPROP_RANGE, &dp.diph);
  return DIENUM_CONTINUE;
}

// static
BOOL CALLBACK
WindowsGamepadService::EnumCallback(LPCDIDEVICEINSTANCE lpddi,
                                    LPVOID pvRef) {
  WindowsGamepadService* self =
    reinterpret_cast<WindowsGamepadService*>(pvRef);
  // See if this device is already present in our list.
  for (unsigned int i = 0; i < self->mGamepads.Length(); i++) {
    if (memcmp(&lpddi->guidInstance, &self->mGamepads[i].guidInstance,
               sizeof(GUID)) == 0) {
      self->mGamepads[i].present = true;
      return DIENUM_CONTINUE;
    }
  }

  Gamepad gamepad;
  memset(&gamepad, 0, sizeof(Gamepad));
  if (self->dinput->CreateDevice(lpddi->guidInstance, &gamepad.device, NULL)
      == DI_OK) {
    gamepad.present = true;
    memcpy(&gamepad.guidInstance, &lpddi->guidInstance, sizeof(GUID));

    DIDEVICEINSTANCE info;
    info.dwSize = sizeof(DIDEVICEINSTANCE);
    if (gamepad.device->GetDeviceInfo(&info) == DI_OK) {
      WideCharToMultiByte(CP_UTF8, 0, info.tszProductName, -1,
                          gamepad.name, sizeof(gamepad.name), NULL, NULL);
    }
    // Get vendor id and product id
    DIPROPDWORD dp;
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwObj = 0;
    dp.diph.dwHow = DIPH_DEVICE;
    if (gamepad.device->GetProperty(DIPROP_VIDPID, &dp.diph) == DI_OK) {
      sprintf(gamepad.idstring, "%x-%x-%s",
              LOWORD(dp.dwData), HIWORD(dp.dwData), gamepad.name);
    }
    DIDEVCAPS caps;
    caps.dwSize = sizeof(DIDEVCAPS);
    if (gamepad.device->GetCapabilities(&caps) == DI_OK) {
      gamepad.numAxes = caps.dwAxes;
      gamepad.numHats = caps.dwPOVs;
      gamepad.numButtons = caps.dwButtons;
      //XXX: handle polled devices?
      // (caps.dwFlags & DIDC_POLLEDDATAFORMAT || caps.dwFlags & DIDC_POLLEDDEVICE)
    }
    // Set min/max range for all axes on the device.
    gamepad.device->EnumObjects(EnumObjectsCallback, &gamepad, DIDFT_AXIS);
    // Set up structure for setting buffer size for buffered data
    dp.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    dp.diph.dwSize = sizeof(DIPROPDWORD);
    dp.diph.dwObj = 0;
    dp.diph.dwHow = DIPH_DEVICE;
    dp.dwData = 64; // arbitrary
    // Create event so DInput can signal us when there's new data.
    gamepad.event = CreateEvent(NULL, FALSE, FALSE, NULL);
    // Set data format, event notification, and acquire device
    if (gamepad.device->SetDataFormat(&c_dfDIJoystick) == DI_OK && 
        gamepad.device->SetProperty(DIPROP_BUFFERSIZE, &dp.diph) == DI_OK &&
        gamepad.device->SetEventNotification(gamepad.event) == DI_OK &&
        gamepad.device->Acquire() == DI_OK) {
      self->mGamepads.AppendElement(gamepad);
      // Inform the GamepadService
      nsRefPtr<GamepadChangeEvent> event =
        new GamepadChangeEvent(self->mGamepads[self->mGamepads.Length() - 1],
                               GamepadChangeEvent::Added);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    }
    else {
      if (gamepad.device) {
        gamepad.device->SetEventNotification(NULL);
        gamepad.device->Release();
      }
      CloseHandle(gamepad.event);
    }
  }
  return DIENUM_CONTINUE;
}

void
WindowsGamepadService::ScanForDevices() {
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    mGamepads[i].present = false;
  }

  dinput->EnumDevices(DI8DEVCLASS_GAMECTRL,
                      (LPDIENUMDEVICESCALLBACK)EnumCallback,
                      this,
                      DIEDFL_ATTACHEDONLY);

  // Look for devices that were removed.
  for (int i = mGamepads.Length() - 1; i >= 0; i--) {
    if (!mGamepads[i].present) {
      nsRefPtr<GamepadChangeEvent> event =
        new GamepadChangeEvent(mGamepads[i],
                               GamepadChangeEvent::Removed);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      CleanupGamepad(mGamepads[i]);
      mGamepads.RemoveElementAt(i);
    }
  }

  mEvents.Clear();
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    mEvents.AppendElement(mGamepads[i].event);
  }

  // These events must be the  last elements in the array, so that
  // the other elements match mGamepads in order.
  mEvents.AppendElement(mThreadRescanEvent);
  mEvents.AppendElement(mThreadExitEvent);
}

// static
DWORD WINAPI
WindowsGamepadService::DInputThread(LPVOID arg) {
  WindowsGamepadService* self = reinterpret_cast<WindowsGamepadService*>(arg);
  self->ScanForDevices();

  while (true) {
    DWORD result = WaitForMultipleObjects(self->mEvents.Length(),
                                          self->mEvents.Elements(),
                                          FALSE,
                                          INFINITE);
    if (result == WAIT_FAILED ||
        result == WAIT_OBJECT_0 + self->mEvents.Length() - 1) {
      // error, or the main thread signaled us to exit
      break;
    }

    unsigned int i = result - WAIT_OBJECT_0;

    if (i == self->mEvents.Length() - 2) {
      // Main thread is signaling for a device rescan.
      self->ScanForDevices();
      continue;
    }

    if (i >= self->mGamepads.Length()) {
      // Something would be terribly wrong here.
      continue;
    }

    // first query for the number of items in the buffer
    DWORD items = INFINITE;
    LPDIRECTINPUTDEVICE8 device = self->mGamepads[i].device;
    if (device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),
                              NULL,
                              &items,
                              DIGDD_PEEK)== DI_OK) {
      while (items > 0) {
        // now read each buffered event
        //TODO: read more than one event at a time
        DIDEVICEOBJECTDATA data;
        DWORD readCount = sizeof(data) / sizeof(DIDEVICEOBJECTDATA);
        if (device->GetDeviceData(sizeof(DIDEVICEOBJECTDATA),
                                  &data, &readCount, 0) == DI_OK) {
          //TODO: data.dwTimeStamp
          GamepadEvent::Type type = GamepadEvent::Unknown;
          int which;
          if (data.dwOfs >= DIJOFS_BUTTON0 && data.dwOfs < DIJOFS_BUTTON(32)) {
            type = GamepadEvent::Button;
            which = data.dwOfs - DIJOFS_BUTTON0;
          }
          else if(data.dwOfs >= DIJOFS_X  && data.dwOfs < DIJOFS_SLIDER(2)) {
            // axis/slider
            type = GamepadEvent::Axis;
            which = (data.dwOfs - DIJOFS_X) / sizeof(LONG);
          }
          else if (data.dwOfs >= DIJOFS_POV(0) && data.dwOfs < DIJOFS_POV(4)) {
            HatState hatState;
            HatPosToAxes(data.dwData, hatState);
            which = (data.dwOfs - DIJOFS_POV(0)) / sizeof(DWORD);
            // Only send out axis move events for the axes that moved
            // in this hat move.
            if (hatState.x != self->mGamepads[i].hatState[which].x) {
              type = GamepadEvent::HatX;
            }
            if (hatState.y != self->mGamepads[i].hatState[which].y) {
              if (type == GamepadEvent::HatX) {
                type = GamepadEvent::HatXY;
              }
              else {
                type = GamepadEvent::HatY;
              }
            }
            self->mGamepads[i].hatState[which].x = hatState.x;
            self->mGamepads[i].hatState[which].y = hatState.y;
          }

          if (type != GamepadEvent::Unknown) {
            nsRefPtr<GamepadEvent> event =
              new GamepadEvent(self->mGamepads[i], type, which, data.dwData);
            NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
          }
        }
        items--;
      }
    }
  }
  return 0;
}

void
WindowsGamepadService::Startup() {
  mThread = CreateThread(NULL,
                         0,
                         DInputThread,
                         this,
                         0,
                         NULL);
  mStarted = (mThread != NULL);
}

void
WindowsGamepadService::Shutdown() {
  if (mThread) {
    SetEvent(mThreadExitEvent);
    WaitForSingleObject(mThread, INFINITE);
    CloseHandle(mThread);
  }
  Cleanup();
  mStarted = false;
}

void
WindowsGamepadService::Cleanup() {
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    CleanupGamepad(mGamepads[i]);
  }
  mGamepads.Clear();
}

void
WindowsGamepadService::CleanupGamepad(Gamepad& gamepad) {
  gamepad.device->Unacquire();
  gamepad.device->SetEventNotification(NULL);
  gamepad.device->Release();
  CloseHandle(gamepad.event);
}

void
WindowsGamepadService::DevicesChanged() {
  SetEvent(mThreadRescanEvent);
}

NS_IMETHODIMP
Observer::Observe(nsISupports* aSubject,
                  const char* aTopic,
                  const PRUnichar* aData) {
  if (strcmp(aTopic, "timer-callback") == 0) {
    mSvc.DevicesChanged();
  } else {
    // set stable timer, since we will get multiple devices-changed
    // notifications at once
    if (mTimer) {
      mTimer->Cancel();
      mTimer->Init(this, kDevicesChangedStableDelay, nsITimer::TYPE_ONE_SHOT);
    }
  }
  return NS_OK;
}

} // namespace dom
} // namespace mozilla
