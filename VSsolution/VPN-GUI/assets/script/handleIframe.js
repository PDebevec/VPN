function isValidIPAddress(ipAddress) {
    var ipRegex = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;

    return ipRegex.test(ipAddress)
}

const iframe = document.getElementById('content')
let side = undefined
let tunnelState = undefined;
let httpsState = undefined;
let setupSettings = {}

window.electronAPI.recvData('server', handleResponse)
window.electronAPI.recvData('client', handleResponse)

iframe.addEventListener('load', () => {
    side = iframe.name

    if (iframe.contentDocument.title == 'setup')
    {
        asignInputValues()
        iframe.contentDocument.getElementById('confirm')
            .addEventListener('click', confirmBtn);
        iframe.contentDocument.getElementById('clear')
            .addEventListener('click', clearBtn);
    }
    else if (iframe.contentDocument.title == 'status')
    {
        checkLocalStorage()
        iframe.contentDocument.getElementById('https')
            .addEventListener('click', handleHttpsBtn);
        iframe.contentDocument.getElementById('tunnel')
            .addEventListener('click', handleTunnelBtn);
    } else {
        return
    }
})

function handleResponse(event, msg) {
    console.log(msg)
    switch (msg.response) {
        case 'tunnel-status':
            tunnelState = msg.tunnel
            httpsState = msg.https
            break
        default:
    }
}
function handleTunnelBtn() {
    let data = {
        action: 'toggle-tunnel'
    }

    if (!tunnelState) {
        const parsed = JSON.parse(localStorage.getItem(side))
        data.args = [
            `-${side[0]}`, parsed.primary, Number(parsed.port), parsed.secondary
        ]
    }

    window.electronAPI.sendData(side, data)
}
function handleHttpsBtn() {
    let data = {
        action: 'toggle-https'
    }

    const parsed = JSON.parse(localStorage.getItem(side))
    if (!httpsState && side == 'server') {
        data.cert = parsed.cert
        data.key = parsed.key
        data.addr = parsed.primary
        data.port = Number(parsed.port)
    } else if(side == 'client') {
        data.toggle = !httpsState ? 'start' : 'stop'
        data.options = {
            hostname: parsed.primary,
            port: parsed.port,
            path: '/'
        }
    }

    window.electronAPI.sendData(side, data)
}
function asignInputValues() {
    if (side == 'client') {
        iframe.contentDocument.getElementById('server-key').classList.add('d-none')
        iframe.contentDocument.getElementById('server-cert').classList.add('d-none')
    } else {
        iframe.contentDocument.getElementById('client-key').classList.add('d-none')
    }
    if (localStorage.getItem(side) != null) {
        setupSettings = JSON.parse(localStorage.getItem(side))
        let parsed = setupSettings
        iframe.contentDocument.getElementById('primary').value = parsed.primary ? parsed.primary : ''
        iframe.contentDocument.getElementById('port').value = parsed.port ? parsed.port : ''
        iframe.contentDocument.getElementById('secondary').value = parsed.secondary ? parsed.secondary : ''
    }
}
function confirmBtn(event) {
    let ids = []
    if (side == 'client') {
        ids = ['encryption', 'primary', 'port', 'secondary'];
    } else if (side == 'server') {
        ids = ['key', 'cert', 'primary', 'port', 'secondary'];
    }
    const contentDocument = iframe.contentDocument;

    ids.forEach(id => {
        const element = contentDocument.getElementById(id);
        if (element) {
            if (!element.value) {
                element.classList.add('is-invalid')
            } else {
                element.classList.remove('is-invalid')
                if (element.files) {
                    const value = element.files[0].path
                    setupSettings[id] = value || setupSettings[id];
                } else {
                    const value = id === 'port' ? Number(element.value) : element.value;
                    setupSettings[id] = value || setupSettings[id];
                }
            }
        }
    });

    localStorage.setItem(side, JSON.stringify(setupSettings));
    console.log(setupSettings);
}
function clearBtn() {
    let ids = ['key', 'cert', 'encryption', 'primary', 'port', 'secondary'];
    localStorage.removeItem(side)
    setupSettings = {}
    ids.forEach(id => {
        iframe.contentDocument.getElementById(id).value = ''
    })
}
function checkLocalStorage() {
    if (localStorage.getItem(side) == null) {
        iframe.contentDocument.getElementById('info-card').classList.add('border-warning', 'text-danger')
        iframe.contentDocument.getElementById('status-text').innerHTML = `Set up the ${side} before stating the VPN!`
        //iframe.contentDocument.getElementById('https-card').classList.add('border-light', 'text-light')
        //iframe.contentDocument.getElementById('tunnel-card').classList.add('border-light', 'text-light')
        //iframe.contentDocument.getElementById('https').classList.add('disabled', 'btn-light')
        //iframe.contentDocument.getElementById('tunnel').classList.add('disabled', 'btn-light')
    } else {
        //iframe.contentDocument.getElementById('https').classList.add('btn-success')
        //iframe.contentDocument.getElementById('tunnel').classList.add('btn-success')
        if (tunnelState) {

        } else {
            iframe.contentDocument.getElementById('info-card').classList.add('d-none')
            window.electronAPI.sendData(side, {
                action: 'get-tunnel-status'
            })
        }
    }
}