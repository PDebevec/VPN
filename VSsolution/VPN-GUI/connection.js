import { parentPort } from 'node:worker_threads'
import * as server from './side/serverConnection.js'
import * as client from './side/clientConnection.js'

if (parentPort === null) {
    throw new Error('This is not a child thread!')
}
