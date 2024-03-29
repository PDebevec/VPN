'use strict';

import { createTray, createMainWindow, mainWindow} from './windows.js'
import { createSSLCertificate, emitter, getStatus } from './connection.js'
import { app, ipcMain } from 'electron';

app.on('ready', () => {
    createMainWindow(app);

    createTray(app);
});

ipcMain.on('frontend-comms', (event, data) => {
    console.log(data)
    switch (data.action) {
        case 'get-vpn-status':
            mainWindow.webContents.send('frontend-comms', getStatus())
            break
        case 'toggle-vpn':
            emitter.emit('internal', {
                action: 'start-vpn-'+data.side,
                data: data.parsed
            })
            break
        case 'create-cert':
            createSSLCertificate((err) => {
                mainWindow.webContents.send('frontend-comms', {
                    response: 'creating-cert-response',
                    err: err.message | undefined
                })
            })
            break
        default:
    }
})

emitter.on('message', (data) => mainWindow.webContents.send('frontend-comms', data))

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});
