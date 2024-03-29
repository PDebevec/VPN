const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
    sendData: (data) => ipcRenderer.send('frontend-comms', data),
    recvData: (callback) => {
        ipcRenderer.listeners('frontend-comms').forEach(listener => {
            ipcRenderer.removeListener('frontend-comms', listener)
        })
        ipcRenderer.on('frontend-comms', callback)
    }
})