const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
    sendData: (side, data) => ipcRenderer.send(side+'-control', data),
    recvData: (side, callback) => {
        ipcRenderer.on(side+'-control', callback)
    }
})