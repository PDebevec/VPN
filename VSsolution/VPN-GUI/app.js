'use strict';

import { createTray, createMainWindow, mainWindow} from './module/windows.js'
import { createSSLCertificate, generateRSAkeyPair } from './module/joinedModules.js'
import emitter from './module/emitter.js'
import { app, ipcMain } from 'electron';

let vpnModule = null

app.on('ready', () => {
    createMainWindow(app);

    createTray(app);
});

ipcMain.on('app-comms', async (event, data) => {
    console.log(data)
    switch (data.action) {
        case 'get-vpn-status':
            if (vpnModule) {
                mainWindow.webContents.send('app-comms', vpnModule.getStatus())
            } else {
                mainWindow.webContents.send('app-comms', {
                    response: 'vpn-status',
                    https: false,
                    pipe: false,
                    tunnel: false,
                })
            }
            break
        case 'close-module':
            vpnModule = null
            break
        case 'toggle-vpn':
            if (vpnModule) {
                emitter.emit(data.side + '-comms', {action: 'check-status'})
                break;
            }
            try {
                vpnModule = await import(`./module/${data.side}Side.js`);;

                emitter.emit(data.side + '-comms', {
                    action: 'start-tunnel',
                    data: data.parsed
                });
            } catch (err) {
                mainWindow.webContents.send('app-comms', { response: 'import-error', err: err.message });
            }
            break
        case 'stop-vpn':
            if (vpnModule) {
                vpnModule.closeTunnel()
            }
            break
        case 'create-cert':
            createSSLCertificate()
                .then(() => mainWindow.webContents.send('app-comms', {response: 'SSL-cert-created'}))
                .catch(err => mainWindow.webContents.send('app-comms', {response: 'SSL-create-error', err}))
            break
        case 'generate-keypair':
            generateRSAkeyPair()
                .then(() => mainWindow.webContents.send('app-comms', {response: 'key-pair-generated'}))
                .catch(err => mainWindow.webContents.send('app-comms', {response: 'Error-generating-keys', err}))
            break
        default:
    }
})

emitter.on('message', (data) => mainWindow.webContents.send('app-comms', data))

emitter.on('close-module', () => { vpnModule = null })

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});
