import { createServer, request } from 'node:https'
import { readFileSync } from 'node:fs'
import { exec } from 'child_process'
import * as net from 'node:net'

let pipe = undefined
let server = undefined
const tunnelPath = '..\\x64\\Release\\vpn-tunnel.exe'
let vpnTunnel = undefined

export function getStatus() {
    return {
        response: 'tunnel-status',
        https: server ? true : false,
        tunnel: vpnTunnel ? true : false
    }
}

export function startTunnel(args, callback) {
    vpnTunnel = exec(`powershell -Command "Start-Process -Verb RunAs '${tunnelPath}' -ArgumentList '${args}'"`, callback)
}
export function stopTunnel(callback) {
    if (vpnTunnel != undefined) {
        exec(`powershell -Command "Start-Process -Verb RunAs -FilePath 'powershell' -ArgumentList '-Command Get-Process vpn-tunnel | Stop-Process'"`, callback)
    }
    vpnTunnel = undefined
    return true
}
export function startPipe(callback) {
    if (pipe instanceof net.Server) {
        return
    }
    pipe = net.createServer((socket) => {

        console.log('connected')
        socket.write('hello')

        socket.on('data', (data) => {

        })

        socket.on('end', () => {
            console.log('disconnected!')
        })
    }).listen('\\\\.\\pipe\\VPNpipe', callback)
}
export function stopHTTPS(callback) {
    server.close(callback)
    server = undefined
}
export function startHTTPS(cert, key, address, port, callback) {
    server = createServer({
        cert: readFileSync(cert),
        key: readFileSync(key)
    }, (req, res) => {

    }).listen(port, address, callback)
}
export function httpsGET(options, callback) {
    options.method = 'GET'
    request(options, (res) => {
        res.on('data', (d) => {
            callback(d, null)
        })

        res.on('error', (e) => {
            callback(null, e.message)
        })
    }).on('error', (e) => {
        callback(null, e.message)
    }).end();
}
export function httpsPOST() {

}