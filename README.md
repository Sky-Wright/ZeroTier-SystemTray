# ZeroTier-SystemTray

A lightweight system tray icon for status and control for ZeroTier on Linux.

Built natively in C++ using Qt5/Qt6, this app integrates directly into your desktop environment. It provides a simple status indicator and service toggle, allowing you to run ZeroTier on-demand without keeping the daemon running 24/7 or dealing with heavy third-party clients.

## Features
- **Dynamic Status Indicator:** Red (stopped), Orange (running/disconnected), and Blue (running/connected).
- **Passwordless Service Toggle:** Toggle the `zerotier-one` systemd service silently using granular Polkit rules (no sudo password prompts).
- **Start Menu Integration:** Desktop launcher file included.
- **Lightweight & Transparent:** No network tokens or credentials accessed; queries status natively via systemd and interface checks.

## Installation (Arch Linux)
Clone the repository and build/install the package:
```bash
git clone https://github.com/yourusername/ZeroTier-SystemTray.git
cd ZeroTier-SystemTray
makepkg -si
```

## Polkit Compatibility
By default, the Polkit rule is configured to allow members of the **`wheel`** group (the standard admin group on Arch/Fedora) to manage the ZeroTier systemd service passwordlessly.

If you are on a Debian- or Ubuntu-based distribution, you can change the group in `/usr/share/polkit-1/rules.d/49-zerotier.rules` (or `49-zerotier.rules` in the repository) from `"wheel"` to `"sudo"`:
```javascript
subject.isInGroup("sudo")
```

## License
Licensed under the [MIT License](LICENSE).
