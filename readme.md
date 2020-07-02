# Android Screen Stream

Remote Control Android from Linux PC.

Build for Android 4.4.2 on Debian Stretch. Require Android NDK.

## Android Side


- [x] send screen frame
- [x] make touchscreen work
- [ ] Reduce processor usage, HW rendering or optimize code
- [ ] Detect device sleep/screenlocked, dont send any frame
- [ ] Make keyboard typing work. using /dev/uinput or java app

## Linux Side

- [x] recv frame, display in X11
- [x] send touch input
- [ ] Auto push android app & start if not running
- [ ] Add hardware button power & volume
