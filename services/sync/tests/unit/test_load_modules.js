const modules = [
                 "addonsreconciler.js",
                 "async.js",
                 "constants.js",
                 "engines/addons.js",
                 "engines/bookmarks.js",
                 "engines/clients.js",
                 "engines/forms.js",
                 "engines/history.js",
                 "engines/passwords.js",
                 "engines/prefs.js",
                 "engines/tabs.js",
                 "engines.js",
                 "ext/Observers.js",
                 "ext/Preferences.js",
                 "identity.js",
                 "jpakeclient.js",
                 "keys.js",
                 "log4moz.js",
                 "main.js",
                 "notifications.js",
                 "policies.js",
                 "record.js",
                 "resource.js",
                 "rest.js",
                 "service.js",
                 "status.js",
                 "util.js",
];

function run_test() {
  for each (let m in modules) {
    _("Attempting to load resource://services-sync/" + m);
    Cu.import("resource://services-sync/" + m, {});
  }
}

