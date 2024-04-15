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
    console.log(data)
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
                    emitter.emit(data.side + '-comms', {action: 'close-tunnel'})
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
    return new Promise((resolve, reject) => {
        if (!ipc instanceof Server) {
            ipc = undefined
            resolve(true)
            return
        }

        try {
            ipc.close()
            ipc = undefined
            resolve(true)
        } catch (err) {
            reject(err.message)
        }
    })
}
export function startTunnel(data) {
    return new Promise((resolve, reject) => {
        if (tunnel) {
            reject(false)
            return
        }
        exec(`powershell -Command "Start-Process -Verb RunAs '${tunnelPath}' -ArgumentList '--${data.side} ${data.primary} ${data.port}'"`,
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
        exec(`powershell -Command "Start-Process -Verb RunAs -FilePath 'powershell' -ArgumentList '-Command Get-Process vpn-tunnel | Stop-Process'"`,
            (err, stdout, stderr) => {
                tunnel = false
                if (err) {
                    reject(err)
                } else {
                    resolve(true)
                }
            })
    })
}

export function checkTunnelStatus() {
    return new Promise((resolve, reject) => {
        exec('powershell -Command "Get-Process vpn-tunnel"', (err, stdout, stderr) => {
            if (err) {
                reject(err.message);
            } else {
                if (stdout.includes('vpn-tunnel')) {
                    resolve(true);
                } else {
                    resolve(false);
                }
            }
        });
    });
}
