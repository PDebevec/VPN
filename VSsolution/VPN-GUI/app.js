'use strict';

import { createTray, createMainWindow} from './windows.js'
import { startHTTPS, startPipe, startTunnel, stopTunnel } from './connection.js'
import { app, ipcMain } from 'electron';

app.on('ready', () => {
    createMainWindow(app);

    createTray(app);
});

ipcMain.on('server-control', (event, data) => {
    console.log(data)
    switch (data.action) {
        case 'start':
            startHTTPS(data.args)
            startPipe()
            startTunnel(data.args.join(' '))
            break
        case 'stop':
            stopTunnel()
            break
        default:
    }
})

ipcMain.on('client-control', (event, data) => {
    switch (data.action) {

        default:
    }
})

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});
