import { Tray, Menu, BrowserWindow } from 'electron'
import { join } from 'path'

let mainWindow
let tray

export const createMainWindow = (app) => {
    mainWindow = new BrowserWindow({
        width: 800,
        height: 600,
        autoHideMenuBar: true,
        webPreferences: {
            preload: join(app.getAppPath(), "/preload.js")
        },
    })

    mainWindow.loadFile('./pages/index.html')

    mainWindow.on('minimize', (event) => {
        event.preventDefault();
        mainWindow.hide();
    });

    mainWindow.webContents.openDevTools()
}

export const createTray = (app) => {
    tray = new Tray(join(app.getAppPath(), '/assets/logo.png'));

    const contextMenu = Menu.buildFromTemplate([
        { label: 'VPN: OFF' },
        { label: 'Open GUI', click: () => mainWindow.show() },
        { label: 'Quit', click: () => app.quit() }
    ]);

    tray.setToolTip('VPM GUI');
    tray.setContextMenu(contextMenu);

    tray.on('click', () => {
        mainWindow.isVisible() ? mainWindow.hide() : mainWindow.show();
    });
}