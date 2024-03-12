import { Tray, Menu, BrowserWindow } from 'electron'
import { Worker } from 'worker_threads'
import { spawn } from 'child_process'
import { join } from 'path'

export const createWorder = () => {
    const worker = new Worker('./connection.js')

    worker.postMessage('messgae')

    worker.on('message', (message) => {
        console.log('returned: ' + message)
    })

    return worker
}

export const createMainWindow = (app) => {
    const win = new BrowserWindow({
        width: 800,
        height: 600,
        autoHideMenuBar: true,
        webPreferences: {
            preload: join(app.getAppPath(), '/preload.js')
        },
        //frame: false
    })

    win.loadFile('./pages/index.html')

    win.on('minimize', (event) => {
        event.preventDefault();
        win.hide();
    });

    return win
}

export const createTray = (mainWindow, app) => {
    let tray = new Tray(join(app.getAppPath(), '/assets/logo.png'));

    const contextMenu = Menu.buildFromTemplate([
        { label: 'VPN: OFF', click: () => mainWindow.show() },
        { label: 'Show App', click: () => mainWindow.show() },
        { label: 'Quit', click: () => app.quit() }
    ]);

    tray.setToolTip('VPM GUI');
    tray.setContextMenu(contextMenu);

    tray.on('click', () => {
        mainWindow.isVisible() ? mainWindow.hide() : mainWindow.show();
    });

    return tray
}