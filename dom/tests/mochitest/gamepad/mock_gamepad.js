var GamepadService = (function() {
  Components.utils.import("resource://gre/modules/ctypes.jsm");
  Components.utils.import("resource://gre/modules/Services.jsm");
  var gred = Services.dirsvc.get("GreD", Components.interfaces.nsIFile);
                        //XXX: cross-platform
  var lib = ctypes.open(gred.path + "/XUL");
                          //XXX: mangling+abi for windows
  var GetGamepadService = lib.declare("_ZN7mozilla3dom14GamepadService10GetServiceEv",
                                      ctypes.default_abi,
                                      ctypes.voidptr_t); // returns GamepadService*

  var GamepadService = {
    self: GetGamepadService(),
    AddGamepad: function(id, numButtons, numAxes) {
      return this._AddGamepad(this.self, id, numButtons, numAxes);
    },
    _AddGamepad: lib.declare("_ZN7mozilla3dom14GamepadService10AddGamepadEPKcjj",
                             ctypes.default_abi,
                             ctypes.uint32_t,  // returns index
                             ctypes.voidptr_t, // GamepadService*
                             ctypes.char.ptr,  // id
                             ctypes.uint32_t,  // numButtons
                             ctypes.uint32_t), // numAxes
    RemoveGamepad: function(index) {
      this._RemoveGamepad(this.self, index);
    },
    _RemoveGamepad: lib.declare("_ZN7mozilla3dom14GamepadService13RemoveGamepadEj",
                                ctypes.default_abi,
                                ctypes.void_t,
                                ctypes.voidptr_t, // GamepadService*
                                ctypes.uint32_t), // index
    NewButtonEvent: function(index, button, pressed) {
      this._NewButtonEvent(this.self, index, button, pressed);
    },
    _NewButtonEvent: lib.declare("_ZN7mozilla3dom14GamepadService14NewButtonEventEjjb",
                                 ctypes.default_abi,
                                 ctypes.void_t,
                                 ctypes.voidptr_t, // GamepadService*
                                 ctypes.uint32_t,  // index
                                 ctypes.uint32_t,  // button
                                 ctypes.bool),     // pressed
    NewAxisMoveEvent: function(index, axis, value) {
      this._NewAxisMoveEvent(this.self, index, axis, value);
    },
    _NewAxisMoveEvent: lib.declare("_ZN7mozilla3dom14GamepadService16NewAxisMoveEventEjjf",
                                     ctypes.default_abi,
                                     ctypes.void_t,
                                     ctypes.voidptr_t, // GamepadService*
                                     ctypes.uint32_t,  // index
                                     ctypes.uint32_t,  // axis
                                     ctypes.float)     // value
  };
  return GamepadService;
})();
