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
 * This file defines a wrapper around libudev so we can avoid
 * linking directly to it and use dlopen instead.
 */

#ifndef DOM_SYSTEM_UDEV_H_
#define DOM_SYSTEM_UDEV_H_

#include <dlfcn.h>

namespace mozilla {
namespace dom {

struct udev;
struct udev_device;
struct udev_enumerate;
struct udev_list_entry;
struct udev_monitor;

class udev_lib {
public:
  udev_lib() : lib(dlopen("libudev.so", RTLD_LAZY | RTLD_GLOBAL)),
               udev(NULL) {
    if (lib && LoadSymbols())
      udev = udev_new();
  }

  ~udev_lib() {
    if (udev) {
      udev_unref(udev);
    }

    if (lib) {
      dlclose(lib);
    }
  }

  operator bool() {
    return lib && udev;
  }

  bool LoadSymbols() {
#define DLSYM(s) \
  do { \
    s = (typeof (s))dlsym(lib, #s); \
    if (!s) return false; \
  } while (0)

    DLSYM(udev_new);
    DLSYM(udev_unref);
    DLSYM(udev_device_unref);
    DLSYM(udev_device_new_from_syspath);
    DLSYM(udev_device_get_devnode);
    DLSYM(udev_device_get_property_value);
    DLSYM(udev_device_get_action);
    DLSYM(udev_enumerate_new);
    DLSYM(udev_enumerate_unref);
    DLSYM(udev_enumerate_add_match_subsystem);
    DLSYM(udev_enumerate_scan_devices);
    DLSYM(udev_enumerate_get_list_entry);
    DLSYM(udev_list_entry_get_next);
    DLSYM(udev_list_entry_get_name);
    DLSYM(udev_monitor_new_from_netlink);
    DLSYM(udev_monitor_filter_add_match_subsystem_devtype);
    DLSYM(udev_monitor_enable_receiving);
    DLSYM(udev_monitor_get_fd);
    DLSYM(udev_monitor_receive_device);
    DLSYM(udev_monitor_unref);
#undef DLSYM
    return true;
  }

  void* lib;
  struct udev* udev;

  // Function pointers returned from dlsym.
  struct udev* (*udev_new)(void);
  void (*udev_unref)(struct udev*);

  void (*udev_device_unref)(struct udev_device*);
  struct udev_device* (*udev_device_new_from_syspath)(struct udev*,
                                                      const char*);
  const char* (*udev_device_get_devnode)(struct udev_device*);
  const char* (*udev_device_get_property_value)(struct udev_device*,
                                                const char*);
  const char* (*udev_device_get_action)(struct udev_device*);

  struct udev_enumerate* (*udev_enumerate_new)(struct udev*);
  void (*udev_enumerate_unref)(struct udev_enumerate*);
  int (*udev_enumerate_add_match_subsystem)(struct udev_enumerate*,
                                            const char*);
  int (*udev_enumerate_scan_devices)(struct udev_enumerate*);
  struct udev_list_entry* (*udev_enumerate_get_list_entry)(struct udev_enumerate*);

  struct udev_list_entry* (*udev_list_entry_get_next)(struct udev_list_entry *);
  const char* (*udev_list_entry_get_name)(struct udev_list_entry*);

  struct udev_monitor* (*udev_monitor_new_from_netlink)(struct udev*,
                                                        const char*);
  int (*udev_monitor_filter_add_match_subsystem_devtype)
    (struct udev_monitor*, const char*, const char*);
  int (*udev_monitor_enable_receiving)(struct udev_monitor*);
  int (*udev_monitor_get_fd)(struct udev_monitor*);
  struct udev_device* (*udev_monitor_receive_device)(struct udev_monitor*);
  void (*udev_monitor_unref)(struct udev_monitor*);
};

}
}


#endif // DOM_SYSTEM_UDEV_H_
