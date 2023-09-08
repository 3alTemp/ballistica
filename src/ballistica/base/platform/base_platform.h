// Released under the MIT License. See LICENSE for details.

#ifndef BALLISTICA_BASE_PLATFORM_BASE_PLATFORM_H_
#define BALLISTICA_BASE_PLATFORM_BASE_PLATFORM_H_

#include "ballistica/base/base.h"

namespace ballistica::base {

class BasePlatform {
 public:
  /// Instantiate the appropriate BasePlatform subclass for the current
  /// environment.
  static auto CreatePlatform() -> BasePlatform*;

  /// Instantiate the appropriate App subclass for the current environment.
  static auto CreateAppAdapter() -> AppAdapter*;

  /// Instantiate the appropriate Graphics subsystem for the current
  /// environment.
  static auto CreateGraphics() -> Graphics*;

#pragma mark APP LIFECYCLE -----------------------------------------------------

  /// Inform the platform that all subsystems are up and running and it can
  /// start talking to them.
  virtual void OnMainThreadStartAppComplete();

  virtual void OnAppStart();
  virtual void OnAppPause();
  virtual void OnAppResume();
  virtual void OnAppShutdown();
  virtual void OnScreenSizeChange();
  virtual void DoApplyAppConfig();

  /// Return whether this platform supports soft-quit. A soft quit is
  /// when the app is reset/backgrounded/etc. but remains running in case
  /// needed again. Generally this is the behavior on mobile apps.
  virtual auto CanSoftQuit() -> bool;

  /// Implement soft-quit behavior. Will always be called in the logic
  /// thread. Make sure to also override CanBackQuit to reflect this being
  /// present. Note that when quitting the app yourself, you should use
  /// g_base->QuitApp(); do not call this directly.
  virtual void DoSoftQuit();

  /// Return whether this platform supports back-quit. A back quit is a
  /// variation of soft-quit generally triggered by a back button, which may
  /// give different results in the OS. For example on Android this may
  /// result in jumping back to the previous Android activity instead of
  /// just ending the current one and dumping to the home screen as normal
  /// soft quit might do.
  virtual auto CanBackQuit() -> bool;

  /// Implement back-quit behavior. Will always be called in the logic
  /// thread. Make sure to also override CanBackQuit to reflect this being
  /// present. Note that when quitting the app yourself, you should use
  /// g_base->QuitApp(); do not call this directly.
  virtual void DoBackQuit();

  /// Terminate the app. This can be immediate or by posting some high
  /// level event. There should be nothing left to do in the engine at
  /// this point.
  virtual void TerminateApp();

#pragma mark IN APP PURCHASES --------------------------------------------------

  void Purchase(const std::string& item);

  // Restore purchases (currently only relevant on Apple platforms).
  virtual void RestorePurchases();

  // Purchase was ack'ed by the master-server (so can consume).
  virtual void PurchaseAck(const std::string& purchase,
                           const std::string& order_id);

#pragma mark ENVIRONMENT -------------------------------------------------------

  /// Get a UUID for the current device that is meant to be publicly shared.
  /// This value will change occasionally due to OS updates, app updates, or
  /// other factors, so it can not be used as a permanent identifier, but it
  /// should remain constant over short periods and should not be easily
  /// changeable by the user, making it useful for purposes such as temporary
  /// server bans or spam prevention.
  auto GetPublicDeviceUUID() -> std::string;

  /// Called when the app should set itself up to intercept ctrl-c presses.
  virtual void SetupInterruptHandling();

#pragma mark INPUT DEVICES -----------------------------------------------------

  // Return a name for a ballistica keycode.
  virtual auto GetKeyName(int keycode) -> std::string;

#pragma mark ACCOUNTS ----------------------------------------------------------

  /// Called when a Python LoginAdapter is requesting an explicit sign-in.
  virtual void LoginAdapterGetSignInToken(const std::string& login_type,
                                          int attempt_id);
  /// Called when a Python LoginAdapter is informing us that a back-end is
  /// active/inactive.
  virtual void LoginAdapterBackEndActiveChange(const std::string& login_type,
                                               bool active);
#pragma mark MISC --------------------------------------------------------------

  /// Open the provided URL in a browser or whatnot.
  void OpenURL(const std::string& url);

  /// Get the most up-to-date cursor position.
  void GetCursorPosition(float* x, float* y);

 protected:
  /// Open the provided URL in a browser or whatnot.
  virtual void DoOpenURL(const std::string& url);

  /// Make a purchase.
  virtual void DoPurchase(const std::string& item);

  BasePlatform();
  virtual ~BasePlatform();

 private:
  /// Called after our singleton has been instantiated.
  /// Any construction functionality requiring virtual functions resolving to
  /// their final class versions can go here.
  virtual void PostInit();

  bool ran_base_post_init_{};
  std::string public_device_uuid_;
};

}  // namespace ballistica::base

#endif  // BALLISTICA_BASE_PLATFORM_BASE_PLATFORM_H_
