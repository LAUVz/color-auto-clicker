const electron = require('electron');
const path = require('path');
const robot = require('robotjs');

const { app, BrowserWindow, globalShortcut, ipcMain } = electron;

let mainWindow: any = null;
let isDetecting = false;
let detectionMode: 'target' | 'change' = 'target';
let targetColor: { r: number; g: number; b: number } | null = null;
let detectionInterval: NodeJS.Timeout | null = null;

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 400,
    height: 420,
    resizable: false,
    alwaysOnTop: true,
    autoHideMenuBar: true,
    webPreferences: {
      nodeIntegration: true,
      contextIsolation: false,
    },
    frame: true,
    icon: path.join(__dirname, '../assets/pointer.ico'),
  });

  mainWindow.setMenuBarVisibility(false);
  mainWindow.loadFile(path.join(__dirname, '../src/index.html'));
  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

function hexToRgb(hex: string): { r: number; g: number; b: number } {
  const num = parseInt(hex, 16);
  return {
    r: (num >> 16) & 255,
    g: (num >> 8) & 255,
    b: num & 255,
  };
}

function getColorAtCursor(): { r: number; g: number; b: number } {
  const pos = robot.getMousePos();
  const hexColor = robot.getPixelColor(pos.x, pos.y);
  return hexToRgb(hexColor);
}

function colorsEqual(c1: { r: number; g: number; b: number }, c2: { r: number; g: number; b: number }): boolean {
  return c1.r === c2.r && c1.g === c2.g && c1.b === c2.b;
}

function performClick() {
  robot.mouseClick();
  setTimeout(() => {}, 50);
}

function startDetection() {
  if (isDetecting) return;

  isDetecting = true;
  mainWindow?.webContents.send('detection-started');

  if (detectionMode === 'change') {
    const baselineColor = getColorAtCursor();

    detectionInterval = setInterval(() => {
      try {
        const currentColor = getColorAtCursor();
        mainWindow?.webContents.send('current-color', currentColor);

        if (!colorsEqual(currentColor, baselineColor)) {
          performClick();
          stopDetection();
          mainWindow?.webContents.send('click-triggered');
        }
      } catch (error) {
        console.error('Detection error:', error);
      }
    }, 1);
  } else if (detectionMode === 'target' && targetColor) {
    detectionInterval = setInterval(() => {
      try {
        const currentColor = getColorAtCursor();
        mainWindow?.webContents.send('current-color', currentColor);

        if (colorsEqual(currentColor, targetColor!)) {
          performClick();
          stopDetection();
          mainWindow?.webContents.send('click-triggered');
        }
      } catch (error) {
        console.error('Detection error:', error);
      }
    }, 1);
  }
}

function stopDetection() {
  if (!isDetecting) return;

  isDetecting = false;
  if (detectionInterval) {
    clearInterval(detectionInterval);
    detectionInterval = null;
  }
  mainWindow?.webContents.send('detection-stopped');
}

app.whenReady().then(() => {
  createWindow();

  globalShortcut.register('num8', () => {
    if (!isDetecting) {
      const color = getColorAtCursor();
      targetColor = color;
      mainWindow?.webContents.send('color-picked', color);
    }
  });

  globalShortcut.register('num2', () => {
    if (isDetecting) {
      stopDetection();
    } else {
      startDetection();
    }
  });

  globalShortcut.register('num0', () => {
    app.quit();
  });

  setInterval(() => {
    if (!isDetecting) {
      try {
        const color = getColorAtCursor();
        mainWindow?.webContents.send('current-color', color);
      } catch (error) {
        // Ignore errors
      }
    }
  }, 50);
});

app.on('window-all-closed', () => {
  globalShortcut.unregisterAll();
  if (process.platform !== 'darwin') {
    app.quit();
  }
});

app.on('activate', () => {
  if (mainWindow === null) {
    createWindow();
  }
});

ipcMain.on('set-mode', (_, mode: 'target' | 'change') => {
  detectionMode = mode;
});

ipcMain.on('pick-color', () => {
  if (!isDetecting) {
    const color = getColorAtCursor();
    targetColor = color;
    mainWindow?.webContents.send('color-picked', color);
  }
});

ipcMain.on('toggle-detection', () => {
  if (isDetecting) {
    stopDetection();
  } else {
    startDetection();
  }
});
