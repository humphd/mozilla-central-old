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

/*
 * LinuxGamepadService: A Linux backend for the GamepadService.
 * Derived from the kernel documentation at
 * http://www.kernel.org/doc/Documentation/input/joystick-api.txt
 */
#include <algorithm>
#include <cstddef>

#include <fcntl.h>
#include <limits.h>
#include <prthread.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "nsThreadUtils.h"
#include "mozilla/dom/GamepadService.h"
#include "udev.h"

// Include this later because it also does #define JS_VERSION
#include <linux/joystick.h>

namespace mozilla {
namespace dom {

static const float kMaxAxisValue = 32767.0;
static const char kJoystickPath[] = "/dev/input/js";

//TODO: should find a USB identifier for each device so we can
// provide something that persists across connect/disconnect cycles.
typedef struct {
  int index;
  int fd;
  int numAxes;
  int numButtons;
  char idstring[128];
  char devpath[PATH_MAX];
} Gamepad;

// Used to post events from the background thread to the foreground thread.
class GamepadEvent : public nsRunnable {
public:
  GamepadEvent(const Gamepad& gamepad,
               const struct js_event& event) : mGamepad(gamepad),
                                               mEvent(event) {
  }

  NS_IMETHOD Run() {
    GamepadService* gamepadsvc = GamepadService::GetService();
    switch (mEvent.type) {
    case JS_EVENT_BUTTON:
      gamepadsvc->NewButtonEvent(mGamepad.index, mEvent.number,
                                 (bool)mEvent.value);
      break;
    case JS_EVENT_AXIS:
      gamepadsvc->NewAxisMoveEvent(mGamepad.index, mEvent.number, 
                                   ((float)mEvent.value) / kMaxAxisValue);
      break;
    }
    
    return NS_OK;
  }

  const Gamepad& mGamepad;
  struct js_event mEvent;
};

class GamepadChangeEvent : public nsRunnable {
public:
  enum Type {
    Added,
    Removed
  };
  GamepadChangeEvent(Gamepad& gamepad,
                     Type type) : mGamepad(gamepad),
                                  mIndex(gamepad.index),
                                  mType(type) {
  }

  NS_IMETHOD Run() {
    GamepadService* gamepadsvc = GamepadService::GetService();
    if (mType == Added) {
      mGamepad.index = gamepadsvc->AddGamepad(mGamepad.idstring,
                                              (int)mGamepad.numButtons,
                                              (int)mGamepad.numAxes);
    }
    else {
      gamepadsvc->RemoveGamepad(mIndex);
    }
    return NS_OK;
  }

private:
  Gamepad& mGamepad;
  PRUint32 mIndex;
  Type mType;
};

class LinuxGamepadService : public GamepadService {
public:
  LinuxGamepadService() : mDeviceThread(NULL) {
    if (pipe(mPipefds) == -1)
      mPipefds[0] = mPipefds[1] = -1;
  }
  virtual ~LinuxGamepadService() {
    if (mPipefds[0] != -1) {
      close(mPipefds[0]);
      close(mPipefds[1]);
    }
  }

private:
  virtual void Startup();
  virtual void Shutdown();
  void AddDevice(struct udev_device* dev);
  void RemoveDevice(struct udev_device* dev);
  void ScanForDevices();
  void Cleanup();
  // Post an event to the main thread
  void PostEvent(const Gamepad& gamepad, const struct js_event& event);
  bool is_gamepad(struct udev_device* dev);


  udev_lib mUdev;
  PRThread* mDeviceThread;
  // Used to signal the background thread to quit.
  int mPipefds[2];
  // Information about currently connected gamepads.
  nsAutoTArray<Gamepad,4> mGamepads;

  static void DeviceThread(void *arg);
};

// Called by GamepadService to instantiate the singleton.
GamepadService* CreateGamepadService() {
  //TODO: prevent double-calling?
  return new LinuxGamepadService();
}

void
LinuxGamepadService::AddDevice(struct udev_device* dev) {
  const char* devpath = mUdev.udev_device_get_devnode(dev);
  if (!devpath) {
    return;
  }

  Gamepad gamepad;
  strncpy(gamepad.devpath, devpath, sizeof(gamepad.devpath));
  gamepad.fd = open(devpath, O_RDONLY | O_NONBLOCK);
  if (gamepad.fd != -1) {
    char name[128];
    ioctl(gamepad.fd, JSIOCGNAME(sizeof(name)), &name);
    snprintf(gamepad.idstring, sizeof(gamepad.idstring),
             "%s-%s-%s",
             mUdev.udev_device_get_property_value(dev,"ID_VENDOR_ID"),
             mUdev.udev_device_get_property_value(dev, "ID_MODEL_ID"),
             name);

    char numAxes, numButtons;
    ioctl(gamepad.fd, JSIOCGAXES, &numAxes);
    gamepad.numAxes = numAxes;
    ioctl(gamepad.fd, JSIOCGBUTTONS, &numButtons);
    gamepad.numButtons = numButtons;
    mGamepads.AppendElement(gamepad);
    // Inform the GamepadService
    nsRefPtr<GamepadChangeEvent> event =
      new GamepadChangeEvent(mGamepads[mGamepads.Length() - 1],
                             GamepadChangeEvent::Added);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

void
LinuxGamepadService::RemoveDevice(struct udev_device* dev) {
  const char* devpath = mUdev.udev_device_get_devnode(dev);
  if (!devpath) {
    return;
  }

  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    if (strcmp(mGamepads[i].devpath, devpath) == 0) {
      nsRefPtr<GamepadChangeEvent> event =
        new GamepadChangeEvent(mGamepads[i],
                               GamepadChangeEvent::Removed);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      close(mGamepads[i].fd);
      mGamepads.RemoveElementAt(i);
      break;
    }
  }
}

void
LinuxGamepadService::ScanForDevices() {
  struct udev_enumerate* en = mUdev.udev_enumerate_new(mUdev.udev);
  mUdev.udev_enumerate_add_match_subsystem(en, "input");
  mUdev.udev_enumerate_scan_devices(en);

  struct udev_list_entry* dev_list_entry;
  for (dev_list_entry = mUdev.udev_enumerate_get_list_entry(en);
       dev_list_entry != NULL;
       dev_list_entry = mUdev.udev_list_entry_get_next(dev_list_entry)) {
    const char* path = mUdev.udev_list_entry_get_name(dev_list_entry);
    struct udev_device* dev = mUdev.udev_device_new_from_syspath(mUdev.udev,
                                                                 path);
    if (!is_gamepad(dev))
      continue;
    AddDevice(dev);
    mUdev.udev_device_unref(dev);
  }

  mUdev.udev_enumerate_unref(en);
}

void
LinuxGamepadService::PostEvent(const Gamepad& gamepad,
                               const struct js_event& event) {
  nsRefPtr<GamepadEvent> gevent = new GamepadEvent(gamepad, event);
  NS_DispatchToMainThread(gevent, NS_DISPATCH_NORMAL);
}

// static
void
LinuxGamepadService::DeviceThread(void *arg) {
  LinuxGamepadService* self = reinterpret_cast<LinuxGamepadService*>(arg);
  self->ScanForDevices();

  // Add a monitor to watch for device changes
  struct udev_monitor *monitor =
    self->mUdev.udev_monitor_new_from_netlink(self->mUdev.udev, "udev");
  self->mUdev.udev_monitor_filter_add_match_subsystem_devtype(monitor,
                                                              "input", NULL);
  self->mUdev.udev_monitor_enable_receiving(monitor);
  int monitor_fd = self->mUdev.udev_monitor_get_fd(monitor);

  while (true) {
    // Add all file descriptors to the fd_set.
    fd_set fds;
    int maxfd = 0;
    FD_ZERO(&fds);
    for (unsigned int i = 0; i < self->mGamepads.Length(); i++) {
      FD_SET(self->mGamepads[i].fd, &fds);
      maxfd = std::max(maxfd, self->mGamepads[i].fd);
    }
    // Watch the pipe that the main thread uses to signal this thread to exit.
    FD_SET(self->mPipefds[0], &fds);
    maxfd = std::max(maxfd, self->mPipefds[0]);
    // Watch the udev monitor for new events.
    FD_SET(monitor_fd, &fds);
    maxfd = std::max(maxfd, monitor_fd);

    // Wait for some data.
    if (select(maxfd + 1, &fds, NULL, NULL, NULL) == -1)
      break;

    if (FD_ISSET(self->mPipefds[0], &fds)) {
      // The main thread has signaled this thread to exit.
      uint8_t byte;
      if (read(self->mPipefds[0], &byte, 1) == 1)
        break;
    }

    if (FD_ISSET(monitor_fd, &fds)) {
      struct udev_device* dev =
        self->mUdev.udev_monitor_receive_device(monitor);
      const char* action = self->mUdev.udev_device_get_action(dev);
      if (self->is_gamepad(dev)) {
        if (strcmp(action, "add") == 0)
          self->AddDevice(dev);
        else if (strcmp(action, "remove") == 0)
          self->RemoveDevice(dev);
      }
      self->mUdev.udev_device_unref(dev);
    }

    for (unsigned int i = 0; i < self->mGamepads.Length(); i++) {
      if(FD_ISSET(self->mGamepads[i].fd, &fds)) {
        //TODO: read >1 event per read
        struct js_event event;
        ssize_t count = read(self->mGamepads[i].fd, &event,
                             sizeof(struct js_event));
        if (count > 0) {
          //TODO: store device state
          if (event.type & JS_EVENT_INIT)
            continue;

          self->PostEvent(self->mGamepads[i], event);
        }
      }
    }
  }

  self->mUdev.udev_monitor_unref(monitor);
}

void
LinuxGamepadService::Startup() {
  if (mPipefds[0] == -1)
    return;

  // Don't bother starting the device thread if
  // libudev couldn't be loaded or initialized.
  if (!mUdev)
    return;

  mDeviceThread = PR_CreateThread(PR_USER_THREAD,
                                  DeviceThread,
                                  this,
                                  PR_PRIORITY_NORMAL,
                                  PR_GLOBAL_THREAD,
                                  PR_JOINABLE_THREAD,
                                  0);
  mStarted = (mDeviceThread != NULL);
}

void
LinuxGamepadService::Shutdown() {
  uint8_t byte = 0;
  if (write(mPipefds[1], &byte, 1) != -1)
    PR_JoinThread(mDeviceThread);
  mDeviceThread = NULL;
  Cleanup();
  mStarted = false;
}

void
LinuxGamepadService::Cleanup() {
  for (unsigned int i = 0; i < mGamepads.Length(); i++) {
    close(mGamepads[i].fd);
  }
  mGamepads.Clear();
}

bool
LinuxGamepadService::is_gamepad(struct udev_device* dev) {
  if (!mUdev.udev_device_get_property_value(dev, "ID_INPUT_JOYSTICK"))
    return false;
  
  const char* devpath = mUdev.udev_device_get_devnode(dev);
  if (!devpath) {
    return false;
  }
  if (strncmp(kJoystickPath, devpath, sizeof(kJoystickPath) - 1) != 0) {
    return false;
  }

  return true;
}

}
}
