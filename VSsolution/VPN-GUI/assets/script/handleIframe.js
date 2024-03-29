function isValidIPAddress(ipAddress) {
    var ipRegex = /^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/;

    return ipRegex.test(ipAddress)
}

const iframe = document.getElementById('content')
let side = undefined
let tunnelState = undefined;
let httpsState = undefined;
let setupSettings = {}

window.electronAPI.recvData(handleResponse)

iframe.addEventListener('load', () => {
    side = iframe.name

    if (iframe.contentDocument.title == 'setup')
    {
        asignInputValues()
        iframe.contentDocument.getElementById('confirm')
            .addEventListener('click', confirmBtn);
        iframe.contentDocument.getElementById('clear')
            .addEventListener('click', clearBtn);
        iframe.contentDocument.getElementById('create-keycert').
            addEventListener('click', createKeyCert)
    }
    else if (iframe.contentDocument.title == 'status')
    {
        checkLocalStorage()
        iframe.contentDocument.getElementById('start-stop')
            .addEventListener('click', toggleVPN);
    } else {
        return
    }
})

function handleResponse(event, msg) {
    console.log(msg)
    if (msg.response == 'vpn-status') {
        httpsState = msg.https
        tunnelState = msg.tunnel
    }
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
    data.parsed.path = side == 'client' ? '/connect' : undefined

    window.electronAPI.sendData(data)
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
function createKeyCert() {
    window.electronAPI.sendData({
        action: 'create-cert'
    })
}
function checkLocalStorage() {
    iframe.contentDocument.getElementById('error-card').classList.add('d-none')
    if (localStorage.getItem(side) == null) {
        iframe.contentDocument.getElementById('info-card').classList.add('border-warning', 'text-danger')
        iframe.contentDocument.getElementById('status-text').innerHTML = `Set up the ${side} before stating the VPN!`
    } else {
        if (tunnelState) {

        } else {
            iframe.contentDocument.getElementById('info-card').classList.add('d-none')
            window.electronAPI.sendData({
                action: 'get-vpn-status'
            })
        }
    }
}