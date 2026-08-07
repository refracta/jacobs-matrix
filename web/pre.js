Module.preRun = Module.preRun || [];
Module.preRun.push(function () {
  // The custom libtcod renderer replaces every screen pixel while locked.
  // Avoid an unnecessary Canvas getImageData() copy on each frame.
  if (typeof SDL !== "undefined") {
    SDL.defaults.copyOnLock = false;
    SDL.defaults.discardOnLock = true;
  }

  // Mount browser-persistent storage before main() attempts to load a save.
  if (typeof FS === "undefined" || typeof IDBFS === "undefined") {
    return;
  }

  try {
    FS.mkdir("/save");
  } catch (error) {
    if (!error || error.errno !== 20) {
      console.warn("Could not create the save directory", error);
    }
  }

  try {
    FS.mount(IDBFS, {}, "/save");
  } catch (error) {
    console.warn("Could not mount persistent save storage", error);
    return;
  }

  addRunDependency("jacob-idbfs");
  FS.syncfs(true, function (error) {
    if (error) {
      console.warn("Could not restore the saved game", error);
    }
    removeRunDependency("jacob-idbfs");
  });
});
