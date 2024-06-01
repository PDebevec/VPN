import { ipc, tunnel, startTunnel, startIPC, stopIPC, closeTunnel } from './joinedModules.js'
import { createPublicKey, publicEncrypt, randomBytes, constants } from 'node:crypto'
import { networkInterfaces } from 'node:os'
import { readFileSync } from 'node:fs'
import { Agent } from 'node:https'
import { Server } from 'node:net'
import emitter from './emitter.js'
import { WebSocket } from 'ws'
import axios from 'axios'

const user = {hash:randomBytes(32).toString('base64url')}

let wss = undefined

const https = axios.create({
    httpsAgent: new Agent({ rejectUnauthorized: false }),
    timeout: 1500
});

export function getStatus(err) {
    return {
        response: 'vpn-status',
        wss: wss instanceof WebSocket ? true : false,
        ipc: ipc instanceof Server ? true : false,
        tunnel,
        err
    }
}
function connectHTTPS(data) {
    return new Promise((resolve, reject) => {
        console.log(`https://${data.primary}:${data.port}/connect/${user.hash}`)
        https.get(`https://${data.primary}:${data.port}/connect/${user.hash}`)
            .then((res) => {
                const keys = randomBytes(96).toString('hex').substring(32, 160)

                const plaintext = Buffer.from(keys + res.data.hash, 'hex')

                const encrypted = publicEncrypt({
                    key: createPublicKey(readFileSync(data.public, { encoding: 'utf-8' })),
                    padding: constants.RSA_PKCS1_PADDING
                }, plaintext).toJSON()

                console.log(`https://${data.primary}:${data.port}/encryption/${user.hash}`)
                https.post(`https://${data.primary}:${data.port}/encryption/${user.hash}`, { encrypted })
                    .then((res) => {
                        user.keys = res.data.keys

                        wss = new WebSocket(`wss://${data.primary}:${data.port}/websocket/${user.hash}`, {rejectUnauthorized: false})
                        
                        wss.on('open', () => {
                            console.log('connected');
                            resolve(data)
                            wss.send(JSON.stringify({ mijav: 'string' }));
                        });

                        wss.on('close', () => {
                            emitter.emit('client-comms', {action:'close-tunnel'})
                            console.log('disconnected');
                        });

                        wss.on('message', (data) => {
                            console.log(data);
                        });
                    })
                    .catch(err => reject(err.message))
            })
            .catch(err => reject(err.message))
    })
}
function closeConnection(data) {
    return new Promise((resolve, reject) => {
        if (!wss) {
            wss = null
            resolve(true)
            return
        }
        try {
            wss.close()
            wss = null
            resolve()
        } catch (err) {
            wss = null
            reject(err.message)
        }
    })
}
function getSecondaryIP() {
    let netInterfaces = networkInterfaces()
    Object.keys(netInterfaces).forEach((interfaceName) => {
        const netInterface = netInterfaces[interfaceName];

        const filteredInterface = netInterface.filter(details =>
            !details.internal &&
            details.family === 'IPv4' &&
            (details.address.startsWith('192.168.') ||
                details.address.startsWith('10.')));

        if (filteredInterface.length > 0) {
            user.secondary = filteredInterface[0].address
            return
        }
    });

    return user.secondary
}
function handlePipeData(data) {
    if (data == 'ACK') {
        emitter.emit('pipe-comms', {
            action: 'start-tunnel',
            keys: user.keys,
            secondary: user.secondary
        })
    }
}
function handlePipeMsg(msg) {
    switch (msg.action) {
        case 'start-tunnel':
            return Buffer.concat([Buffer.from(msg.keys, 'hex'), Buffer.from(msg.secondary + '\0')])
        case 'close-tunnel':
            return Buffer.from('FIN\0')
            break;
    }
}

emitter.on('client-comms', async (msg) => {
    console.log(msg)
    switch (msg.action) {
        case 'start-tunnel':
            if (!getSecondaryIP()) {
                emitter.emit('message', getStatus('Device IP not found!'))
                break
            }

            connectHTTPS(msg.data)
                .then(data => {
                    emitter.emit('message', getStatus())
                    data.side = 'client'

                    startIPC(data, handlePipeData, handlePipeMsg)
                        .then(data => {
                            emitter.emit('message', getStatus())

                            startTunnel(data, `${data.low} ${data.high}`)
                                .then(data => {
                                    emitter.emit('message', getStatus())
                                })
                                .catch((err) => {
                                    emitter.emit('message', getStatus(err))
                                    emitter.emit('client-comms', { action: 'check-status' })
                                })
                        })
                        .catch((err) => {
                            emitter.emit('message', getStatus(err))
                            emitter.emit('client-comms', { action: 'check-status' })
                        })
                }).catch(err => {
                    emitter.emit('message', getStatus(err))
                    wss = undefined
                    emitter.emit('close-module')
                })
            break;
        case 'close-tunnel':
            await closeTunnel()
                .catch(err => emitter.emit('message', getStatus(err)))
            emitter.emit('message', getStatus())
            await stopIPC()
                .catch(err => emitter.emit('message', getStatus(err)))
            emitter.emit('message', getStatus())
            await closeConnection(msg.data)
                .catch(err => emitter.emit('message', getStatus(err)))
            emitter.emit('message', getStatus())
            emitter.emit('close-module')
            break;
        case 'check-status':
            if (wss || ipc instanceof Server || tunnel) {
                emitter.emit('client-comms', { action: 'close-tunnel' })
            }
            else {
                emitter.emit('client-comms', { action: 'start-tunnel' })
            }
            break;
        default:
    }
})
