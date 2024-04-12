import { exec } from 'node:child_process'
import { createServer } from 'node:net'
import emitter from './emitter.js'
import { rejects } from 'node:assert'

export let ipc = undefined
export let tunnel = undefined

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
    console.log('starting pipe')
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
                    emitter.emit('message', {
                        action: 'pipe-disconnected'
                    })
                })

                emitter.on('pipe-comms', (msg) => {
                    let res = handlePipeMsg(msg)
                    if (res) {
                        socket.write(res)
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
    ipc.close()
    ipc = undefined
}
export function startTunnel(data) {
    return new Promise((resolve, reject) => {
        tunnel = exec(`powershell -Command "Start-Process -Verb RunAs '${tunnelPath}' -ArgumentList '${data.side} ${data.primary} ${data.port}'"`,
            (err, stdout, stderr) => {
                if (err) {
                    reject(err.message)
                } else {
                    resolve()
                }

            })
    })
}
export function closeTunnel() {
    return new Promise((resolve, reject) => {
        exec(`powershell -Command "Start-Process -Verb RunAs -FilePath 'powershell' -ArgumentList '-Command Get-Process vpn-tunnel | Stop-Process'"`,
            (err, stdout, stderr) => {
                if (err) {
                    reject(err)
                } else {
                    resolve(true)
                }
            })
        tunnel = undefined
    })
}

