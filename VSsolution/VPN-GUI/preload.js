const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
    sendData: (data) => ipcRenderer.send('app-comms', data),
    recvData: (callback) => {
        ipcRenderer.listeners('app-comms').forEach(listener => {
            ipcRenderer.removeListener('app-comms', listener)
        })
        ipcRenderer.on('app-comms', callback)
    }
})