import { Tray, Menu } from 'electron';
import { join } from 'path';

export const createTray = (mainWindow, app) => {
    let tray = new Tray(join(app.getAppPath(), '/assets/logo.png'));

    const contextMenu = Menu.buildFromTemplate([
        { label: 'Show App', click: () => mainWindow.show() },
        { label: 'Quit', click: () => app.quit() }
    ]);

    tray.setToolTip('My Electron App');
    tray.setContextMenu(contextMenu);

    mainWindow.on('minimize', (event) => {
        event.preventDefault();
        mainWindow.hide();
    });

    tray.on('click', () => {
        mainWindow.isVisible() ? mainWindow.hide() : mainWindow.show();
    });
}