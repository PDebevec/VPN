import { createServer } from 'node:https'
import { readFileSync } from 'node:fs'
import { exec } from 'child_process'
import * as net from 'node:net'

let pipe
let server
const tunnelPath = '..\\x64\\Release\\vpn-tunnel.exe'
let vpnTunnel = undefined

export function startTunnel(args) {
    vpnTunnel = exec(`powershell -Command "Start-Process -Verb RunAs '${tunnelPath}' -ArgumentList '${args}'"`, (err, stdout, stderr) => {
        if (err) {
            return err
        }
    })
    return true
}
export function stopTunnel() {
    if (vpnTunnel != undefined) {
        exec(`powershell -Command "Start-Process -Verb RunAs -FilePath 'powershell' -ArgumentList '-Command Get-Process vpn-tunnel | Stop-Process'"`,
            (err, stdout, stderr) => {
                if (err) {
                    return err
                }
            })
    }
    return true
}
export function startPipe() {
    pipe = net.createServer((socket) => {

        console.log('connected')
        socket.write('hello')

        socket.on('data', (data) => {

        })

        socket.on('end', () => {
            console.log('disconnected!')
        })
    }).listen('\\\\.\\pipe\\VPNpipe', () => { })
}
export function startHTTPS(cert, key) {
    server = createServer({
        cert: readFileSync(cert),
        key: readFileSync(key)
    }, (req, res) => {

    }).listen()
}
