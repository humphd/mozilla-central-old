// Pointer Lock tests all need to flip the same prefs.

var testPrefs = (function() {
  var fullScreenApiEnabled = SpecialPowers.getBoolPref("full-screen-api.enabled"),
      fullScreenApiAllowTrusted =
        SpecialPowers.getBoolPref("full-screen-api.allow-trusted-requests-only");

  return {
    init: function() {
      SpecialPowers.setBoolPref("full-screen-api.enabled", true);
      SpecialPowers.setBoolPref("full-screen-api.allow-trusted-requests-only",
                                false);
    },
    restore: function() {
      SpecialPowers.setBoolPref("full-screen-api.enabled", fullScreenApiEnabled);
      SpecialPowers.setBoolPref("full-screen-api.allow-trusted-requests-only",
                                fullScreenApiAllowTrusted);
    }
  };
})();
