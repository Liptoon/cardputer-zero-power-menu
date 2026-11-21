# Application Store Publish Record

Keep this file in the application project root. Update it when package
metadata, listing copy, assets, privacy choices, or submission status changes.

Portal: <https://dev.cardputer.cc/#/upload>

## Package

- Debian package path: `dist/power-menu_0.1.2_arm64.deb` (build with `make deb`)
- Package: `power-menu`
- Version: `0.1.2`
- Architecture: `arm64`
- Maintainer: `Publius <liptoon@gmail.com>`
- APPLaunch desktop path: `/usr/share/APPLaunch/applications/power-menu.desktop`
- Executable path: `/usr/share/APPLaunch/bin/power-menu`

## Source and Ownership

- Public source repository: `https://github.com/Liptoon/cardputer-zero-power-menu`
- Expected GitHub uploader/owner: `Liptoon`
- Hide uploader email: `true`

## Listing

- Application name: `Power Menu`
- One-line summary (maximum 80 characters): `Restart or shut down the CardputerZero safely`
- Description: `Proste menu z restartem i bezpiecznym wyłączaniem systemu CardputerZero. Wybór potwierdzasz przyciskami, a ekran czeka do faktycznego rebootu lub poweroffu.`
- Categories (maximum 6): `System Tools`
- License: `MIT` (own code); icon `power_settings_new` stays Apache-2.0, see `assets/ICON-SOURCE.md`
- Share code: `ZPWR`
- Author: `Publius` (GitHub `Liptoon`)
- Permissions: camera, microphone, IMU, network, additional hardware, background service and external display all `false`

## Assets

- 256 x 256 source icon: `applaunch/share/images/power-menu.png`
- Icon packaged in `.deb`: `share/images/power-menu.png`
- Screenshot 1 (320 x 170): `store/screenshots/menu.png`
- Screenshot 2 (320 x 170): `store/screenshots/confirm-restart.png`
- Screenshot 3 (320 x 170): `store/screenshots/confirm-shutdown.png`
- Screenshot 4 (320 x 170): `store/screenshots/menu-shutdown.png`
- Optional screenshot 5 (portal supports up to 6): not provided
- Optional screenshot 6 (portal supports up to 6): not provided
- First-release demo video: `zpwr.mp4`

## Preflight

- [ ] `.deb` exists and can be parsed.
- [ ] Control fields contain Package, Version, Architecture, and Maintainer.
- [ ] Architecture is appropriate for the package (`arm64` for native binaries).
- [ ] APPLaunch `.desktop`, executable, and PNG icon exist inside the package.
- [ ] Package name ownership and update version are valid.
- [ ] Listing copy and categories have been reviewed.
- [x] Four clean 320 x 170 baseline screenshots exist.
- [ ] No credentials, tokens, or private server details are embedded.
- [x] The user explicitly approved external submission.

## Submission Result

- Status: `ready to submit with czdev publish`
- Submitted at: pending
- Portal message: pending
- Actions URL: pending
- Tracking or pull-request URL: pending
