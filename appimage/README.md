Creating AppImage
=================

Downloading dependencies
------------------------

Downloading linuxdeploy-x86_64.AppImage and appimagetool-x86_64.AppImage at:

- AppImageTool: https://github.com/AppImage/AppImageKit/releases
- LinuxDeploy: https://github.com/linuxdeploy/linuxdeploy/releases

Be sure that both AppImage are in the current directory and are executable

Generating the Aquaria AppImage
------------------------------------------

Compile the `aquaria` executable as specified in the project root README.md. Then put the executable in the current directory.

Now for the AppImage generation.

```bash
./linuxdeploy-x86_64.AppImage --executable aquaria --appdir AppDir --icon-file aquaria.png  --desktop-file aquaria.desktop
cp -rp ../files AppDir/usr/bin/override
./appimagetool-x86_64.AppImage AppDir/
```
