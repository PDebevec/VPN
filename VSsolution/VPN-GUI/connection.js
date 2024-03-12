import * as https from 'https'
import { parentPort } from 'worker_threads'

if (parentPort === null) {
    throw new Error('This is not a child thread!')
}

parentPort.on('message', (message) => {
    parentPort.postMessage(message)
})