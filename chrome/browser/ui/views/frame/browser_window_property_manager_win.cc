// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/frame/browser_window_property_manager_win.h"

#include "base/command_line.h"
#include "base/prefs/pref_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/windows_version.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_shortcut_manager_win.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/host_desktop.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/common/pref_names.h"
#include "ui/base/win/shell.h"
#include "ui/views/win/hwnd_util.h"

BrowserWindowPropertyManager::BrowserWindowPropertyManager(BrowserView* view)
    : view_(view) {
  DCHECK(view_);
  profile_pref_registrar_.Init(view_->browser()->profile()->GetPrefs());

  // Monitor the profile icon version on Windows so that we can set the browser
  // relaunch icon when the version changes (e.g on initial icon creation).
  profile_pref_registrar_.Add(
      prefs::kProfileIconVersion,
      base::Bind(&BrowserWindowPropertyManager::OnProfileIconVersionChange,
                 base::Unretained(this)));
}

BrowserWindowPropertyManager::~BrowserWindowPropertyManager() {
}

void BrowserWindowPropertyManager::UpdateWindowProperties(HWND hwnd) {
  DCHECK(hwnd);
  Browser* browser = view_->browser();
  Profile* profile = browser->profile();

  // Set the app user model id for this application to that of the application
  // name. See http://crbug.com/7028.
  base::string16 app_id = browser->is_app() ?
      ShellIntegration::GetAppModelIdForProfile(
          base::UTF8ToWide(browser->app_name()),
          profile->GetPath()) :
      ShellIntegration::GetChromiumModelIdForProfile(profile->GetPath());
  base::string16 icon_path_string;
  base::string16 command_line_string;
  base::string16 pinned_name;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  ProfileShortcutManager* shortcut_manager = NULL;

  // The profile manager may be NULL in testing.
  if (profile_manager)
    shortcut_manager = profile_manager->profile_shortcut_manager();

  if (!browser->is_app() && shortcut_manager &&
      profile->GetPrefs()->HasPrefPath(prefs::kProfileIconVersion)) {
    const base::FilePath& profile_path = profile->GetPath();

    // Set relaunch details to use profile.
    CommandLine command_line(CommandLine::NO_PROGRAM);
    base::FilePath icon_path;
    shortcut_manager->GetShortcutProperties(profile_path, &command_line,
                                            &pinned_name, &icon_path);
    command_line_string = command_line.GetCommandLineString();
    icon_path_string = icon_path.value();
  }
  ui::win::SetAppDetailsForWindow(
      app_id,
      icon_path_string,
      command_line_string,
      pinned_name,
      hwnd);
}

// static
scoped_ptr<BrowserWindowPropertyManager>
    BrowserWindowPropertyManager::CreateBrowserWindowPropertyManager(
        BrowserView* view) {
  if (base::win::GetVersion() < base::win::VERSION_WIN7 ||
      view->browser()->host_desktop_type() == chrome::HOST_DESKTOP_TYPE_ASH) {
    return scoped_ptr<BrowserWindowPropertyManager>();
  }

  return scoped_ptr<BrowserWindowPropertyManager>(
      new BrowserWindowPropertyManager(view));
}

void BrowserWindowPropertyManager::OnProfileIconVersionChange() {
  UpdateWindowProperties(views::HWNDForNativeWindow(view_->GetNativeWindow()));
}
