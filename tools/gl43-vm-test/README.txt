Pixie OpenGL 4.3 VM graphics test

1. Enable Accelerate 3D Graphics in VMware and install VMware Tools in Windows.
2. Extract the entire ZIP into a writable folder inside the Windows guest.
3. From that folder, double-click Run VM Graphics Test.cmd.
4. The exported Light Lab player runs for five seconds and saves its last frame.
5. The Editor opens the same scene. Check that the viewport shows the sprites,
   lights and shadows. Click Play and Stop, then close the Editor.
6. Bring back Results/player.log, Results/player.png and Results/editor.log.
   If startup fails, bring back the logs and the visible error message.

The test requests an OpenGL 4.3 core context and uses GLSL 430 and the test
bindings even if the graphics driver returns a newer context. Check GL_VERSION,
GL_RENDERER and GL_VENDOR in the logs to identify the actual guest driver.

The package includes the x64 Release runtime, an installed test SDK, a
self-contained Editor and the exported scene. Compilation stays on the host.
This package is for the VM graphics smoke test.
