'use strict';

import { createTray, createMainWindow, mainWindow} from './windows.js'
import { getStatus, httpsGET, startHTTPS, startPipe, startTunnel, stopHTTPS, stopTunnel } from './connection.js'
import { app, ipcMain } from 'electron';
import { netLog } from 'electron/main';

app.on('ready', () => {
    createMainWindow(app);

    createTray(app);
});

ipcMain.on('server-control', (event, data) => {
    console.log(data)
    switch (data.action) {
        case 'get-tunnel-status':
            mainWindow.webContents.send('server-control', getStatus())
            break
        case 'toggle-https':
            if (data.key && data.cert && data.addr && data.port) {
                startHTTPS(data.cert, data.key, data.addr, data.port, () => {
                    mainWindow.webContents.send('server-control', getStatus())
                })
            } else {
                stopHTTPS(() => {
                    mainWindow.webContents.send('server-control', getStatus())
                })
            }
            break
        case 'toggle-tunnel':
            if (getStatus().tunnel) {
                stopTunnel((err, stdout, stderr) => {
                    if (err) {
                        mainWindow.webContents.send('server-control', {
                            response: 'tunnel-error',
                            error: err
                        })
                    } else {
                        mainWindow.webContents.send('server-control', {
                            response: 'tunnel-stoped',
                            stdout
                        })
                    }
                })
            } else {
                startPipe(() => {
                    mainWindow.webContents.send('server-control', {
                        response: 'pipe-started'
                    })
                });
                startTunnel(data.args.join(' '), (err, stdout, stderr) => {
                    if (err) {
                        mainWindow.webContents.send('server-control', {
                            response: 'tunnel-error',
                            error: err
                        })
                    } else {
                        mainWindow.webContents.send('server-control', {
                            response: 'tunnel-started',
                            stdout
                        })
                    }
                })
            }
            break
        default:
    }
})

ipcMain.on('client-control', (event, data) => {
    console.log(data)
    switch (data.action) {
        case 'toggle-https':
            httpsGET(data.options, (data, err) => {
                if (err) {
                    mainWindow.webContents.send('client-control', err)
                } else {
                    mainWindow.webContents.send('client-control', data)
                }
            })
            break
        case 'toggle-tunnel':
            if (getStatus().tunnel) {
                stopTunnel((err, stdout, stderr) => {
                    if (err) {
                        mainWindow.webContents.send('client-control', {
                            response: 'tunnel-error',
                            error: err
                        })
                    } else {
                        mainWindow.webContents.send('client-control', {
                            response: 'tunnel-stoped',
                            stdout
                        })
                    }
                })
            } else {
                startPipe(() => {
                    mainWindow.webContents.send('client-control', {
                        response: 'pipe-started'
                    })
                });
                startTunnel(data.args.join(' '), (err, stdout, stderr) => {
                    if (err) {
                        mainWindow.webContents.send('client-control', {
                            response: 'tunnel-error',
                            error: err
                        })
                    } else {
                        mainWindow.webContents.send('client-control', {
                            response: 'tunnel-started',
                            stdout
                        })
                    }
                })
            }
            break;
        default:
    }
})

app.on('window-all-closed', () => {
    if (process.platform !== 'darwin') {
        app.quit();
    }
});
