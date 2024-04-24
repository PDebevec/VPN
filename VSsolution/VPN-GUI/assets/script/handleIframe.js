function isValidIPAddress(ipAddress) {
    var ipRegex = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;

    return ipRegex.test(ipAddress)
}

const iframe = document.getElementById('content')
let frameDoc = undefined
let side = undefined
let tunnelState = undefined;
let httpsState = undefined;

iframe.addEventListener('load', () => {
    frameDoc = document.getElementById('content').contentDocument
    side = iframe.name

    if (side == 'server') {
        let other = document.getElementById('client-collapse-btn')
        if (!other.classList.contains('collapsed')) {
            other.click()
        }
    }
    else if (side == 'client') {
        let other = document.getElementById('server-collapse-btn')
        if (!other.classList.contains('collapsed')) {
            other.click()
        }
    }

    if (frameDoc.title == 'setup')
    {
        asignInputValues()
        frameDoc.getElementById('confirm')
            .addEventListener('click', confirmBtn);
        frameDoc.getElementById('clear')
            .addEventListener('click', clearBtn);
        frameDoc.getElementById('create-keycert')
            .addEventListener('click', () => window.electronAPI.sendData({ action: 'create-cert' }))
        frameDoc.getElementById('create-keypair')
            .addEventListener('click', () => window.electronAPI.sendData({ action: 'generate-keypair' }))
    }
    else if (frameDoc.title == 'status')
    {
        checkLocalStorage()
        frameDoc.getElementById('start-stop')
            .addEventListener('click', toggleVPN);
    } else {
        return
    }
})

window.electronAPI.recvData((event, msg) => {
    if (msg.response == 'vpn-status') {
        changeStatus(msg)
    }
})
function changeStatus(status) {
    if (status.err) {
        frameDoc.getElementById('error-message').innerHTML = status.err
        frameDoc.getElementById('error-card').classList.remove('d-none')
        return
    } else {
        frameDoc.getElementById('error-message').innerHTML = ''
        frameDoc.getElementById('error-card').classList.add('d-none')
    }

    const statuses = ['wss', 'ipc', 'tunnel'];

    statuses.forEach((s) => {
        const span = frameDoc.getElementById(`${s}-status`);
        if (status[s]) {
            span.classList.remove('bg-danger');
            span.classList.add('bg-success');
            span.innerHTML = "ON";
        } else {
            span.classList.remove('bg-success');
            span.classList.add('bg-danger');
            span.innerHTML = "OFF";
        }
    });
}
function toggleVPN() {
    let data = {
        action: 'toggle-vpn',
        side
    }

    const parsed = JSON.parse(localStorage.getItem(side))
    data.side = side;
    parsed.port = Number(parsed.port)
    data.parsed = parsed

    window.electronAPI.sendData(data)
}
function asignInputValues() {
    if (side == 'client') {
        frameDoc.getElementById('server-cert').classList.add('d-none')
        frameDoc.getElementById('server-key').classList.add('d-none')
    } else if (side == 'server') {
        frameDoc.getElementById('client-key').classList.add('d-none')
        frameDoc.getElementById('local-ips').classList.add('d-none')
    }
    if (localStorage.getItem(side) != null) {
        let parsed = JSON.parse(localStorage.getItem(side))
        frameDoc.getElementById('primary').value = parsed.primary ? parsed.primary : ''
        frameDoc.getElementById('port').value = parsed.port ? parsed.port : ''
        frameDoc.getElementById('low').value = parsed.low ? parsed.low : ''
        frameDoc.getElementById('high').value = parsed.high ? parsed.high : ''
    }
}
function confirmBtn(event) {
    let ids = []

    if (side == 'client') {
        ids = ['public', 'primary', 'port', 'low', 'high'];
    } else if (side == 'server') {
        ids = ['key', 'cert', 'primary', 'port'];
    }

    let settings = {}

    ids.forEach(id => {
        const element = frameDoc.getElementById(id);
        if (element) {
            if (!element.value) {
                element.classList.add('is-invalid')
            } else {
                element.classList.remove('is-invalid')
                if (element.files) {
                    const value = element.files[0].path
                    settings[id] = value || settings[id];
                } else {
                    const value = id === 'port' ? Number(element.value) : element.value;
                    settings[id] = value || settings[id];
                }
            }
        }
    });

    localStorage.setItem(side, JSON.stringify(settings));
    console.log(settings);
}
function clearBtn() {
    let ids = ['key', 'cert', 'public', 'primary', 'port', 'low', 'high'];
    localStorage.removeItem(side)
    ids.forEach(id => {
        frameDoc.getElementById(id).value = ''
    })
}
function checkLocalStorage() {
    frameDoc.getElementById('error-card').classList.add('d-none')

    window.electronAPI.sendData({
        action: 'get-vpn-status'
    })

    if (localStorage.getItem(side) == null) {
        frameDoc.getElementById('status-text').innerHTML = `Set up the ${side} info before stating the VPN!`
        frameDoc.getElementById('info-card').classList.remove('d-none')
        frameDoc.getElementById('start-stop').classList.add('disabled')
    } else {
        frameDoc.getElementById('info-card').classList.add('d-none')
    }
}