'use strict';

import { app, BrowserWindow, Tray, Menu } from 'electron';
import { join } from 'node:path';
import { createTray } from './windows.js'

let mainWindow;

app.on('ready', () => {
    mainWindow = new BrowserWindow({ width: 800, height: 600 });
    mainWindow.loadFile('index.html');

    createTray(mainWindow, app);
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