import { exec } from 'node:child_process'
import { createServer, Server } from 'node:net'
import emitter from './emitter.js'

export let ipc = undefined
export let tunnel = false

const tunnelPath = '..\\x64\\Release\\vpn-tunnel.exe'

export function createSSLCertificate() {
    return new Promise((resolve, reject) => {
        exec('C:\\"Program Files"\\OpenSSL-Win64\\bin\\openssl.exe req -x509 -newkey rsa:2048 -keyout auth\\server.key -out auth\\server.crt -sha256 -days 3650 -nodes -subj "/C=SI/ST=Local state/L=Local city/O=Private VPN inc./OU=personal department/CN=localhost"',
            (err, stdout, stderr) => {
                if (err) {
                    reject(err.message)
                } else {
                    resolve()
                }
            })
    })
}
export function generateRSAkeyPair() {
    return new Promise((resolve, reject) => {
        exec('C:\\"Program Files"\\OpenSSL-Win64\\bin\\openssl.exe genrsa -out auth\\private.key 2048',
            (err, stdout, stderr) => {
                if (err) {
                    reject(err.message)
                } else {
                    exec('C:\\"Program Files"\\OpenSSL-Win64\\bin\\openssl.exe rsa -pubout -in auth\\private.key -out auth\\public.key',
                        (err, stdout, stderr) => {
                            if (err) {
                                reject(err.message)
                            } else {
                                resolve()
                            }
                        })
                }
            })
    })
}
export function startIPC(data, handlePipeData, handlePipeMsg) {
    return new Promise((resolve, reject) => {
        try {
            ipc = createServer((socket) => {
                emitter.emit('message', {
                    action: 'pipe-connected'
                })

                socket.write('ACK\0')

                socket.on('data', (data) => {
                    let res = handlePipeData(data.toString('utf-8'))
                    if (res) {
                        socket.write(res)
                    }
                })

                socket.on('end', () => {
                    if (tunnel) {
                        emitter.emit(data.side + '-comms', { action: 'check-status'})
                    }
                })

                emitter.on('pipe-comms', (msg) => {
                    let res = handlePipeMsg(msg)
                    if (res) {
                        socket.write(res)
                    } else {
                        socket.destroy()
                    }
                })
            })

            ipc.listen('\\\\.\\pipe\\VPNpipe', () => resolve(data))
            
            ipc.maxConnections = 1;

        } catch (e) {
            reject(e.message)
        }
    })
}
export function stopIPC() {
    return new Promise((resolve, reject) => {
        if (!ipc instanceof Server) {
            ipc = undefined
            resolve(true)
            return
        }

        emitter.emit('pipe-comms', {action: 'end-connections'})
        ipc.close()
        ipc = undefined
        emitter.removeAllListeners('pipe-comms');
        resolve(true)
    })
}
export function startTunnel(data, ipRange) {
    return new Promise((resolve, reject) => {
        if (tunnel) {
            reject(false)
            return
        }
        exec(`powershell -Command "Start-Process -Verb RunAs '${tunnelPath}' -ArgumentList '--${data.side} ${data.primary} ${data.port} ${ipRange}'"`,
            (err, stdout, stderr) => {
                if (err) {
                    reject(err.message)
                } else {
                    tunnel = true
                    resolve()
                }
            })
    })
}
export function closeTunnel() {
    return new Promise((resolve, reject) => {
        if (!tunnel) {
            tunnel = false
            resolve(true)
            return
        }

        if (emitter.emit('pipe-comms', { action: 'close-tunnel' })) {
            tunnel = undefined
            resolve(true)
        }else
            reject(false)
    })
}
