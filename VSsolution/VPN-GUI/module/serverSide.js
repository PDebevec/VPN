import { tunnel, ipc, startTunnel, startIPC, stopIPC, closeTunnel } from './joinedModules.js'
import { createPrivateKey, privateDecrypt, constants, randomBytes } from 'node:crypto'
import { Server, createServer } from 'node:https'
import { WebSocketServer, WebSocket } from 'ws'
import { networkInterfaces } from 'node:os'
import { readFileSync } from 'node:fs'
import emitter from './emitter.js'
import * as net from 'node:net'
import express from 'express'

let users = []
const secondaryIPs = []

let server = undefined;
let app = undefined;
let wss = undefined;

export function getStatus(err) {
    return {
        response: 'vpn-status',
        wss: wss ? true : false,
        ipc: ipc instanceof net.Server ? true : false,
        tunnel,
        err
    }
}
function startHTTPS(data) {
    return new Promise((resolve, reject) => {
        console.log(data)

        try {
            app = express()

            server = createServer({
                cert: readFileSync(data.cert),
                key: readFileSync(data.key)
            }, app)

            wss = new WebSocketServer({ noServer: true })

            server.listen(data.port, data.primary, () => resolve(data))

            wss.on('connection', WSSonNewConnection)

            app.use(express.json())
            server.on('upgrade', upgradeToWebSocket);

            app.get('/connect/:user', (req, res) => GETconnect(req, res))
            app.get('/disconnect/:user', (req, res) => GETdisconnect(req, res))
            app.post('/encryption/:user', (req, res) => POSTencryption(req, res))

        } catch (err) {
            reject(err.message)
            return
        }
    })
}
function WSSonNewConnection(ws, req) {
    console.log(`connected ${req.userHash}`)

    ws.on('message', (msg) => {
        console.log(msg)
    })

    ws.on('close', (code, res) => {
        emitter.emit('pipe-comms', { action: 'disconnect-user', hash: req.userHash, secondary:users[req.userHash].secondary })
    })

    ws.on('error', (err) => {
        console.log(err.message)
        try {
            ws.close()
        } catch (err) {

        }
    })
}
function upgradeToWebSocket(req, socket, head) {
    let split = req.url.split('/')
    if (!(split[1] == 'websocket' && split.length == 3)) {
        socket.end('HTTP/1.1 400 Bad Request');
        return;
    }

    if (!users[split[2]]) {
        socket.end('HTTP/1.1 400 Bad Request');
        return;
    }

    req.userHash = split[2]

    wss.handleUpgrade(req, socket, head, (ws) => {
        wss.emit('connection', ws, req);
    });
}
function GETconnect(req, res) {
    console.log('get connect')
    if (!req.params.user) {
        res.status(400)
        res.send({ err: 'Connecting user not specified!' })
        return
    }

    users[req.params.user] = { connected: true, hash: randomBytes(64).toString('hex') }

    res.status(200)
    res.send({hash: users[req.params.user].hash})
}
function GETdisconnect(req, res) {
    console.log('get disconnect')
    if (!req.params.user) {
        res.status(400)
        res.send({err:'User not specified!'})
        return
    }

    users[req.params.user] = null

    res.status(200)
    res.send({disconnected: true})
}
function POSTencryption(req, res) {
    console.log('post encryption')
    if (!req.params.user) {
        res.status(400)
        res.send({err: 'User not specified!'})
        return;
    }
    if (!users[req.params.user]) {
        res.status(400)
        res.send({ err: 'User not connected!' })
        return
    }
    if (!users[req.params.user].connected) {
        res.status(400)
        res.send({ err: 'User not connected!' })
        return
    }

    const decrypted = privateDecrypt({
        key: createPrivateKey(readFileSync('.\\auth\\private.key', { encoding: 'utf-8' })),
        padding: constants.RSA_PKCS1_PADDING
    }, Buffer.from(req.body.encrypted)).toString('hex')

    if (users[req.params.user].hash != decrypted.substring(128)) {
        res.status(400)
        res.send({ err: 'Decrypted hash does not match for specified user!' })
        return
    }

    users[req.params.user].encryption = decrypted.substring(0, 128)
    users[req.params.user].secondary = secondaryIPs.shift()

    emitter.emit('pipe-comms', {
        action: 'tunnel-user',
        userHash: req.params.user
    })

    res.status(200)
    res.send({ keys: users[req.params.user].encryption })
}
function closeConnection() {
    return new Promise((resolve, reject) => {
        if (!server instanceof Server) {
            server = null
            wss = null
            resolve(true)
            return
        }

        try {
            server.close()
            server = null
            wss.close()
            wss = null
            resolve()
        } catch (err) {
            server = null
            wss = null
            reject(err.message)
        }
    })
}
function findSecondaryIP(data) {
    const netinfo = networkInterfaces()
    let secondary = false

    for (let interf in netinfo) {
        netinfo[interf].forEach(item => {
            if (item.address == data.primary) {
                secondary = true
                return
            }
        })
        if (!secondary) {
            return undefined
        }

        netinfo[interf].forEach(item => {
            if (item.family == 'IPv4' && item.address != data.primary) {
                secondaryIPs.push(item.address)
            }
        })
    }

    return secondaryIPs.length
}
function handlePipeData(data) {
    console.log(data)
}
function handlePipeMsg(msg) {
    console.log(msg)
    switch (msg.action) {
        case 'tunnel-user':
            return Buffer.concat([Buffer.from(users[msg.userHash].encryption, 'hex'), Buffer.from(users[msg.userHash].secondary + '\0')])
            break
        case 'disconnect-user':
            return Buffer.from(`FIN${msg.secondary}\0`)
        default:
    }
}

emitter.on('server-comms', async (msg) => {
    console.log(msg)
    switch (msg.action) {
        case 'start-tunnel':
            if (findSecondaryIP(msg.data) <= 0) {
                emitter.emit('message', getStatus('No secondary ips on network interface!'))
                break
            }

            startHTTPS(msg.data)
                .then(data => {
                    emitter.emit('message', getStatus())
                    data.side = 'server'

                    startIPC(data, handlePipeData, handlePipeMsg)
                        .then(data => {
                            emitter.emit('message', getStatus())
                            data.ipRange = ''

                            startTunnel(data, '')
                                .then(data => {
                                    emitter.emit('message', getStatus())
                                })
                                .catch(async (err) => {
                                    emitter.emit('message', getStatus(err))
                                    emitter.emit('server-comms', {action: 'check-status'})
                                })
                        })
                        .catch(async (err) => {
                            emitter.emit('message', getStatus(err.message))
                            emitter.emit('server-comms', { action: 'check-status' })
                        })
                }).catch(err => {
                    emitter.emit('message', getStatus(err.message))
                    server = undefined
                    wss = undefined
                })
            break;
        case 'close-tunnel':
            await closeTunnel()
                .catch(err => emitter.emit('message', getStatus(err)))
            emitter.emit('message', getStatus())
            await stopIPC()
                .catch(err => emitter.emit('message', getStatus(err)))
            emitter.removeAllListeners('pipe-comms')
            emitter.emit('message', getStatus())
            await closeConnection()
                .catch(err => emitter.emit('message', getStatus(err)))
            emitter.emit('message', getStatus())
            emitter.emit('close-module')
            break;
        case 'check-status':
            if (server instanceof Server || ipc instanceof net.Server || tunnel) {
                emitter.emit('server-comms', { action: 'close-tunnel' })
            }
            else {
                emitter.emit('server-comms', {action: 'start-tunnel'})
            }
            break;
        default:
    }
})
