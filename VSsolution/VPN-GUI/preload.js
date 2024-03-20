const { contextBridge, ipcRenderer } = require('electron/renderer')

contextBridge.exposeInMainWorld('electronAPI', {
    sendData: (side, data) => ipcRenderer.send(side+'-control', data),
    recvData: (side, callback) => {
        ipcRenderer.listeners(side + '-control').forEach(listener => {
            ipcRenderer.removeListener(side+'-control', listener)
        })
        ipcRenderer.on(side+'-control', callback)
    },
    removeListener: () => {
        ipcRenderer.listeners('server-control').forEach(listener => {
            ipcRenderer.removeListener('server-control', listener)
        })
        ipcRenderer.listeners('client-control').forEach(listener => {
            ipcRenderer.removeListener('client-control', listener)
        })
    }
})