'use strict';

import { app, BrowserWindow } from 'electron';
import { createTray, createMainWindow, createWorder } from './windows.js'

app.on('ready', () => {
    const worker = createWorder(); 

    const mainWindow = createMainWindow(app);

    const tray = createTray(mainWindow, app);
});

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});

app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
        createWindow();
    }
});