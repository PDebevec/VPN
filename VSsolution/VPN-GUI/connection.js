import { createServer, request, Server, Agent } from 'node:https'
import { privateDecrypt, publicEncrypt, randomBytes } from 'node:crypto'
import { exec, ChildProcess } from 'child_process'
import { EventEmitter } from 'node:events'
import { readFileSync } from 'node:fs'
import * as net from 'node:net'
import express from 'express'
import axios from 'axios'

let pipe = undefined
let HTTPSserver = undefined
let appServer = undefined
const tunnelPath = '..\\x64\\Release\\vpn-tunnel.exe'
//const tunnelPath = '.\\vpn-tunnel.exe'
let tunnel = undefined
let users = []
const axiosAgent = axios.create({
    httpsAgent: new Agent({ rejectUnauthorized: false })
});
export const emitter = new EventEmitter()

export function getStatus() {
    return {
        response: 'vpn-status',
        https: HTTPSserver instanceof Server || HTTPSserver == true ? true : false,
        pipe: pipe instanceof net.Server ? true : false,
        tunnel: tunnel  instanceof ChildProcess? true : false
    }
}

export function generateRSAKeyPair(callback) {
    exec('C:\\"Program Files"\\Git\\usr\\bin\\openssl.exe genpkey -algorithm RSA -out auth\\private.key',
        (err, stdout, stderr) => {
            callback(err)
        })
    exec('C:\\"Program Files"\\Git\\usr\\bin\\openssl.exe rsa -pubout -in auth\\private.key -out auth\\public.key',
        (err, stdout, stderr) => {
            callback(err)
        })
}
export function createSSLCertificate(callback) {
    exec('C:\\"Program Files"\\Git\\usr\\bin\\openssl.exe req -x509 -newkey rsa:2048 -keyout auth\\server.key -out auth\\server.crt -sha256 -days 3650 -nodes -subj "/C=SI/ST=Local state/L=Local city/O=Private VPN inc./OU=personal department/CN=localhost"',
        (err, stdout, stderr) => {
            callback(err)
    })
}
export function startTunnel(args) {
    console.log('starting tunnel')
    return new Promise((resolve, reject) => {
        if (tunnel instanceof ChildProcess) {
            stopTunnel(resolve, reject)
            return
        }
        tunnel = exec(`powershell -Command "Start-Process -Verb RunAs '${tunnelPath}' -ArgumentList '${args}'"`,
            (err, stdout, stderr) => {
                if (err) {
                    reject(err.message)
                } else {
                    resolve()
                }

        })
    })
}
export function stopTunnel(resolve, reject) {
    if (tunnel instanceof ChildProcess) {
        exec(`powershell -Command "Start-Process -Verb RunAs -FilePath 'powershell' -ArgumentList '-Command Get-Process vpn-tunnel | Stop-Process'"`,
            (err, stdout, stderr) => {
                if (err) {
                    reject(err)
                } else {
                    resolve(true)
                }
            })
    }
    tunnel = undefined
}
export function toggleHTTPSServer(cert, key, address, port) {
    return new Promise((resolve, reject) => {
        if (HTTPSserver instanceof Server) {
            stopHTTPSServer(resolve, reject)
            return
        }

        try {
            appServer = express()

            appServer.use(express.json())

            HTTPSserver = createServer({
                cert: readFileSync(cert),
                key: readFileSync(key)
            }, appServer).listen(port, address, resolve)

            handleHTTPSrequests()
        } catch (e) {
            reject(e.message)
            return
        }
    })
}
function stopHTTPSServer(resolve, reject) {
    HTTPSserver.close()
    HTTPSserver = undefined
    appServer = undefined
    stopPipe(resolve, reject)
}
export function startPipe() {
    console.log('starting pipe')
    return new Promise((resolve, reject) => {
        if (pipe instanceof net.Server) {
            stopPipe(resolve, reject)
            return
        }
        try {
            pipe = net.createServer((socket) => {
                emitter.emit('message', {
                    action: 'pipe-connected'
                })
                socket.write('hello')

                socket.on('data', (data) => {

                })

                socket.on('end', () => {
                    emitter.emit('message', {
                        action: 'pipe-disconnected'
                    })
                })
            }).listen('\\\\.\\pipe\\VPNpipe', resolve)
        } catch (e) {
            reject(e.message)
        }
    })
}
function stopPipe(resolve, reject) {
    if (pipe instanceof net.Server) {
        pipe.close()
    }
    pipe = undefined
    stopTunnel(resolve, reject)
    tunnel = undefined
}
export function httpsGET(url, data) {
    return new Promise((resolve, reject) => {
        axiosAgent.get(url)
            .then(res => {
                if (res.status != 200) {
                    reject(`server status: ${res.status}`)
                    return
                }

                if (!res.data) {
                    reject('No data from server')
                    return
                }

                const keys = publicEncrypt(readFileSync(data.encryption), Buffer.concat([randomBytes(32), randomBytes(32)]))
                
                httpsPOST(`https://${data.primary}:${data.port}/connected/${res.data}`, {keys})
                    .then((res) => resolve(res.status, [res.data.substring(0, 64), res.data.substring(64)]))
                    .catch((err) => reject(err.message))
            })
            .catch(err => {
                reject(err.message)
            })
    })
}
function httpsPOST(url, data) {
    return axiosAgent.post(url, data)
}
function handleHTTPSrequests() {
    appServer.get('/connect', (req, res) => {
        let userHex = randomBytes(32).toString('hex')
        users[userHex] = {connected:true};

        res.status(200)
        res.send(userHex)
    })

    appServer.post('/connected/:user', (req, res) => {
        if (!users[req.params.user]) {
            res.status(400)
            res.send('user not connected')
            return
        }

        if (!req.body.keys) {
            res.status(400)
            res.send('keys not provided!')
            return
        }

        const keys = privateDecrypt(readFileSync('./auth/private.key'), Buffer.from(req.body.keys.data)).toString('hex')

        res.status(200)
        res.send(keys)
    })
}

emitter.on('internal', (msg) => {
    console.log(msg)
    switch (msg.action) {
        case 'start-tunnel':
            startPipe()
                .then(() => {
                    startTunnel(msg.args)
                        .then((data) => {
                            emitter.emit('message', data)
                        }).catch((err) => {
                            emitter.emit('message', err)
                        })
                }).catch((err) => {
                    emitter.emit('message', err)
                })
            break;
        case 'start-vpn-server':
            toggleHTTPSServer(msg.data.cert, msg.data.key, msg.data.primary, msg.data.port)
                .then((data) => {
                    emitter.emit('message', getStatus())
                    if (!data) {
                        startPipe()
                            .then((data) => {
                                emitter.emit('message', getStatus())
                                if (!data) {
                                    startTunnel(`-s ${msg.data.primary} ${msg.data.port} ${msg.data.secondary}`)
                                        .then((data) => {
                                            emitter.emit('message', getStatus())
                                        })
                                        .catch(err => emitter.emit('message', { response: 'tunnel-error', err }))
                                }
                            })
                            .catch(err => emitter.emit('message', {response: 'pipe-error', err}))
                    }
                })
                .catch(err => emitter.emit('message', {response: 'https-server-error', err}))
            break
        case 'start-vpn-client':
            httpsGET(`https://${msg.data.primary}:${msg.data.port}${msg.data.path}`, msg.data)
                .then((data, keys) => {
                    if (!(data instanceof Number) && !(keys instanceof Array)) {
                        emitter.emit('message', {response: 'response-data-error'})
                        return
                    }
                    HTTPSserver = true
                    emitter.emit('message', getStatus())
                    startPipe()
                        .then((data) => {
                            emitter.emit('message', getStatus())
                            if (!data) {
                                startTunnel(`-c ${msg.data.primary} ${msg.data.port} ${msg.data.secondary}`)
                                    .then((data) => {
                                        emitter.emit('message', getStatus())
                                    })
                                    .catch(err => emitter.emit('message', { response: 'tunnel-error', err }))
                            }
                        })
                        .catch(err => emitter.emit('pipe-error', err))
                })
                .catch(err => emitter.emit('message', {response: 'server-connection-error', err}))
            break
        default:
            break;
    }
})