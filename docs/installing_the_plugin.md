# Blender plugin

This project has a blender plugin to help edit the blend files. To create it as a zip file run this command.

> I am using blender 4.2.9

```bash
make tools/mesh_export.zip
```

then install the `tools/mesh_export.zip` file generated from running tha command in blender by going to. `Edit -> Preferences` This will open the preferences window. Now you select the `Add-ons` tab and select the drop down arrow in the top right of the window. Select `Install from disk...` and select the zip file. A plugin called `Level editor` should appear. Make sure the check box next to it is check to enable it.

## Sym link

It can be annoying to reinstall the plugin everytime a change is made. You can create a symbolic link to the `tools/mesh_export` folder in the blender plugins folder to make it update anytime the files in the repo are changed. To do this locate where the plugins are installed. This is where it is located on my windows machine `C:\Users\<user>\AppData\Roaming\Blender Foundation\Blender\4.2\scripts\addons`. I open this in a terminal and then create a symbolic link with the name `mesh_export` to the file in this repo. How to do this is operating system dependent so you will need to lookup how that is done.
